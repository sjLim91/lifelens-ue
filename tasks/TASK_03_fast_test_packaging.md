# TASK 03 — FAST TEST 프로젝트 슬림화 + LifeLensCore를 Unreal 모듈에 링크

브랜치: `task/03-fast-test`
선행: TASK_01, TASK_02
참고: `docs/BUILD_STRATEGY_v1.2.md` FAST TEST 정의

## 목표

Android 패키징 1회의 시간을 최소로 만들고, TASK_02의 코어가 Unreal 안에서 돌게 한다. 이 TASK의 산출물은 "하네스에서 본 그 하루가 폰 화면에서 큐브 캐릭터로 보이는" FAST TEST APK다. 그래픽 품질은 목표가 아니다.

## 작업 내용

### 1. 프로젝트 슬림화

- `Config/DefaultGame.ini`
  - `[/Script/UnrealEd.ProjectPackagingSettings]`에 `+MapsToCook=/Game/Maps/L_FastTest` 만 남긴다.
  - `bCookAll=False`, `bCompressed=True`(Oodle 기본), 암호화 비활성.
- `Config/DefaultEngine.ini`
  - `[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]`: `bBuildForArm64=True`, `bBuildForX8664=False`, `bPackageDataInsideApk=True`(FAST TEST에서는 OBB 분리하지 않음), 텍스처 포맷은 ASTC만.
  - 모바일 렌더링: Vulkan 하나만 활성. OpenGL ES 비활성(빌드 시간 절감).
- `LifeLens.uproject`
  - `Plugins` 배열에서 실제로 쓰지 않는 플러그인은 `"Enabled": false`로 명시. 특히 온라인 서브시스템, VR, 에디터 전용 유틸리티.
- `Content/`
  - `L_FastTest.umap`: 바닥 평면 + 방 1개(박스 브러시 또는 단순 StaticMesh) + Smart Object 위치를 나타내는 큐브 7개(Bed, Toilet, Sink, Fridge, Chair, Table, Sofa). 라이팅은 단일 Directional Light, Lumen 비활성.

### 2. LifeLensCore 링크

- `Source/LifeLens/LifeLens.Build.cs`에서 `Source/LifeLensCore/include`를 `PublicIncludePaths`에 추가하고, 코어 소스를 같은 모듈에 포함하거나(간단) 별도 `LifeLensCore` 모듈로 감싼다(권장). 코어 소스는 여전히 엔진 헤더를 포함하지 않는다.
- `ALLSimulationActor` (또는 `ULLSimulationSubsystem`): `lifelens::Simulation`을 소유하고, `Tick`마다 시뮬레이션 tick을 진행(게임 시간 배속 적용). 코어의 이벤트 콜백을 받아 화면 좌상단에 마지막 10줄을 텍스트로 표시(UMG 최소 위젯).
- `ALLCharacterProxy`: 코어의 `CharacterId` 하나를 나타내는 큐브 액터. 코어가 보고하는 `GridPos`로 위치를 보간 이동.

이 TASK에서 NavMesh, 애니메이션, 실사 캐릭터는 다루지 않는다.

### 3. 워크플로우 연결

- `android-package.yml`의 `fast` 프로필이 위 설정으로 패키징하도록 확인.
- 패키징 성공 시 APK를 GitHub Release(pre-release, 태그 `fast-<run_number>`)로 업로드하는 스텝을 추가해 폰에서 바로 다운로드 가능하게 한다.

## 완료 조건

- [ ] `android-package.yml`(fast) 1회 소요 시간이 TASK_01 완료 시점 대비 줄었고, 수치를 PR에 기록했다.
- [ ] APK를 폰에 설치하면 크래시 없이 실행되고, 큐브 캐릭터가 하루 동안 오브젝트 사이를 이동하며 화면 텍스트 로그가 하네스 로그와 같은 순서로 찍힌다.
- [ ] 같은 seed로 하네스와 APK의 하루 이벤트 순서가 일치한다 (코어 분리가 실제로 됐다는 증거).
