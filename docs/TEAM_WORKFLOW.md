# LifeLens 2인 협업 규칙 — 쭌 / 다겸

이 문서는 `sjLim91`(쭌)과 `STILLofficial`(다겸), 그리고 두 사람이 사용하는 AI 에이전트가 동시에 LifeLens를 개발할 때 코드 충돌과 중복 작업을 방지하기 위한 공통 규칙이다.

모든 에이전트는 작업 시작 전에 `AGENTS.md`와 이 문서를 읽는다.

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
