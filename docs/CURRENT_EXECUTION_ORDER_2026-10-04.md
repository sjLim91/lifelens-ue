# LifeLens Current Execution Order — 2026-10-04

기준 main: `fde409c8d148296b9e30b14944c1d4a6676fd387` (#631)

이 문서는 2026-10-04 기준 **현재 구현 상태와 다음 실행 순서의 canonical short-form**이다.
과거 milestone 문서와 감사 문서에 남아 있는 오래된 "현재 상태"보다 이 문서와 실제 GitHub main/PR/Actions를 우선한다.

---

## 1. 현재 제품 상태

활성 제품 경로:

`LifeLensCore -> WASM -> React/TypeScript/Three.js Web Observer`

현재 큰 단계:

- C1 정착/생존 기반: ✅ 완료
- C2 장기실행/성능: 🟡 기반 개선 중
- C3 개방형 문명 프레임워크: ✅ 완료
- C4 건강/질병 기반: ✅ 기능 완료, 장기 생존 밸런스는 통합검증 중
- C5 교육/전문화/경제/기관: ✅ 완료
- C6 이주/복수 정착지/교역: 🟡 기능 기반 대부분 완료, closeout 검증 중
- C7 인구/정착지 성숙: ⏸ C6/장기 생존 gate 전까지 시작 금지
- Cognitive Agent: 🟡 COG-0/1 main 반영, COG-2 PR 진행
- F1~F8 역사/산업/현대/미래문명 확장: ⏳ C7 이후

---

## 2. 최근 main에 완료된 핵심

### C6 / 장기 안정화

- #592 — 가구 단위 이주 + 정착지 쇠퇴
- #594 — 정착지별 지식/기술 격차
- #595 — 정착지별 자원/생산 전문화
- #596 — 정착지 간 협력/긴장 관계
- #613/#614/#617 — 주요 장기 성능 hot path 최적화
- #620 — 실제 infrastructure footprint 기반 주민/교역 귀속
- #622 — 원거리 탐색지 시설 증식/유령 정착지 억제
- #623 — facility daisy-chain 기반 정착지 확장 차단
- #625 — 다중 정착지 위생시설 생성/개량 지역화

### Web 관찰성

- #611 — 주민 행동별 모션/수면 전환
- #615 — 날씨/보행/오염/불 환경 결과
- #618 — 사회 사건/관계 변화
- #619 — 시설 건설/마모/수리
- #621 — 가족/임신/출산/성장/육아/생애주기
- #624 — 채집/운반/저장/제작/건설/재배
- #631 — 정착지/이주 압력/교역망 관찰 시각화

### Cognitive Agent

- #628 / COG-0 — typed CognitiveRequest/CognitiveProposal + fail-closed Core validation
- #629 / COG-1 — 무료 로컬 endpoint adapter, JSON schema, bounded scheduler, timeout/stale response handling

---

## 3. R9 장기실행에서 확인된 현재 사실

검증 seed:

- `4242001`
- `874213954`

### 성능 개선

#623 이후 facility/chunk runaway는 크게 완화됐다.

seed 4242001:
- day 365: living 4, facilities 21, chunks 4
- day 1000: facilities 22, chunks 4
- 1000일 wall clock 약 1분 35초

기존 R8b의 같은 seed:
- day 365 facilities 151 / chunks 240
- day 1000 facilities 194 / chunks 281
- 1000일 약 21분 42초

따라서 **facility-chain runaway는 closeout 가능한 수준으로 개선**된 것으로 본다.

### 아직 남은 구조 문제

1. **조기 정착지 분할**
   - seed 874213954 day 100:
   - living 4
   - settlements 2
   - active settlements 2
   - migrationCandidates 0
   - 실제 이주 압력 없이 초기 4명이 두 정착지로 갈라질 수 있음.

2. **교역 귀환 미완료**
   - seed 874213954 day 365:
   - tradeDepartures 8
   - tradeExchanges 1
   - tradeReturns 0
   - 교환 자체는 실제로 발생하지만 return mission이 survival preemption 등으로 유실될 가능성이 있음.

3. **1000일 장기 생존 실패**
   - seed 4242001: total 5 / living 0
   - seed 874213954: total 5 / living 0
   - 사망의 주된 원인은 illness.
   - 숫자 튜닝보다 contamination / sanitation / health / dependent care 원인 경로를 먼저 확인한다.

4. **다중 위생시설 snapshot 정합성**
   - #625로 복수 정착지에 복수 sanitation site가 정당해졌지만,
   - 현재 snapshot validator의 legacy world-global active-site 제한을 제거하는 #627이 아직 open이다.

---

## 4. 현재 open PR

### #627 — C6 다중 정착지 위생시설 저장 검증 허용

상태:
- 이전 head의 Core / Preflight / WASM / Weather-Sleep-Era는 모두 PASS.
- 현재 main보다 뒤에 있으므로 latest-main 재적용/재검증 필요.

우선순위: **P0 / 첫 번째**

이유:
현재 simulation authority가 허용하는 합법 상태를 snapshot validator가 거부할 수 있는 계약 불일치다.

### #630 — Cognitive Core -> Web read-only request bridge

상태:
- exact-head Core / WASM / Web / cognition / long regression 모두 PASS.
- #631 이후 main drift가 있으므로 latest-main 동기화 후 다시 exact-head 검증한다.

우선순위: **P0.5 / #627 직후**

이 PR까지 들어가도 모델 제안은 Simulation 행동에 영향을 주지 않는다.

### #626 — 초기 정착지 우연한 2인 분할 차단

상태:
- 로직 수정은 준비됨.
- 표시된 Core 실패는 테스트 assertion 실패가 아니라 동일 SHA push 결과 대기 timeout.
- 현재 main보다 많이 뒤에 있으므로 latest-main 재적용이 필수.
- 머지 전 R10으로 실제 seed 874 day100 behavior 확인이 필요.

우선순위: **P1 / #627/#630 뒤**

---

# 5. 고정 실행 순서

이 순서를 임의로 바꾸지 않는다. 새 문제는 해당 단계 안에서 해결한다.

## STEP 1 — #627 snapshot contract closeout

1. latest main에서 #627의 실제 필요한 2-file diff만 재적용
2. Core snapshot roundtrip targeted test
3. exact-head Core / Preflight / WASM / Weather-Sleep-Era
4. green이면 merge
5. main SHA 확인

완료 조건:
- 두 개 이상의 distant active sanitation site가 snapshot encode/decode 가능
- byte-stable re-encode
- deterministic continuation 유지

---

## STEP 2 — #630 Cognitive read-only bridge closeout

1. latest main으로 동기화
2. #631 Web 변경 보존
3. Core-owned bounded Personality/Emotion/Memory/Belief/Relationship context 확인
4. optional old-runtime fallback 확인
5. exact-head 전체 gate
6. merge

완료 조건:
- local model adapter가 Web 추론으로 context를 재구성하지 않음
- Core가 고른 read-only context를 받음
- 아직 Simulation state mutation 없음

---

## STEP 3 — #626 premature split closeout

1. latest main에서 재적용
2. targeted settlement-capacity tests
3. exact-head Core gates
4. **R10 two-seed 장기런 실행**
5. seed 874213954 day100에서:
   - migrationCandidates=0인데 accidental settlement 2가 생기지 않는지 확인
6. facility/chunk/runtime가 #623 개선치를 회귀시키지 않는지 확인
7. 검증 후 merge

중요:
단순 2인 co-location만으로 empty frontier durable settlement bootstrap을 허용하지 않는다.
실제 household/group commitment 또는 기존 lived infrastructure가 있어야 한다.

---

## STEP 4 — Trade return mission persistence

현재 확인된 구조:

`Trade exchange -> returning=true -> origin target`

은 존재한다.

그러나 critical Hunger/Thirst preemption이 runtime pendingContext를 clear하면
**귀환 의무 자체가 사라질 수 있다.**

수정 원칙:

- survival preemption은 유지
- trader가 굶거나 목마른데 강제로 귀환시키지 않음
- survival 해결 후 **원래 return obligation을 resume**
- trade가 accidental migration이 되지 않음
- 저장/불러오기와 결정론을 깨지 않음
- 임시 field hack으로 binary snapshot contract를 몰래 변경하지 않음

완료 조건:
- 실제 exchange 후 return observable
- critical survival 중단 후 return resume
- route failure/partner death 등 실제 실패 사유는 clean cancellation
- targeted snapshot/replay test 포함

---

## STEP 5 — R10 current-main long-run gate

STEP 1~4가 main에 들어간 뒤 동일 seed로 다시 실행한다.

체크포인트:
- 100일
- 365일
- 1000일

seed:
- 4242001
- 874213954

검증 우선순위:

1. crash / determinism / snapshot
2. living population
3. self-care completion
4. family / pregnancy / birth
5. settlement count / active settlement
6. migration candidate -> actual household move
7. trade departure / exchange / return
8. facilities / chunks / runtime
9. illness / contamination / dependent survival

---

## STEP 6 — Health / long-run survival causal fix

R10에서도 1000일 population collapse가 반복될 때만 진행한다.

먼저 측정:
- contamination dose source
- sanitation availability/use
- natural-water exposure
- pathogen load
- illness onset/recovery
- immunity/resilience
- infant/child provisioning
- caregiver interruption

금지:
- 사망확률 무작정 하향
- seed별 예외
- illness off
- Web-side 보정

완료 조건:
원인이 실제 simulation causal loop 안에서 수정되고 동일 seed A/B로 개선이 설명 가능해야 한다.

---

## STEP 7 — C6 canonical closeout

다음이 모두 만족되면 C6를 닫는다.

- premature founder fragmentation 없음
- household/group migration 실제 이동 유지
- settlement decline/abandonment 정상
- local sanitation independent
- local knowledge/production specialization 정상
- settlement group relations 정상
- real inter-settlement exchange + physical return 정상
- two-seed 1000-day run crash/timeout 없음
- facility/chunk runaway 없음

그 뒤 로드맵 상태를:

`C6 ✅ Complete`

로 변경한다.

---

## STEP 8 — C7 Population / Settlement Maturation 시작

C7 목표:

`4 founders`
→ `relationships / families`
→ `households`
→ `children`
→ `larger labor + demand`
→ `denser infrastructure`
→ `mature first settlement`
→ `carrying pressure`
→ `household/group migration`
→ `second settlement`

주의:
- birth rate를 인위적으로 높이는 단계가 아님
- village level을 시간으로 unlock하지 않음
- 실제 가족/자원/시설/물/위생/노동 압력에서 성장해야 함

---

# 6. Cognitive Agent 병렬 순서

C6/C7 critical path를 막지 않는 별도 lane으로 유지한다.

## COG-0 — ✅ 완료 (#628)

typed request/proposal + Core validation.

## COG-1 — ✅ 완료 (#629)

무료 local provider + bounded scheduler.

## COG-2 — 🟡 #630

Core-owned read-only context bridge.

## COG-3 — accepted cognition event persistence / replay

**행동에 영향을 주기 전에 반드시 먼저 한다.**

저장할 최소 provenance:

- actor
- simulation minute
- trigger
- context fingerprint
- provider/model identity
- accepted typed intent
- priority
- target
- rationale digest

save/load 이후 이미 수락된 과거 판단을 다시 LLM에게 묻지 않는다.

## COG-4 — validated strategic intent -> Utility/Civilization bias

LLM이 action을 직접 실행하지 않는다.

`Thought -> typed Intention -> Core validation -> existing Goal/Utility candidate`

만 허용한다.

## COG-5 — retrieval / reflection quality

- 관련 Memory retrieval
- Belief/Relationship context 품질
- repeated failure reflection
- personality별 전략 차이 평가

## COG-6 — population scaling

- bounded concurrency
- dedupe
- batching
- cognition LOD
- household/leader representative reasoning
- 100/300/1000 resident cost validation

---

# 7. 병렬 작업 원칙

Codex/Web 시각화는 다음 조건에서 병렬 가능:

- Core authority를 새로 만들지 않음
- active Core PR 파일과 겹치지 않음
- DTO truth만 소비
- fake outcome/road/settlement/migration 없음

Core closeout 중 Web 품질 개선은 가능하지만,
C7 기능 추가나 새 문명 단계는 STEP 7 이전에 시작하지 않는다.

---

# 8. 지금 당장 다음 작업

**다음 실제 코드 작업은 #627 최신-main 재적용/검증이다.**

그 뒤:

`#627 -> #630 -> #626 + R10 split check -> trade return -> full R10 -> health causal fix(if needed) -> C6 close -> C7`

이 순서를 현재 canonical execution order로 고정한다.
