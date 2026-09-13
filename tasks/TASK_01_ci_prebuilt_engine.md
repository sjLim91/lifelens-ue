# TASK 01 — CI에서 엔진 소스 빌드 제거, 프리빌드 이미지 + 캐시 + 워크플로우 분리

브랜치: `task/01-ci-prebuilt-engine`
선행: 없음
참고: `docs/BUILD_STRATEGY_v1.2.md`

## 목표

현재 워크플로우는 매 실행마다 Unreal Engine을 소스에서 컴파일한다. 이를 제거하고, Epic 프리빌드 컨테이너로 교체하며, 프로젝트 산출물을 캐시하고, 워크플로우를 "빠른 것"과 "무거운 것"으로 분리한다.

## 사전 조건 (사람이 해야 함 — 성준)

1. Epic Games 계정과 GitHub 계정 연결 (Epic 계정 설정 → Connections → GitHub). 연결되면 GitHub `EpicGames` 조직 초대가 온다. 수락해야 GHCR 이미지 pull 권한이 생긴다.
2. GitHub Personal Access Token 생성 (`read:packages` 권한). 저장소 Secrets에 `GHCR_TOKEN`으로 등록.
3. 사용할 엔진 버전을 하나 정한다 (예: 5.4.x). `LifeLens.uproject`의 `EngineAssociation`과 일치시킨다.

## 작업 내용

### 1. 기존 워크플로우 분석 후 제거 대상 표시

`.github/workflows/` 안의 모든 파일을 읽고, 엔진 저장소를 clone하거나 `Setup.sh` / `GenerateProjectFiles.sh` / `Build.sh` 를 엔진에 대해 실행하는 스텝을 전부 찾아 목록으로 적는다. 이 스텝들은 삭제한다. 프로젝트(`LifeLens.uproject`)를 대상으로 하는 스텝만 남긴다.

### 2. `android-package.yml` 작성 (수동 실행 전용)

아래 골격을 기준으로 작성한다. 태그, NDK 버전, 경로는 반드시 실제 값으로 확인 후 채운다.

```yaml
name: android-package
on:
  workflow_dispatch:
    inputs:
      profile:
        description: "fast | full"
        default: "fast"

jobs:
  preflight:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Check uproject engine version
        run: |
          grep EngineAssociation LifeLens.uproject
          test -f LifeLens.uproject
      - name: Disk before cleanup
        run: df -h

  package:
    needs: preflight
    runs-on: ubuntu-latest
    container:
      image: ghcr.io/epicgames/unreal-engine:dev-slim-5.4   # 실제 태그 확인 후 고정
      credentials:
        username: ${{ github.actor }}
        password: ${{ secrets.GHCR_TOKEN }}
    steps:
      - uses: actions/checkout@v4

      - name: Free disk space
        run: |
          # 컨테이너 안이므로 호스트 정리 액션 대신 불필요 디렉터리 제거
          rm -rf /usr/share/dotnet /opt/ghc /usr/local/lib/android || true
          df -h

      - name: Restore project caches
        uses: actions/cache@v4
        with:
          path: |
            Intermediate
            DerivedDataCache
            Saved/Cooked
          key: ue-${{ hashFiles('LifeLens.uproject') }}-${{ hashFiles('Source/**') }}-${{ hashFiles('Content/**') }}
          restore-keys: |
            ue-${{ hashFiles('LifeLens.uproject') }}-${{ hashFiles('Source/**') }}-
            ue-${{ hashFiles('LifeLens.uproject') }}-

      - name: Install Android SDK/NDK (engine-required versions)
        run: |
          # /home/ue4/UnrealEngine/Engine/Extras/Android/SetupAndroid.sh 를 열어
          # 요구하는 SDK/NDK/JDK 버전을 확인하고 그 버전을 설치한다.
          # 임의의 최신 버전을 설치하지 않는다.
          echo "TODO: SetupAndroid.sh 기준 버전 설치"

      - name: Build + Cook + Package (FAST)
        if: ${{ github.event.inputs.profile == 'fast' }}
        run: |
          /home/ue4/UnrealEngine/Engine/Build/BatchFiles/RunUAT.sh BuildCookRun \
            -project=$GITHUB_WORKSPACE/LifeLens.uproject \
            -platform=Android -cookflavor=ASTC \
            -clientconfig=Development \
            -build -cook -stage -package -pak \
            -map=L_FastTest \
            -nocompileeditor -unattended -utf8output

      - name: Verify APK
        run: find Binaries -name "*.apk" -print -quit | grep -q apk

      - uses: actions/upload-artifact@v4
        with:
          name: lifelens-android-${{ github.event.inputs.profile }}
          path: Binaries/Android/*.apk
          retention-days: 7
```

주의:
- `container.image` 태그는 GHCR에서 실제 존재하는 태그로 확인해 고정한다. 존재하지 않는 태그를 추측으로 쓰지 않는다.
- 이미지 안의 엔진 경로(`/home/ue4/UnrealEngine`)도 이미지 문서에서 확인한다.
- 디스크가 부족하면 `dev-slim`보다 작은 변형이 있는지 확인하고, 없으면 러너 호스트에서 정리 액션(`jlumbroso/free-disk-space`)을 `container` 없이 먼저 실행하는 2단계 구조로 바꾼다.

### 3. `core-tests.yml` 작성 (push마다)

```yaml
name: core-tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Configure & build LifeLensCore
        run: |
          cmake -S Source/LifeLensCore -B build -DCMAKE_BUILD_TYPE=Release
          cmake --build build -j
      - name: Run tests
        run: ctest --test-dir build --output-on-failure
      - name: Run one-day harness
        run: ./build/harness/ll_harness --days 1 --seed 42 | tee harness.log
      - uses: actions/upload-artifact@v4
        with:
          name: harness-log
          path: harness.log
```

`Source/LifeLensCore`가 아직 없으면(TASK_02 이전) 이 워크플로우는 빈 CMake 프로젝트라도 통과하도록 최소 골격을 만든다.

### 4. 문서 갱신

`docs/BUILD_STRATEGY_v1.2.md` 하단에 "확정 값" 섹션을 추가하고 실제 사용한 이미지 태그, 엔진 경로, SDK/NDK/JDK 버전을 기록한다.

## 완료 조건

- [ ] 워크플로우 어디에도 엔진 소스 clone / 엔진 컴파일 스텝이 없다.
- [ ] `core-tests.yml`이 push 시 3분 이내에 통과한다.
- [ ] `android-package.yml`을 수동 실행하면 preflight → package 순으로 돌고, 실패하더라도 **엔진 컴파일 단계에서 시간을 쓰지 않는다.**
- [ ] 두 번째 수동 실행에서 캐시 히트 로그가 확인된다.
- [ ] 위 결과(실행 링크, 소요 시간)를 PR 본문에 기록했다.
