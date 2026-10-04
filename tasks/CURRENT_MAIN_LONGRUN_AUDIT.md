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
- [ ] audit harness compile
- [ ] seed 874213954 — 100/365/1000일 완료
- [ ] seed 4242001 — 100/365/1000일 완료
- [ ] 두 seed 결과 비교
- [ ] 구조적 starvation / 장기 고착 분류
- [ ] 수정이 필요한 경우 causal cluster별 별도 PR 계획
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


## R5 실행 메모 — 2026-10-04

- 기준 조합: main `23f2021657c1e037311b969f3ca7a67154b70bd5` + #614 latest-main 재적용 head `b57db7a67fdb1b9355655313d52f13796b1d95d4`.
- 포함: #615 환경 시각화, #616 모바일 overlay 수정, #613 planning cadence 중복 제거, #614 migration resource 1-pass scan.
- 목적: 2개 기준 seed의 100/365/1000일에서 행동/인구/문명 결과를 유지하면서 장기 CPU·RSS·snapshot 증가율이 실제로 개선되는지 확인.
- 특히 seed 4242001의 materialized chunk/resource growth 구간에서 elapsed time을 이전 R2/R3와 비교한다.
- 이 브랜치는 audit trigger 전용이며 main merge 대상이 아니다.


## R5 결과 — 2026-10-04

두 seed 모두 1000일 완료.

### seed 874213954
- day100 elapsed: R4 93,070ms -> R5 41,227ms
- day365 elapsed: R4 919,401ms -> R5 389,876ms
- day100/day365 authoritative 상태값은 R4와 동일.
- day1000 elapsed: 881,837ms
- wall clock: 14:42.03
- max RSS: 56,688KB
- day1000 chunks=126, facilities=146, resourceNodes=1847, snapshotBytes=4,599,599

### seed 4242001
- day100 elapsed: R4 12,612ms -> R5 8,233ms
- day365 elapsed: R4 863,366ms -> R5 379,065ms
- day100/day365 authoritative 상태값은 R4와 동일.
- day1000 elapsed: 1,279,927ms
- wall clock: 21:20.28
- max RSS: 57,596KB
- day1000 chunks=157, facilities=156, resourceNodes=2443, snapshotBytes=4,552,753

### 분류
- #613 + #614 조합은 day365까지 trajectory/state를 바꾸지 않으면서 hot-path 비용을 크게 줄였다.
- R4는 두 seed 모두 30분 제한 안에 1000일 완료를 확인하지 못했지만 R5는 둘 다 완료.
- 아직 4242001이 21분대로 무겁기 때문에 추가 behavior-neutral scan 제거 여지가 있음.
- 다음 후보: settlement foundation decision에서 동일 activity-anchor demand 중복 관찰 제거 (#617).
