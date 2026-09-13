# COWORK — 다겸 PC 로컬 빌드 지시서

이 문서는 **다겸의 컴퓨터에서 실행되는 에이전트(Cowork / Claude Code)**를 위한 것이다.
성준이 폰에서 Astra로 짜서 push한 코드를 다겸의 PC에서 컴파일하고, FAST TEST APK를 뽑아 성준에게 보낼 수 있는 상태로 만드는 것이 역할이다.

저장소 루트의 `AGENTS.md` 규칙은 여기서도 그대로 적용된다. 이 문서는 그 위에 "로컬 PC에서만 할 수 있는 일"을 추가한다.

---

## 0. 역할 분담 (에이전트가 스스로 하려 들면 안 되는 것)

| 사람이 한다 (다겸) | 에이전트가 한다 |
|---|---|
| Epic Games 런처 설치, Epic 계정 로그인 | 설치된 엔진 경로 탐지 |
| 런처에서 Unreal Engine 설치 (버전은 `LifeLens.uproject`의 `EngineAssociation`과 동일) | 엔진 버전과 uproject 일치 확인 |
| Android Studio 설치 후 엔진의 `SetupAndroid` 스크립트 1회 실행 | SDK/NDK/JDK 경로가 엔진 설정에 잡혔는지 확인 |
| GitHub 로그인 / SSH 키 | 저장소 clone, pull, 브랜치 작업 |
| APK를 성준에게 전송 (카톡 등) | `dist/`에 APK와 빌드 노트 생성 |
| 성준과의 의사소통 | 빌드 노트에 성준이 읽을 내용 작성 |

에이전트는 메시지를 보내거나, 계정에 로그인하거나, 설치 프로그램을 실행하지 않는다. 그 단계에 도달하면 멈추고 다겸에게 정확히 무엇을 해야 하는지 알려준다.

---

## 1. 절대 규칙 (로컬 추가분)

1. `Engine/` 디렉터리(런처로 설치된 엔진) 안의 파일은 **읽기만** 한다. 수정·삭제 금지.
2. 수정은 저장소 안의 `Source/`, `Config/`, `Content/`, `.github/`, `docs/`, `tasks/`에만 한다. 저장소 밖에 쓰는 곳은 `dist/` 하나뿐이다.
3. `Binaries/`, `Intermediate/`, `DerivedDataCache/`, `Saved/`, `dist/`는 커밋하지 않는다. `.gitignore`에 없으면 추가한다.
4. `git push --force`, 브랜치 삭제, `main` 직접 push는 하지 않는다. 다겸에게 물어본다.
5. 컴파일 에러를 고칠 때 성준의 설계 의도(AGENTS.md, SPEC)를 바꾸지 않는다. 문법·링크·include 오류 수준만 고치고, 설계 변경이 필요해 보이면 고치지 말고 빌드 노트에 "성준 판단 필요"로 적는다.
6. 한 번의 빌드 시도가 30분을 넘기면 중단하고 원인을 보고한다.

---

## 2. 한 번만 하는 작업 — 환경 확인

### 2-1. Preflight (에이전트)

다음을 탐지해서 표로 보고한다. 하나라도 없으면 다음 단계로 가지 않고 다겸이 할 일을 안내한다.

- OS (Windows / macOS)
- 설치된 Unreal Engine 경로와 버전
  - Windows 기본: `C:\Program Files\Epic Games\UE_<ver>\`
  - macOS 기본: `/Users/Shared/Epic Games/UE_<ver>/`
- `LifeLens.uproject`의 `EngineAssociation` 값과 위 버전의 일치 여부
- Android SDK / NDK / JDK 경로
  - 엔진의 `Engine/Config/BaseEngine.ini` 및 사용자 설정(`Engine/Saved/Config` 또는 `~/Library/Application Support/Epic/UnrealEngine/...`)의 `[/Script/AndroidPlatformEditor.AndroidSDKSettings]` 항목
  - 환경변수 `ANDROID_HOME`, `NDKROOT`, `JAVA_HOME`
- 엔진이 요구하는 SDK/NDK/JDK 버전: `Engine/Extras/Android/SetupAndroid.(bat|sh|command)` 파일 안의 변수 값을 읽어서 보고. **절대 최신 버전을 임의로 설치하지 않는다.**
- 디스크 여유 공간 (최소 30GB 권장)

### 2-2. 다겸이 할 일 (Preflight에서 빠진 것만)

에이전트는 빠진 항목에 대해 아래 문구를 그대로 안내한다.

- 엔진 없음 → "Epic Games 런처 → Unreal Engine → 라이브러리 → `<uproject의 버전>` 설치. 설치 옵션에서 **Target Platforms → Android** 체크."
- Android 툴체인 없음 → "Android Studio 설치 후, 한 번 실행해서 기본 SDK 설치. 그다음 엔진 폴더의 `Engine/Extras/Android/SetupAndroid.bat`(맥은 `.command`)을 실행. 끝나면 컴퓨터 재시작."
- 버전 불일치 → "uproject의 EngineAssociation을 설치된 버전으로 바꿀지, 엔진을 그 버전으로 설치할지 결정 필요. 성준과 상의."

---

## 3. 반복 작업 — 빌드 루프

성준이 push할 때마다 아래를 수행한다. 다겸이 "빌드해줘"라고 하면 이 절차다.

### 3-1. 동기화

```
git fetch --all
git checkout <성준이 알려준 브랜치, 기본 main>
git pull
git log -1 --oneline   # 빌드 노트에 기록
```

### 3-2. 프로젝트 컴파일 (에디터 타깃)

Windows:
```
"<Engine>\Engine\Build\BatchFiles\Build.bat" LifeLensEditor Win64 Development -Project="<repo>\LifeLens.uproject" -WaitMutex
```

macOS:
```
"<Engine>/Engine/Build/BatchFiles/Mac/Build.sh" LifeLensEditor Mac Development -Project="<repo>/LifeLens.uproject"
```

- 성공: 3-3으로.
- 실패: 에러 로그에서 **첫 번째 에러**부터 원인을 특정하고, 규칙 1-5 범위 안에서 고친다. 고친 뒤 재컴파일. 세 번 시도해도 안 되면 멈추고 보고.
- 고친 내용은 `ue: fix <내용>` 커밋으로 남긴다. push는 다겸 확인 후.

### 3-3. (선택) 에디터 실행 확인

다겸이 화면을 볼 수 있을 때만. 에디터를 실행하고 `L_FastTest` 맵이 열리는지 확인한다.

Windows: `"<Engine>\Engine\Binaries\Win64\UnrealEditor.exe" "<repo>\LifeLens.uproject"`
macOS: `open "<Engine>/Engine/Binaries/Mac/UnrealEditor.app" --args "<repo>/LifeLens.uproject"`

에이전트는 실행만 하고, "열렸는지" 판단은 다겸이 한다.

### 3-4. FAST TEST APK 패키징

`docs/BUILD_STRATEGY_v1.2.md`의 FAST TEST 정의를 따른다.

Windows:
```
"<Engine>\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="<repo>\LifeLens.uproject" -platform=Android -cookflavor=ASTC -clientconfig=Development -build -cook -stage -package -pak -map=L_FastTest -nocompileeditor -unattended -utf8output
```

macOS: `RunUAT.sh`로 동일 인자.

- 결과 APK 위치: `<repo>/Binaries/Android/*.apk` (없으면 `Saved/StagedBuilds/Android_ASTC/` 아래를 찾는다)
- 첫 성공까지 걸린 시간과 두 번째부터의 시간을 빌드 노트에 기록한다.

### 3-5. 배포 준비

```
dist/
  LifeLens_fast_<YYYYMMDD>_<커밋7자리>.apk
  BUILD_NOTES_<YYYYMMDD>_<커밋7자리>.md
```

`BUILD_NOTES`에 반드시 포함:

- 커밋 해시와 브랜치
- 이번 빌드에 포함된 변경 요약 (git log 기준, 성준이 읽을 수 있게 한국어)
- 에이전트가 고친 컴파일 오류 목록 (있다면)
- "성준 판단 필요" 항목 (있다면)
- 성준이 폰에서 확인해야 할 것 (예: "큐브가 냉장고로 이동하는지", "화면 좌상단 로그가 찍히는지")
- 알려진 문제

다겸이 APK와 노트를 성준에게 보낸다. 에이전트는 여기서 멈춘다.

---

## 4. 코어 테스트도 로컬에서

`Source/LifeLensCore`가 있으면 APK 이전에 항상 먼저 돌린다. 여기서 실패하면 APK를 뽑지 않는다.

```
cmake -S Source/LifeLensCore -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/harness/ll_harness --days 1 --seed 42
```

하네스 로그를 `dist/HARNESS_<커밋7자리>.log`로 저장해 APK와 함께 보낸다. 성준이 APK 화면과 대조하는 용도다.

---

## 5. 처음 한 번 끝났을 때 갱신할 문서

첫 로컬 APK가 성공하면:

1. `docs/BUILD_STRATEGY_v1.2.md` 하단에 "로컬 빌드 확정 값" 섹션 추가: OS, 엔진 버전, SDK/NDK/JDK 버전, 컴파일 시간, 패키징 시간.
2. `tasks/TASK_01_ci_prebuilt_engine.md` 상단에 "로컬 빌드가 주 경로가 되었으므로 CI는 백업. 우선순위 하향" 한 줄 추가.
3. 위 두 변경을 `docs:` 커밋으로 남기고 다겸 확인 후 push.

---

## 6. 시작 명령 (다겸이 Cowork에 붙여넣을 것)

```
저장소 루트의 AGENTS.md와 tasks/COWORK_LOCAL_BUILD.md를 읽어라.
지금은 2절(환경 확인)만 수행한다. Preflight 결과를 표로 보고하고, 빠진 항목이 있으면 2-2의 문구로 내가 할 일을 알려준 뒤 멈춰라. 설치 프로그램 실행이나 로그인은 하지 마라.
```

환경이 갖춰진 뒤:

```
tasks/COWORK_LOCAL_BUILD.md 3절 빌드 루프를 수행해라. 브랜치는 main. 코어 테스트 → 에디터 타깃 컴파일 → FAST TEST APK → dist/ 생성 순서. 컴파일 오류는 1-5 규칙 범위에서만 고치고, 끝나면 BUILD_NOTES 내용을 보여줘라.
```

이후 반복은 "빌드해줘" 한 마디면 된다.
