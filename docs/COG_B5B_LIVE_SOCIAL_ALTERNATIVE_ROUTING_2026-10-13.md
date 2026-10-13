# COG-B5b — 기존 사회행동 토큰 경로에 대안 예산 연결 (2026-10-13)

## B5a 위의 최소 실제 적용

주민이 Core Unified Utility로 **Social을 이미 선택한 경우에만** 기존 `preferAutonomousSocialGoalTargetWithGoal`의 단일 B4a 행동 후보 대신 B5a `composeAutonomousExperimentOptions`를 사용한다.

- 주민의 `riskTolerance`·`patience`로 **추상 위험·노력 예산**의 허용 범위를 결정한다. 개인 차이가 실제로 동일한 사회 기회에서 시도 가능한 Approach/Repair 방법에 영향을 줄 수 있다.
- Core가 지원하는 기존 SocialUtility 점수가 유효한 대안에 대해서만, 기존 `baseline.utility`, 거리/생존 긴급 차단, 목표 우선도, 개인의 **실제로 경험한** 결과 신호를 유지하며 선택한다.
- 새 활동 슬롯을 만들지 않는다. 정상적인 `Simulation::trySocialDecision`의 `PendingContextAction` 발행, 실제 이동/실행, #708의 `AutonomousGoalActionLink` 저장/결과 인과를 **그대로 사용**한다.
- B5a 계획에서 보인 선택지가 실제 가능한지 각 시점에 Core가 확인하며, 경로/타깃 문제가 있으면 기존 선택을 유지한다.
- 위험·노력 proxy는 진짜 장거리 소요시간, 실제 음식 비용, 성공률 또는 위험을 측정한 값이 아니다. Core 물리 비용을 별도로 차감하지 않는다.

## 보호 계약

- 일반 생존/문명/돌봄/교역/시설 행동의 일정과 실행 엔진을 수정하지 않는다.
- 통합 Core Utility가 Social을 선택하지 않으면 이 경로의 실행 기회는 0.
- 동일한 Core 상황/시드/개인에 대한 선택은 결정론적; 타인의 내면 truth를 주민에게 노출하지 않는다.
- Needs·자원·날씨·관계·World truth는 이 선택 함수 자체에서 절대 변경되지 않는다.
- 선택 후 실제 물리 행동이 성공해야만 #707/#708 경험 학습이 이루어진다. 예상/계획만으로 달성으로 기록하지 않는다.
- 기본 무료·로컬·오프라인. Snapshot 버전, 바이너리 포맷, Web ABI 변경 없음.

## 검증·병합

- #712(B5a)의 빌드·테스트가 선행.
- 기존 `test_autonomous_social_goal_routing`에 위험 회피/인내심의 방법 선택 보호 회귀 추가.
- exact-head Core Tests, WASM, Preflight, 장기 365일/결정론 검증 성공 후만 병합.
- B5c부터 개인의 실제 다회 경험과 실패를 근거로 가설 불확실도를 업데이트하고, 시도 비용의 Core 보존형 회계를 도입. 실험 자체가 완료됐다고 과장하지 않는다.
