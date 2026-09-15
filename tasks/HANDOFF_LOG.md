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

### Relationship Core v1 완료
- 작성자: 쭌 측 AI
- 브랜치/PR: `jjun/relationship-core-v1`, PR #4
- 상태: `DONE / main 병합 완료`
- 검증:
  - LifeLens Core Tests PASS
  - LifeLens Preflight PASS
- 병합 커밋: `a6e1c75d8f72ecd5aef0854525d1c9135d2dfc0d`
- 상대가 알아야 할 점:
  - Relationship 원본 상태는 13차원이며 UI/AI에서 단일 affinity로 축약해 원본을 대체하면 안 된다.
  - 읽기 DTO/API는 TASK_03 Bridge 통합 후 별도 노출한다.

### Emotion Core v1 완료
- 작성자: 쭌 측 AI
- 브랜치/PR: `jjun/emotion-core-v1`, PR #6
- 상태: `DONE / main 병합 완료`
- 변경:
  - Joy, Sadness, Anger, Fear, Embarrassment, Pride, Jealousy, Affection, Anxiety, Relief, Grief 11개 감정 차원
  - 사건 기반 EmotionEventType / EmotionDelta
  - 시간 감쇠 decay
  - 기존 요약 채널 valence/arousal 유지
  - `test_emotion` 추가
- 검증:
  - LifeLens Core Tests PASS
  - LifeLens Preflight PASS
- 병합 커밋: `c58b03a840c43be57c4a7b4173cd4b1b2aec3542`
- 상대가 알아야 할 점:
  - UI는 필요에 따라 세부 감정 또는 valence/arousal 요약을 표시할 수 있지만, 세부 감정 원본을 잃지 않는다.
  - TASK_03 Bridge/Build 및 다겸 UI 파일은 수정하지 않았다.

### Memory Core v1 시작
- 작성자: 쭌 측 AI
- 브랜치: `jjun/memory-core-v1`
- 상태: `DOING / 검증 대기`
- 목적:
  - MASTER SPEC 29~30의 Memory → Belief → Relationship → Decision 기반을 만들기 위한 기억 데이터 모델 구축.
- 예정 범위:
  - Who / What / Where / When / Emotion / Importance / Confidence / Witnessed / Source / Decay / Tags
  - 기억 신뢰도/중요도 시간 감쇠
  - 태그/인물/최근성 기반 recall score
  - C++17 Core 테스트
- 상대가 알아야 할 점:
  - 다겸 UI 파일과 충돌하지 않으며 Core 내부 전용 작업이다.
  - Memory read API는 TASK_03 Bridge 통합 후 별도 요청/노출한다.

### Memory Core v1 완료
- 작성자: 쭌 측 AI
- 브랜치/PR: `jjun/memory-core-v1`, PR #7
- 상태: `DONE / main 병합 완료`
- 변경:
  - MemoryRecord: Who / What / Where / When / Emotion / Importance / Confidence / Witnessed / Source / Decay / Tags
  - MemorySource: DirectWitness / ToldByOther / Inferred
  - 중요도·감정강도에 따른 시간 감쇠 보호
  - source reliability를 반영한 effectiveConfidence
  - 인물/태그/최근성/중요도/감정강도 기반 recallScore
  - bestRecall / recallAbove
  - 기존 `MemoryEntry` 이름은 alias로 호환 유지
  - `test_memory` 추가
- 검증:
  - LifeLens Core Tests PASS
  - LifeLens Preflight PASS
- 병합 커밋: `e60127e0f910c075ff824ae088304080c6e84f80`
- 상대가 알아야 할 점:
  - NPC는 직접 본 것, 전해 들은 것, 추론한 것을 동일한 확신도로 취급하지 않는다.
  - Memory 원본은 Core가 소유하고 UI에는 Bridge/read DTO를 통해 노출한다.

### Belief Core v1 시작
- 작성자: 쭌 측 AI
- 브랜치: `jjun/belief-core-v1`
- 상태: `DOING / 검증 대기`
- 목적:
  - SPEC 30의 `Memory → Belief → Relationship → Decision` 중 Memory에서 Belief로 가는 첫 연결 계층 구축.
- 예정 범위:
  - subject + proposition 단위 BeliefRecord
  - 지지/반박 evidence 누적
  - Memory effectiveConfidence / importance 기반 evidence strength
  - belief stance/confidence 계산 및 시간 갱신
  - C++17 Core 테스트
- 상대가 알아야 할 점:
  - 다겸 UI 영역 및 TASK_03 Bridge/Build 파일은 수정하지 않는다.
  - Belief는 향후 상세 주민 패널의 고급 정보가 될 수 있으나 기본 HUD에는 노출하지 않는다.

### Belief Core v1 완료
- 작성자: 쭌 측 AI
- 브랜치/PR: `jjun/belief-core-v1`, PR #8
- 상태: `DONE / main 병합 완료`
- 변경:
  - subject + proposition 기반 BeliefRecord
  - stance [-1,1], confidence [0,1]
  - supportWeight / contradictionWeight 분리 및 evidence count
  - Memory effectiveConfidence + importance 기반 evidence strength
  - MemorySource 신뢰도 차이가 Belief confidence에 연결
  - Character에 BeliefState 추가
  - `test_belief` 추가
- 검증:
  - LifeLens Core Tests PASS
  - LifeLens Preflight PASS
- 병합 커밋: `0295715f7449d76340c7244efd948afcf8aff74c`
- 상대가 알아야 할 점:
  - NPC가 Memory를 실제로 ingest하지 않으면 해당 Belief가 생기지 않는다.
  - 지지/반박 증거를 동시에 보존하므로 믿음이 단순 true/false로 고정되지 않는다.
  - 기본 Observer HUD에는 Belief 전체를 노출하지 않고, 캐릭터 상세/디버그 계층에서 필요 시 read DTO를 제공한다.

### Social Cognition Core v1 완료
- 작성자: 쭌 측 AI
- 브랜치/PR: `jjun/social-cognition-v1`, PR #9
- 상태: `DONE / main 병합 완료`
- 변경:
  - SocialEvent / SocialEventType 및 `processSocialEvent` 추가
  - 처리 순서: `Emotion → Memory → Belief → Relationship`
  - 직접 당함 / 직접 목격 / 전해 들음 / 추론을 perception scale로 차등 처리
  - MemorySource 신뢰도가 Belief confidence 및 Relationship 변화 강도에 이어지도록 연결
  - 제3자 목격은 개인 당사자보다 약한 observational Relationship 변화 적용
  - Intimacy/Rejection처럼 상충되는 사회 증거를 support/contradiction으로 함께 보존
  - `test_social_cognition` 추가
- 검증:
  - LifeLens Core Tests PASS
  - LifeLens Preflight PASS
- 병합 커밋: `4622760cc7025e88746240f2ff151bd5d01d6568`
- 상대가 알아야 할 점:
  - 관계 UI는 사건 하나가 즉시 단일 친밀도 숫자를 바꾸는 구조로 가정하면 안 된다.
  - 향후 상세 패널에서 직접 경험/소문/추론 여부를 표현할 수 있도록 read DTO 계층에서 source/confidence를 선택적으로 제공할 수 있다.
  - TASK_03 Bridge/Build와 다겸 UI 파일은 수정하지 않았다.

### Social Utility Core v1 완료
- 작성자: 쭌 측 AI
- 브랜치/PR: `jjun/social-utility-v1`, PR #10
- 상태: `DONE / main 병합 완료`
- 변경:
  - SocialIntent: Approach / Avoid / Repair / Comfort
  - Relationship / Emotion / Memory / Belief / Personality를 종합한 social utility 계산
  - 기존 Physical Goal score와 Social Utility를 비교하는 Unified Utility Decision 추가
  - 긴급 Needs는 Social보다 우선할 수 있도록 유지
  - `test_social_utility` 추가
- 검증:
  - LifeLens Core Tests PASS
  - LifeLens Preflight PASS
- 병합 커밋: `20e1537bbf0fee603203d484a48a712d7538336c`
- 상대가 알아야 할 점:
  - 캐릭터 현재행동 UI는 앞으로 Physical Goal만 가정하면 안 되고 SocialIntent도 표현할 수 있어야 한다.
  - 기본 Observer 화면에는 요약 행동만 표시하고 세부 utility는 디버그/상세 계층에서만 필요하다.

### Social Execution Core v1 완료
- 작성자: 쭌 측 AI
- 브랜치/PR: `jjun/social-execution-v1`, PR #11
- 상태: `DONE / main 병합 완료`
- 변경:
  - `DecisionExecution.h` 추가
  - Approach → PositiveInteraction, Repair → Apology, Comfort → Comfort 사건 실행
  - Avoid는 상대에게 가짜 Memory를 생성하지 않고 actor의 fear/anxiety를 일부 완화
  - Physical 선택은 기존 Planner 경로로 넘길 수 있도록 분리 유지
  - `test_decision_execution` 추가
- 검증:
  - LifeLens Core Tests PASS
  - LifeLens Preflight PASS
- 병합 커밋: `8123a6904afff5a0dcfd550536d2b8c81280f350`
- 상대가 알아야 할 점:
  - SocialIntent는 이제 점수만 있는 값이 아니라 실제 Core 상태/사회 사건을 바꾸는 실행 단위다.
  - 다겸 UI가 현재행동을 표시할 때 Approach/Repair/Comfort/Avoid 상태를 받을 수 있도록 Bridge DTO가 필요하다.

### Simulation Social Loop v1 완료
- 작성자: 쭌 측 AI
- 브랜치/PR: `jjun/simulation-social-loop-v1`, PR #12
- 상태: `DONE / main 병합 완료`
- 변경:
  - `Simulation`이 `RelationshipBook`을 소유하도록 확장
  - 기존 1인 물리행동 `setupDemo()` 유지 + `setupSocialDemo()` 별도 추가
  - 5분 의사결정 루프에서 Unified Utility가 Social을 고르면 실제 SocialIntent 실행
  - social cooldown으로 반복 상호작용 스팸 방지
  - Social action 로그에 intent/target/utility 기록
  - `test_simulation_social_loop` 추가, 같은 seed 결정론 확인
- 검증:
  - LifeLens Core Tests PASS
  - LifeLens Preflight PASS
- 병합 커밋: `c4d946e0e70f2b4125648890b82c8f5e3fb81d5e`
- 상대가 알아야 할 점:
  - Core Simulation 자체에서 이제 Physical Needs와 Social Cognition이 같은 tick 흐름 안에서 경쟁하고 실제 행동으로 이어진다.
  - 향후 Unreal Bridge는 이 로그/현재 SocialIntent/관계·감정 read DTO를 노출하면 Observer UI가 소비할 수 있다.
  - TASK_03 Android Run `34739283266`은 이 변경보다 이전 SHA로 시작했으므로 현재 실행에는 포함되지 않는다.

### Observer Read Model v1 완료
- 작성자: 쭌 측 AI
- 브랜치/PR: `jjun/observer-read-model-v1`, PR #13
- 상태: `DONE / main 병합 완료`
- 변경:
  - `ObserverReadModel.h` 추가
  - `ResidentObservation`: id/name/Needs/emotion valence·arousal·intensity/current activity/target 제공
  - `RelationshipObservation`: 13개 관계 차원 + socialBond/romancePotential + targetName 제공
  - `Simulation::observeResident()` / `observeAllResidents()` 추가
  - Social action 실행 중 `Social + intent + target` 상태를 보존하고 action 종료 시 해제
  - `test_observer_read_model` 추가
- 검증:
  - LifeLens Core Tests PASS
  - LifeLens Preflight PASS
  - Deterministic harness PASS
- 병합 커밋: `1887fd8b816900123856b74828237dcf9ec14ad1`
- 상대가 알아야 할 점:
  - 다겸 UI는 Core 내부 포인터/컨테이너 대신 이 read DTO를 기준으로 표시 로직을 설계하면 된다.
  - 현재 DTO에는 Core가 가진 이름/Needs/감정/현재행동/관계가 포함된다. 나이/성별 등 Unreal ResidentData 전용 필드는 Bridge에서 기존 데이터와 합쳐 제공해야 한다.
  - 다음 쭌 측 통합 작업은 TASK_03 Bridge에서 이 DTO를 Unreal USTRUCT/읽기 API로 변환하는 것이다.

---

## 다음 인수인계 포인트

### 쭌 측이 다겸 측에 제공해야 하는 것
- PR #2 실제 UHT/UBT 결과
- Core Bridge가 `main`에 병합되면 다겸 UI에서 사용 가능한 읽기 API 목록
- Android fast pipeline의 seed/fast 실제 성공 여부와 사용법
- `ResidentObservation`을 Unreal Bridge에서 변환한 UI용 read DTO/API 목록

### 다겸 측이 쭌 측에 제공해야 하는 것
- `dagyeom/observer-ui-v2`에서 수정한 파일 목록
- UI가 추가로 필요로 하는 Simulation/Core read API
- 화면/터치/관찰 UX 확인 결과
- PR 생성 시 shared file 포함 여부


## 2026-09-14 — usage-limit interruption recovery

- Author: 쭌 측 AI.
- Verified remote main `4e8f50d92bb7eb08884122a001ac73174d057703` and P24 branch `48d6142834ad2fe2b34b228cec57e092e0577e36`.
- PR #34 is open/mergeable; Core Tests `34792467903` and Preflight `34792467916` succeeded. WORK_STATE incorrectly said implementation had not started; corrected before further code edits.
- TASK_03 PR #2 remains FROZEN. Run `34739283266` is completed/failure; no rerun or branch changes.
- Read-only discovery also confirmed open Dagyeom PRs #17, #26, #29, #30. Their branches are not ours to change.
- Next checkpoint: review P24 code/tests, then record merge SHA; new integration work starts on a separate branch.
- Interruption policy: push each coherent change before external waits, record exact branch/HEAD/PR/check IDs/next command. Chat availability does not imply a background AI is still working; re-read GitHub on resume.


### 2026-09-14 — P24 merge held: assertions exposed a Core memory defect

- Author: 쭌 측 AI; branch `jjun/core-validation-recovery-v1` (base `6ee98d6`).
- CMake is absent locally; used installed g++ C++17 with assertions enabled. `test_observer_read_model_v2` passed, but `test_relationship` aborted at line 16 after reverse-relation insertion.
- Root cause: RelationshipBook::getOrCreate returns a vector element reference, then subsequent insertion reallocates it. Existing test holds both directions. This is a real dangling reference, not a P24 DTO failure.
- CI root cause: core-tests.yml configures Release; CMake defines NDEBUG, eliminating legacy assert checks (including expressions with side effects). Old PASS cannot establish assertion-based correctness.
- Changed priority: repair this bounded Core/validation defect before merging P24 or starting Unreal read APIs. TASK_03 remains FROZEN.
- Next: reference-stable storage, Release assertion guard, full tests and targeted ASan/UBSan, then PR/CI and durable checkpoint.

### Core validation recovery — first implementation checkpoint

- RelationshipBook now uses insertion-ordered `std::deque`; returned references/pointers survive subsequent insertions. Iterators still must not span insertions. All existing `all()` consumers use range iteration; no explicit vector consumers were found.
- Added growth regression with 1,000 inserted relationships and both directed references retained. Original code reproduced AddressSanitizer `heap-use-after-free`; corrected targeted ASan/UBSan run passed.
- All Core test targets use `lifelens_add_test`, which undefines NDEBUG even in Release. A new `test_assertions_enabled` fails compilation if this guarantee disappears.
- Structural preflight PASS. Full CMake Release suite and deterministic harness pending at this checkpoint; local CMake was installed in scratch to validate the actual configuration.
- Next: full Release CTest, confirm optimized test flags include `-UNDEBUG`, PR/CI. No Unreal/UI/TASK_03 files changed.

### P24 Observer Read Model v2 — refreshed validation checkpoint

- PR #35 merged as `31b2263de3f4a9c80650d1139213d0c38acc8058`; main Core CI `34793860254` and Preflight `34793860178` passed.
- Merged main checkpoint `bccb8b149877b87146700a7fdf5aacf12e9c0a84` into P24, resolving the CMake conflict by keeping every main test and registering `test_observer_read_model_v2` with `lifelens_add_test`.
- 27/27 local CMake Release tests passed; all 27 compiled with assertions active. Same-seed one-day harness and structural preflight passed. P24 head CI is the next gate.
- P24 provides 11 emotion axes plus summaries; family partner/history/cohabitation/pregnancy/parent-child-sibling copies; population/life-stage/household/couple/pregnancy counts and major LifeHistory record count.
- Scope: these are Core read DTOs, not a live Unreal bridge. `majorLifeEvents` counts individual LifeHistory entries, not deduplicated world events. The DTO does not yet expose GenerationContinuity; the separate existing assessment API remains available.
- Integration finding: Core `Simulation` currently owns World/RelationshipBook but no family books; Unreal `ULLSimulationSubsystem` still owns its separate resident/save data. A future bridge must first establish authoritative state and GUID mapping, preserve save/load identity, and avoid presenting absent Core data as a real empty family.
- Frozen TASK_03 and all Dagyeom branches remain untouched. Next: PR #34 refreshed CI → merge → record exact next integration task in main.

### 2026-09-14 — Observer Runtime Bridge v1 checkpoint

- 작성자: 쭌 측 AI
- 브랜치/PR: `jjun/observer-runtime-bridge-v1`, PR #37
- 기준: P24가 병합된 `main` merge SHA `b4403faf138c153bcd83be266cde5026ea53b831`에서 분리 시작.
- 변경 범위:
  - `Source/LifeLens/Simulation/LLCoreReadTypes.h`
  - `Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h/.cpp`
  - `Source/LifeLens/Simulation/LLCoreCompileUnit.cpp`
  - `Source/LifeLens/LifeLens.Build.cs`
  - `Tools/validate_bootstrap.py`
  - 상태/인수인계 문서
- 구현:
  - 최신 LifeLensCore `ResidentObservation` / P24 emotion observation을 Unreal Blueprint/read DTO로 투영.
  - Needs, 11개 Emotion 축, valence/arousal/intensity, Physical/Social activity와 target, 방향성 Relationship 13차원, socialBond/romancePotential, Memory/Belief count, 기본 world resident count를 읽기 전용으로 제공.
  - 식별은 이름이 아니라 `(WorldSeed, Core CharacterId)`로 결정론적 `FGuid`를 생성해 유지.
  - Core는 표준 C++17로 유지하고 Unreal reflection/type은 Bridge 쪽에만 둠.
  - 현재 Core `Simulation`이 family books를 권위 상태로 소유하지 않으므로 Family/Household/Romance/Pregnancy aggregate를 가짜 빈 데이터로 노출하지 않음.
- 검증:
  - PR #37 head `1cd3139d4756386d4bd4ee3a3b59d2c74467ed7a` 기준 LifeLens Preflight Run `34794889890` PASS.
  - Preflight는 Core/Unreal 경계 역침투 검사와 필수 Bridge 계약을 포함함.
  - 현재 세션 로컬 컨테이너는 외부 GitHub DNS가 차단되어 checkout 검증은 실행하지 못했음; 코드 실패가 아님.
  - 실제 Unreal UHT/UBT는 아직 PENDING이며 APK/패키징 성공을 주장하지 않음.
- 상대가 알아야 할 점:
  - `Source/LifeLens/UI/**`는 수정하지 않았고 다겸 UI 브랜치와 직접 충돌시키지 않음.
  - 다겸은 이 PR 병합/컴파일 검증 후 `GetResidentObservations`, `GetResidentObservation`, `GetWorldObservation`을 소비할 수 있음.
  - 기존 TASK_03 PR #2와 Android Run `34739283266`은 사용자 지시대로 FROZEN 상태 그대로이며 수정/재실행하지 않았음.
- 다음 행동:
  - 문서 checkpoint 반영 후 최신 PR head의 Preflight를 다시 확인.
  - UHT/UBT 검증 경로는 FROZEN TASK_03을 되살리지 않고 별도 최신-main 검증 작업으로 처리.

### 2026-09-14 — Production New Game + Autonomous Family progression completion

- 작성자: 쭌 측 AI
- PR #42: `jjun/production-new-game-runtime-v1`, merge `5c6f2c071ca00bcd4b26d6e002bf5c618d02ff1e`.
- PR #43: `jjun/autonomous-family-progression-v1`, feature head `81dbb57d190810cc3318e82ba952064d50fd8fdc`, merge `179e3a65aaa6ff8d2243117c7aebfd760812c73d`.
- 변경 범위:
  - #42: production New Game를 Core-authoritative로 전환하고 founder Sex/Age/LifeStage/Personality를 Unreal read DTO에 노출.
  - #43: Core-only family progression. 관계/성격 기반 chemistry → dating → cohabitation → engagement → marriage → pregnancy → birth를 `Simulation` runtime에 연결.
  - 출산된 child는 실제 World resident가 되며 genetics / genealogy / household / parent-child links / LifeHistory / runtime state를 보유.
  - marriage는 Genealogy spouse link를 갱신하고 Simulation은 authoritative BirthBook을 소유.
- 검증:
  - #42 Preflight `34799244015` PASS; Unreal Linux Compile `34799244011` PASS including actual UHT/UBT.
  - #43 Core Tests `34801572846` PASS including Configure / Build / Test / Deterministic harness smoke.
  - #43 Preflight `34801572858` PASS.
  - #43 first failed Core run `34801444613` was a duplicate helper-name compile collision only; fixed in final head and superseded by the passing run.
- 상대가 알아야 할 점:
  - NEW GAME는 4명으로 시작하지만 이제 출산으로 `World::characters`가 증가할 수 있으므로 UI/Presentation에서 인구 4명 고정을 가정하면 안 된다.
  - 기존 Observer family/world read API는 실제 자동 진행 상태를 읽게 된다.
  - 다겸 영역 파일은 #42/#43에서 수정하지 않았다.
  - old TASK_03 PR #2 / Run `34739283266`은 FROZEN 그대로다.
- 다음 쭌 측 우선순위:
  - Full Core Save/Load v1: 진화된 전체 Core 상태를 snapshot/restore하고 load 후 deterministic continuation을 검증한 뒤 Unreal SaveGame을 adapter로 전환한다.

### 2026-09-14 — Full Core Save/Load + Unreal SaveGame v2 completion

- 작성자: 쭌 측 AI
- PR #44: `jjun/core-save-load-v1`, feature head `64d2fb25f6453112aabaef2d809b0528c9dc4566`, merge `2b3f9882703ed73cb8318ae262f26bebb995c209`.
- PR #45: `jjun/unreal-savegame-adapter-v1`, feature head `547b3f94e0c3df6df15d723e72c8084cd10a7c29`, merge `3c646ba331b8199a295fd6f2e9cac1235d844679`.
- 변경 범위:
  - #44: `SimulationStateSnapshot v1`로 World/RNG/Character 전체 상태, Relationship/Genealogy/Romance/Household/Pregnancy/Birth books, runtime plan/cooldown/position/social state, Core logs를 capture/restore.
  - #45: versioned Core binary codec(`LLSNAP01`)과 Unreal SaveGame v2 adapter를 추가하여 실제 파일 저장 경로가 Core snapshot bytes를 권위 상태로 저장/복원.
  - v2 Load는 임시 Core candidate에서 decode+restore 검증 후 live world를 교체하고, compatibility projection/GUID index를 복원된 Core에서 다시 생성.
  - v1 old save는 migration 용도로만 seed+minute replay를 유지하며 legacy Residents/Relationships 배열은 권위 상태로 복원하지 않음.
  - UI/Characters/Content 파일은 수정하지 않음.
- 검증:
  - #44 Core Tests `34802611336` PASS incl Configure / Build / Test / deterministic harness; Preflight `34802611299` PASS.
  - #44 rich-state deep roundtrip + unsupported-version rejection + load 후 추가 10,000분 deterministic continuation PASS.
  - #45 Core Tests `34803226434` PASS incl deterministic harness.
  - #45 Structural Preflight `34803226458` PASS.
  - #45 Unreal Linux Compile `34803226433` PASS including actual UE 5.6 UHT + UBT.
  - snapshot binary canonical roundtrip, corrupt/truncated/trailing payload rejection, post-load continuation test PASS.
- 상대가 알아야 할 점:
  - 저장/로드 후 UI는 cached compatibility 배열이 아니라 현재 `ULLCoreBridgeSubsystem` read DTO를 다시 읽어 화면을 구성해야 한다.
  - `(WorldSeed, Core CharacterId) -> FGuid` 안정 식별 의미는 restore 후에도 유지된다.
  - NEW GAME 4명 고정 가정은 금지이며 출산/세대 진행으로 주민 수가 증가할 수 있다.
  - old TASK_03 PR #2 / Run `34739283266`은 계속 FROZEN이다.
- 다음 쭌 측 우선순위:
  - Core Decision → Unreal Physical Action Bridge v1: Core가 결정한 물리/사회 행동을 Unreal World/AI가 실행·표현하게 하여 남아 있는 split-brain을 제거한다.

### 2026-09-14 — Core Physical Action Bridge + Witness/Rumor completion

- 작성자: 쭌 측 AI
- PR #47: `jjun/witness-rumor-core-v1`, feature head `8c03b349d8489a847b4e4fb7e22dc974e91be8cf`, merge `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c`.
- PR #46: `jjun/core-physical-action-bridge-v1`, feature head `31c00cb0d751bb33825df31c7e758e60ff54402c`, merge `753df19657ea634ea2fa7c2ac935f6273ce14c10`.
- 변경 범위:
  - #47: pure-Core `SocialFact`, `KnowledgeReceipt`, `SocialStatement`, `SocialKnowledgeBook`와 direct/heard provenance, deterministic retell attenuation/distortion, trust-sensitive acceptance, duplicate/loop suppression, Memory/Belief integration을 추가.
  - #46: Core `ResidentObservation`의 typed physical/social authority를 Unreal `FLLCoreActionDirective`로 투영하고, `ALLWorldDirector`가 legacy `ChooseAction()` 및 projection-only outcome mutation 대신 Core directive를 미러하도록 전환.
  - #46 physical intents: Eat / Drink / Sleep / Toilet / Hygiene. Social intents: Approach / Avoid / Repair / Comfort.
  - 기존 `ELLActionIntent` ordinal 0–6은 보존하고 `Drink`는 7로 append하여 Blueprint/persisted compatibility를 유지.
  - #46 changed files는 9개 Jjun-owned Core/Simulation/World/validator이며 Dagyeom UI/Character presentation/Content는 수정하지 않음.
  - #47 changed files는 정확히 3개 pure-Core 파일이며 #46과 파일 중첩 없음.
- 검증:
  - #47 Core Tests `34806559374` PASS incl Configure / Build / Test / deterministic harness.
  - #47 Structural Preflight `34806559379` PASS.
  - #46 Core Tests `34805882778` PASS incl deterministic harness.
  - #46 Structural Preflight `34805882776` PASS.
  - #46 Unreal Linux Compile `34805882789` / Run #10 PASS; UE 5.6 image verify, UHT, UBT 전부 SUCCESS.
- 상대가 알아야 할 점:
  - 현재 행동의 simulation authority는 covered intents에 대해 Core이며, UI/Character presentation에서 별도 action chooser를 만들면 안 된다.
  - 다겸은 `ULLCoreBridgeSubsystem::GetResidentActionDirective()`로 typed current action + stable target ResidentId를 읽을 수 있다.
  - #47은 아직 domain-only이므로 rumor/witness UI 필드를 가정하면 안 된다. live Simulation/read DTO wiring은 후속 작업이다.
  - #46은 compile 검증까지 완료했지만 실제 PIE/실기기에서 이동/anchor/animation 상호작용 품질 검증은 후속 runtime QA가 필요하다.
  - old TASK_03 PR #2 / Run `34739283266`은 계속 FROZEN이다.
- 다음 쭌 측 후보:
  - Physical Interaction / Smart Object Execution v1 또는 Witness / Rumor Runtime Wiring v1. 새 작업은 state lock + 별도 branch 생성 전까지 시작된 것으로 간주하지 않는다.

### 2026-09-14 — World Affordance + Civilization Foundation + Persistence completion

- 작성자: 쭌 측 AI
- PR #48: `jjun/physical-smart-object-v1`, feature head `505e356d277cb0896109f6d4cdd75bcf06cdab54`, merge `a90ff6a5d870858a9e555ddf1e6e526bb7aa34e1`.
- PR #49: `jjun/civilization-foundation-v1`, feature head `ac8e0868f819a49d8b2c0aec947978258da56028`, merge `36bd1ac81192bc689c1e811553f068f255642508`.
- PR #50: `jjun/civilization-runtime-state-v1`, feature head `543ee006a48d8a97d7cf37de65f03c74c1b60e3e`, merge `c31c422c305a3a79a9553d37ac86247aa31d1853`.
- 변경 범위:
  - #48: reservable World Affordance execution infrastructure. Current modern-looking bootstrap anchors are development-only and the same reservation/use layer is intended for resource nodes, fires, work surfaces, storage, crafting stations, tools, machines and furniture.
  - #49: pure-Core material/resource/inventory/storage/personal-knowledge/experiment/discovery/crafting foundation. Technology begins as individual knowledge; no global recipe unlock or forced era gate.
  - #50: `Character` owns authoritative `IndividualCivilizationState`; `World` owns ResourceNode/StorageSite collections. NEW GAME natural resource substrate includes Stone / Flint / Wood / Fiber / Clay / Water / PlantFood.
  - #50 snapshot binary format v2 persists resource depletion, resident inventory, personal Knowledge level/confidence/practice and shared storage. Legacy binary v1 decode remains supported with deterministic civilization migration.
  - `docs/CIVILIZATION_PROGRESSION_v1.md` is canonical alongside the original master spec.
- 검증:
  - #48 Structural Preflight `34808292399` PASS; Unreal Linux Compile Run #14 `34808292682` PASS including actual UE 5.6 UHT+UBT.
  - #49 Core Tests `34809360826` PASS incl Configure / Build / Test / deterministic harness; Preflight `34809360739` PASS.
  - #50 Core Tests `34810329867` PASS incl Configure / Build / Test / deterministic harness; Preflight `34810329862` PASS.
- 상대가 알아야 할 점:
  - LifeLens는 완성된 현대 가정에서 시작하는 게임으로 고정하지 않는다. 장기 방향은 자원 채집 → 저장 → 실험/발견 → 개인 지식 → 제작 → 지식 전파 → 세대 누적 문명이다.
  - 다겸 UI/Character Presentation은 원시 자원/도구부터 이후 기술까지 수용 가능한 데이터 주도형 표현을 유지한다.
  - #50의 Inventory/Knowledge/Resource 상태는 Core 내부 권위 상태로 존재하지만 아직 Observer/Unreal read DTO로 노출되지 않았으므로 UI에서 placeholder를 만들지 않는다.
  - 현재 main HUD는 계속 observer-first로 단순하게 유지한다.
  - old TASK_03 PR #2 / Run `34739283266`은 계속 FROZEN이다.
- 다음 쭌 측 잠금:
  - **Autonomous Civilization Action Loop v1** — Core에서 Needs / curiosity / inventory / personal knowledge / world resources를 바탕으로 Gather / Store / Experiment / Craft를 자율 선택·실행하도록 연결한다. 이후 knowledge transmission과 Observer read DTO로 이어간다.

### 2026-09-14 — Autonomous Civilization Action Loop completion

- 작성자: 쭌 측 AI
- PR #51: `jjun/autonomous-civilization-loop-v1`, feature head `729536ac3f8f2222e68c0d0d1a1726c98ffe19fa`, merge `55d5211160c8edad32b01177e2b9326a9faa2b78`.
- 변경 범위:
  - pure-Core `CivilizationIntent` Gather / Store / Experiment / Craft를 Physical/Social과 분리된 세 번째 utility 축으로 추가.
  - 긴급 Needs가 있으면 생존이 문명보다 우선하며, 문명 행동은 15분 decision slot에서만 경쟁하여 기존 생활/사회 루프를 굶기지 않는다.
  - Gather는 실제 finite ResourceNode를 감소시키고 Inventory와 gathering skill을 갱신한다.
  - Store는 carrying pressure가 높을 때 surplus를 shared StorageSite로 옮긴다.
  - Experiment는 개인 Knowledge/prerequisite/material을 검사하며 실패 시 재료 소모 + Hypothesized, 성공 시 해당 주민 개인만 Reproducible discovery를 얻는다.
  - Craft는 개인 Reproducible 기술과 실제 입력재료가 있을 때만 재현된다.
  - WorldSeed + CharacterId 기반 개인 preference와 simulation minute 기반 experiment roll로 별도 persisted counter 없이 결정론을 유지한다.
  - renewable ResourceNode는 일 단위로 재생되고, repeated NEW GAME는 resource/storage를 초기 상태로 reset한다.
  - Civilization event는 Core log에 deterministic하게 기록한다.
- 검증:
  - Core Tests `34811869666` PASS: Configure / Build / 전체 Test / deterministic harness smoke.
  - Structural Preflight `34811869612` PASS.
  - 전용 테스트는 urgent hunger survival priority, personal knowledge divergence, autonomous Store, Experiment→Discovery→Craft, 20,000분 NEW GAME 진행, same-seed canonical bytes, Save/Load 후 추가 2,500분 exact continuation, repeated NEW GAME reset을 검증한다.
- 상대가 알아야 할 점:
  - 이제 문명 기능은 데이터만 존재하는 것이 아니라 실제 Core residents가 자율적으로 실행한다.
  - Civilization을 SocialIntent에 섞지 않았으므로 기존 Observer의 Social 의미가 오염되지 않는다.
  - 아직 Civilization current-action/read DTO는 Unreal/UI에 공개하지 않았으므로 다겸 UI는 placeholder를 만들지 않는다.
  - old TASK_03 PR #2 / Run `34739283266`은 계속 FROZEN이다.
- 다음 쭌 측 잠금:
  - **Civilization Knowledge Transmission v1** — 발견/제작 지식이 개인에게 고립되지 않도록 #47 Witness/Rumor provenance를 재사용해 목격·모방·직접 교육으로 불완전하게 전파한다.

## 2026-09-14 — Observer HUD v2 Integration Sprint R1-R3 완료

- 작성자: 쭌 측 AI / 다겸 협업 지원
- 대상: `dagyeom/observer-ui-v2`, PR #17
- 상태: `DONE / main 병합 완료`
- 최종 PR #17 head: `5eaff7dd6d579606331919b89b0060a776a7ad80`
- main merge: `aa194db7c5b500cdf5041fd6d43b25f97b9dd0b6`
- R1: helper #56 merged; verify-only #57 closed; Preflight `34824371968` PASS; Unreal Run #17 `34824371965` PASS (UHT/UBT/link)
- R2: helper #58 merged; verify-only #59 closed; corrected Preflight `34833994138` PASS; corrected Unreal Run #20 `34833994155` PASS (UHT/UBT/link)
- 최초 R2 Run #18 실패는 verify tree에서 R1 UI 4파일을 누락한 검증 구성 오류였고 제품/helper 코드 오류가 아님.
- R3: PR 설명 최신화, Codex review thread 2건 resolved/unresolved 0, `ASSIST_LOCK-17-R2` 해제 및 UI 소유권 다겸 복귀.
- 반영: LEVEL 0은 얇게 유지, LEVEL 1 narrow-screen wrapping 유지, LEVEL 2에 Core-backed Emotion/Relationships/Family/Knowledge & Gear(Civilization) 상세 제공.
- 현재 행동은 Core resident observation 우선, legacy actor intent는 transitional fallback.
- UI는 read-only presentation이며 simulation authority/cache를 새로 만들지 않음.
- 상대가 알아야 할 점: 쭌 측 Observer blocker 0개. 다음 순서 PR #26 → #29/#30 → #36 → #38 → integrated runtime → Android smoke APK.

## 2026-09-15 — Primitive sanitation experimentation progression 완료

- 작성자: 쭌 측 AI
- 브랜치/PR: `jjun/primitive-sanitation-progression-v1`, PR #78
- 상태: `DONE / main 병합 완료`
- validated head: `e78f6d211589b74008ff0fdf73e9be1048e2d807`
- main merge: `3a9739682034ad7c5009da5f75a11b0f07fed55b`
- 변경 범위:
  - `PrimitiveSanitation.h`를 추가해 #77의 resident sanitation-problem Belief를 실제 문명 실험 전제조건으로 소비.
  - `TechniqueId::DesignatedSanitationArea` / `ExperimentKind::DesignateSanitationArea`를 기존 Civilization experiment 체계에 추가.
  - 실험은 **문제 인식 + 낮은 오염도 후보지**가 모두 있을 때만 가능.
  - 실패는 `Hypothesized`, 성공은 발견자 개인의 `Reproducible` 지식으로 남음.
  - 기존 witness/teaching provenance 및 Civilization observer 범위를 새 기술까지 확장.
  - 기존 Civilization snapshot extension으로 개인 기술 지식 지속성 보존.
  - 전역 `LatrineUnlocked` 또는 자동 Toilet/Latrine SmartObject 생성 없음.
- 검증:
  - Structural Preflight run `34921390473` PASS.
  - Core Tests run `34921390510` PASS, **43/43** including `test_primitive_sanitation_progression`.
  - deterministic harness smoke PASS.
  - Unreal Linux Compile run `34921390491` PASS; UE 5.6 image verify + UHT + UBT + link PASS.
- 상대가 알아야 할 점:
  - 지정 배변구역은 아직 **개인 발견 지식**이며 실제 persistent facility/affordance는 다음 슬라이스에서 생성한다.
  - 다른 주민에게는 기존 목격/교육 경로로만 지식이 전달되며 세계 전역 기술 해금은 없다.
  - 다음 쭌 레인: learned `DesignatedSanitationArea` + authoritative Core GridPos를 persistent Core-owned sanitation affordance로 materialize하고 World가 같은 위치를 소비하도록 연결.
  - 다겸 #67 branch/UI/Character 파일은 건드리지 않았다.

## 2026-09-15 — Designated sanitation area authoritative affordance 완료

- 작성자: 쭌 측 AI
- 브랜치/PR: `jjun/designated-sanitation-affordance-v1`, PR #79
- 상태: `DONE / main 병합 완료`
- validated head: `49c76863b1957df09b7d814f7117826e92707021`
- main merge: `90f2b4f9cdc480e99886632e09323b2feab9625c`
- 변경 범위:
  - `DesignatedSanitationArea` Reproducible 지식은 자동 시설 생성이 아니라 기존 Civilization `Craft` 축을 실제 실행해야 Core-owned `PrimitiveSanitationSite`로 materialize됨.
  - site는 stable id, authoritative GridPos, establisher/minute, active flag, useCount를 Core가 소유.
  - Core `SanitationUseTarget` / Unreal `GetSanitationUseTarget(...)`로 designated flag + site id + exact GridPos를 한 번에 전달.
  - World affordance 우선순위는 authored Preferred/Primitive → Core designated Primitive → Natural → Emergency로 유지.
  - designated completion은 exact active site id + GridPos가 모두 일치해야 하며 stale/wrong target은 fail-closed.
  - 성공한 designated use는 실제 ACK 위치에 HumanWaste residue를 남기고 site useCount를 증가시킴.
  - snapshot binary format v5 + primitive sanitation extension으로 site identity/location/active/useCount를 Save/Load 보존; v1-v4는 site 없음으로 호환 decode.
  - global `LatrineUnlocked`나 자동 modern Toilet/Latrine SmartObject는 추가하지 않음.
- 검증:
  - Structural Preflight run `34923460056` PASS.
  - Core Tests run `34923459974` PASS, **44/44** including `test_designated_sanitation_affordance`.
  - deterministic harness smoke PASS.
  - Unreal Linux Compile run `34923459949` PASS; UE 5.6 image verify + UHT + UBT + link PASS.
  - 첫 Core run의 2개 실패는 기존 테스트가 snapshot format `4`를 숫자로 하드코딩한 회귀였고, `SimulationSnapshotBinaryFormatVersion` 상수 사용으로 수정 후 최종 44/44 PASS. 새 #79 affordance 테스트 자체는 첫 run에서도 PASS였음.
- 상대가 알아야 할 점:
  - 이제 지정 배변구역은 지식만이 아니라 실제 persistent Core affordance이며 World가 같은 위치/identity를 소비한다.
  - 다음 쭌 레인은 **Dug pit / primitive latrine progression — READY_NOW**. 아직 새 브랜치는 만들지 않았고 구현 시작으로 간주하면 안 된다.
  - 다음 단계에서도 실제 지식/재료/도구/작업 없이 pit/latrine이 즉시 생기면 안 되며, primitive containment의 위생 개선 효과는 환경 residue와 연결해야 한다.
  - 다겸 PR #67 / UI / Character appearance / Content 영역은 #79에서 수정하지 않았다.

## 2026-09-15 — sanitation progression / scalable world design closeout

### 2026-09-15 — PR #80 Dug pit progression + World Genesis canonicalization
- 작성자: 쭌 측 AI
- 브랜치/PR: `jjun/primitive-latrine-progression-v1`, PR #80
- 상태: `DONE / main 병합 완료`
- validated head: `fc918693bcd265f3c021f0bd826f6ba5a7113678`
- main merge: `291926cf78d12c1c61284eb9c59a50e7c70e54e7`
- 구현:
  - 개인 `DugSanitationPit` 지식/실험을 추가하고 기존 지정 배변구역을 같은 site ID/GridPos의 `DugPit`으로 개선.
  - 지식만으로 즉시 시설이 바뀌지 않으며 실제 반복 굴착 작업량이 필요함.
  - 현재 runtime에 실제 Dig 도구가 없으므로 v1은 느린 수작업 굴착을 허용하고 향후 Dig 도구가 같은 work contract를 가속하도록 설계.
  - DugPit 완성은 기존 HumanWaste 양을 삭제하지 않고 노출 강도/확산 반경을 줄이는 containment 효과를 적용.
  - 이후 DugPit 사용도 HumanWaste를 계속 생성하지만 open designated area보다 낮은 exposure profile을 사용.
  - snapshot 외부 포맷 v5 유지, primitive sanitation extension만 v2로 확장하고 v1 호환 decode 유지.
  - `DesignatedSanitationArea`와 `DugSanitationPit`이 Core/Unreal civilization read 범위에 모두 포함됨.
- 검증:
  - Structural Preflight run `34926841905` PASS.
  - Core Tests run `34926841895` PASS, **45/45**.
  - deterministic harness smoke PASS.
  - Unreal Linux Compile run `34926841907` PASS; UE 5.6 image verify + UHT + UBT + link PASS.
- 추가 canonical design:
  - `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`를 신규 canonical companion으로 확정.
  - 최종 월드는 작은 고정 arena가 아니라 `WorldSeed -> Macro World -> deterministic lazy chunks -> persistent human/environmental change -> carrying-capacity pressure -> migration -> additional settlements -> regional society/civilization` 구조를 따른다.
  - untouched chunk natural baseline은 `WorldSeed + ChunkCoord`로 탐사 순서와 무관하게 결정론적으로 생성.
  - 생성 후 인간/자연 변화는 persistent history가 권위이며 reload 시 reroll 금지.
  - 인구 증가 시 고정 맵에 압축하지 않고 토지 이용 고도화, 탐사, 이주, 분가/집단 분리, 복수 정착지 형성으로 확장.
  - 초기 자연환경에는 현대/정착 인프라가 없고 생존 가능한 시작 후보지만 평가한다.
  - WorldSeed와 초기 PopulationSeed/stream을 분리하여 같은 자연환경에서 다른 창립자 구성도 향후 지원 가능하게 한다.
  - Android는 논리 세계 전체를 고품질 Actor로 유지하지 않고 active presentation chunks / simulation LOD를 사용한다.
- 다음 쭌 레인:
  - HumanWaste Environmental Visual Feedback — READY_NOW.
  - World Genesis WG-1(seed/macro/chunk contracts)과 WG-2(persistence/streaming boundary)는 production World Visual Environment가 고정 맵 가정에 묶이기 전에 gate로 구현/확정.
- 상대가 알아야 할 점:
  - 다겸 PR #67 / Character/UI/Content 영역은 #80에서 수정하지 않았다.
  - World Visual Environment 작업은 `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`를 반드시 읽고 고정 소형맵을 제품 구조로 굳히지 않는다.

## 2026-09-15 — Character Appearance v1 PR #67 closeout 완료

- 작성자: 쭌 측 AI / 다겸 lane assist
- 상태: `DONE / main squash merge 완료`
- PR #67: `dagyeom/character-appearance-v1`
- final head: `982d930a53f199b33ebf7ca3d4f5b72f72f8a91b`
- main merge: `915906357d9752a5654b3dfeb85795419d885b59`
- helper PR #81: `jjun/assist-67-closeout` → `dagyeom/character-appearance-v1`, merge `982d930a53f199b33ebf7ca3d4f5b72f72f8a91b`
- closeout 변경:
  - tracked `Content/Characters/Quaternius/Import/__pycache__/make_headonly_gltf.cpython-314.pyc` 제거.
  - `.gitignore`에 `__pycache__/`, `*.pyc` 추가.
  - `LLResidentCharacter.cpp` appearance/presentation 중복 include 제거.
  - male Peasant `Regular` 노출 피부 슬롯이 `SkinToneAxis`에 따라 body/head와 동일한 light/dark BaseColorTexture를 선택하고 동일 tint를 적용하도록 수정.
  - latest main 동기화 후 #67 mergeability 복구.
- 검증:
  - #67 final-head Structural Preflight `34929703738` PASS.
  - #67 final-head Unreal Linux Compile `34929703712` PASS; UE 5.6 image verification / UHT / UBT / link 모두 SUCCESS.
  - helper PR #81 integration Core Tests `34929611584` PASS, **45/45** + deterministic harness smoke PASS.
  - 기존 `CHANGES_REQUESTED` review는 closeout 검증 후 dismissed, final review APPROVED.
- 제품 결과:
  - Quaternius CC0 deterministic humanoid appearance baseline이 main에 병합됨.
  - Peasant 기본 의상, head-only 파생 body, hair/skin/body variation, identity-bound deterministic appearance, Save/Load appearance continuity baseline 포함.
  - Character presentation은 simulation/action authority를 소유하지 않음.
- 다음 다겸 레인:
  - **Character Motion Bootstrap — READY_NOW**.
  - 범위: Idle / Walk / Jog, velocity/movement-state 기반 전환, orientation smoothing, Idle-slide 제거.
  - Core/World 이동 및 action authority는 유지하고 animation은 결과를 표현만 한다.
- 병렬 쭌 레인:
  - HumanWaste Environmental Visual Feedback — READY_NOW.



## 2026-09-15 — HumanWaste Environmental Visual Feedback v1 완료

- 작성자: 쭌 측 AI
- 브랜치/PR: `jjun/human-waste-visual-feedback-v1`, PR #82
- 상태: `DONE / main squash merge 완료`
- validated head: `8c54ca100c28b12a77375ad48626c4d513087b04`
- main merge: `831ba22ce17ca5fef8a92f2288e18a0495248a7a`
- 변경 범위:
  - `ULLEnvironmentalResidueVisualizerComponent` 추가. Core Bridge `GetEnvironmentObservation(...)`의 authoritative residue read DTO만 소비.
  - 하나의 HISM으로 여러 HumanWaste record를 표현하여 residue별 heavy Actor/Niagara 무한 생성을 피함.
  - Core Grid XY를 World 위치로 변환하고 WorldStatic surface trace로 지면 Z를 결정.
  - amount/intensity/radius/age를 visual footprint 및 per-instance custom data에 반영.
  - DesignatedArea residue baseline `intensity 0.42 / radius 3`, DugPit `0.16 / radius 1` 차이를 결과 기반으로 표현하며 site kind authority를 Presentation에 복제하지 않음.
  - BeginPlay/SaveLoad restore 후 authoritative state에서 강제 rebuild; 이후 signature가 바뀔 때만 refresh.
  - max 128 visual instances + cull distances로 Android-first 비용 경계 설정.
  - `GetEnvironmentalResidueVisualCount()` QA hook 및 structural validator/Preflight gate 추가.
- 검증:
  - Structural Preflight `34931331778` PASS, environmental visual feedback validator 포함.
  - Unreal Linux Compile `34931331779` PASS; UE 5.6 image verify / UHT / UBT / link 모두 SUCCESS.
  - merge checkpoint PR comments/reviews/unresolved threads 0.
- authority 경계:
  - visual layer는 deposit/contain/cleanup/hygiene/facility 상태를 수정하지 않는다.
  - visual-only state는 SaveGame authority가 아니며 Core residue에서 재구성한다.
- 다음 쭌 레인:
  - **World Genesis WG-1 — READY_NOW**.
  - `WorldSeed + GenerationVersion + ChunkCoord` deterministic contract와 exploration-order-independent untouched-chunk baseline을 먼저 확정한다.
  - `PopulationSeed`는 자연환경 seed와 분리하며, production World Visual이 작은 고정 arena에 묶이기 전에 WG-1/WG-2 gate를 통과한다.

## 2026-09-15 — 쭌 측 AI

### World Genesis WG-1 완료 — PR #83
- 작성자: 쭌 측 AI
- 브랜치/PR: `jjun/world-genesis-wg1`, PR #83
- 상태: `DONE / main 병합 완료`
- validated head: `c4d724b9650647ad986cd1ab235f4d8052840014`
- merge SHA: `f5c8cbab3aa41c6a37c3bae06838eeb583749771`
- 변경 범위:
  - `Source/LifeLensCore/include/lifelens/WorldGenesis.h`
  - Core `World` / `Simulation` world-population seed separation
  - `test_world_genesis_wg1`
  - `Tools/validate_world_genesis_wg1.py` + Preflight gate
- 구현:
  - `WorldSeed`, `PopulationSeed`, `WorldGenerationVersion` 분리
  - stable `ChunkCoord`, 음수 좌표 floor mapping
  - `(WorldSeed, GenerationVersion, ChunkCoord)` 기반 order-independent untouched chunk baseline
  - terrain/climate/resource/detail deterministic substream seeds
  - founder/name/trait/genetics/initial familiarity randomization은 PopulationSeed 전용 RNG 사용
  - PopulationSeed 변경이 자연 chunk identity를 바꾸지 않도록 계약 고정
- 검증:
  - Preflight `34933128958`: PASS
  - Core Tests `34933128953`: **46/46 PASS**
  - deterministic harness smoke: PASS
  - Unreal Linux Compile `34933128950`: PASS, UE 5.6 image verify / UHT / UBT / link PASS
  - merge checkpoint comments/reviews/unresolved threads: 0
- 상대가 알아야 할 점:
  - WG-1은 이제 design-only가 아니라 실제 Core runtime 계약이다.
  - 다음 쭌 레인은 **WG-2 Macro World + viable initial start-site selector — READY_NOW**.
  - 아직 실제 biome/river/terrain chunk가 생성되는 단계는 아니다; WG-2/WG-3 이후다.
  - 다겸 Motion/World Visual 작업은 WG-1 좌표/seed authority를 침범하지 말고, production map 고정 전 WG-2 boundary를 반영해야 한다.


### 다겸 측 AI — Character Motion Bootstrap 착수

- 작성자: 다겸 측 AI
- 브랜치: `dagyeom/character-motion-v1` (main `f5c8cba` 기준)
- 커밋: `df81c7a`
- 상태: `WAITING_CI / PIE 확인 대기`
- Sync Before Work: origin/main `f5c8cba`, PR #67 머지(`9159063`) 및 리뷰 3건 반영 확인, TEAM_BOARD의 `Character Motion Bootstrap — READY_NOW` 확인
- 구현:
  - `Content/Characters/Quaternius/UAL/BS_ResidentLocomotion` (BlendSpace1D, 속도축 0~600 cm/s, Idle/Walk/Jog/Sprint 4샘플). 생성 스크립트 `Import/make_locomotion_blendspace.py`
  - `Source/LifeLens/Characters/LLResidentMotionComponent.h/.cpp` 신규. 액터 이동량을 0.2초 창으로 측정해 BlendSpace 입력으로 전달, 마지막 이동 방향으로 바디 yaw 보간
  - `ll.DebugMotion` CVar(기본 0)
  - `LLResidentCharacter`에 MotionComponent 추가(추가만)
- 상대가 알아야 할 점:
  - 월드 디렉터가 주민을 매 프레임 이동시키지 않아 프레임 단위 속도 표본이 0과 최대치를 오갑니다. 표현 계층에서 0.2초 창 평균으로 흡수했습니다. Core/World 쪽 변경은 요청하지 않습니다
  - 표현용 회전은 바디 메시에만 적용하고 액터 회전은 건드리지 않았습니다. 이동·행동 권한은 그대로 Core/World에 있습니다
  - 400 유닛을 넘는 단일 스텝은 순간 이동(로드/그리드 복원)으로 간주해 속도 0 처리합니다

### 다겸 측 AI — Motion Bootstrap T-포즈 원인 및 수정

- 작성자: 다겸 측 AI
- 브랜치/PR: `dagyeom/character-motion-v1`, PR #84
- 커밋: `0be0aab`
- 상태: `PIE 재확인 대기`
- PIE 확인 결과(`70c6544`): 주민 4명 T-포즈, 이전 Idle 재생도 사라짐. 이동과 HUD 현재 행동 표시는 정상
- 원인:
  - BlendSpace 샘플을 Python에서 `sample_data` 속성으로 직접 기록하면 샘플은 저장되지만 런타임 삼각분할 데이터가 생성되지 않음. `UBlendSpace::GetSamplesFromBlendInput`이 모든 입력에 0개를 반환해 스켈레탈 메시가 레퍼런스(T) 포즈로 평가됨
  - 삼각분할은 에디터 전용 `AddSample` / `ValidateSampleData` / `ResampleData` 경로에서만 생성되며 Python에 노출되어 있지 않음
  - 헤드리스 검증에서 기록한 `samples=4`는 `SampleData` 개수라 이 결함을 드러내지 못했음. 진단 로그에 해석된 샘플 수를 추가해 `resolved=0 ok=0`으로 특정
- 수정:
  - `LLLocomotionBlendSpaceBuilder` (신규, 에디터 전용): 위 API를 스크립트에 노출
  - `make_locomotion_blendspace.py`: 빌더 사용으로 전환, 저장 후 0/75/150/260/375/600 cm/s에서 샘플 해석 여부를 검증하고 0이면 실패 처리
  - `LLResidentMotionComponent`: 시작 시 BlendSpace가 샘플을 해석하지 못하면 경고 후 외형 컴포넌트의 Idle 재생을 유지. T-포즈로 떨어지지 않음
  - 재검증: headless `-game`에서 `resolved=1`, 로컬 Build.sh Succeeded, preflight PASS
- 상대가 알아야 할 점:
  - 스크립트로 BlendSpace를 만들 때 `sample_data` 직접 기록은 런타임에서 동작하지 않음. 에디터 전용 경로를 거쳐야 하며, 저장 후 `GetSamplesFromBlendInput` 해석 수로 검증해야 함
