## R11-B trade-journey final trigger — 2026-10-05

- source product head: `1052c173eba16f4364f9e01d31ed57ad70490fcb` (#642 candidate)
- audit branch: `fix/p0-c6-trade-journey-r11b-20261005`
- includes active TradeJourney exclusion from local immediate exchange so agreed cargo cannot be mutated during survival preemption.
- seeds: 874213954, 4242001.
- checkpoints: 100 / 365 / 1000 days.
- acceptance focus:
  - departures must yield real exchanges when a valid journey is scheduled.
  - exchanged journeys must physically return.
  - critical survival still preempts and resumes.
  - no accidental migration or runaway caused by journey persistence.
- evidence-only branch; do not merge.

## 2026-10-04 R10 current-main reconciliation

- #626/#635 premature split blocker: **NO LONGER REPRODUCING on current main**.
- seed874 current main:
  - d100 living4 settlements1 active1 chunks32.
  - d365 living3 settlements3 active1 chunks53.
  - d1000 living3 total5 birth1 married1 pregnancy1 settlements5 active1 chunks91.
- seed424 current-main 365-day control:
  - d100 living4 settlements1 active1 assigned4 chunks28.
  - d365 living3 settlements1 active1 assigned3 chunks99.
- #635 candidate rejected:
  - seed424 d365 chunks141.
  - seed874 d1000 living1 with no generation growth.
- current trade evidence is now the next C6 blocker:
  - seed874 d365 departures2 / exchanges0 / returns0.
  - seed874 d1000 departures8 / exchanges0 / returns0.
- exploration/chunk growth is a separate current-main debt and must not be conflated with premature settlement splitting.

# Current-Main Long-Run Audit — 2026-10-04 Reconciled

초기 감사 기준: `b277e091bc39d071ea30ba0d57281251da811dba`  
현재 제품 기준: `74c372e6cc5506e57df46cbcf502cab2e630c623` (#633)

이 체크리스트는 2026-10-02의 최초 감사 상태를 2026-10-04 R9 결과와 현재 C6 closeout 상태에 맞춰 갱신한다.

## 완료된 감사 기반

- [x] stale #539/#540 runtime patch wholesale merge 금지
- [x] current contracts에 맞춘 `ll_balance_audit`
- [x] 100 / 365 / 1000-day checkpoint
- [x] seed 874213954 / 4242001
- [x] snapshot bytes / RSS / elapsed
- [x] settlement / migration / trade metrics
- [x] family / population / health metrics
- [x] R9 two-seed 1000-day run 완료
- [x] #623 facility-chain runaway 개선 효과 확인
- [x] day365 가족/임신이 실제로 진행 가능한 seed 확인
- [x] 실제 inter-settlement exchange 증거 확인
- [x] 현재 남은 causal blocker 분류

## R9 핵심 결과

### seed 4242001

day 100:
- living 4
- settlements 1
- facilities 18
- chunks 2

day 365:
- living 4
- household 1
- married 1
- pregnancy 1
- settlements 1
- facilities 21
- chunks 4

day 1000:
- total 5
- living 0
- facilities 22
- chunks 4
- illness deaths 4
- deprivation death 1
- wall clock 약 1분 35초

### seed 874213954

day 100:
- living 4
- settlements 2
- active settlements 2
- migrationCandidates 0
- facilities 68
- chunks 93

day 365:
- living 4
- married 1
- pregnancy 1
- settlements 2
- trade departures 8
- trade exchanges 1
- trade returns 0

day 1000:
- total 5
- living 0
- facilities 90
- chunks 115
- illness deaths 5
- wall clock 약 16분 26초

## 현재 판정

### 해결된 축

- [x] 100일 조기 대량사망은 baseline보다 크게 완화
- [x] 성인 self-care가 가족/문명 루프를 365일까지 완전히 starvation시키지는 않음
- [x] facility daisy-chain runaway는 크게 완화
- [x] 가족 -> 결혼 -> 임신 -> 출산 가능성 확인
- [x] 복수 정착지와 실제 exchange 가능성 확인

### 미해결 축

- [x] #633: 복수 active sanitation site snapshot roundtrip / byte identity / deterministic continuation 확정
- [ ] #626: migration candidate 0인데 초기 2정착지가 생기는 premature split 제거
- [ ] trade exchange 후 physical return completion 보장
- [ ] 1000일 illness-dominant population collapse 원인 분류/수정
- [ ] infant/dependent survival의 장기 수지 재검증
- [ ] seed 874의 chunk/runtime 증가가 #626 이후 충분히 안정되는지 확인
- [ ] R10 two-seed 100/365/1000 완료
- [ ] C6 canonical closeout
- [ ] C7 진입 승인

## R10 실행 전 코드 순서

1. #633 snapshot contract — ✅ 완료
2. #634 Cognitive read-only bridge closeout
3. #626 premature split
4. trade return mission persistence
5. R10

## R10 분석 우선순위

1. crash / determinism / snapshot
2. population survival
3. physical self-care completion
4. family / pregnancy / birth / dependent care
5. settlement count / active settlement / assigned residents
6. migration candidate -> actual household relocation
7. trade departure / exchange / return
8. facility / chunk / runtime growth
9. contamination / illness / recovery / mortality

## C7 진입 조건

다음이 모두 충족되기 전 C7 기능 확장을 시작하지 않는다.

- 365일 생존/가족 루프가 구조적으로 유지
- migration pressure 없는 accidental founder split 없음
- household migration은 실제 이동으로 유지
- inter-settlement trade는 exchange 후 physical return까지 완료
- 1000일 crash/timeout 없음
- facility/chunk runaway 없음
- illness/dependent-care 문제는 causal explanation이 가능하고 치명적 collapse가 구조적으로 완화
- save/load가 복수 settlement/sanitation 상태를 정상 보존

## 금지

- 감사 결과를 맞추기 위한 seed별 예외
- mortality 숫자부터 임의 하향
- Web-side simulation correction
- migration/settlement를 UI에서 생성
- causality를 건너뛰는 fast-forward 보정
