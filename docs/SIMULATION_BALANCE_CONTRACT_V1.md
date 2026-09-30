# LifeLens Balance Contract v1 (candidate)

기준: `main@0e409b53089739f737856ad508bfbdd99c80e8cb`  
상태: **후보 계약 — 장기 A/B 계측으로 확정 전**

이 문서는 "숫자 하나를 바꿔 다른 루프가 깨지는" 문제를 막기 위한 시뮬레이션 계약이다.  
최종 구현은 이 계약을 만족하는지 1/7/30/100/365/500/1000일 동일 seed A/B로 검증한다.

## A. 행동 선점 계약

### A-1. 짧은 실제 상호작용
이미 **상호작용 단계에 들어간** Eat / Drink / UseToilet / Wash는 이동이 끝난 뒤에는 원칙적으로 완료한다.

- 이동 중: Critical Hunger/Thirst가 선점 가능
- 실제 사용 중: 수분 이내의 bounded action은 완료
- 완료 후: 다음 planning boundary에서 가장 심각한 Need 재평가

목적: UseToilet 같은 2~8분 행동이 1분마다 취소/재계획되는 planner thrash 제거.

### A-2. 수면
Sleep은 장시간 행동이므로 절대 비선점 행동으로 만들지 않는다.

후보 계약:
- wake band: 기존 0.72 유지
- dominance margin: 0.05
- Hunger/Thirst/Bladder가 wake band를 넘는 것만으로는 깨우지 않음
- 경쟁 Need가 현재 Sleep Need보다 **0.05 초과 더 심각**할 때만 중단
- Critical survival preemption도 실제 수면 중에는 같은 상대심각도 원칙 사용
- 침상으로 이동 중에는 기존처럼 Critical survival 선점 가능

이 방식은 0.72→0.90처럼 전체 threshold를 올리지 않고, "무엇이 더 심각한가"만 비교한다.

## B. Need band 계약

v1 첫 A/B에서는 기존 band를 최대한 보존한다.

- ordinary urgent: 0.70
- survival provision: 0.74
- critical Hunger/Thirst: 0.90

이 숫자 자체보다 먼저 **행동 취소 루프를 제거**한다.  
A/B 후 필요하면 band를 중앙 Ruleset으로 이동하며 조정한다.

## C. Social 계약

현재 `!hasUrgentPhysicalNeed` 호출부 hard gate는 제거 후보이다.

원칙:
- Critical survival만 Social 시작을 강제 차단
- 그 외에는 Physical과 Social이 실제 Utility로 경쟁
- 이미 시작한 짧은 Social context를 사소한 Physical pressure가 매분 지우지 않음
- 초기 관계 형성을 가능하게 하는 stranger bootstrap 범위를 보장

현재 `minimumSocialUtility=0.18`은 평균 stranger Approach 약 0.115에 비해 높다.

A/B 후보 범위:
- minimum social utility: **0.10 ~ 0.14**
- 시작점: **0.12**
- 단, Social이 Physical urgent를 압도하지 않도록 기존 physical winner 비교는 유지

## D. KnowledgeTeaching 계약

KnowledgeTeaching은 Social/Civilization과 별도 scheduler로 생존 행동을 무조건 지우면 안 된다.

필수:
- teacher가 active Physical plan 중이면 teaching 시작 금지
- learner의 active critical self-care도 방해하지 않음
- 동일 teacher가 매시간 계속 선택되는 독점 방지
- 장기 시간예산에서 KnowledgeTeaching이 한 주민의 20%를 지속적으로 넘지 않도록 계측

1차 구현은 **plan.empty() gate**부터 적용하고, cooldown은 A/B 후 결정한다.

## E. Physical 회복량 계약

### E-1. Bladder
현재 수지는 기본적으로 약 6회/day/person 수준이라 planner thrash 제거 전에는 수치 변경하지 않는다.

### E-2. Hygiene
현재 원시 fallback:
- base hygiene pressure: 1.008/day
- outdoor toilet 추가 burden 약 0.299/day
- raw-water wash 회복: 0.072/action

이 조합은 1인당 하루 18회 안팎의 wash를 요구해 비현실적이다.

v1 목표:
- 원시생활 wash: 평균 **1~3회/day/person**
- 위생 인프라 발전 후: 평균 **1~2회/day/person**
- hygiene가 0.999 이상인 시간: 장기 평균 **<5%**

후보 조정축:
1. hygienePerMinute 감소
2. natural-water Wash 총 회복량 증가
3. sanitation pit 사용 시 hygiene/residue burden 감소가 실제 physical runtime에 반영

한 축만 독립 조정하지 않는다.

### E-3. Sleep
현재 outdoor sleep 순회복이 너무 낮아 10시간 max session으로 정상 회복이 어렵다.

v1 목표:
- 초기 야외 수면: 대략 8~11시간 범위에서 의미 있는 회복
- SleepingPlace/Shelter: 야외보다 확실히 효율적
- Sleep=0.999 이상 장기 체류: **<5%**
- Sleep completion: 0회 상태 금지

## F. Water 경제 계약

행동 scheduler를 고치면 Wash 완료수가 급증할 수 있으므로 water budget과 함께 검증한다.

목표:
- 4인 초기집단에서 지속 접근 가능한 freshwater 1개 생활권이 즉시 구조적 적자가 되지 않음
- sanitation/storage 발전은 물 운반/세정 비용을 낮추는 방향
- resource node 수만 늘려 문제를 숨기지 않음

## G. Food / Cultivation 계약

현재 4인 fallback Eat 수요는 약 20.6 unit/day이고, 1 plot 생산 상한은 장기 생존에 의미가 거의 없다.

v1 목표:
- 야생 식량만으로 영구 정착을 보장하지 않음
- 그러나 Cultivation을 발견/운용하면 **실제 food budget에 의미 있는 기여**를 해야 함
- population 증가 시 desired plot 수가 생산 필요량과 함께 증가
- 100~365일 정상 seed에서 cultivation opportunity가 존재하는데도 plot=0이 영구 지속되지 않도록 함

조정축:
- Eat 1회 회복량
- plant food unit semantics
- plots/resident
- growth days
- harvest yield
- seed cost / care / season

## H. 장기 합격 기준

아래는 최종 v1 병합 전에 확인한다.

- Critical Hunger/Thirst 해결책이 있으면 장시간 Idle 금지
- UseToilet plan start/completion 비율이 planner churn 수준으로 무너지지 않음
- Sleep 세션이 실제 완료되거나 정상 wake reason으로 종료
- Sleep/Hygiene/Bladder 0.999 체류가 장기적으로 상시 상태가 아님
- Social이 100일 동안 사실상 0회가 되는 구조적 starvation 제거
- KnowledgeTeaching이 특정 주민 시간예산을 독점하지 않음
- Civilization/Cultivation이 Physical loop 때문에 영구적으로 후보에서 사라지지 않음
- resource economy가 행동 정상화 후에도 구조적 적자가 아님

## I. 적용 순서

1. Action commitment + relative sleep preemption
2. Hygiene/Sanitation/Water budget
3. Social bootstrap + hard gate
4. KnowledgeTeaching scheduler
5. Food/Cultivation economy
6. Ruleset 중앙화
7. 전 기간 다중 seed 회귀
