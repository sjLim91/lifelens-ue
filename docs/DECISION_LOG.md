# LifeLens Decision Log

> 목적: 쭌과 AI가 대화 중 확정한 **의미 있는 제품/설계/개발 원칙**을 저장소에 남겨, 새 세션이나 장기 작업 재개 시 다시 설명하지 않아도 되게 한다.
>
> Truth order: **actual GitHub > canonical repository docs > chat/memory/local guess**.

## 문서 역할

- `docs/LIFELENS_SPEC_v1.1.md` — 제품 최상위 요구사항 / 불변조건.
- `docs/DEVELOPMENT_MILESTONES.md` — 장기 개발 순서 / milestone acceptance.
- `tasks/WORK_STATE.md` — 현재 ACTIVE / WAITING_CI / READY_NOW / BLOCKED 상태.
- `tasks/TEAM_BOARD.md` — ownership / locks / Integration Requests.
- `tasks/HANDOFF_LOG.md` — 의미 있는 merge / failure root cause / ownership transition.
- **`docs/DECISION_LOG.md` — 대화 중 합의된 설계·정리·작업 판단 원칙.**

이 문서는 모든 대화 문장을 복사하지 않는다. 향후 구현 선택을 바꾸거나, 세션이 바뀌면 잊기 쉬운 결정만 기록한다.

결정이 기존 Master Spec을 바꾸는 수준이면 이 로그만 수정하지 않고 Master Spec에도 반영한다. 현재 진행상태가 바뀌면 `WORK_STATE`, ownership/lock이 바뀌면 `TEAM_BOARD`도 별도로 갱신한다.

---

# 2026-09-15

## D-001 — 리뷰 요청은 실제 영향 기준으로 판단한다

**결정**
- `Config/**` 등 shared file을 수정했다는 이유만으로 다겸 리뷰를 기계적으로 요청하지 않는다.
- 다겸 소유 영역, 공개 인터페이스, 실제 presentation 동작에 영향을 주거나 상대 작업을 깨뜨릴 가능성이 있을 때만 리뷰/확인을 요청한다.
- 쭌 lane 내부 작업은 정의된 CI/validation gate가 충족되면 쭌 측에서 진행한다.

**배경**
- #96은 production map 연결 + observer camera/config 작업으로 쭌 lane이었고, Dagyeom WorldPresentation 구현을 수정하지 않았다.
- 불필요한 리뷰 요청은 협업 지연과 알림 노이즈만 만든다.

---

## D-002 — 긴 UE Compile 동안 안전한 병렬 작업을 계속한다

**결정**
- 긴 Unreal compile/package가 도는 동안 계속 polling하거나 손 놓고 기다리지 않는다.
- 현재 PR/branch와 파일 충돌이 없는 read-only 조사, 다음 milestone 설계, 독립된 Core 분석을 진행한다.
- long build의 결과가 필요한 merge/closeout만 기다린다.

**예시**
- #96 compile 동안 `SimulationRuleset` / Needs / UtilityAI / snapshot 영향범위를 조사한다.

---

## D-003 — Hardcoding Cleanup의 기본 순서를 유지한다

**결정**
- A: UE runtime Config foundation.
- B: pure Core `SimulationRuleset` + Needs / UtilityAI.
- C: LifeStage / Relationship / Romance / Pregnancy / Family.
- D: legacy projection / Social / Fun / outcome compatibility 정리.
- E: names / assets / content catalogs.

**원칙**
- Core는 pure C++17/표준 라이브러리 기반을 유지한다.
- 안정적 protocol/determinism invariant는 코드에 남을 수 있다.
- 변경 가능한 제품 규칙은 versioned immutable ruleset으로 이동한다.
- Unreal presentation/runtime tuning은 Config/Data Asset 등으로 외부화한다.

---

## D-004 — 출시 전에는 과거 Save/Snapshot 호환을 제품 요구사항으로 취급하지 않는다

**결정**
- 현재 LifeLens는 실제 사용자에게 배포된 버전이 없으므로, 과거 개발 중 SaveVersion / Snapshot binary version과의 하위호환을 보존할 제품 요구사항이 없다.
- 구버전 migration/backward compatibility 코드는 현재 구조를 복잡하게 만들거나 활성 작업과 직접 겹칠 때 제거할 수 있다.
- 새 구조 도입 시 과거 개발 포맷을 살리기 위한 migration layer를 새로 만들지 않는다.

**주의**
- 이름에 `Legacy` / `Compatibility`가 붙었다는 이유만으로 무조건 삭제하지 않는다.
- 현재 Unreal presentation/WorldDirector가 실제로 소비하는 Core→UE adapter는 소비처가 제거되기 전까지 runtime contract일 수 있다.

---

## D-005 — 중간에 발견한 문제는 세 등급으로 판단한다

새 작업 중 발견한 코드 부채/설계 문제를 무조건 미루거나 무조건 즉시 처리하지 않는다.

### NOW — 지금 처리
- 지금 안 고치면 2~3단계 뒤 수정 비용이 크게 증가한다.
- authority/ownership 오류, 구조적 중복, 현재 milestone 기반을 왜곡하는 문제.

### WITH CURRENT WORK — 현재 작업에 포함
- 같은 파일/구조를 곧 다시 뜯어야 하며 현재 milestone을 완결하려면 같이 고치는 편이 더 싸다.
- 현재 작업 목적과 같은 layer/validation scope에 속한다.

### DEFER — 기록 후 보류
- 현재 milestone 품질에 영향이 작다.
- 다른 ownership lane까지 불필요하게 건드린다.
- CI/충돌 위험만 키우고 지금 해결해도 미래 비용 감소가 작다.

**핵심 판단 질문**
> 지금 안 하면 2~3단계 뒤에 더 비싸지는가?

그렇다면 NOW/WITH CURRENT WORK 후보로 본다. 아니면 계획된 milestone을 우선한다.

---

## D-006 — 계획을 유지하되 코드청소를 맹목적으로 미루지 않는다

**결정**
- LifeLens의 milestone 순서는 유지한다.
- 다만 활성 milestone과 강하게 결합된 기술부채는 같이 제거할 수 있다.
- '계획 밖'이라는 이유만으로 미래 비용이 커질 문제를 방치하지 않는다.
- 반대로 '발견했다'는 이유만으로 unrelated cleanup을 새 milestone처럼 확대하지 않는다.

**현재 적용**
- #96 마무리 후 Cleanup B가 기본 다음 단계다.
- B에서 `SimulationRuleset` 도입과 직접 충돌하는 snapshot/save legacy 구조는 필요한 범위까지 정리할 수 있다.
- Social/Fun projection 전체 제거처럼 B와 직접 관계없는 cleanup은 D 단계 또는 별도 적절한 milestone까지 보류한다.

---

## D-007 — 대화로 확정한 결정은 canonical docs에 남긴다

**결정**
- 향후 쭌과 대화 중 구현 방향/제품 원칙/협업 방식이 확정되면, 세션 기억에만 의존하지 않는다.
- 의미 있는 결정은 이 Decision Log에 추가한다.
- 제품 요구사항 변경이면 Master Spec에도 반영한다.
- 현재 상태 변경이면 WORK_STATE에도 반영한다.
- ownership/lock/Integration Request 변경이면 TEAM_BOARD에도 반영한다.

**목적**
- 새 세션에서 사용자가 요구사항을 다시 설명하지 않게 한다.
- AI가 과거 대화 기억만 믿고 실제 저장소와 다른 방향으로 가는 것을 방지한다.

---

## D-008 — 사용자 UI는 한국어 기본, 사회행동은 실제로 관찰 가능해야 한다

**결정**
- 일반 사용자용 LifeLens UI의 기본 표시 언어는 한국어로 한다.
- Core의 enum/action/event id는 언어 중립 식별자를 유지하고, Presentation/localization layer에서 사용자 언어로 변환한다.
- `UseToilet`, `Comfort`, `Argue` 같은 raw English identifier를 최종 사용자 UI에 그대로 노출하지 않는다.
- 주민 간 사회행동은 관계 수치만 바꾸고 끝나서는 안 된다. Core에서 실제로 발생한 actor/target/intent/outcome을 Presentation이 접근, facing/gaze, animation, 아이콘/말풍선, Event Feed/History로 관찰 가능하게 표현한다.
- 모든 일상 대화를 큰 말풍선으로 도배하지 않는다. 일상/의미 있는 사회행동/중요 사건의 중요도 단계에 따라 표현 강도를 나눈다.
- 대사는 초기부터 유료 LLM/API에 의존하지 않는다. social intent + personality + relationship + emotion + memory/context를 조합한 deterministic/data-driven 한국어 표현으로 기본 기능이 완전하게 동작해야 한다.
- Presentation은 Core에서 발생하지 않은 고백/다툼/위로/관계변화를 꾸며내지 않는다.

**구현 기준**
- canonical companion: `docs/SOCIAL_COMMUNICATION_LOCALIZATION_v1.md`.
- Jjun/Core lane: social action/event authority, outcome/read contract, presentation에 필요한 deterministic context.
- Dagyeom presentation lane: localization 표시, speech bubble/Event Feed, gaze/body/animation 표현.
- shared contract가 실제로 필요할 때만 Integration Request를 연다.

**목적**
- LifeLens의 핵심 경험을 "내부 숫자가 변하는 시뮬레이션"이 아니라 "사람들이 서로 소통하며 살아가는 사회를 관찰하는 경험"으로 유지한다.
