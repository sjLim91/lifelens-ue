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

## 2026-09-13 — 다겸 측 AI

### Observer HUD v2 PR 생성
- 작성자: 다겸 측 AI
- 브랜치/PR: `dagyeom/observer-ui-v2`, PR #17
- 상태: `REVIEW / 실제 UHT·UBT 검증 대기`
- 변경:
  - `Source/LifeLens/UI/LLObserverHUD.h/.cpp` 재작성: LEVEL 0 개요 띠 + 주민 스트립, LEVEL 1 Quick Inspector(이름/나이/현재행동/상태 요약/성격 단어), LEVEL 2 상세 패널(Overview / Needs / Personality / Traits & Skills 탭)
  - `Source/LifeLens/UI/LLObserverLabels.h` 추가: 관측용 문구와 Needs/Personality/Skill 라벨 변환
  - `Source/LifeLens/UI/LLObservationSubsystem.h/.cpp`: 관측 단계(World / Quick / Detail), `OpenDetail` / `CloseDetail` / `StepBack`, `OnObservationLevelChanged`
  - `Source/LifeLens/UI/LLObserverPlayerController.h/.cpp`: 탭 라우팅(HUD 크롬 → 주민 액터 → 빈 곳은 한 단계 뒤로)
  - 욕구/관계 수치는 화면에 표시하지 않고 Good / Fine / Low / Very low 라벨만 사용
  - 관계 표시는 제외 (다차원 Relationship read DTO 이후 추가 예정)
  - Codex 리뷰 2건 반영 (선택 중 힌트 숨김, 카드 폭/줄바꿈)
- 검증:
  - Structural preflight PASS (`Tools/validate_bootstrap.py`, 로컬 + CI)
  - LifeLens Core Tests PASS (CI, Core 미변경)
  - 실제 UHT/UBT 미실행. 다겸 맥에 Unreal Engine 미설치. CI 또는 쭌 측 확인 필요.
- 상대가 알아야 할 점:
  - `Source/LifeLens/UI/**`만 수정. 공유 파일 변경 없음.
  - `Tools/validate_bootstrap.py` 65행이 `LLObserverHUD.cpp`에서 `Tap/click a resident for details` 문자열을 확인함. 문구를 `LLObserverLabels.h`로 옮겼으므로 검사 대상을 헤더로 바꿔 주면 좋겠음. 그때까지 cpp 주석에 동일 문장 유지.
  - 한글 폰트 에셋 필요 (`Content/UI`). 주민 이름이 한글이라 `GEngine->GetSmallFont()`로는 깨질 수 있음. 로컬 에디터가 없어 아직 에셋을 만들지 못함.
  - PR #13 `ResidentObservation` / `RelationshipObservation`이 Unreal Bridge read API로 나오면 LEVEL 2에 Relationships / Emotion 탭 추가 예정.

### Observer HUD v2 PR 생성 — 정정
- 작성자: 다겸 측 AI
- 브랜치/PR: `dagyeom/observer-ui-v2`, PR #17
- 커밋: `82f126d`, `9941d77`, `2c4b25c`, `cc7f876`
- 상태: `REVIEW / 검증 대기`
- 변경 범위:
  - `Source/LifeLens/UI/LLObserverHUD.h`, `.cpp`
  - `Source/LifeLens/UI/LLObserverLabels.h`
  - `Source/LifeLens/UI/LLObservationSubsystem.h`, `.cpp`
  - `Source/LifeLens/UI/LLObserverPlayerController.h`, `.cpp`
  - `tasks/TEAM_BOARD.md`, `tasks/HANDOFF_LOG.md`
- 검증 상태:
  - `core-tests.yml` PASS
  - structural preflight PASS
  - 실제 UHT/UBT: 검증 대기
  - 화면/동작 확인: 검증 대기
- 상대가 알아야 할 점:
  - `tasks/TEAM_BOARD.md` 변경 포함. SHARED FILE CHANGE. 영향 범위: 다겸 행 상태, Merge Queue 항목.
  - 이전 항목의 "공유 파일 변경 없음"은 이 항목으로 정정.
  - `Tools/validate_bootstrap.py` 65행 문자열 검사 대상: `LLObserverHUD.cpp`. 동일 문자열을 cpp 주석에 유지.
  - 한글 폰트 에셋 필요: `Content/UI`.
  - 보드 다겸 측 다음 작업 3의 "관계" 항목 미포함.

### Integration Request 기록
- 작성자: 다겸 측 AI
- 브랜치/PR: `dagyeom/observer-ui-v2`, PR #17
- 상태: `REVIEW / 검증 대기`
- 변경 범위:
  - `tasks/TEAM_BOARD.md` Integration Requests: 다차원 Relationship 읽기 API, `Tools/validate_bootstrap.py` 65행 검사 대상
- 검증 상태:
  - 코드 변경 없음
- 상대가 알아야 할 점:
  - 상세 패널 "관계" 항목은 Relationship 읽기 API 제공 전까지 미구현.

### TEAM_BOARD 충돌 처리
- 작성자: 다겸 측 AI
- 브랜치/PR: `dagyeom/observer-ui-v2`, PR #17
- 커밋: `26e919d`
- 상태: `REVIEW / 검증 대기`
- 변경 범위:
  - `tasks/TEAM_BOARD.md`: origin/main `6ce0dd8` 내용 유지. 다겸 행 상태 `REVIEW`, Merge Queue PR #17 항목, Integration Request 재적용 및 추가.
- 검증 상태:
  - 코드 변경 없음
- 상대가 알아야 할 점:
  - 쭌 측 섹션(Active Work 쭌 행, 완료된 병렬 작업, 쭌 측 현재 공유사항, 쭌 측 다음 작업, 다겸 측 다음 작업)은 origin/main 그대로.
  - Integration Request 추가: SocialIntent 현재행동 DTO, 감정 읽기 API, 가족 요약 읽기 API, World overview 집계 읽기 API.

### Observer HUD v2 — 보드 다겸 측 다음 작업 3 갱신 반영 정정
- 작성자: 다겸 측 AI
- 브랜치/PR: `dagyeom/observer-ui-v2`, PR #17
- 상태: `REVIEW / 검증 대기`
- 변경 범위:
  - 코드 변경 없음
- 검증 상태:
  - 해당 없음
- 상대가 알아야 할 점:
  - LifeStage: LEVEL 2 Overview 탭에 표시됨 (`FLLResidentData::LifeStage`).
  - 감정 / 관계 / 가족 요약: 읽기 API 제공 전까지 미구현. Integration Request 기록.
  - 현재행동의 SocialIntent(Approach / Repair / Comfort / Avoid): 읽기 API 제공 전까지 미구현. Integration Request 기록.
  - TEAM_WORKFLOW 10절 "Character visual proxy/표현 개선": `Content/Characters/**` 에셋 필요. 로컬 Unreal Editor 없음. 착수 불가.

### World overview 정보 계층 v1
- 작성자: 다겸 측 AI
- 브랜치/PR: `dagyeom/observer-ui-v2`, PR #17
- 커밋: `2fcf04e`
- 상태: `REVIEW / 검증 대기`
- 변경 범위:
  - `Source/LifeLens/UI/LLObserverHUD.h`, `.cpp`: World overview 패널 (SPEC 61). LEVEL 0에서 상단 개요 띠 탭으로 열고 닫음. 날짜/시간, 인구, 생애단계별 인원.
  - `Source/LifeLens/UI/LLObserverLabels.h`: World overview 문구.
- 검증 상태:
  - structural preflight PASS (`Tools/validate_bootstrap.py`)
  - 실제 UHT/UBT: 검증 대기
  - 화면/동작 확인: 검증 대기
- 상대가 알아야 할 점:
  - 사용 읽기 API: `ULLSimulationSubsystem::GetResidents`, `GetSimulationMinute`, `FLLResidentData::LifeStage`.
  - Households / Couples / Married Couples / Pregnancies / Major Events: 읽기 API 제공 전까지 미표시. Integration Request 기록.

### Observer HUD v2 — 로컬 화면 확인 및 탭 판정/커서 수정
- 작성자: 다겸 측 AI
- 브랜치/PR: `dagyeom/observer-ui-v2`, PR #17
- 커밋: `845eb28`
- 상태: `REVIEW / 검증 대기`
- 변경 범위:
  - `Source/LifeLens/UI/LLObserverPlayerController.cpp`: BeginPlay에서 `FInputModeGameAndUI`(DoNotLock, 캡처 중 커서 숨김 해제) 적용, HUD 탭 판정에 viewport 크기 전달
  - `Source/LifeLens/UI/LLObserverHUD.h`, `.cpp`: 탭 좌표를 마지막 DrawHUD 캔버스 크기로 매핑, 탭 로그 1줄
- 검증 상태:
  - 로컬 `Build.sh LifeLensEditor Mac Development` (`3b578c6`): Result: Succeeded → 에디터 PIE 화면 확인: LEVEL 0 정상(띠, 스트립, 한글 이름), LEVEL 1 정상(이름·나이·Now·요약·성격·Details). LEVEL 1 카드 클릭 시 LEVEL 2 미진입, LEVEL 0으로 복귀. PIE에서 마우스가 뷰포트에 갇히고 커서 미표시.
  - 원인: `Config/DefaultInput.ini`의 `DefaultViewportMouseCaptureMode=CapturePermanently_IncludingInitialMouseDown`, `DefaultViewportMouseLockMode=LockOnCapture`로 커서가 숨겨지고 Slate high-precision 마우스 모드가 되어 `FSceneViewport::CachedCursorPos`가 첫 클릭 위치에서 갱신되지 않음. `GetMousePosition` / `GetHitResultUnderCursor`가 첫 클릭 위치를 반환하여 카드 클릭이 빈 곳으로 판정됨.
  - 수정 후 로컬 `Build.sh LifeLensEditor Mac Development` (`845eb28`): Result: Succeeded. structural preflight PASS.
  - 수정 후 화면/동작 재확인: 검증 대기
- 상대가 알아야 할 점:
  - `Config/**`는 수정하지 않음. 커서/잠금은 PlayerController의 `SetInputMode`로 런타임에 덮어씀.
  - 참고(다겸 영역 아님, 기록만): PIE 맵에 라이트 없음. 주민 액터가 매우 작게 보임. 모바일 가상 조이스틱 표시됨(`DefaultInput.ini` `DefaultTouchInterface=/Engine/MobileResources/HUD/DefaultVirtualJoysticks`).
  - 에디터 실행 시 `Config/DefaultEngine.ini`, `Config/DefaultInput.ini`가 자동 수정됨. 커밋하지 않고 폐기함.

### Observer HUD v2 — 탭 좌표계 통일 및 주민 선택 반경
- 작성자: 다겸 측 AI
- 브랜치/PR: `dagyeom/observer-ui-v2`, PR #17
- 커밋: `9512618`
- 상태: `REVIEW / 검증 대기`
- 변경 범위:
  - `Source/LifeLens/UI/LLObserverHUD.h`, `.cpp`: DrawHUD에서 scene view rect 원점 기록, `ViewportToCanvas`로 뷰포트 픽셀→캔버스 픽셀 매핑
  - `Source/LifeLens/UI/LLObserverPlayerController.h`, `.cpp`: 주민 선택을 화면 투영 위치 기준 반경(48 논리픽셀 × Slate DPI 배율)으로 판정, 정확 트레이스 히트는 2차 판정. 탭 로그에 최종 좌표·최근접 주민 거리·반경 기록
- 검증 상태:
  - 재확인(`845eb28`): 마우스 창 밖 이동 정상. 주민 클릭 시 LEVEL 1 미표시. 로그 `tap (1479, 603) viewport (2846, 1712) canvas (2846, 1601)`
  - 원인: 관찰 카메라(`ACameraActor`, aspect 제약)로 HUD 캔버스가 뷰포트 안 16:9 view rect(2846×1601)이며 원점이 레터박스만큼 어긋남. 마우스/터치/투영 좌표는 뷰포트 픽셀. 주민 액터가 화면에서 매우 작아 정확 트레이스 히트가 빗나감
  - 수정 후 로컬 `Build.sh LifeLensEditor Mac Development` (`9512618`): Result: Succeeded. structural preflight PASS
  - 수정 후 화면/동작 재확인: 검증 대기
- 상대가 알아야 할 점:
  - `Config/**`, `Characters/**`, `Core/LLLifeLensGameMode.cpp` 미수정. 레터박스 원인은 GameMode의 관찰 카메라 aspect 제약이며 UI 쪽 좌표 매핑으로 대응함.
  - `Tools/validate_bootstrap.py`의 `GetHitResultUnderCursor` / `GetHitResultUnderFinger` 검사에 맞춰 정확 히트 경로 유지.

### Observer HUD v2 — 주민 선택을 렌더 바운즈 투영 사각형으로 변경, ll.DebugTapTargets
- 작성자: 다겸 측 AI
- 브랜치/PR: `dagyeom/observer-ui-v2`, PR #17
- 커밋: `50beab0`
- 상태: `REVIEW / 검증 대기`
- 변경 범위:
  - `Source/LifeLens/UI/LLObserverHUD.h`, `.cpp`: `ProjectResidentTapRect`(액터 렌더 바운즈 8꼭짓점 투영 → 뷰포트 픽셀 사각형, 터치 반경만큼 확장), `TouchTargetRadiusPixels`, 콘솔 변수 `ll.DebugTapTargets`(기본 0, 1이면 바운즈 노란색·탭 사각형 청록색·원점 십자 빨간색 표시)
  - `Source/LifeLens/UI/LLObserverPlayerController.h`, `.cpp`: 탭이 확장 사각형 안에 들면 선택, 여러 개면 바운즈에 가장 가까운 주민. 정확 트레이스 히트는 2차 판정 유지
- 검증 상태:
  - 재확인(`9512618`): 주민 클릭 시 LEVEL 1 미표시. 로그 `nearestResident 130~194 px radius 76 px exactHit no`
  - 원인: `ALLResidentCharacter`의 보이는 컴포넌트는 액터 원점 중심의 DebugBody 큐브(55×55×90)와 원점 위 125유닛의 NameLabel 텍스트. 카메라 거리에서 125유닛 ≈ 180px라 이름 텍스트 클릭이 원점 투영 기준 반경(76px)을 벗어남. `ProjectWorldLocationToScreen`은 레터박스 view rect 오프셋을 포함하므로 좌표계는 일치
  - 수정 후 로컬 `Build.sh LifeLensEditor Mac Development` (`50beab0`): Result: Succeeded. structural preflight PASS
  - 수정 후 화면/동작 재확인: 검증 대기
- 상대가 알아야 할 점:
  - `Characters/**` 미수정. 참고(다겸 영역 아님, 기록만): 주민 액터가 카메라 거리(약 1900유닛)에서 큐브 55유닛 ≈ 80px로 작게 보임. 이름 라벨이 원점 위 125유닛.
  - `ll.DebugTapTargets 1`로 투영 사각형을 화면에서 대조 가능.

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
