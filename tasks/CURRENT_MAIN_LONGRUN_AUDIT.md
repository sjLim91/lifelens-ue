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


## P0-B post-merge revalidation
- target main: `467e5e881f6c67e44652ce57db9cb1e4da911339`
- scope: seeds 4242001 + 874213954, 100 / 365 / 1000일
- focus: Baby hunger/thirst saturation, parenting time, caregiver preemption, long-run runtime
