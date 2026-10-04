# Current-Main Long-Run Audit — 2026-10-02

기준 main: `b277e091bc39d071ea30ba0d57281251da811dba`  
브랜치: `audit/current-main-longrun-20261002`

## 이번 작업 범위

- [x] stale #539/#540를 그대로 병합하지 않기로 확정
- [x] #540에서 계측 아이디어만 선별
- [x] 최신 PresentationActionKind / Health / Family / C6 계약에 맞춘 새 `ll_balance_audit` 작성
- [x] 100/365/1000일을 한 번의 1000일 실행에서 checkpoint로 기록
- [x] 2개 기준 seed 병렬 workflow 추가
- [x] snapshot encoded size / runner RSS / elapsed time 기록
- [x] C6 settlement / migration / trade 지표 추가
- [x] 가족 / 인구 / 건강 지표 추가
- [x] audit harness compile
- [x] seed 874213954 — 100/365/1000일 완료
- [x] seed 4242001 — 100/365/1000일 완료
- [x] 두 seed 결과 비교
- [x] 구조적 starvation / 장기 고착 분류
- [x] 수정이 필요한 경우 causal cluster별 별도 PR 계획
- [ ] C6 canonical closeout 여부 확정
- [ ] C7 진입 여부 확정

## 분석 우선순위

1. crash / deterministic / snapshot 문제
2. Hunger / Thirst / Sleep / Toilet / Hygiene 고착
3. Social / Family starvation
4. Cultivation / Civilization starvation
5. 너무 이른 migration 또는 settlement fragmentation
6. trade mission 고착 / 실제 교환 부재
7. health feedback 이상
8. population/chunk/snapshot 성능 증가율

## 금지

- 감사 PR에 임의의 밸런스 숫자 수정 섞기
- #539 runtime patch wholesale merge
- #540 위생 후보 실험 wholesale merge
- 결과가 마음에 안 든다는 이유로 seed별 예외 하드코딩
- 브라우저에서 Core 결과를 보정


## R6 실행 메모 — 2026-10-04

- 기준 main: `9af1872212601e131cdac00a6321847ab36a1ec8`.
- 포함: #613 planning cadence 중복 utility 제거, #614 migration resource 1-pass scan, #617 settlement demand 재사용, #618 social-event Web presentation.
- 목적: 최신 main의 100/365/1000일 trajectory와 CPU/RSS/snapshot 증가율을 R5와 비교하고, C6 closeout 전에 population/family/trade/settlement starvation을 다시 분류.
- 핵심 확인:
  - 두 baseline seed에서 100/365 상태가 R5와 동일한지.
  - #617 이후 elapsed time 추가 개선 폭.
  - day1000 living/birth/pregnancy/household/couple 상태.
  - trade departure/exchange/return이 실제로 완료되는지.
  - 시설 수가 인구 대비 과잉 증가하는지.
  - migration/settlement fragmentation 및 inactive/abandoned settlement 신호.
- 이 브랜치는 audit trigger 전용이며 main merge 대상이 아니다.


## R6 결과 / C6 closeout 분류 — 2026-10-04

기준 main: `9af1872212601e131cdac00a6321847ab36a1ec8`
workflow: `37176747571` — SUCCESS.

### 성능 / 결정론
- seed 874213954: day100 41,544ms / day365 392,282ms / day1000 887,746ms, wall 14:47.94, max RSS 56,660KB.
- seed 4242001: day100 8,242ms / day365 378,768ms / day1000 1,278,305ms, wall 21:18.67, max RSS 57,620KB.
- R5와 상태 trajectory 및 주요 snapshot 규모는 동일하고 #617의 macro 장기 runtime 차이는 noise 수준이다.
- 둘 다 1000일 crash/timeout 없이 완료. 현재 runtime/snapshot 비용은 C7 개발을 막는 즉시 blocker는 아니다.

### 생존 / 가족
- 874213954: day365 living=2, day1000 living=2, married household 1, active pregnancy 1, births=0.
- 4242001: day365 living=3 + married household 1, day1000 living=2(남2), births=0; 사망 원인은 deprivation 1 + illness 1.
- family/conception authority 자체는 실행 가능하지만 두 baseline seed 모두 1000일까지 출생 0이므로 population maturation은 아직 closeout으로 간주하지 않는다.
- Social은 day100에 두 seed 모두 0분이지만 장기적으로 발생한다. 완전한 영구 starvation은 아니나 관계 형성이 늦다.

### C6 settlement / trade
- 874213954: day365 settlements=6 active=2 residentsAssigned=0, day1000 settlements=7 active=3 residentsAssigned=2, trade exchange/return=0.
- 4242001: day365 settlements=4 active=1 residentsAssigned=0, day1000 settlements=3 active=1 residentsAssigned=0. trade departure=1이나 exchange/return=0.
- Balance Contract의 경고 신호인 “새 정착지가 형성되지만 주민이 배정되지 않음”이 두 seed 모두 실측됨.
- trade가 end-to-end runtime regression에서는 가능하지만 baseline 장기 world에서는 resident-to-settlement assignment가 자주 0이라 실제 교역 후보가 구조적으로 약해진다.

### 첫 causal cluster
`SettlementNetwork::observeSettlementNetwork`는 infrastructure를 service-radius single-link union으로 묶은 뒤,
resident assignment를 cluster의 산술 평균 anchor 한 점과의 거리로 판정한다.
시설이 사슬처럼 퍼진 큰 cluster에서는 주민이 실제 시설 바로 옆에 있어도 centroid가 멀어져 unassigned가 될 수 있다.
`SettlementTrade::settlementClusterForPosition`도 같은 anchor-only 판정을 사용한다.

따라서 C6 closeout 전 첫 수정은:
1. cluster의 실제 infrastructure service footprint에 resident를 배정한다.
2. trade의 settlement lookup도 동일 service-footprint 계약을 사용한다.
3. compact 2-settlement 기존 회귀는 유지한다.
4. chained infrastructure에서 endpoint resident가 centroid 때문에 탈락하지 않는 새 regression을 추가한다.
5. 수정 후 동일 seed R7 365/1000일로 residentsAssigned/trade completion을 재측정한다.

C6 canonical closeout: **HOLD**
C7 Population / Settlement Maturation: **HOLD — 위 C6 assignment/trade causal cluster 수정 후 재판정**
