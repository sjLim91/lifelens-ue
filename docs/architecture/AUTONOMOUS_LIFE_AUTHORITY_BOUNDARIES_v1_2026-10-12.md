# LifeLens 자율 인공생명 — 권한·책임·인과 경계 계약 v1

> 상태: **CANONICAL DESIGN / 책임 경계 확정 · 신규 모듈 구현 전**
>
> 확정일: 2026-10-12 (KST)
>
> 기준 코드: [main `08c280acd18a660417fa2c26910dee875cd60e95`](https://github.com/sjLim91/lifelens-ue/commit/08c280acd18a660417fa2c26910dee875cd60e95)
>
> 저장소: `sjLim91/lifelens-ue` / 활성 제품: `LifeLensCore (C++) -> WASM -> React/TypeScript/Three.js Web Observer`
>
> **이 문서는 구현 완료 보고서가 아니다.** 아래 '확정'은 향후 변경의 설계 심사 기준이고, 제안 타입/인터페이스는 구현 전 계약 후보이다. 실제 적용 여부는 PR·코드·검증 결과로 판단한다.

## 0. 최상위 목적 및 원칙

LifeLens는 지구형 세계 안의 인공 생명체가 **자기 몸, 개별 경험, 제한된 인식, 타 생명체와의 상호작용**을 통해 행동을 바꾸고, 장기적으로 개발자가 지정하지 않은 집단적 생활 방식을 만들어 낼 가능성을 관찰하는 프로젝트다.

**확정 불변식**

1. **Core-only authority**: 월드·주민·동물·자원·질병·시설·위치·시간·수량의 권위 있는 변경은 Core의 검증된 실행 경로에서만 발생한다.
2. **자율적 의도 / 통제된 결과**: AI는 문제 인식·목표·가설·행동 조합·시도 여부를 스스로 선택한다. Core는 실현 가능성·비용·물리·생리·사회적 결과를 판정한다.
3. **개인 지식 ≠ 월드 진실**: 주민은 직접 관찰하거나 전달받은 정보만 자신의 기억·신념으로 갖는다. 믿음은 왜곡·오해·망각될 수 있지만 월드 사실을 바꿀 수 없다.
4. **발전 단계 강제 금지**: '100일 농업 / 200일 사육 / 500일 정착' 등의 날짜 해금, 사냥→길들이기→가축화 필수 경로, 자동 마을/국가 생성은 금지한다.
5. **재현성과 영속성**: 동일 Core 버전/룰셋/시드/동일하게 승인된 입력 이벤트 순서에서 동일 결과를 재생할 수 있어야 한다. 저장/복원으로 이미 결정한 일을 다시 추론하거나 보상하지 않는다.
6. **거짓 관찰 금지**: 실제 시뮬레이션에 없는 행동·동물·기술·사냥 성공·실측 지역 날씨는 Web/UI나 인지 모델이 만들어 표시하지 않는다.
7. **오프라인 우선**: 무료·로컬 경로로 기본 AI와 생존이 동작해야 한다. 외부 LLM/유료 API/클라우드를 필수 의존성으로 추가하지 않는다.

**개발 PR의 단계**와 **주민 문명의 발생 순서**는 전혀 다른 개념이다. 구현 의존성을 단계로 나누어도 주민의 행동을 같은 순서로 강제할 근거가 될 수 없다.

## 1. 권한 소유 표 (확정)

| 영역 / 변경 대상 | 최종 결정 및 데이터 소유자 | AI / 외부 구성요소의 허용 역할 | 금지 |
|---|---|---|---|
| WorldSeed/PopulationSeed/버전, Planet·Region·Chunk identity | Core World / Genesis | Core가 승인한 월드 선택·관찰 요청 | AI·Web의 좌표/시드/초기 자연 조작 |
| 지형·고도·수계·기후·토양·식생·자원량 | Core Environment & Ecology | 관찰된 부분을 바탕으로 해석·탐색·이용 방법 제안 | GPU/Web이 자원·비·강·지형 truth 생성 |
| 인간/동물의 위치·몸·Needs·질병·노화·번식·사망 | Core Life / Ecology | 접근·회피·돌봄·먹이·행동 목표 제안 | AI가 Needs/건강/개체수를 직접 수정 |
| 물체 조작·이동·건설·소비·물리 제약 | Core Action Authority | 실행 가능한 primitive 조합/순서/대상 제안 | 유효하지 않은 원격 행위, 순간이동, 무에서 물체·식량 생성 |
| 주민별 목표·가설·선택·계획 후보 | Autonomous Cognition (Core 내 기본 엔진; 선택적 로컬 provider) | 개인의 관찰/기억으로 자유롭게 후보 생성·평가 | 신규 후보 생성에 기성 Goal/Technology 목록을 종착역으로 강제 |
| 신뢰·기억·신념·지식 전달의 내용 | 주민별 Core Learning/Memory/Belief·Social | 직접 경험·추론·대화·모방의 제한된 입력 | 아직 관찰하지 않은 WorldTruth를 주민의 확정 지식으로 주입 |
| 경험 판정·실제 자원 비용·성공/실패 인과 | Core Outcome / Event authority | 결과 분석·가설·방법별 기대치 갱신 | AI/LLM의 텍스트 자체를 실행 성공으로 간주 |
| 관계·거래·협업·정착지 변화·집단적 관습 | Core Social / Civilization | 주민별 상호작용 제안·학습·모방 | 날짜/시대/지시문으로 관계·시설·문명 자동 확정 |
| 저장·복원·RNG·행동 이벤트 저널 | Core Snapshot / Replay | 입력 후보·추론 메타데이터 제공 | 브라우저 히스토리를 권위 있는 과거 결정으로 대체 |
| 화면/카메라/LOD/애니메이션/차트·분석 | WASM DTO + Web Observer | Core read-only 결과를 표현하고 원인 추적 | 렌더러가 실제 실행/시뮬레이션 상태를 변경 |

'Core 내 기본 엔진'은 설계상 책임 위치를 뜻하며, 현재 main에 범용 목표 생성기가 존재한다는 뜻이 아니다.

## 2. 계층별 책임과 데이터 이동

```text
Core World / Ecology / Life (only physical truth)
        |
        v bounded, actor-visible observation
Core Perception -> Individual Memory / Belief / Personality / Relationship
        |
        v event-triggered, finite context
Autonomous Cognition
  - discrepancy / opportunity detection
  - GoalSpec + Hypothesis candidates
  - feasible ActionPrimitive / PlanStep sequences
  - predicted outcomes, uncertainty, alternatives
        |
        v typed proposals only
Core Proposal Validator / Action Authority
  - actor & world identity, perception provenance, freshness
  - target/ownership/distance/resource/precondition/cost guards
  - critical Needs override, bounded execution, commit
        |
        v actual outcome events only
Core Experience / Memory / Knowledge / Social Transmission
        |
        +-----> next personal perception and plan
        |
        +-----> WASM read-only DTO -> React/Three.js Observer
```

**주의:** 기존 UE5 StateTree/SmartObject/Utility 패턴은 참고 또는 타 플랫폼 실행 경로일 뿐, 활성 Web 제품의 simulation authority는 LifeLensCore다. 프레젠테이션 물리 모션은 Core가 확인한 상태를 소비한다.

### 2.1 World & Ecology

- 시드, 버전, 좌표, 청크 기준선, 지형·기상·수질, 개체 수용력, 물질 재고, 재생/소모, 에너지·위치 제약을 소유한다.
- 관찰자나 AI의 요청으로 아직 존재하지 않는 자원을 생성하지 않는다.
- 동물의 생물학·이동·스트레스·먹이·번식/사망은 실제 Core 상태와 시점에 따라 갱신한다.
- 지구 결속이 아직 실제 지역별 온도·날씨·종 분포와 연결되지 않았다면 `geographicallyConditioned=false`, `regionalWeatherApplied=false` 등 미구현 상태를 유지한다.

### 2.2 Life & Action

- 기존 Needs/Health/UtilityAI/Planner/ContextAction, CoreNavigation, CivilizationExecution 경로를 **안정된 실행기로 재사용**한다.
- 단, 기본 행동을 더욱 작은 affordance로 분리할 수 있는 API는 점진적으로 추가한다. 기존 고정 Goal/기술 enum을 무턱대고 삭제·변경하지 않는다.
- 실행 가능 여부, 경로/충돌, 재고, 도구, 능력, 접근 가능성, 안전 임계 및 결과 발생은 **전적으로 Core 검증/commit**에서 결정된다.
- 긴급 생존 행동은 인지 지연·실험 동기·학습된 부정적 경험보다 우선한다.

### 2.3 Autonomous Cognition

- 할 일: 실제로 알고 있는 세계에서 문제/기회 탐지, **새 목표 상태** 설정, 경쟁 목표의 우선순위 결정, 가설과 실험 제안, primitive 조합 탐색.
- 못 할 일: 직접 `World`/인벤토리/시설/위치/신체/기술/관계 수정, 미검증 실행 성공 선언, 실행 불가능한 행동 등록.
- 목표는 새로운 설명 문구만 만드는 것이 아니라 Core에서 검증할 수 있는 **대상·목표 상태 조건·근거·종료 조건**을 갖춰야 한다.
- 개인의 성격·위험 성향·호기심·현재 욕구·추론된 신념에 따라 **같은 객관 세계에서도 다른 후보**가 나올 수 있어야 한다.

### 2.4 Learning & Memory

- 실제 action outcome의 attempt ID·시간·대상·자원 비용·성공/실패·기대와 결과 차이로만 직접 경험을 갱신한다.
- 실패나 부상은 원인 학습 자료이나, 사건 발생 자체를 잘못된 믿음으로 덮어쓰지 않는다.
- 주민의 **원인 해석·확신·기억**은 편향되거나 틀릴 수 있다. 타 주민의 전달 내용은 직접 목격과 동등하게 취급하지 않는다.
- 생리적 Needs 감소율 또는 생존 필수 행동의 Utility 점수를 경험 학습으로 약화시키지 않는다. 학습은 **목표 달성 방법·대안·위험 추정**에 적용한다.
- 학습 가치 검증은 기억 개수 대신 **같은 조건에서 후속 행동이 달라졌는지**로 한다.

### 2.5 Social Emergence / Civilization

- 관계, 거래, 지식 전파, 출생과 공동 생활은 기존 Core truth를 따른다.
- 타인의 행동을 실제로 보거나 신뢰 가능한 전언으로 접한 경우에만 학습/모방의 후보가 된다.
- 기술명·시대상·사회적 분류는 **관찰 가능한 결과를 정리하는 라벨**이다. 라벨이 물리 효과를 자동 생성하거나 강제 순서를 정하지 않는다.
- 다만 물리적으로 재현 불가능한 기술을 무제한 인정할 수 없으므로, 초기에는 기존 `TechnologyRegistry`/레시피/재료 성질을 실행 가능성 판정에 재사용한다. 장기적으로 재료·행동 조합 규칙을 확장한다.

### 2.6 Observer & Analysis

- Web은 Core-produced DTO를 표시한다. 추론의 제안·수락·거부·실제 실행·결과 및 학습을 **서로 다른 이벤트**로 보여 준다.
- '사육 발생'은 실제 먹이 제공, 개체 생존, 지속적 동물 관계, 장소/시설 사용 등 Core 이벤트의 장기 조건을 보고 후행 분류할 수 있다. 이를 발생시키는 `unlockHusbandry()` 같은 권위 명령은 금지한다.
- 읽기 전용 Shadow Mode 출력은 실제 행동으로 표시하거나 저장된 사건으로 취급하지 않는다.

## 3. 인터페이스 계약 후보 (아직 코드에 없음)

타입 이름은 **예시·리뷰용**이며 구현 전에 현행 Core 타입과 겹침을 검토해야 한다.

### 3.1 `PerceptionFact`

- `actorId, observationMinute, regionId, targetId/position, sensoryChannel, factKind, observedValue, confidence, sourceEventId`.
- Core가 actor의 실질적 인식 범위/물리적 접근/전달 경로를 검사한다.
- World의 비공개 미래 결과, 멀리 떨어진 자원·오염·동물 위치 등 전지적 데이터 주입 금지.

### 3.2 `GoalSpec`

- `goalId, actorId, createdMinute, motive, targetEntity/region, desiredStatePredicate, evidenceRefs, priority, deadline(optional), expiry, status`.
- `desiredStatePredicate`는 Core가 **읽고 평가할 수 있는 유한 typed predicate** 조합만 허용한다.
- 후보 목표의 **내용과 결합**은 AI가 생성할 수 있으나, Core에서 검사할 수 없는 상태 조건은 권위 목표로 수락하지 않는다.
- 현재 고정 `Goal`/`CognitiveIntentKind`와의 호환 어댑터는 유지할 수 있다. 고정 Enum을 AI의 최종 발상 범위로 삼지 않는다.

### 3.3 `Hypothesis` / `PlanCandidate`

- 가설: `hypothesisId, goalId, evidenceRefs, predictedOutcome, confidence, expectedCost, expectedRisk, expiry`.
- 계획: `planId, actorId, goalId, orderedPrimitiveSteps, inputRefs, preconditions, abortConditions, maxSteps, maxDuration`.
- primitive는 `Observe, Move, Approach, Avoid, Pick, Place, Transfer, Combine, Separate, Process, Feed, Communicate` 등의 **예시 분류**이다. 도입한 모든 primitive가 Core 실행을 가진다는 뜻은 아니다.
- 계획 자유도는 **실제로 등록·구현되고 검증 가능한 primitive의 조합** 범위로 제한한다. 자유 텍스트 지시/스크립트/임의 코드 실행 금지.
- 실패 또는 조건 변경으로 중단될 수 있으며, 일부 실행 성공을 전체 목표 성공으로 자동 환산하지 않는다.

### 3.4 `ValidatedDecision` / `OutcomeEvent`

- 수락 입력: `actor/worldId, simulationMinute, generationVersion, policyVersion, contextFingerprint, providerId, acceptedGoalOrPlanId, typedPayload, decisionId`.
- 결과: `decisionId, attemptId, sourceActionId, before/afterRefs, actualCost, success/failureReason, eventMinute, witness/provenance`.
- 결과 이벤트는 Core commit 이후 발행한다. 관련 자원, 물리 결과, 관계 변화는 한 권위 트랜잭션/검증된 단계적 결과에 묶인다.
- 원인 연쇄는 `perception -> hypothesis -> chosen plan -> accepted action -> actual outcome -> updated belief -> transmitted fact`로 추적할 수 있어야 한다.
- rationale·대화 문구·LLM 출력 문자열은 **설명/후보 데이터**일 뿐 실행 권한이 없다.

## 4. 제안 → 실행의 안전 게이트 (확정 순서)

1. **관찰**: Core의 시뮬레이션 시계·개체·인식 범위에서 근거를 읽는다.
2. **후보 생성**: 기본 로컬 결정론 정책 또는 선택적 외부 로컬 모델이 목표/가설/계획 후보를 만든다.
3. **정형화**: 길이·배열 크기·유효 숫자·허용 primitive·소유자·대상·시간·버전을 검사한다.
4. **권한 검증**: actor 생존/능력, 인식 출처, target 실제 존재, 접근 경로, 자원/도구, stale context, 중복 요청, 생존 우선순위 검증.
5. **기록**: 미래에 영향을 줄 외부·비결정론적 선택은 **accepted-input journal**에 결정 시점과 함께 기록한다. 같은 이벤트 재적용 금지.
6. **실행**: Core의 기존 Action/ContextAction/Navigation/World mutation API가 자원 비용과 실제 동작을 확정한다.
7. **결과**: Core가 attempt/outcome을 기록한다. 실패·부상·소모도 실제 결과다.
8. **학습**: 해당 actor의 기억·신념·습관 후보·방법 효용을 조정하고, 타인은 실제 관찰·대화를 통한 경우에만 전파받는다.
9. **표시**: WASM/Web은 Core events/DTO를 읽어서 보여 준다.

**실행/학습 실패 시:** 기존 생존/사회/문명 루프는 계속 진행할 수 있어야 한다. 외부 모델 응답 대기, 매 프레임 AI 호출, 무한 재시도, 데이터 유실/중복 보상 금지.

## 5. 결정론 / 저장 / replay (확정)

- 현재 main의 `SimulationSnapshotVersion=3`, `SimulationSnapshotBinaryFormatVersion=8` 계약을 무단 변경하지 않는다.
- 기존 `WorldSeed`, `PopulationSeed`, `WorldGenerationVersion`, GUID, SimulationRuleset과 같은 정체성 및 정렬 순서를 유지한다.
- Pure deterministic candidate generation이면 같은 state/input에 같은 후보를 출력해야 한다. 난수는 WorldRNG 소비 순서를 오염시키지 않도록 안정된 분리 도메인과 사건 식별자를 사용한다.
- **로컬 LLM도 비결정론적 결과를 낼 수 있다.** temperature=0은 재현성 보증이 아니므로 채택 결정과 조건 지문을 Core 저널에 기록하고, replay/save/load 중 **이미 승인된 과거 결과를 재질의하지 않는다**.
- stale proposal, ghost actor, 유효하지 않은 target, 월드 변경, 계획 중 자원 고갈, 중복 decisionId는 fail-closed.
- 새 persisted field는 기존 codec의 모든 경로(캡처/검증/바이너리 encode/decode/restore/재개)와 테스트를 함께 변경한다. 호환 방식이 명확하지 않으면 Snapshot 스키마 변경 PR을 별도로 격리한다.
- 검증 예: save/load 중간 시점, 중복 input replay, 다른 행동 타이밍, 빠른 시뮬레이션/일수 건너뛰기, long-run, 서로 다른 seed, 손상 파일 원자 거부.
- 현재 `#686 ECO-A2a`의 동물 바이너리 코덱은 **독립 코덱 후보**다. 그 자체로 World Snapshot 통합을 완료한 것이 아니다. `#667/#668 COG-3`도 **main 미병합** 상태이며 통합 전 중복/호환성을 반드시 해결해야 한다.

## 6. 자율 동물·사냥·길들이기·사육 사례로 본 경계 (확정)

| 현상 | 세계/Core가 제공해야 할 실제 법칙 | 주민/동물이 자율 선택하는 것 | 판정/표시 |
|---|---|---|---|
| 사냥 | 시야·거리·도구·추적·도주·부상·사망·사체·유한 육류 | 사냥을 시도할지, 누구와 함께할지, 위험을 감수할지 | 실제 사냥/도축 성공 후에만 고기·가죽/뼈 생성 |
| 동물 관찰 | 독립 동물 개체, 서식처, 냄새/시각/흔적 | 관찰·회피·접근·추적·모방 | 관찰된 사실만 개인 기억으로 이동 |
| 먹이 제공 | 실제 음식 수량 소모, 개체 반응·허기·위험·스트레스 | 먹이를 제공/중단/반복할지, 대상 선정 | 동물 반응을 실제 개체 상태로 기록 |
| 길들이기 | 개체별 연속 경험, 신뢰/경계·스트레스·학습의 제약 | 상호작용을 반복·보호·이용할지 | 실제 관계/행동 변화가 확인되면 후행 관찰 |
| 사육 | 울타리 충돌·지형·공간·먹이·돌봄·자연 번식 | 가두기·방목·이동·번식 관리·포기 | 지속 행동으로 판정; '사육 해금' 없음 |
| 가축화 | 여러 세대에 걸친 유전·번식/선택 압력의 실제 결과 | 개체 선택·보호·교배·교환 등을 할지 | 유전·행동 변화가 세대 간 누적된 후 관찰 |

**절대 주의:** '사냥 → 관찰 → 길들이기 → 사육 → 가축화'는 **어떠한 필수 이벤트 체인도 아니다**. 집단에 따라 순서가 다르거나 해당 현상이 일어나지 않아도 정상이다. ECO-A1은 현재 *독립 모듈*이고 World 주민의 실제 사냥·축산까지 구현한 것이 아니다.

## 7. 기술/문명 발견의 허용 범위 (확정)

- 지금 존재하는 `TechnologyRegistry`, `TechniqueRecipe`, `CivilizationIntent`는 **기존 실행 가능성·재료·전파·호환을 보장하는 어댑터**로 재사용한다.
- 단, 고정 레지스트리는 문명 발전의 완결된 목록이 아니다. 향후 개체는 원자 affordance를 새로운 순서로 조합할 수 있어야 한다.
- '돌을 다른 용도로 사용'하는 발상은 AI가 할 수 있다. 그러나 Core에 충돌/타격/용도/내구 변화가 구현되지 않았다면 **성공으로 확정할 수 없다**.
- 새 물리 메커니즘/조합법이 필요하면 별도 Core primitive/물질 모델 PR로 추가하되, 주민 행동의 목적이나 시대별 발생 일정을 강제로 설정하지 않는다.
- 문명 라벨(농업·사육·마을·교역)은 Core에 **실제로 발생한 지속 행동·자원 변화·지식 전파**를 측정한 결과. 라벨 획득 자체가 자원을 주거나 역할·시설을 자동 변경하지 않는다.

## 8. 기존 main 자산 / 추가 확장 / 미구현 경계

| 자산 | main의 현 상태 | 책임·후속 방향 |
|---|---|---|
| `World`, `Simulation`, `SimulationClimate`, `Hydrology`, `NaturalWorldChunk` | 실제 procedural world truth | 유지. verified Earth data pack 지역 conditioning은 별도 |
| `UtilityAI.h` 여섯 생존 Goal, `Planner.h`, `ContextAction.h`, `CivilizationDecision.h` | 고정 Goal/Intent 및 Core 실행 | 생존 안전성 유지. 원자 상호작용/합성 계획 API 추가 |
| `Character` Personality/Emotion/Memory/Belief, `ActionExperienceLearning.h` | 개체 차이·일부 행동 경험 기록 | 경험 provenance/가설별 효용·실행 방법 학습으로 확장 |
| `CognitiveAgent.h` | 14개 전략 의도, read-only context, 검증 계약 | `GoalSpec`/가설/PlanCandidate 합성 계약 추가. 현 목록을 최종 행동 한계로 두지 않음 |
| `web/src/cognition` / Shadow Mode | 무료 로컬 모델 read-only 제안, Core 행동 영향 없음 | 입력 기록/선택 결과 replay 확보 후에만 실제 동작 허용 |
| `CivilizationProgression.h`, 지식 전파 | 미리 등록된 기술·전파·사회 상태 | 물리/행동 능력 검증용으로 유지, 후행 창발 분류 확장 |
| `WildlifeEcology.h` ECO-A1 | 독립 개체군/먹이/번식/포식 모듈 | World/Snapshot/DTO/주민 AI 연결 미완 |
| 지구 시나리오·`earthSolarJson()` | 지구본 결속과 태양 관찰 | 실제 지역 날씨/지형/종 분포 **미구현**, UI가 진실처럼 표시 금지 |

현재 문서 작성과 같은 시점에 다른 PR이 병합될 수 있다. 최신 `main` 변경사항이 위 현황을 추월하면 **실제 코드/검증이 우선**이다.

## 9. 미병합 작업 경계 및 병합 선행조건

1. **COG-3 #667, #668**: 같은 수락/저널 문제에 대한 별도 Draft 후보. 중복 저널·snapshot codec·actor/time fingerprint 충돌 가능성을 비교해 **하나의 source of truth**로 통합해야 한다. 둘을 독립 기능처럼 연속 병합 금지.
2. **COG-L2 #666**: 환경 경험에 기반한 수원 선택 실험. 현재 긴급 생존 Utility를 학습으로 낮추지 않는 기존 교정 원칙 유지. 부모/의존성 및 100~1000일 회귀 검증 없이 병합하지 않는다.
3. **ECO-A2a #686**: 독립 야생동물 binary codec PR. Core World나 사용자 save에 동물이 자동 등록되는 것과 혼동하지 않는다.
4. **기존 미병합 건강/육아/사회 감사 PR**: 원래 Draft·병합 금지 조건, 질병/생존 리그레션 게이트를 새 인지 PR이 우회할 수 없다.
5. 인지 구조에 따른 대규모 변경을 PR 하나에 묶지 않는다. 권한 경계, read-only instrumentation, typed proposal, accepted-input journal, 계획 실행, learning, 생태계 wiring은 **검증 가능한 작은 변경**으로 분리한다.

## 10. 단계별 구현 순서와 별도 승인 게이트

아래는 **개발 의존성**이며, AI나 사회의 강제 발전 순서가 아니다.

| 단계 | 별도 PR/변경 범위 | 병합 판정 |
|---|---|---|
| B0 (이 문서) | 권한·책임 경계 확정, main 코드 무변경 | 문서 링크/충돌 확인 |
| B1 Audit | 읽기 전용 인과 계측: 후보/선택/실행/실패/학습/전파 출처 | simulation truth 비변경, 데이터 누락/중복 검증 |
| B2 Decision persistence | COG-3 후보 통합, accepted event/snapshot/replay 단일 계약 | duplicate/stale/restart/event order/호환 gate |
| B3 Goal / Hypothesis | Core 검증 가능한 동적 목표 및 가설, shadow-only | 기존 생존·자원 변화 없음, 유효/무효 목표 분류 |
| B4 Action composition | 제한된 primitive plan, staged Core commit, fail-closed | 자원 보존·물리 경로·부상·중단/복구·시간 건너뛰기 |
| B5 Method learning | 실제 경험 기반 방법 선택, 개인별 모델 | Needs/Health 회귀 없음, 후속 행동 변화 인과 |
| B6 Social emergence | 지식/방법/오해의 사람 간 전달·모방 | 전파 provenance, 비강제 집단 행동 |
| B7 Ecology/world integration | 실제 동물/식생/환경과 인지 연결 | World+Snapshot+WASM DTO truth, 100/365/1000/3000일 |
| B8 Earth-scale | verified 지역 팩, 상이한 2개 지역 이상의 세계 | seed/region identity, 지역 환경→행동 차이 증명 |

**별도 정식 승인 필요:** 세계의 물리/생리 법칙, 외형/생활 규칙 등 사용자가 정한 설계 규칙을 바꿔야 하는 경우. 단순 AI 행동 후보 추가가 이를 우회하는 근거가 될 수 없다.

## 11. 필수 테스트 및 관찰 질문

- **Truth**: 무에서 고기·물·식물·시설이 생기지 않는가? 주민 위치/행동이 Core와 동일한가?
- **Perception**: 직접 보지 않은 지식을 알고 있다고 행동하지 않는가? 직접 목격/전언/추론 출처가 구분되는가?
- **Autonomy**: 같은 환경에서도 경험/성격이 다른 주민이 다른 목표·방법을 선택하는가?
- **Learning**: 실패 후 같은 일을 무한 반복하지 않고 다음 방법 선택이 변화하는가? 긴급 Needs는 그대로 보호되는가?
- **Hypothesis**: 검증 가능한 가설만 실제 실행되며 반증/포기가 허용되는가?
- **Emergence**: 특정 날짜/기술 해금 없이 집단 행동이 발생했는가? 사육/농업/교역 등이 **없어도 실패 처리하지 않는가**?
- **Determinism**: seed 및 accepted-input stream이 같으면 tick/skip/save/replay 결과 및 주요 사건 ID가 같은가?
- **Stability**: 100/365/1000/3000일 seed 비교에서 생존 루프·전염병·자원 고갈·사회 활동·지역 이주가 어떤 인과로 발생하는가?
- **Performance**: Android급 저사양 환경에서도 인지 이벤트 예산과 LOD가 제한되며 Core 기본 정책은 로컬로 실행 가능한가?

## 12. PR 리뷰 질문 / 변경 거부 기준

각 신규 기능 PR에서 다음을 명시한다.

- [ ] AI가 선택할 자유와 Core가 판단할 제약이 **명확히 분리**되어 있는가?
- [ ] 기존 기술/시대를 따라가도록 **강제 시나리오**를 숨겨 넣지 않았는가?
- [ ] 대상·거리·능력·물체·자원·생리 법칙을 **Core가 최종 검증**하는가?
- [ ] 관찰/전언/추론의 출처와 불확실성이 **개별 주민별**로 유지되는가?
- [ ] 중복 실행, 저장/복원 후 재추론, 세계 간 데이터 섞임을 막는가?
- [ ] 기존 생존·질병·수면·가족·사회·교역 회귀를 실행했는가?
- [ ] Web은 실제 발생한 인과를 보여 주며 없는 동작/환경을 꾸며 내지 않는가?
- [ ] 무료·로컬 기본 경로 및 Android 성능 예산을 준수하는가?
- [ ] 설계 규칙 변경이라면 별도 사전 승인과 영향 범위 보고가 있었는가?

하나라도 핵심 권위 위반이면 기능이 시각적으로 좋아도 **병합 불가**이다.

## 13. 참조 · 우선순위

우선순위: **최신 main의 실제 Core 코드/검증 결과 > 명시적 사용자 확정 설계 원칙(본 문서) > 세부 구현안/오래된 회의 기록**. 충돌 발견 시 자의적으로 코드나 물리 규칙을 교체하지 말고 차이를 문서화하고 변경을 분리한다.

- [기존 Cognitive Agent Layer v1](../COGNITIVE_AGENT_LAYER_v1.md) — COG-0~6 목표·제안/검증·로컬 모델 경계
- [Human Realism Foundation v1](../HUMAN_REALISM_FOUNDATION_v1.md) — world truth와 개체 인식/믿음 구분
- [World Architecture v2](../WORLD_ARCHITECTURE_v2.md) — 세계 우선·Core/Observer 권위·좌표·지구 규모
- [Cognitive Shadow Mode](../COGNITIVE_SHADOW_MODE.md) — Web read-only 제안, 실행 권한 미부여
- [Experience Learning Phase1](../COGNITIVE_EXPERIENCE_LEARNING_PHASE1_2026-10-12.md) — 생존 needs 점수 학습 변조 회귀/교정 사례
- [ECO-A1 Core 설계/구현](../ECO_A1_WILDLIFE_CORE_FOUNDATION_2026-10-12.md) — 현재 독립 동물 프로토타입 경계
- [ECO #683](https://github.com/sjLim91/lifelens-ue/issues/683) — 자율 사냥/길들이기/사육 비강제 원칙
- [COG-3 후보 #667](https://github.com/sjLim91/lifelens-ue/pull/667), [#668](https://github.com/sjLim91/lifelens-ue/pull/668), [COG-L2 #666](https://github.com/sjLim91/lifelens-ue/pull/666), [ECO-A2a #686](https://github.com/sjLim91/lifelens-ue/pull/686) — **문서 기준 시점에 미병합**

---

### 한 문장 계약

> **개발자는 세계의 법칙과 실행 가능한 상호작용을 만들고, 인공 생명체는 자신의 불완전한 경험을 바탕으로 목표·방법·관계·미래를 자율적으로 선택한다. 오직 Core에서 실제 발생한 결과만 세계의 역사로 남는다.**
