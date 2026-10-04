# LifeLens Current Execution Order — 2026-10-04

기준 main: `49aaf6c2be943aeeb7b4552d7e2a787af00005a0` (#634)

이 문서는 2026-10-04 현재 구현/검증 상태와 다음 실행순서의 canonical short-form이다.
실제 GitHub main / PR / exact-head Actions / 같은-head 장기감사 증거가 과거 문서보다 우선한다.

---

## 1. 현재 상태

- C1 정착/생존 기반: ✅
- C3 개방형 문명 프레임워크: ✅
- C4 건강/질병 기반: ✅ 기능 기반, 장기 인과 검증 진행
- C5 교육/전문화/경제/기관: ✅
- C6 이주/복수 정착지/교역: 🟡 closeout
- C7 인구/정착지 성숙: ⏸ C6 closeout 전 시작 금지
- COG-0 #628: ✅
- COG-1 #629: ✅
- COG-2 #634: ✅ Core-owned read-only cognition context
- COG-3: 다음 cognition 단계 — accepted cognition persistence/replay

현재 제품 경로:
`LifeLensCore -> WASM -> React/TypeScript/Three.js Web Observer`

---

## 2. 2026-10-04 완료

### #633 — 다중 정착지 sanitation snapshot

✅ 완료.

- 복수 active sanitation site snapshot encode/decode 허용
- byte-stable re-encode
- deterministic continuation
- exact-head Core/Preflight/WASM/Weather-Sleep-Era PASS

### #634 — Cognitive COG-2

✅ 완료.

- Core가 Needs / Personality / Emotion / Memory / Belief / Relationship bounded context 구성
- WASM/Web read-only bridge
- malformed/old runtime fail-closed
- 모델 결과는 아직 Simulation authority 아님

---

## 3. premature founder split 재검증 결과

과거 R9 seed 874213954에서:

- day100 settlements=2
- activeSettlements=2
- migrationCandidates=0

이 관찰되어 #626 계열 수정이 준비됐다.

그러나 **현재 main `49aaf6c2` 동일 seed/동일 조건 대조군에서는 원래 blocker가 재현되지 않았다.**

### current main — seed 874213954

day100:
- living=4
- settlements=1
- active=1
- migrationCandidates=0
- chunks=32

day365:
- living=3
- settlements=3
- **active=1**
- migrationCandidates=0
- chunks=53

day1000:
- total=5
- living=3
- bornAfterStart=1
- married=1
- pregnancy=1
- settlements=5
- **active=1**
- chunks=91

추가 settlement cluster는 물리적 history로 남을 수 있으나 lived active settlement는 1개다.

### current main — seed 4242001, 365-day control

day100:
- living=4
- settlements=1
- active=1
- residentsAssigned=4
- migrationCandidates=0
- chunks=28

day365:
- living=3
- settlements=1
- active=1
- residentsAssigned=3
- migrationCandidates=0
- chunks=99

따라서 #626/#635가 겨냥한 **day100 accidental active second settlement**는 current main에서 no-longer-reproducing으로 판정한다.

### #635 판정

❌ MERGE 금지 / CLOSED.

#635 후보는 원래 split을 줄였지만 current main보다 장기 결과를 악화시켰다.

예:
- seed424 day365 chunks: current main 99 vs #635 candidate 141
- seed874 day1000 living: current main 3 vs #635 candidate 1
- current main은 birth/marriage/pregnancy까지 진행, #635 candidate는 세대성장이 멈춤

따라서 stale blocker를 억지로 고치지 않는다.

---

## 4. 현재 별도 성능/탐험 관찰

current main에서도 chunk 수는 R9/#623 당시보다 증가한다.

- seed424 day100 chunks=28
- seed424 day365 chunks=99
- seed874 day365 chunks=53
- seed874 day1000 chunks=91

이 값은 premature split blocker와 분리한다.

현재 사실:
- migrationCandidates=0이어도 ordinary/critical resource exploration으로 chunk는 실제 materialize될 수 있음.
- chunk는 lookahead가 아니라 주민이 실제 Explore target에 도착해 성공했을 때 생성됨.
- ordinary exploration max radius는 6 chunks.
- local resource sufficiency 평가는 3-chunk radius를 사용한다.

따라서 탐험 범위/자원 판정권의 계약 불일치 가능성은 **별도 current-main C2/C6 성능 debt**로 추적한다.
#635에 섞지 않는다.

---

## 5. 현재 최우선 blocker — inter-settlement trade mission completion

current main seed 874213954:

day365:
- tradeDepartures=2
- tradeExchanges=0
- tradeReturns=0

day1000:
- tradeDepartures=8
- tradeExchanges=0
- tradeReturns=0

즉 현재 문제는 단순 return만이 아니라 **출발한 trade mission이 실제 exchange까지 안정적으로 완주하지 못하는 것**이다.

현재 Core 계약은:

`departure -> destination -> exchange -> returning=true -> origin -> return complete`

를 의도한다.

하지만 critical Hunger/Thirst preemption은 현재 runtime activity를 취소하면서
`pendingContext.clear()`를 호출할 수 있다.

이 경우:
- trade outbound mission
- 또는 exchange 후 return obligation

이 사라질 수 있다.

### 수정 원칙

- Hunger/Thirst 생존 우선은 절대 낮추지 않음
- trader를 굶긴 채 mission 강행하지 않음
- 생존행동 후 **미완료 trade journey를 resume**
- dead/missing partner, invalid settlement, 실제 exchange 불가 등은 clean cancellation
- trade mission이 accidental migration이 되지 않음
- save/load와 deterministic replay 보존
- 기존 runtime field를 억지로 재사용하는 hack 금지

### 완료조건

- departure observable
- destination 실제 이동
- actual inventory exchange
- survival preemption 발생 시 mission resume
- physical return observable
- save/load 중 mission continuity
- deterministic replay
- exact-head tests
- same-seed long-run에서 departures/exchanges/returns 증거

---

## 6. 고정 실행 순서

### STEP 1 — ✅ #633 snapshot closeout
완료.

### STEP 2 — ✅ #634 Cognitive COG-2
완료.

### STEP 3 — ✅ premature split blocker 재검증
- #626/#635 merge 없이 close
- current main에서 원래 day100 active split no-repro 확인

### STEP 4 — 🔴 현재: Trade mission completion/persistence
1. outbound / exchange / return lifecycle 분해
2. survival preemption에서 deferred mission 보존
3. snapshot/replay 계약 설계
4. targeted tests
5. exact-head CI
6. two-seed evidence

### STEP 5 — current-main exploration/chunk growth diagnosis
- trade 수정과 별개로 ordinary/critical exploration materialization 계측
- exploration count / chunk bbox / distance
- local resource radius vs ordinary search radius 정합성
- 숫자 임의 하향 금지

### STEP 6 — current-main long-run health/family
- seed874 current main은 day1000 living3 + birth/marriage/pregnancy까지 개선됨
- seed424 current-main 1000일 결과로 health 결론 갱신
- 필요한 경우 contamination / illness / dependent-care causal fix

### STEP 7 — C6 canonical close
다음이 모두 green일 때:
- active settlement authority 안정
- household migration 실제 이동
- settlement-local sanitation/save 정상
- real trade exchange + return 정상
- long-run crash/timeout 없음
- chunk/facility runaway 설명 및 허용범위 확정
- 가족/인구 루프가 C7 검증을 수행할 만큼 유지

### STEP 8 — C7 Population / Settlement Maturation
그 다음 시작.

---

## 7. Cognitive 병렬 순서

- COG-0 #628 ✅
- COG-1 #629 ✅
- COG-2 #634 ✅
- COG-3 accepted cognition event persistence / deterministic replay
- COG-4 validated strategic intent -> existing Utility/Civilization bias
- COG-5 memory retrieval / reflection quality
- COG-6 population cognition scaling

**COG-3 persistence/replay 전에 LLM 판단을 Simulation 행동 authority에 연결하지 않는다.**

---

## 8. 지금 당장 다음 작업

`Trade mission completion/persistence -> exploration/chunk diagnosis -> current-main long-run health -> C6 close -> C7`

현재 실제 코드 작업은 **trade mission persistence**다.
