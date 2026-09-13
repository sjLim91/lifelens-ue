# LifeLens 2인 협업 규칙 — 쭌 / 다겸

이 문서는 `sjLim91`(쭌)과 `STILLofficial`(다겸), 그리고 두 사람이 사용하는 AI 에이전트가 동시에 LifeLens를 개발할 때 코드 충돌과 중복 작업을 방지하기 위한 공통 규칙이다.

모든 에이전트는 작업 시작 전에 `AGENTS.md`와 이 문서를 읽는다.

## 0. MASTER SPEC 불변조건 — 역할 분담보다 우선

이 협업 문서는 제품 요구사항을 새로 정의하거나 축소하지 않는다. **제품의 최상위 기준은 `docs/LIFELENS_SPEC_v1.1.md`이며, 쭌/다겸의 역할 분담과 작업 편의 때문에 아래 요구사항을 삭제·완화·대체할 수 없다.** 빌드/검증 방식만 `docs/BUILD_STRATEGY_v1.2.md`와 `BUILD_LESSONS.md`의 실제 검증 결과를 따른다.

두 사람과 두 AI 모두 다음을 제품 불변조건으로 취급한다.

- LifeLens는 처음부터 **Unreal-native**로 개발한다. HTML/JavaScript/Three.js/Vercel 기반 게임 런타임을 되살리지 않는다.
- 플레이어의 기본 역할은 Controller가 아니라 **Observer**다. 메인 화면은 전체 상황을 한눈에 볼 수 있게 단순하게 유지하고, 캐릭터 선택 시 개인 상태/관계/상황의 상세정보를 추가로 보여준다.
- 정식 **NEW GAME은 성인 남성 2명 + 성인 여성 2명, 총 4명**으로 시작한다. 고정 캐릭터를 기본 시작 인구로 사용하지 않는다.
- 초기 4명의 이름, GUID, 외형/유전, 성격, Traits, 능력, 선호, 배경, 초기 상태는 **NEW GAME 최초 1회만** WorldSeed 기반으로 생성한다.
- WorldSeed와 생성 결과는 Save에 영구 보존한다. 같은 World를 Load할 때 4명을 다시 랜덤 생성하지 않는다. 다른 NEW GAME은 다른 WorldSeed와 다른 사회를 만든다.
- 캐릭터 이름은 ID가 아니다. 모든 사람은 영구 GUID/독립 ID를 가진다.
- 초기 4명은 미리 커플로 지정하지 않는다. Stranger/Low Familiarity 수준에서 시작하며, 친분·갈등·연애·결혼은 자율 시뮬레이션 결과로 형성한다.
- 캐릭터는 직접 명령 없이 Needs, Personality, Traits, Emotion, Memory, Relationship, Preference, Skill, Background 등의 영향을 받아 **자율적으로 의사결정**해야 한다. 정해진 Story Route를 강제하지 않는다.
- 인생주기 핵심 범위는 **친구/갈등 → 연애 → 파트너/동거 → 결혼 → 임신 → 출산 → 육아 → 성장 → 노화 → 죽음 → 세대교체**까지 포함한다. 가족/가계도와 부모-자녀 관계도 영속 데이터로 관리한다.
- Relationship은 단일 숫자가 아니라 다차원 관계 상태를 사용한다.
- 생활 시스템에는 배고픔/갈증/수면/위생/방광/건강 등 욕구, 직업/경제/자원/시설, 시간/날씨, 이동/경로/시선/거리유지/상호작용 확장성을 유지한다.
- 실시간 3D, 현실적인 인간 표현과 움직임, 관찰 카메라/Follow/시네마틱 관찰을 장기 목표로 유지한다. Android 최적화 때문에 Character Identity나 AI 철학을 다른 게임처럼 바꾸지 않는다.
- 플랫폼 우선순위는 **Android → Windows PC**다. Core Simulation의 규칙은 플랫폼 공통으로 유지하고 그래픽/Simulation LOD로 성능을 조절한다.
- 유료 API/유료 클라우드/유료 런타임을 필수 의존성으로 사용하지 않는다.
- 기능 수보다 안정적인 기반이 우선이다. 긴 Unreal/Android 빌드 전에 Core 테스트와 빠른 Preflight를 사용하며, 같은 실패를 원인 확인 없이 반복하지 않는다.
- 실제 APK/Artifact가 존재하기 전에는 Android 빌드 성공이라고 표현하지 않는다.

**역할 분담은 위 요구사항을 누가 구현할지를 나누는 규칙일 뿐, 요구사항 자체를 줄이는 규칙이 아니다.** 예를 들어 다겸이 UI만 담당하더라도 Observer-first 철학 전체를 따라야 하며, 쭌이 Core를 담당하더라도 연애/가족/세대 시스템을 임의로 범위 밖으로 제거할 수 없다.

## 1. 단일 기준 저장소

- 유일한 Source of Truth: `sjLim91/lifelens-ue`
- `main`은 항상 통합 가능한 안정본으로 유지한다.
- 기능 개발은 반드시 개인/작업 브랜치에서 한다.
- 다른 사람의 작업 브랜치에 직접 push하지 않는다.

## 2. 담당 영역

### 쭌 / sjLim91 / 쭌 측 AI — Simulation & Integration Owner

주 담당:
- `Source/LifeLensCore/**`
- `Source/LifeLens/AI/**`
- `Source/LifeLens/Simulation/**`
- `Source/LifeLens/World/**`
- Save/Load, 관계/연애/가족/세대 로직
- Core ↔ Unreal bridge
- Android build / GitHub Actions / CI
- 성능, 결정론, 테스트, 통합

쭌 측 AI는 위 영역의 구현과 통합을 우선한다.

### 다겸 / STILLofficial / 다겸 측 AI — Observer & Presentation Owner

주 담당:
- `Source/LifeLens/UI/**`
- `Source/LifeLens/Characters/**` 중 외형/표현 계층
- `Content/UI/**`
- `Content/Characters/**`
- Observer HUD / Resident detail / 선택 UX
- 캐릭터 정보 표현, 레이아웃, 카메라 관찰 UX
- 시각적 피드백, 디버그 표시, 사용자 관찰 경험

다겸 측 AI는 시뮬레이션 규칙을 직접 변경하지 않고 이미 공개된 읽기 API를 사용한다.

## 3. 공동 소유 파일 — 단독 수정 금지

아래 파일은 한쪽이 임의로 구조를 바꾸면 서로의 작업이 깨질 수 있으므로 공동 소유한다.

- `LifeLens.uproject`
- `Source/LifeLens/LifeLens.Build.cs`
- `Source/LifeLens/Core/LLTypes.h`
- `Config/**`
- `.github/workflows/**`
- `Tools/validate_bootstrap.py`
- Core ↔ Unreal 공개 인터페이스

공동 소유 파일 수정이 필요하면:
1. 자신의 브랜치에서 수정한다.
2. PR 본문에 `SHARED FILE CHANGE`를 적는다.
3. 왜 필요한지와 영향 범위를 적는다.
4. 상대 작업이 진행 중이면 merge 전에 diff를 확인한다.

## 4. API 경계 규칙

- UI는 Simulation/Core의 내부 컨테이너를 직접 수정하지 않는다.
- UI에서 새 데이터가 필요하면 Core/Simulation에 직접 임시 필드를 추가하지 않는다.
- 필요한 읽기 API를 PR 또는 `tasks/TEAM_BOARD.md`의 Integration Request에 기록한다.
- Simulation은 UI 구현 클래스를 직접 참조하지 않는다.
- Core는 Unreal 타입(`UObject`, `FString`, `TArray`, `CoreMinimal.h`)을 절대 포함하지 않는다.
- Character visual actor는 영속 상태의 원본이 아니다. 원본 데이터는 Simulation/Core가 소유한다.

의존 방향:

`LifeLensCore -> Unreal Bridge/Simulation -> Character/World -> Observer UI`

반대 방향의 강한 의존성은 만들지 않는다.

## 5. 브랜치 규칙

쭌:
- `jjun/<scope>-<task>` 또는 기존 task 지시서 브랜치 `task/<number>-...`
- 예: `jjun/social-ai`, `jjun/android-build`

다겸:
- `dagyeom/<scope>-<task>`
- 예: `dagyeom/observer-ui`, `dagyeom/resident-card`

긴 작업은 기능 하나당 브랜치 하나를 사용한다.

## 6. 동시 작업 잠금 규칙

작업 시작 시 `tasks/TEAM_BOARD.md`에 아래를 기록한다.

- 담당자
- 브랜치
- 작업 범위
- 수정 예정 파일/디렉터리
- 상태: `TODO`, `DOING`, `REVIEW`, `DONE`, `BLOCKED`

상대가 `DOING`으로 잡은 동일 파일은 수정하지 않는다.
정말 필요한 경우 먼저 Integration Request를 남기고, 공용 인터페이스만 합의해서 변경한다.

## 7. PR / Merge 규칙

- `main` 직접 기능 개발 금지.
- 한 PR은 한 작업 목적만 가진다.
- PR 본문에 변경 파일, 테스트 결과, 영향 범위를 적는다.
- Core/AI 변경: `core-tests.yml` PASS 필수.
- Unreal 구조 변경: structural preflight PASS 필수.
- Android APK 성공 여부는 실제 `.apk` Artifact 존재 전까지 성공이라고 표현하지 않는다.
- 공동 소유 파일이 포함된 PR은 상대 영역 영향 여부를 확인한 뒤 merge한다.
- 상대 브랜치가 오래된 경우 merge 전에 최신 `main` 기준 충돌 여부를 확인한다.

## 8. 빌드 규칙

- 빠른 Core 테스트를 가장 먼저 사용한다.
- 구조 검증과 실제 UHT/UBT 컴파일을 구분한다.
- Unreal/Android 장시간 빌드는 수동 마일스톤 검증에 사용한다.
- 같은 실패 원인으로 장시간 빌드를 반복하지 않는다.
- `dev-slim-5.6.0`은 Android target을 포함하지 않는 것이 실제 로그로 확인되었으므로 Android APK 경로에 사용하지 않는다.
- Android source-build 경로는 필요한 경우에만 허용하며, 정확한 실패 Step을 확인한 뒤 재실행한다.
- 비용이 발생하는 CI/API/클라우드는 사용하지 않는다.

## 9. 서로의 AI에게 적용되는 동일 규칙

쭌 측 AI와 다겸 측 AI 모두:
- 요구사항을 임의 축소하거나 다른 프로젝트로 바꾸지 않는다.
- LOCAL OBSERVER 코드를 LifeLens 런타임으로 되살리지 않는다.
- 계획만 쓰지 말고 가능한 범위에서 실제 구현한다.
- 성공 여부를 추측하지 않는다.
- 작업 전 `AGENTS.md`, `docs/LIFELENS_SPEC_v1.1.md`, `docs/BUILD_STRATEGY_v1.2.md`, `docs/TEAM_WORKFLOW.md`, `tasks/TEAM_BOARD.md`를 확인한다.
- 상대 담당 영역을 수정해야 하면 먼저 Integration Request를 남긴다.

## 10. 현재 권장 분업

### 쭌 측
- TASK_03 Core ↔ Unreal bridge 검증
- UE 5.6 UHT/UBT 실제 컴파일
- Android FAST TEST/APK 파이프라인
- LifeLensCore Needs/Utility/관계 로직 확장

### 다겸 측
- Observer HUD v2
- 선택 주민 상세 패널
- World overview 정보 계층
- Character visual proxy/표현 개선
- Core/Simulation read API만 사용해 UI 구성

이 분업은 `tasks/TEAM_BOARD.md`가 최신 상태를 가진다.
