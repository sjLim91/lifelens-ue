# LifeLens Cognitive Agent Layer v1

## 1. 목표

LifeLens 주민의 전략적 사고를 `Needs/Utility AI`만으로 끝내지 않고,
Personality / Emotion / Memory / Belief / Relationship / 현재 세계 사실을 바탕으로
장기 목표와 사회적 판단을 제안할 수 있는 인지 계층을 추가한다.

이 계층의 목적은 "LLM이 게임을 대신 실행"하는 것이 아니다.

```
Core truth
  -> CognitiveRequest
  -> local reasoning backend
  -> CognitiveProposal
  -> Core validation
  -> strategic bias / goal candidate
  -> existing Utility / Context Action / Civilization execution
  -> outcome
  -> Memory / Belief / Relationship update
```

**Core / World가 유일한 simulation authority라는 기존 원칙은 유지한다.**

---

## 2. 무료 / 로컬 우선 원칙

기본 제품 경로는 유료 API를 요구하지 않는다.

지원 방향:

1. 브라우저 내부 로컬 추론(WebGPU 가능 모델)
2. PC의 로컬 inference companion
3. localhost OpenAI-compatible endpoint를 제공하는 llama.cpp 계열 런타임

어떤 backend를 쓰더라도 Core contract는 동일하다.
특정 모델명이나 클라우드 업체를 simulation contract에 넣지 않는다.

클라우드 API는 향후 사용자가 명시적으로 opt-in하는 선택 기능이 될 수 있지만,
LifeLens 기본 기능의 필수 의존성이 아니다.

---

## 3. 역할 분리

### 기존 Utility AI가 계속 담당

- 배고픔 / 갈증 / 수면 / 화장실 / 위생
- 즉각적인 생존 preemption
- 실제 자원 접근 가능 여부
- 시설/자원/행동의 물리적 실행
- 성공/실패 판정

### Cognitive Agent가 담당할 수 있는 것

- 반복 실패 원인에 대한 전략 변경 제안
- 식량/물 안정성 개선 방향
- 장기 재배/도구/시설 우선순위
- 특정 주민과 협력/갈등 해결/교육/교역을 시도할지 판단
- 이주 압력이 있을 때 장기 선택지 비교
- 발견/상실/출산/사망 같은 큰 사건 이후 재계획

### Cognitive Agent가 절대 직접 하지 않는 것

- inventory 수량 변경
- 시설 생성
- 관계 수치 변경
- 기술 해금
- 이동 teleport
- 임신/사망/질병 결과 결정
- 존재하지 않는 주민/시설/자원 생성
- Core allowlist에 없는 행동 실행

---

## 4. COG-0 계약

현재 foundation의 Core 타입:

- `CognitiveTriggerKind`
- `CognitiveIntentKind`
- `CognitiveRequest`
- `CognitiveProposal`
- `CognitiveValidationResult`

`buildCognitiveRequest()`는 주민의 현재:

- Needs
- Personality
- Emotion
- 중요 Memory
- 강한 Belief
- 관계 salience

를 bounded context로 뽑는다.

메모리/믿음/관계의 최대 개수는 caller가 명시하며,
Core 내부에서 무제한 prompt를 만들지 않는다.

`allowedIntents`는 반드시 Core가 제공한다.

모델은 allowlist에 없는 intention을 제안해도 거부된다.

---

## 5. Fail-closed 원칙

다음 응답은 모두 거부한다.

- trigger 없는 요청
- 다른 actor id 응답
- 죽었거나 직접 돌봄이 필요한 actor
- `None` intention
- allowlist 외 intention
- NaN / 범위 밖 priority
- 대상 주민이 필요한 intention인데 target 없음
- 대상 주민이 필요 없는 intention에 임의 target 지정
- 죽은 주민 / 자기 자신 / 존재하지 않는 주민 target

`rationale`은 설명용 텍스트다.
Core는 rationale 문자열을 파싱해 행동 권한을 만들지 않는다.

---

## 6. 사고 호출 정책

LLM을 매 frame / 매 resident마다 호출하지 않는다.

Cognitive reasoning은 이벤트 기반으로 요청한다.

대표 trigger:

- RepeatedFailure
- ResourceScarcity
- SocialConflict
- MajorLifeEvent
- Discovery
- MigrationPressure
- LeadershipDecision
- Reflection

Critical survival은 LLM 결과를 기다리지 않는다.
기존 생존 Utility AI가 즉시 처리한다.

모델이 없거나 timeout/error가 나면 기존 Core AI가 그대로 계속 동작해야 한다.

---

## 6A. Local provider security boundary

The default Web adapter accepts only explicit loopback endpoints:
`localhost`, `127.0.0.1`, or `::1` over HTTP/HTTPS.

It does not attach an Authorization header and it rejects ordinary LAN/public
hostnames. This keeps the default "free local" path from silently turning into a
paid/cloud endpoint.

The provider uses an OpenAI-compatible `/v1/chat/completions` request shape only
as a transport convention. That does **not** make OpenAI or any cloud service a
runtime dependency.

The local model receives bounded factual context and is asked for one typed JSON
proposal. It is explicitly told not to invent world facts and not to emit hidden
chain-of-thought. Core validation remains mandatory before any future execution.

## 7. 결정론 / 저장

LLM의 temperature를 0으로 만드는 것만으로 결정론을 보장한다고 가정하지 않는다.

실제 CognitiveProposal이 simulation future에 영향을 주기 시작하는 단계에서는
**accepted cognition event**를 Core 입력 이력으로 저장해야 한다.

최종적으로 최소 다음 provenance가 필요하다.

- actor
- simulation minute
- trigger
- context fingerprint
- backend/model identity
- accepted typed intent
- priority
- target
- rationale 또는 rationale digest

save/load 이후 이미 수락된 과거 결정을 다시 모델에 질의하지 않는다.

COG-0/1/2는 cognition을 Simulation 행동 authority에 연결하지 않으므로
snapshot format을 변경하지 않는다.

**COG-3의 persistence/replay contract가 준비되기 전에는 COG-4 행동 bias를 main에 연결하지 않는다.**

---

## 8. 인구 규모 대응

주민 4명에서는 모두 깊은 사고가 가능하지만,
100명/1000명에서 모든 주민을 같은 빈도로 호출하면 안 된다.

향후 scheduler 원칙:

- urgent strategic trigger 우선
- leadership / household / institution representative reasoning
- 중요도가 낮은 주민은 기존 Utility AI 중심
- 일정 시간 동안 동일 원인 반복 request dedupe
- bounded queue / bounded concurrency
- remote/LOD population은 coarse cognition
- 결과가 늦으면 stale context 응답을 폐기

---

## 9. 단계 계획

### COG-0 — ✅ 완료 (#628)
Core request/proposal/validation contract.
모델의 free text는 simulation authority가 아니며 Core allowlist/validation을 통과해야 한다.

### COG-1 — ✅ 완료 (#629)
Web cognition adapter + deterministic fake backend.

Implemented contracts:
- loopback-only OpenAI-compatible local provider
- no Authorization/cloud credential path
- dynamic JSON schema restricted to the Core-supplied allowed-intent list
- bounded concurrency and bounded waiting queue
- request timeout that still settles if a backend ignores AbortSignal
- newer same-resident request invalidates an older late response
- simulation-minute stale-response rejection
- reset/dispose fail-closed behavior
- deterministic fake provider for integration tests

COG-1 is intentionally **not wired into Simulation decisions yet**.
A successful local model response is still presentation/runtime-side data with no
authority to change Core state. COG-4 introduces the first validated strategic
decision input after replay/persistence boundaries are ready.

### COG-2 — 🟡 Core-owned read-only context bridge (#634)

목표:
- Core가 Personality / Emotion / Memory / Belief / Relationship의 bounded context를 직접 구성
- WASM/Web은 그 context를 읽기만 함
- Web이 cognition truth를 재구성하지 않음
- local model proposal은 아직 Simulation state를 변경하지 않음

#634는 latest main@#633에 재적용되었고 exact-head 재검증 후 merge한다.

### COG-3 — accepted cognition event persistence / deterministic replay

**행동 영향보다 먼저 구현한다.**

최소 저장 provenance:
- actor
- simulation minute
- trigger
- context fingerprint
- backend/model identity
- accepted typed intent
- priority
- target
- rationale digest

save/load 이후 이미 수락된 과거 결정을 다시 모델에게 질의하지 않는다.

### COG-4 — validated strategic intent -> existing Utility/Civilization bias

모델은 action을 직접 실행하지 않는다.

`Thought -> typed Intention -> Core validation -> existing Goal/Utility candidate`

만 허용한다.

Core의 실제 자원/기술/위치/관계 feasibility가 항상 최종 authority다.

### COG-5 — retrieval / reflection quality

- relevant Memory retrieval
- Belief / Relationship context quality
- repeated-failure reflection
- personality별 장기전략 차이 검증
- local model 품질/latency 비교

### COG-6 — population cognition scaling

- bounded concurrency
- dedupe
- batching
- cognition LOD
- household/leader representative reasoning
- 100 / 300 / 1000 resident 비용 검증

---

## 10. 완료 기준

이 시스템의 성공 기준은 "말을 그럴듯하게 한다"가 아니다.

동일한 객관적 상황에서도:

- 성격
- 과거 경험
- 믿음
- 인간관계
- 현재 감정
- 장기 상황

차이 때문에 주민별 전략 선택이 실제로 달라지고,
그 선택은 항상 Core의 물리적/사회적 현실을 통과해야 한다.

최종 목표:

`Thought -> Intention -> Core-validated Goal -> Action -> Outcome -> Memory -> Reflection`

이 인지 루프가 기존 LifeLens의 자율 생존/사회/문명 시스템 위에서 동작하는 것이다.
