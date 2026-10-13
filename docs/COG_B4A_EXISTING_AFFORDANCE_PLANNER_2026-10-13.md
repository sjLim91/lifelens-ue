# LifeLens COG-B4a — 검증된 목표에서 기존 Core 행동 후보로의 연결

> 2026-10-13 / 기반: #697 (GoalSpec typed contract), #699 (개인기억 출처), #701 (개인 목표 생성), #702 (Core→WASM→Web 관찰)

## 개발 경계

LifeLens의 주민은 사전에 정해진 시대별 기술 발전/사육 순서를 따라가지 않는다. 개발자가 제공하는 것은 물리적 세계의 실제 결과와 각 행위의 비용·제약이다. AI는 개인이 인지한 환경에서 원하는 변화를 표현하고, 기존에 존재하는 물리·사회적 수단을 조합해 목표를 시도해야 한다.

목표는 행동이 아니다. 목표 후보를 받아서 즉시 세계 상태·자원·생존·관계를 바꾸면 무에서 유를 만드는 규칙 엔진이 된다. **B4a는 기존 Core 엔진으로 실행 가능성이 있는 작은 행동 단위 후보만 구성하며, 실행 자체는 하지 않는다.**

## 구현

- 신규 `AutonomousAffordancePlanner.h`: `draftAutonomousAffordancePlan(World, RelationshipBook, CognitiveRequest, GoalSpec)`.
- 입력 `CognitiveRequest`는 Core의 **현재 개인 문맥**으로 다시 구성하여 기존 `cognitiveRequestFingerprint`와 대조한다. 개인 Needs·관계·기억·목격/전언 출처·성격·허용 의도가 바뀐 이전 문맥이나 타인의 위조 문맥은 거부한다.
- GoalSpec은 기존 B3 `validateAutonomousGoalCandidate`를 재사용해 현재 Core 주민 및 권한과 다시 확인한다.
- 허용하는 초기 실제 Core 엔트리포인트는 두 종류뿐:
  - 신체적 욕구를 **감소시키는** 목표 → 현재 `Goal::Eat/Drink/Sleep/UseToilet/Wash` 후보. `actionAvailableFor`로 실제 음식·물 등 가능한 입력 제약을 검사한다.
  - 현재 인지 문맥에 있는 주민과 신뢰를 높이려는 목표 → 기존 `SocialIntent::Approach/Repair` 후보. 상대 실재/생존과 실제 Core의 사회 점수 타당성을 확인한다.
- 1~6개 목표 조건을 순서대로 계획 후보에 연결한다. 이미 충족된 Need/Relationship 조건은 불필요한 행동을 추가하지 않는다.
- 하나라도 **자원 부족/시도 불가/지원하지 않는 종류**이면 전체 계획을 실패로 분류하고 제안된 부분 행동을 모두 비운다. 부분 실행을 암묵적으로 허용하지 않는다.
- 긴급 허기·갈증·수면·배뇨 상황에서는 사회적 행동 우선 계획을 차단한다. 기존 `UtilityAI`의 긴급 생존 우선순위가 유지된다.
- 돌·목재 재고 증가 목표나 타인의 생존 유지처럼 아직 감각정보·행동이 충분히 연결되지 않은 목적은 **Unsupported**. 도구/사냥/사육/건축을 만들거나 기술을 자동 해금하지 않는다.

## 상태와 의미

- `CandidateOnly`: 기존 Core에서 사용할 **의도 이름**을 찾았다는 의미다. 이동 경로, 접근 가능, 시설 예약, 물리적 도달, 상호작용, 성공, 관찰 결과는 검증하지 않았다.
- `AlreadySatisfied`: 인지 문맥의 허용된 목표 상태가 이미 충족돼 제안 행동이 없다.
- `Blocked`: 실제 자원 부족 또는 긴급 생존/사회적 실행 불가.
- `Unsupported`: 해당 목표를 증명 가능한 기존 행동으로 연결할 수 없다.
- `RejectedContext` / `RejectedGoal`: 타인의 문맥, 시간 불일치, 위조·부정확한 근거 또는 유효하지 않은 목표.

상태를 `ReadyToExecute`라고 부르지 않는다. 미래의 실제 행동 실행 단계에서는 반드시 **매 스텝 동일 주민·동일 시점 및 물리적 비용/이동/소비/관계 결과를 다시 확인**해야 한다.

## 결정론·성능·호환성

- 읽기 전용, Core-owned validation, 신규 난수 소비 없음.
- `World`, `Simulation::beginPlan`, `UtilityAI`, 사회행동, 시설, 생태, COG-3 accepted journal, WASM ABI, Snapshot 3/Binary 8 변경 없음.
- 무료 로컬 계산, 외부 LLM/API/클라우드 의존 없음.
- bounded 최대 6개 step과 기존 제한 8건 개인 기억/신념/관계 읽기.

## 검증 및 후속 실행 연결

`test_autonomous_affordance_planner`는 시도 가능한 수면·관계 결합, 긴급 생존 차단, 개인 Core 문맥 위조/시점 변경 거부, 음식 자원 없는 식사 제안 거부 및 음식 실제 보유 시 후보 허용, 물질 목표 unsupported, 위험한 생리 증가 목표 unsupported, 이미 충족된 목표 no-op, 재실행 일치, RNG/욕구/관계 무변경을 검사한다.

다음 단계 **B4b**는 계획을 실제 행동에 적용할 때 기존 `Simulation`의 Core-owned 승인·물리 실행·실패 결과 기록 경로를 사용해야 한다. 행동 결과 없는 영구 목표/강제 문명 발전은 허용하지 않는다. 계획된 변화를 AI 가설과 비교해 주민별 신념과 다음 방법 선택에 학습으로 연결한다.
