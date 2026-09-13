# LifeLens Shared Handoff Log

이 파일은 쭌(sjLim91) / 다겸(STILLofficial) / 양쪽 AI가 서로의 작업을 인수인계하는 **append-only 공용 변경 로그**다.

## 사용 규칙

1. 작업 시작 전 반드시 최신 `main`의 이 파일과 `tasks/TEAM_BOARD.md`를 읽는다.
2. 코드/설정/워크플로우를 실제 변경했으면 작업 종료 전에 이 파일에 기록한다.
3. 기록에는 반드시 `작성자`, `브랜치`, `커밋/PR`, `변경 범위`, `검증 상태`, `상대가 알아야 할 점`을 남긴다.
4. 아직 검증되지 않은 것은 `검증 대기`로 적고 성공이라고 쓰지 않는다.
5. 상대 영역을 건드려야 하는 사항은 먼저 `tasks/TEAM_BOARD.md`의 Integration Request에 남긴다.
6. 과거 기록은 삭제하거나 덮어쓰지 않는다. 잘못된 기록은 새 항목에서 정정한다.
7. 브랜치가 `main`보다 오래된 경우 상대의 최근 변경을 확인한 뒤 merge/rebase하고, 공용 파일 충돌을 먼저 해결한다.

---

## 2026-09-13 — 쭌 측 AI

### LifeLensCore TASK_02 완료
- 작성자: 쭌 측 AI
- 브랜치/PR: `task/02-core-sim`, PR #1
- 상태: `main` 병합 완료
- 변경:
  - 순수 C++17 `Source/LifeLensCore/**`
  - Needs / Personality / SmartObject / Utility AI / Planner / 1분 tick Simulation
  - 콘솔 harness와 Needs/Utility/Determinism 테스트
  - `core-tests.yml`
- 검증:
  - Configure / Build / tests / deterministic harness smoke 모두 PASS
- 상대가 알아야 할 점:
  - 다겸 UI는 Core 내부를 직접 수정하지 않고 Unreal/Simulation의 읽기 API를 통해 데이터를 소비한다.
  - `LifeLensCore`에는 Unreal 타입을 넣지 않는다.

### TASK_03 Core ↔ Unreal Bridge
- 작성자: 쭌 측 AI
- 브랜치/PR: `task/03-fast-test`, PR #2
- 상태: `REVIEW / 실제 Android UHT·UBT 검증 진행 중`
- 변경:
  - `LifeLens.Build.cs`에 LifeLensCore include 경로 연결
  - `LLCoreCompileUnit.cpp`로 순수 Core 구현을 Unreal 모듈에서 컴파일
  - `ULLCoreBridgeSubsystem` 추가
  - `StartFastTest(seed)`, `AdvanceCoreMinutes()`, 최근 Core 이벤트 조회
  - structural preflight에 Core/Unreal 경계 검사 추가
- 검증:
  - structural preflight PASS
  - 실제 Android UHT/UBT는 Run `34739283266`에서 검증 중
- 중요:
  - 현재 Run `34739283266`은 시작 SHA `4a8b8d494d7e953d0eb98c2f322cdec3595a45e2` 기준이다.
  - 따라서 아래 Android fast-reuse 파이프라인 변경은 **현재 실행 중 Run에는 포함되지 않는다.**

### Android 재빌드 방지 파이프라인
- 작성자: 쭌 측 AI
- 브랜치: `ci/android-fast-reuse` → `task/03-fast-test`
- PR: #3 (`task/03-fast-test` 기준으로 병합 완료)
- 상태: `코드 반영 완료 / seed-fast 실제 운영 검증 대기`
- 변경:
  - Android workflow를 `seed / fast / full` 모드로 분리
  - `seed`: UE 5.6 + Android 엔진 컴파일 산출물을 재사용 캐시에 준비
  - `fast`: 엔진 변경을 금지하고 LifeLens 프로젝트 변경분 중심으로 빌드
  - cache miss 시 자동 full fallback 금지, 즉시 실패
  - `docs/ANDROID_FAST_PIPELINE.md` 추가
- 목적:
  - 반복적인 몇 시간짜리 Unreal Engine 전체 재컴파일 방지
- 상대가 알아야 할 점:
  - 다겸 측은 `.github/workflows/**`를 직접 수정하지 않는다.
  - UI 변경 검증을 위해 APK가 필요하면 쭌 측에 fast build 요청을 남긴다.
  - fast/seed가 아직 실제 성공 검증되기 전에는 검증 완료라고 간주하지 않는다.

### 2인 협업 규칙 정리
- 작성자: 쭌 측 AI
- 상태: `main 반영 완료`
- 변경:
  - `docs/TEAM_WORKFLOW.md`
  - `tasks/TEAM_BOARD.md`
  - `.github/CODEOWNERS`
  - `AGENTS.md`, `CLAUDE.md`
  - 다겸 전용 브랜치 `dagyeom/observer-ui-v2` 생성
- 역할:
  - 쭌 측: Core / AI / Simulation / World / Build / Integration
  - 다겸 측: Observer UI / Character presentation / Content/UI
- 상대가 알아야 할 점:
  - 역할 분담은 `docs/LIFELENS_SPEC_v1.1.md` 요구사항을 축소하지 않는다.
  - 공용 파일 및 상대 `DOING` 파일은 임의 수정 금지.

### Parallel Relationship Core v1 시작
- 작성자: 쭌 측 AI
- 브랜치: `jjun/relationship-core-v1`
- 상태: `DOING / 검증 대기`
- 목적:
  - TASK_03 Android 빌드를 기다리지 않고 제품 기능 개발을 병렬 진행.
  - MASTER SPEC 31의 단일 친밀도 금지 원칙과 13개 Relationship 차원을 Core에 반영.
- 변경 범위:
  - `Source/LifeLensCore/include/lifelens/Relationship.h`
  - `Source/LifeLensCore/tests/test_relationship.cpp`
  - `Source/LifeLensCore/CMakeLists.txt`
- 구현 내용:
  - Affection, Trust, Respect, Comfort, Familiarity, Attraction, RomanticInterest, SexualAttraction, Commitment, Conflict, Jealousy, Fear, Grudge
  - 방향성 관계(`from -> to`)로 모델링하여 두 사람의 감정이 자동 대칭이 되지 않음.
  - 사건 기반 RelationshipDelta와 0~1 clamp.
  - socialBond / romancePotential 파생 점수는 의사결정 보조값일 뿐 원본 13차원을 대체하지 않음.
- 검증:
  - Core CI를 PR에서 실행 예정. 아직 PASS로 간주하지 않음.
- 상대가 알아야 할 점:
  - 다겸 UI에서 향후 관계를 표시할 때 단일 `affinity`가 아니라 위 다차원 데이터가 기준이 된다.
  - 현재 다겸 UI 브랜치와 파일 충돌 없음. `Source/LifeLens/UI/**`는 수정하지 않음.

---

## 다음 인수인계 포인트

### 쭌 측이 다겸 측에 제공해야 하는 것
- PR #2 실제 UHT/UBT 결과
- Core Bridge가 `main`에 병합되면 다겸 UI에서 사용 가능한 읽기 API 목록
- Android fast pipeline의 seed/fast 실제 성공 여부와 사용법
- Relationship Core v1이 병합되면 UI용 Relationship read DTO/API 요청 형식

### 다겸 측이 쭌 측에 제공해야 하는 것
- `dagyeom/observer-ui-v2`에서 수정한 파일 목록
- UI가 추가로 필요로 하는 Simulation/Core read API
- 화면/터치/관찰 UX 확인 결과
- PR 생성 시 shared file 포함 여부
