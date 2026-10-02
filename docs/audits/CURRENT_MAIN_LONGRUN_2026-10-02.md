# Current-Main Long-Run Audit — 2026-10-02

기준 제품 main: `b277e091bc39d071ea30ba0d57281251da811dba`  
감사 구현 branch: `audit/current-main-longrun-20261002`  
워크플로: `LifeLens Current-Main Long-Run Audit`  
검증 seed: `874213954`, `4242001`

## 1. 결론

현재 Core는 1000일 headless 실행 자체는 빠르고 안정적으로 완료한다. 그러나 **C7 Population / Settlement Maturation에 바로 진입하면 안 된다.**

두 기준 seed에서 공통적으로 다음 장기 통합 문제가 확인됐다.

1. 성인 self-care에서 Hunger/Thirst 선점과 장거리 Toilet 이동이 반복되어 Sleep/Bladder/Hygiene가 구조적으로 고착될 수 있다.
2. 질병이 실제 장기 사망의 주된 원인으로 나타난다.
3. 영유아 돌봄 행동은 실행되지만 현재 빈도/공급 체계가 Baby의 생리 증가율을 따라가지 못한다.
4. 한 seed는 가족/재배/사회 루프까지 진입하지만, 다른 seed는 100일 안에 4명 중 3명이 사망하면서 세계 발전 자체가 사실상 종료된다.
5. 두 seed 모두 복수 정착지/교역이 자연발생하지 않았다. 현재 결과만으로 C6가 실패했다고 볼 수는 없지만, 인구·가족이 충분히 유지되지 않아 C6의 자연 장기행동을 검증할 조건이 성립하지 않았다.
6. snapshot 크기는 소규모 인구에도 1000일에서 12.5~29.3MB까지 증가한다. C2 scale debt로 별도 분석이 필요하다.

따라서 순서는 다음으로 고정한다.

`P0 self-care/local sanitation -> P0 dependent care -> 동일 seed A/B -> C6 장기 재검증 -> C7`

---

## 2. 실행 성능

### seed 874213954

- 1000일 wall clock: 약 65초
- 최대 RSS: 약 161MB
- 1000일 snapshot: 29,348,205 bytes

### seed 4242001

- 1000일 wall clock: 약 54초
- 최대 RSS: 약 75MB
- 1000일 snapshot: 12,467,286 bytes

현재 4~5명 규모의 CPU 실행 시간 자체는 병목이 아니다. 다만 인구가 증가한 상태의 scaling은 아직 검증되지 않았다.

---

## 3. seed 874213954 — 조기 사회 붕괴

### 인구

| Day | Total | Living | Deceased | Births |
|---:|---:|---:|---:|---:|
| 100 | 4 | 1 | 3 | 0 |
| 365 | 4 | 1 | 3 | 0 |
| 1000 | 4 | 1 | 3 | 0 |

100일 이전 사망 3건은 모두 **Illness**였다.

- illness deaths: 3
- accident deaths: 0
- environmental exposure deaths: 0
- other deaths: 0

가족/관계 결과:
- households 0
- dating/marriage/pregnancy 0
- birth 0

### sole survivor self-care

1000일 survivor id=2:

- Sleep saturation: 1,304,683 / 1,440,000 min = 약 90.6%
- Bladder saturation: 1,288,498 / 1,440,000 min = 약 89.5%
- Hygiene saturation: 1,336,607 / 1,440,000 min = 약 92.8%
- Toilet starts 171,823 / completes 717 = 완료율 약 0.42%
- Sleep starts 825 / completes 32
- preemptions 55,509
- Social time 114 min
- Civilization time 32,538 min
- Idle time 1,073,524 min
- max illness severity 1.0

이 상태는 단순한 높은 Needs가 아니라 **계획 시작/이동/선점/재계획 churn**으로 분류한다.

### 정착/문명

1000일:
- active settlements: 1
- migration candidates: 0
- trade routes: 0
- cultivated plots: 0
- storage facilities: 1
- fire pits: 1
- sleeping places: 5
- shelters: 0
- furnaces: 0

초기 인구 붕괴 때문에 C6 자연발생 검증 조건이 성립하지 않았다.

---

## 4. seed 4242001 — 가족 루프는 진행하지만 장기 생존 불안

### 인구/가족

| Day | Living | Deceased | Married | Pregnancy | Births |
|---:|---:|---:|---:|---:|---:|
| 100 | 3 | 1 | 0 | 0 | 0 |
| 365 | 3 | 1 | 1 | 1 | 0 |
| 1000 | 2 | 3 | 0 | 0 | 1 |

이 seed는 실제로:

`dating -> marriage -> pregnancy -> birth`

까지 진행했다. 가족 시스템이 장기적으로 완전히 starvation된 것은 아니다.

그러나 1000일까지 사망 3건은 모두 **Illness**였다.

### 정착/생산

1000일:
- active settlement 1
- cultivated plots 2
- storage facilities 3
- fire pit 1
- sleeping places 8
- shelter 1
- trade route 0

이 seed에서는 재배/사회/가족이 작동했지만 인구가 충분히 늘기 전에 성인 사망이 누적됐다.

---

## 5. Baby dependent-care 결과

출생한 resident id=5 관찰 시간: 591,601 min.

Baby는 `requiresDirectCare`이므로 직접 Physical plan을 수행하지 않는 것이 의도된 동작이다. 따라서 생리 Needs는 caregiver의 Parenting action이 해결해야 한다.

### 실제 수행된 Parenting

1000일 누적:

- Feed: 695
- PutToSleep: 580
- Bathe: 298
- ToiletAssist: 266
- Play: 1
- Hold / Educate / Discipline / Comfort / HealthCare: 0

즉 **돌봄 시스템이 전혀 실행되지 않은 것은 아니다.**

### 그러나 Baby Needs

- Hunger saturation: 약 45.7%
- Thirst saturation: 약 92.9%
- Sleep saturation: 약 49.4%
- Bladder saturation: 약 75.2%
- Hygiene saturation: 약 52.0%
- max illness severity: 1.0

현재 기본 Need 증가율에 Baby profile이 적용되면 Baby는 성인보다:
- metabolism 1.18x
- sleep tendency 1.55x

를 사용한다.

현재 Parenting은 30분 cadence에서만 후보를 만들고, caregiver가 **pending context도 없고 plan도 완전히 비어 있을 때만** 돌봄 후보가 될 수 있다. 따라서 성인 생존/문명 행동이 계속 이어지는 상황에서는 urgent dependent care가 낮은 우선순위로 밀릴 수 있다.

또한 Feed는 caregiver가 **이미 들고 있는 PlantFood/portable Water**만 사용할 수 있다. 아이가 굶거나 목말라도 caregiver가 아이를 위해 먼저 식량/물을 확보하는 전용 provisioning path는 현재 없다.

이 둘을 P0 원인으로 분류한다.

---

## 6. Self-care source diagnosis

### 6.1 urgentPhysicalGoal 불균형

현재 `Simulation::beginPlan`의 urgent direct candidate는:

- Eat
- Drink
- UseToilet

만 비교한다.

Sleep과 Wash는 `hasUrgentPhysicalNeed`에는 포함되지만 `urgentPhysicalGoal` 후보에는 없다.

결과적으로 Sleep/Hygiene가 포화여도 Eat/Drink/UseToilet의 urgent path가 계속 우선될 수 있다.

### 6.2 critical provision preemption

`preemptForCriticalSurvival`은 Hunger/Thirst가 critical band에 들어오면 진행 중 행동을 중단할 수 있다.

짧은 Toilet/Wash 상호작용이 이미 시작된 뒤에는 보호하지만, **목표까지 이동 중인 Toilet은 여전히 중단 가능**하다.

### 6.3 sanitation target가 settlement-local이 아님

`resolveSanitationUseTarget`은 현재:

`activePrimitiveSanitationSite(sites)`

가 하나라도 있으면 거리/생활권과 무관하게 그 site를 반환한다.

즉 C6에서 storage/fire/furnace는 지역 정착지 권위로 분리됐지만 sanitation은 아직 world-global active site 성격이 남아 있다.

이는:
- 먼 화장실로 반복 출발
- 이동 중 Hunger/Thirst critical preemption
- 다시 Toilet 계획
- 다시 먼 site 이동

형태의 churn을 만들 수 있는 구조다.

P0 수정은 임의의 Need 수치 완화보다 **지역 위생 affordance + urgent physical fairness + 이동 commitment**를 먼저 해결한다.

---

## 7. Health diagnosis

두 seed에서 관찰된 모든 장기 사망은 Illness였다.

이는 C4 mortality 자체가 반드시 과도하다는 뜻은 아니다. 현재 health input은 실제로:

- Hunger
- Thirst
- Sleep
- Hygiene
- local contamination

을 받는다.

따라서 먼저 self-care와 dependent-care 고착을 해결한 후 같은 seed에서 illness/mortality가 얼마나 감소하는지 비교해야 한다.

**사망 확률을 먼저 낮추지 않는다.**

---

## 8. C6 판정

현재 C6-A/B/C 기능 회귀는 통과해 있지만, 이번 두 자연 장기 seed에서는:

- settlement count = 1
- migration candidate = 0
- trade route = 0

으로 유지됐다.

현재 자연 자원량이 충분했고, 특히 인구가 성장하기 전에 사망이 발생했으므로 이것만으로 migration/trade bug라고 판정하지 않는다.

C6 natural long-run acceptance는 self-care / family 생존 수정 후 동일 seed를 다시 돌려:
- 첫 정착지 성숙
- carrying pressure
- 탐색/이주
- 두 번째 정착지
- 실제 왕복 교역

조건을 다시 검증한다.

---

## 9. 수정 우선순위

### P0-A — Self-care / local sanitation

1. urgent Physical 후보에서 Sleep/Wash starvation 제거.
2. Hunger/Thirst의 실제 생존 우선은 유지.
3. 이미 시작했거나 합리적으로 완료 가능한 자기관리 commitment 보호.
4. sanitation target를 settlement/local-distance 기반으로 전환.
5. 포화 Bladder에서 비현실적으로 먼 지정 위생장소 때문에 영구 이동하지 않도록 emergency/local fallback 계약 추가.
6. 동일 seed 100/365/1000 A/B.

### P0-B — Dependent care

1. Baby/Toddler physiological urgency가 성인의 낮은 우선순위 Civilization/Social보다 앞서도록 care scheduling.
2. caregiver가 아이를 위해 필요한 Food/Water를 실제로 확보하는 provisioning path.
3. Feed/PutToSleep/ToiletAssist/Bathe의 빈도가 child physiology 증가율을 실제로 감당하는지 수지 검증.
4. 직접 행동 불가라는 Baby 계약은 유지.
5. 실제 resource 소비/이동/Parenting interaction 유지.

### P1 — C2 scale debt

- snapshot size 구성요소 분석
- SocialKnowledge / residue / history growth
- Web Worker
- OPFS/IndexedDB persistence
- population/entity LOD
- permanent GUID

---

## 10. C7 진입 판정

현재: **BLOCKED**

다음이 확인되기 전에는 C7 feature expansion을 시작하지 않는다.

- 두 baseline seed에서 self-care saturation/churn 제거
- 100일 조기 질병붕괴가 구조적으로 완화
- Baby physiological care가 장기적으로 실제 회복
- 1000일 crash/timeout 없음 유지
- 가족 루프가 starvation되지 않음
- C6 natural migration/trade를 검증할 만큼 인구/정착지가 유지됨

