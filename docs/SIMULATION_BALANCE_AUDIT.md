# LifeLens 시뮬레이션 밸런스 전수검사

기준: `main@0e409b53089739f737856ad508bfbdd99c80e8cb`  
작성일: 2026-09-30  
목적: 장기 진행에서 한 수치 수정이 생존/사회/문명 루프를 연쇄적으로 깨뜨리는 문제를 막기 위해, Core의 시간·Need·Utility·행동효과·자원경제 수치를 하나의 계약으로 재정의한다.

> 이 문서는 먼저 **현재값과 충돌을 고정**한다. 이 브랜치에서는 근거 없이 런타임 밸런스 값을 바꾸지 않는다. #539의 실험 변경은 기준에 포함하지 않는다.

## 1. 1차 결론

현재 장기 고착은 단일 임계치 버그가 아니다. 아래 네 문제가 결합되어 있다.

1. **생존 임계치가 분산되어 있다.**
   - 일반 Urgent: `0.70`
   - 수면 중 깨우기: `0.72`
   - 긴급 식량/물 확보: `0.74`
   - Civilization 일반 진입 상한: `maximumResidentNeed < 0.74`
   - Critical survival preemption: `0.90`
   - 호출부의 Social 진입은 사실상 **모든 Need가 0.70 미만일 때만** 허용된다.
2. **원시 생활 fallback의 소비량과 자연 재생량이 장기적으로 맞지 않는다.**
   - 4인 기준 Water fallback 이론 소비량은 약 **96 unit/day**까지 올라갈 수 있다.
   - 자연 Water node 재생량은 **28 unit/day/node**다.
   - PlantFood fallback 이론 소비량은 4인 기준 약 **20.6 unit/day**다.
   - PlantFood node 재생량은 **9 unit/day/node**다.
3. **재배가 현재 식량경제를 대체할 생산력이 없다.**
   - plot 1개 / 주민 4명
   - 기준 성장 24일에 환경/수분/계절/돌봄 계수가 추가로 곱해진다.
   - 수확량은 3~10 unit이고 씨앗 1 unit을 소비한다.
   - 최상의 단순 상한으로도 순생산은 약 **9 / 24 = 0.375 unit/day/plot** 수준이다.
   - 반면 4인 fallback 식량 소비는 약 **20.6 unit/day**다.
4. **Social은 Utility 경쟁 이전에 호출부에서 차단된다.**
   - `beginPlan()`의 `hasUrgentPhysicalNeed`는 Hunger/Thirst/Sleep/Bladder/Hygiene 중 하나라도 `>=0.70`이면 true다.
   - `trySocialDecision()`은 `!hasUrgentPhysicalNeed`일 때만 호출된다.
   - 따라서 장기적으로 어느 하나의 Need가 계속 0.70 부근에 머무르면 사회 행동은 Utility 점수가 아무리 높아도 후보에 오르지 못한다.

## 2. Need 증가율

기본 `SimulationRuleset`:

| Need | 분당 증가 | 하루 누적(클램프 전) | 0 → 0.70 | 0 → 0.90 |
|---|---:|---:|---:|---:|
| Hunger | 0.0010 | 1.440 | 11.67h | 15.00h |
| Thirst | 0.0013 | 1.872 | 8.97h | 11.54h |
| Sleep | 0.0008 | 1.152 | 14.58h | 18.75h |
| Bladder | 0.0011 | 1.584 | 10.61h | 13.64h |
| Hygiene | 0.0007 | 1.008 | 16.67h | 21.43h |

실제 주민은 metabolism / sleepTendency와 환경 압력을 추가로 받으므로 위 시간은 기준값이다.

## 3. Physical fallback 행동 효과

### 원시/비시설 fallback

| 행동 | 사용시간 | 1회 효과 | 이론적 필요 횟수/일/인 |
|---|---:|---:|---:|
| Eat | 1분 | Hunger -0.28 | 5.14 |
| Drink | 1분 | Thirst -0.32 | 5.85 |
| Sleep | 30~600분 | Sleep -0.00150/분 | 별도 계산 |
| UseToilet | 2분 | Bladder -0.26, Hygiene +0.024 | 6.09 |
| Wash | 4분 | Hygiene -0.072 | 14.00 + 용변 오염 |

야외 용변은 행동 자체의 Hygiene +0.024 외에도 residue 처리에서 Hygiene +0.025가 추가된다.  
즉 야외 용변 1회당 총 Hygiene 부담은 약 **+0.049**다.

기준 Bladder 증가율로 야외 용변은 약 6.09회/일이고, 이로 인한 추가 Hygiene 부담은 약 **0.299/day**다.  
따라서 base Hygiene 1.008/day와 합치면 Wash fallback 필요량은 약 **18.15회/day/person**까지 상승한다.

### Water 경제의 이론적 fallback 상한

- Drink: 약 5.85 water/day/person
- Wash: 약 18.15 water/day/person
- 합계: 약 **24.0 water/day/person**
- 4인: 약 **96 water/day**
- 자연 Water node 재생: **28/day/node**

따라서 현재 원시생활은 **물 1개 node로 장기 유지가 구조적으로 불가능**하다. 3~4개의 지속 접근 가능한 freshwater node 또는 훨씬 효율적인 위생 인프라가 필요하다.

> 이 값은 행동이 Need 증가를 정확히 상쇄한다는 steady-state 근사다. 실제 환경압력/경로/시설/성격에 따라 달라지지만, 현재 규모 차이를 확인하는 용도로 충분하다.

## 4. Sleep 회복 계약

현재 fallback sleep recovery는 `0.00150/min`, 기본 Sleep 증가율은 `0.0008 * sleepTendency`다.

| sleepTendency | fallback 순회복/분 | 1.00 → 0.12 필요시간 | 10시간 수면 후 Need |
|---:|---:|---:|---:|
| 0.8 | 0.00086 | 17.05h | 0.484 |
| 1.0 | 0.00070 | 20.95h | 0.580 |
| 1.2 | 0.00054 | 27.16h | 0.676 |

현재 최대 수면 세션은 10시간이다. 따라서 야외 수면은 정상 성인도 `RestedSleepNeedTarget=0.12`에 도달할 수 없고, sleepTendency가 높은 주민은 10시간을 자도 Urgent 0.70 바로 아래에서 끝난다.

침상/시설 수면은 약 `0.00185~0.00240/min` 계열의 회복을 사용해 훨씬 낫다.  
**시설 유무가 생존가능성 자체를 지나치게 갈라놓는지** 검증이 필요하다.

## 5. Food 경제

기본 Hunger 증가: 1.44/day/person.  
fallback Eat 1회: -0.28 + PlantFood 1개 소비.

- 약 **5.14 food/day/person**
- 4인 약 **20.57 food/day**
- 자연 PlantFood 재생: **9/day/node**

즉 접근 가능한 PlantFood node가 1~2개뿐이면 장기적으로 적자다. 여기에 씨앗 투입과 부패까지 더해진다.

### 부패
- 휴대 식량 freshness: -0.085/day
- 저장 식량 freshness: -0.045/day
- spoil threshold: 0.08

초기 freshness 1.0 가정 시 단순 수명은 대략:
- 휴대: 약 11일
- 저장: 약 21일

저장 인프라가 유효하지만, 생산 자체가 부족한 경우 부패 최적화만으로는 해결되지 않는다.

## 6. Cultivation 경제

현재:
- `CultivationResidentsPerPlot = 4`
- `CultivationFoodReservePerResident = 4`
- `CultivationBaseGrowthDays = 24`
- seed: PlantFood 1 unit
- harvest: 최소 2, 정상 계산식은 약 3~10 unit
- daily growth에는 fertility / moisture / temperature / season / care가 모두 곱해진다.
- care는 매일 -0.055, moisture도 증발/강우 영향을 받는다.

가장 낙관적인 단순 상한조차 **순 9 unit / 24일 = 0.375 unit/day/plot**이다.  
4인 fallback food budget 약 20.57/day와 비교하면 **50배 이상 부족**하다.

따라서 현재 Cultivation은 "문명 발전 표시"는 가능하지만 **식량 자립 시스템으로 기능하지 못한다.**

## 7. 의사결정 임계치와 주기

| 항목 | 현재값 | 위치/의미 |
|---|---:|---|
| Utility urgent threshold | 0.70 | 일반 Physical urgent |
| Sleep wake threshold | 0.72 | 수면 중 Hunger/Thirst/Bladder |
| Survival provision threshold | 0.74 | 식량/물 Retrieve/Gather/Explore |
| Civilization need gate | < 0.74 | 일반 Civilization 경쟁 |
| Critical survival | 0.90 | 생존 preemption |
| Social minimum utility | 0.18 | Unified Utility |
| Civilization minimum utility | 0.14 | Unified Utility |
| Social > Physical margin | 1.05x | Unified Utility |
| Civilization > winner margin | 1.08x | Unified Utility |
| Physical replan cadence | 5분 | plan empty 시 |
| Social/Civilization cadence | 15분 | 일반 context decision |
| Parenting scan cadence | 30분 | dependent care |
| Daily world economy | 1440분 | regen/cultivation/spoilage |
| Social context timeout | 45분 | 이동 포함 |
| Civilization timeout | 120분 | 일반 |
| Explore timeout | 360분 | 장거리 탐색 |
| failure backoff | 30분 | 3회 실패 후 |
| critical failure backoff | 5분 | Hunger/Thirst critical |

### 구조적 충돌

1. `0.70`, `0.72`, `0.74`는 의미가 다르지만 너무 가깝고 서로 다른 파일에 하드코딩되어 있다.
2. Social은 Unified Utility 내부에서는 Physical과 경쟁할 수 있지만, 실제 `beginPlan()` 호출부가 Need >=0.70에서 Social 호출 자체를 막는다.
3. Civilization은 일반적으로 Need <0.74에서만 경쟁하지만, missing provision은 예외로 강제 진입한다.
4. 실패 backoff 30분은 0.70~0.89의 비critical 상태에서 행동 실패가 반복되면 장시간 Idle을 만들 수 있다.

## 8. 현재 가장 높은 위험도

### P0 — 장기 생존 경제
- fallback Wash 물 소비량
- 야외 용변의 Hygiene 재오염량
- fallback Eat 회복량과 PlantFood 재생량
- Cultivation 생산성

### P0 — Decision starvation
- Social의 `hasUrgentPhysicalNeed` hard gate
- missing provision과 Sleep/Toilet/Wash 사이의 우선순위
- active action을 critical preemption이 언제 중단할지

### P1 — Sleep
- 야외 수면 최대 10시간과 실제 순회복률의 불일치
- 시설 수면과 야외 수면의 격차
- 수면 중 깨우기 threshold와 dominance 정책의 분리 필요

### P1 — Cadence / backoff
- 5/15/30분 cadence가 동일 Utility space를 서로 다른 빈도로 샘플링
- failure backoff가 비critical 생존 Needs를 30분 방치할 수 있음

### P2 — Family / lifecycle / development
- 가족 전이/임신/육아 수치는 별도 도메인으로 잘 분리되어 있으나, 성인 생존·사회 시간예산이 안정된 뒤 실제 발생 빈도를 장기 계측해야 한다.

## 9. 밸런스 계약 재설계 원칙

최종 값은 계측 후 확정한다. 우선 구조는 다음처럼 통일한다.

### 9.1 단계
- **Normal**: 모든 Physical / Social / Civilization이 Utility 경쟁
- **Pressure**: Physical에 가중치 증가. Social/Civilization은 금지하지 않음
- **Urgent**: 해결 가능한 자기관리/생존 행동을 강하게 우선
- **Critical**: 즉시 생존 preemption. 단, 이미 수행 중인 더 심각한 생존 자기관리는 비교 후 유지

단순히 “Need 하나가 0.70 이상이면 사회활동 금지” 같은 호출부 hard gate는 제거 대상으로 검토한다.

### 9.2 중앙화 대상
아래 정책 숫자는 `SimulationRuleset` 또는 하위 Ruleset으로 이동한다.

- Need band thresholds
- sleep wake / dominance
- social/civilization minimum utility와 winner margin
- decision cadences
- plan failure backoff
- Physical action durations/effects
- resource regeneration
- spoilage
- cultivation demand/growth/yield
- 필요 시 sanitation burden

목표는 **같은 개념의 숫자를 한 곳에서만 정의**하는 것이다.

## 10. 장기 계측 항목

검증 구간:
- 1일
- 7일
- 30일
- 100일
- 365일
- 500일
- 1000일

각 구간에서 최소 다음을 기록한다.

### 주민
- Need 평균 / 최대 / 1.0 체류시간
- Physical / Social / Civilization / Parenting / Idle 시간 비율
- 행동 실패 / route failure / backoff 횟수
- 수면 총시간 / 세션 길이 / 중단 횟수
- 식사/음수/용변/씻기 횟수
- 사회 상호작용 횟수 및 종류

### 세계
- Water / PlantFood 생산·재생·소비·부패
- 저장량
- 탐색한 chunk 수
- 재배 plot 수 / 파종 / 수확 / net yield
- 시설 건설/수리/폐허화
- 기술 발견/전파
- 관계 변화/연애/가족 전이

## 11. 회귀 테스트 방향

순간적인 "첫 행동이 Eat여야 한다"만으로 정책을 고정하지 않는다.

필수 계약 예시:
- 해결 가능한 Critical Hunger/Thirst의 재계획 지연은 5분 이내
- 접근 가능한 해결책이 있는데 Need=1.0에서 장시간 Idle 금지
- 극심한 피로 상태가 식량 탐색 때문에 영구적으로 수면을 못 하는 상황 금지
- 높은 sociability 주민이 모든 Needs가 critical이 아닌 장기간 동안 Social 후보 자체에서 영구 배제되지 않음
- 농업이 정착한 4인 집단에서 장기 food budget이 구조적으로 음수가 되지 않음
- sanitation 발전이 Water 소비량을 실질적으로 낮춰야 함

## 12. 다음 작업 순서

1. 모든 Core 숫자를 **정책 / 표현 / 순수 물리·월드생성**으로 분류
2. 정책 숫자만 중앙 Ruleset 후보로 목록화
3. 장기 측정 harness 작성
4. 현행 baseline 측정
5. 변경안 A/B를 같은 seed 세트로 비교
6. 한 묶음씩 적용
   - 생존경제
   - 자기관리
   - Social/Civilization 경쟁
   - Sleep
   - cadence/backoff
7. 1/7/30/100/365/500/1000일 회귀 후 main 병합



## 13. Baseline 실측 — main 0e409b53

계측기: `ll_balance_audit`  
seed: `874213954`, `4242001`  
구간: 1 / 7 / 30 / 100일

### 13.1 100일 결과 요약

#### seed 874213954

4명 합산 576,000 person-minute 기준:

- Physical: 265,704분 (**46.1%**)
- Social: 41분 (**0.007%**)
- Civilization: 25,083분 (**4.35%**)
- KnowledgeTeaching: 62,764분 (**10.9%**)
- Idle/inactive: 222,408분 (**38.6%**)

Need가 0.999 이상으로 포화된 시간:

- Hygiene: **86.2%**
- Sleep: **75.4%**
- Bladder: **40.0%**

개별 주민 중 한 명은 100일 동안 Hygiene가 137,034분 포화되어 전체 기간의 약 95%를 사실상 최대치에서 보냈다.

#### seed 4242001

- Social presentation minute: **0분**
- Hygiene 0.999+ 포화: **84.0%**
- Sleep 0.999+ 포화: **54.9%**
- Bladder 0.999+ 포화: **6.1%**

seed가 달라도 **Hygiene와 Sleep이 장기간 상한에 붙고 Social이 사실상 사라지는 현상은 재현**된다.

### 13.2 이미 7일에 붕괴 조짐

seed 874213954의 7일 시점:
- 주민별 Hygiene 평균: 약 0.86~0.96
- 주민별 Sleep 평균: 약 0.77~0.92
- Social: 두 주민 0분, 나머지도 각각 9~10분 수준

seed 4242001의 7일 시점:
- 주민별 Hygiene 평균: 약 0.92~0.95
- 주민별 Sleep 평균: 약 0.90~0.96
- Social: 전원 0분

즉 사용자가 400일에서 관찰한 고착은 400일에 처음 생기는 문제가 아니다. **1주 이내부터 누적되며 장기 진행에서 눈에 띄게 굳는 구조적 불균형**이다.

### 13.3 자원량만으로 설명되지 않음

100일 시점에도 자연 Water/PlantFood 총량은 새 chunk 탐색과 재생 때문에 완전히 0이 아니었다.

예: seed 874213954
- generated chunks: 5
- natural Water: 523
- natural PlantFood: 1457

그런데도 Sleep/Hygiene 포화와 Social starvation이 지속됐다.

따라서 장기 고착의 1차 원인은 단순한 "물이 다 떨어짐/음식이 다 떨어짐"이 아니라:
- 행동 회복량
- preemption
- 결정 cadence
- Social hard gate
- 별도 scheduler
의 결합이다.

## 14. Out-of-band scheduler 전수검사

### KnowledgeTeaching — P0/P1 경계 위험

`advanceCivilizationKnowledgeTeaching()`은 매 정각(`minute % 60 == 0`) 실행된다.

현재 특징:
- teacher의 `pendingContext`만 확인하고 **진행 중 Physical plan은 확인하지 않는다.**
- 후보가 선정되면 teacher의 기존 `goal=Idle`, `plan.clear()`, navigation clear를 수행한다.
- 즉 Hunger/Thirst가 Critical까지 가지 않은 상태에서는 Sleep/Toilet/Wash 등 현재 행동을 **Utility 경쟁 없이 시간당 한 번 끊을 수 있다.**
- KnowledgeTeaching context timeout은 45분이다.
- 별도 social cooldown과 같은 teaching cooldown은 없다.

100일 baseline에서 특정 주민은 KnowledgeTeaching presentation이:
- 53,171분 / 144,000분 = **36.9%**
- 다른 seed에서도 54,884분 = **38.1%**
까지 올라갔다.

이는 "지식 전파"가 생활의 일부가 아니라 일부 주민의 시간예산을 독점할 수 있다는 강한 신호다.

**조정 원칙 후보**
- KnowledgeTeaching도 Unified Utility 또는 최소한 동일한 Pressure/Urgent gate에 참여
- 현재 Physical plan을 임의로 clear하지 않음
- teacher/learner별 cooldown 도입 검토
- 반복 teaching의 marginal utility 감소
- Need와 Social/Civilization 시간예산을 계측한 뒤 빈도 확정

Parenting은 30분 cadence지만 caregiver가 `pendingContext.active()==false`이고 `plan.empty()`일 때만 후보가 되므로 KnowledgeTeaching과 같은 즉시 plan-clear 문제는 현재 확인되지 않았다.

## 15. 계측기 보강

첫 baseline의 `PresentationAction` 분 단위 샘플은 1분짜리 Eat 같은 짧은 행동을 완료 직후 놓칠 수 있다.  
따라서 계측기에 아래 이벤트 카운터를 추가했다.

- Physical start / completion: Eat, Drink, Sleep, Toilet, Wash
- Social event
- Civilization event
- Sleep interruption
- Critical preemption
- route failure / timeout

다음 장기 실행부터는 "행동 시간"과 "행동 횟수"를 같이 사용한다.
