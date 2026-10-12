# COG-GOAL B3c — 주민 자율 목표 후보를 Web에서 읽기 전용으로 관찰

> 계약: `docs/architecture/AUTONOMOUS_LIFE_AUTHORITY_BOUNDARIES_v1_2026-10-12.md`. 기준: B3 (#697) + B3b (#701).
>
> **실제 Core 행동/Goal 우선순위에 영향 없음.** UI에서 보이는 것은 승인된 행동 명령이 아닌 결정론적 목표 **후보**이다.

## 구조

```text
Core World -> Core bounded CognitiveRequest
    -> AutonomousGoalComposer (const inputs, bounded deterministic candidates)
    -> WebClientBridge::autonomousGoalCandidatesJson (read-only JSON)
    -> LifeLensWebBindings.cpp (optional WASM ABI method)
    -> web/src/runtime/core-bridge.ts (bounded validation, old runtime fallback)
    -> ShadowObserver.inspectCoreAutonomousGoals (no LLM, no side effects)
    -> 주민 고급 관찰 > "Core 자율 목표 미리보기" 버튼
```

JSON 데이터는 주민 ID, 현재 Simulation minute, 출처 `CoreLocalGoalComposer`, `affectsSimulation:false`, 최대 6개 조건 목록/우선도/증거 인덱스로 구성된다. 주민 화면에서는 허기·갈증·피로·배뇨·위생·관계 신뢰 등 *현재 제안한 상태 목표*를 한국어로 표시한다.

## 무엇이 실제로 가능한가

- Web에 오래된 Core WASM 빌드가 있어 메서드가 없는 경우 UI는 미지원 상태를 표시한다. fake data를 채우지 않는다.
- 개별 주민 이름·현재 Core 시각·해당 주민의 실제 bounded cognition context만 사용한다.
- 사용자 관찰 요청에 따라 최신 Core 문맥을 읽는다. 관찰을 하지 않아도 Simulation은 그대로 진행된다.
- 로컬 AI 서버 설치, 모델 로드, 온/오프 설정, 외부 API 키 없이 동작한다. Shadow Mode가 OFF여도 후보 조회 가능하다.
- UI가 목표 후보의 **실행/결과/보상을 발생시키거나 Core snapshot/save/replay에 기록하지 않는다**. 실행한 행동은 기존 Core Resident Activity가 따로 보여준다.
- JSON 구조와 길이, priority, minute, enum 종류, id, 조건 수, 근거 인덱스, 실행 금지 마커를 Web에서 검증한다. 불일치 시 표시하지 않는다.

## 완료되지 않은 핵심

이 후보가 실제 계획/물체 조작/이동으로 변환되는 연결은 없다. Core action authority와 collision/resource/Needs/failure outcome를 검증할 B4 Planner 계약이 먼저 필요하다. 도구/문명/동물 행동에 대한 인간의 새로운 아이디어나 기술 해금도 이 PR에서 발생하지 않는다.

## 테스트

- Core `test_cognitive_web_bridge`: 동일 주민, 유효/무효 trigger/actor, 읽기 전후 worldOverview JSON 동일함.
- `web/tests/cognitive-agent/run.mjs`: DTO 안정성, 기존 WASM 미지원 대응, 타 주민 ID 응답/위조 metric/NaN/과한 후보 거부.
- `web/tests/cognitive-agent/shadow.mjs`: Shadow OFF에서 Core 목표 조회 가능, LLM 요청 0건, World 무변경.
- `web/tests/cognitive-agent/shadow-ui.mjs`: 주민 고급 관찰 UI의 한국어 표시·기존 Shadow 기능 회귀.
- Web Typecheck/Build, Web WASM, Core Tests, 장기 Seed 검증과 Presenter CI 성공 후 병합.
