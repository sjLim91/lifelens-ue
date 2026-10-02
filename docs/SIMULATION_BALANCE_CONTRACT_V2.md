# LifeLens Balance Contract v2 — Current-Main Integration Gate

기준 시작점: `main@b277e091bc39d071ea30ba0d57281251da811dba`  
상태: **통합검증 계약 — 결과 확정 전**  
목적: C3~C6까지 확장된 현재 Core가 100/365/1000일에서 생존·가족·문명·다중 정착지 루프를 함께 유지하는지 검증한다.

이 문서는 예전 #540의 실험 런타임 값을 재사용하지 않는다. #540에서 유효했던 것은 **장기 계측 방법과 실패 유형**뿐이며, 현재 main을 새 기준선으로 다시 측정한다.

## 1. 절대 원칙

- Core/World가 유일한 simulation authority다.
- 장기 감사 때문에 Needs, Utility, 이동, 자원, 가족, 문명 값을 몰래 보정하지 않는다.
- 같은 seed / ruleset / snapshot은 결정론을 유지한다.
- 시간가속/장기실행은 인과 결과를 생략하지 않는다.
- Presentation/Web은 결과를 추정하거나 보정하지 않는다.
- stale #539/#540 runtime changes는 wholesale merge하지 않는다.

## 2. 검증 체크포인트

기본 장기 검증:

- 100일 — 생존 루프와 초기 정착
- 365일 — 첫 마을 성숙 가능성 / 가족 진행 / 기술·생산 발전
- 1000일 — 장기 고착, 자원압력, 이주·복수 정착지·교역의 안정성

기본 seed:
- 874213954
- 4242001

추가 seed는 이상 현상 재현 또는 수정 검증 때 사용한다.

## 3. 생존 / Physical 계약

구조적 실패로 간주할 신호:

- 해결 가능한 자원이 있는데 Hunger/Thirst가 장시간 포화 상태
- Sleep 완료 또는 의미 있는 회복 세션이 사실상 0
- Toilet/Wash가 반복 시작되지만 완료되지 않는 planner churn
- Hygiene/Bladder/Sleep 0.999 포화가 대부분의 관찰 시간을 차지
- 한 생존 행동이 나머지 자기관리를 영구 starvation

계측:
- Need 평균/최대/포화분/최장 포화 streak
- Physical start/completion
- 수면 세션 길이와 실제 회복량
- preemption / timeout / route failure

## 4. 인간 / 가족 계약

100~1000일 사이 다음 시스템이 서로를 영구 차단하지 않아야 한다.

- Social
- Dating / Engagement / Marriage
- Pregnancy / Birth
- Parenting
- LifeStage progression
- Aging / Death

특정 seed에서 사건이 반드시 발생해야 한다는 강제 스토리 계약은 두지 않는다. 다만 여러 seed에서 **관계/가족 루프가 구조적으로 0에 고정**되면 starvation 후보로 본다.

계측:
- living/deceased/population
- household/couple stage
- active pregnancy
- births after NEW GAME
- child/teen population
- major life event count

## 5. 문명 / 생산 계약

- Civilization 행동이 Physical/Social/Teaching에 영구적으로 밀리지 않는다.
- Cultivation 기회가 있는데도 모든 정상 seed가 장기적으로 plot=0에 고정되지 않는다.
- 실제 시설·도구·기술 prerequisite 없이 발전하지 않는다.
- 시설/자원 경제가 장기적으로 무한 증식 또는 즉시 붕괴하지 않는다.
- KnowledgeTeaching이 특정 주민의 시간예산을 비정상적으로 독점하지 않는다.

계측:
- 시설 종류별 운영 수
- 저장소 / 재배지
- Civilization / Teaching 시간
- 자연/운반/저장 식량·물
- generated chunk 수

## 6. C6 다중 정착지 계약

C6의 목적은 4명을 무조건 빠르게 흩어놓는 것이 아니다.

바람직한 흐름:
`첫 생활권 안정 -> 가족/인구 성장 -> carrying pressure -> 탐색/이주 -> 새 정착지 -> 실제 왕복 교역`

경고 신호:
- 초기 인구 4명이 너무 이른 시점에 2+2 등으로 갈라져 두 정착지가 모두 취약
- migration pressure가 계속 높지만 실제 정착 변화가 전혀 없음
- 새 정착지가 형성되지만 주민이 배정되지 않거나 필수 인프라가 전혀 생기지 않음
- 정착지 사이 재고가 물리 이동 없이 변함
- Trade context가 출발/도착/귀환 중 고착
- trade route가 생기지만 실제 exchange evidence가 없음

계측:
- settlementCount / activeSettlementCount / assigned residents
- 현재 migration candidate 수 / 개인별 candidate 시간
- trade route / active trade route / exchange evidence
- trade departure / exchange / return 횟수

## 7. 건강 계약

- 환경 오염/질병/부상은 실제 노출에서 파생한다.
- 모든 주민이 장기간 Critical에 고정되거나, 반대로 위험 환경에서도 건강 영향이 항상 0인 경우를 조사한다.
- 질병/부상이 생존행동 전체를 영구 고착시키지 않아야 한다.

계측:
- checkpoint health stage 분포
- 주민별 max illness / injury / environmental stress

## 8. 성능 / 저장 계약

현재 감사에서는 우선 다음을 기록한다.

- 100/365/1000일까지 wall-clock elapsed time
- GitHub runner Maximum resident set size (`/usr/bin/time -v`)
- snapshot encoded byte size
- population / materialized chunk 증가량

이 결과를 C2의 worker/LOD/history batching 설계 기준선으로 사용한다.

## 9. 이번 게이트의 판정 방식

첫 실행은 **관찰 기준선 확정**이다. 첫 결과에 맞춰 임의의 숫자를 즉시 튜닝하지 않는다.

1. 두 seed 결과 수집
2. 구조적 고착을 분류
3. 원인이 같은 문제를 하나의 causal cluster로 묶음
4. 수정 우선순위 확정
5. 최신 main에서 한 축씩 수정
6. 동일 seed / 동일 기간 A/B
7. 1000일 재검증

## 10. C7 진입 조건

C7 — Population / Settlement Maturation은 아래를 확인한 뒤 시작한다.

- 365일에서 생존 행동 고착이 없음
- Social/Civilization이 구조적으로 starvation되지 않음
- 가족 루프가 Core에서 실행 가능함이 장기 실측으로 확인됨
- C6 migration/trade가 장기 실행에서 무한 고착/순간이동 없이 유지
- 1000일 실행이 crash/timeout 없이 완료
- snapshot 크기와 runtime 비용이 다음 단계 개발을 막을 수준이 아님

C7은 단순 출산율 증가가 아니라
`4명 -> 가족 -> 가구 -> 인구 증가 -> 첫 정착지 성숙 -> carrying pressure -> 분가/이주`
를 만드는 단계로 진행한다.
