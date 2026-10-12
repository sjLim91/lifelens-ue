# LifeLens 자율 학습형 주민 AI — 경험 피드백 1차 (2026-10-12)

## 방향
기능별로 행동을 강제로 정하는 대신 주민이 세계에서 실제로 행동하고, 그 결과를 자신의 경험으로 축적하여 다음 선택이 달라지는 **범용 선택–실행–결과–기억–재선택** 루프의 첫 구현.

## 1차 범위
- 현재 Core의 `Character.beliefs`와 `Character.memory`를 그대로 사용하여 성공/실패 이력을 Core truth에 저장.
- `Simulation::failPlan`에서 실제 물리 행동 실패, `advanceAction`의 실제 완료 시 성공 기록. 즉각적인 생존 preemption은 실패로 기록하지 않는다.
- 동일 intent + outcome 최근 기록은 갱신하고 신규 기억은 만들지 않아 개인별 최대 10개 행동 결과 기억만 추가. 응답/판단은 개인마다 다르게 축적된다.
- 지원 행동은 Eat/Drink/Sleep/UseToilet/Wash의 **물리적 계획**부터 시작하며, 성공/실패 이력은 `BeliefRecord`의 누적 증거로 표현한다.
- 과거의 기록으로 Goal의 **비긴급** 선호만 최대 ±10% 변화. 절박한 배고픔/갈증/수면/배변·위생과 원래 Critical Survival preemption의 우선순위는 **그대로 유지**.
- 행동 후보·실행 가능성·자원 소비·건설·질병·사망은 기존 Core 시스템이 판정하며 어떤 것도 AI가 만들어 내지 않는다.
- 기존 Character memory/belief binary serialization을 재사용: 새 snapshot version, Core API/ABI, 유료 LLM, RNG 없음.

## 한계
이는 범용 행동 피드백을 **물리적 목표 5종에 연결한 첫 단계**이며, 아직 COG-4 로컬 언어모델의 전략 제안을 Core에 실행시키는 구현은 아니다. Civilization/Social outcome event와 학습 가능한 대안 행동 후보 확장은 별도 변경으로 필요.

COG-3 외부 인지 수락 event persistence/replay가 준비되기 전에는 Shadow Mode 출력이 Core 행동을 직접 지시하지 못한다. 이번 기능은 **Core 내부 결정론적 경험 학습**이므로 외부 모델의 수락·저장·재실행 계약과 분리된다.

## 검증
`test_action_experience_learning`: 반복 실패 후 선호 감소, 성공 후 회복, 동일 tick 중복 학습 방지, Memory bounded, 다른 주민의 학습 격리, urgent Needs 우선, 저장·불러오기 후 learned score 일치.
`test_core_save_load`/`test_determinism` 포함 전체 Core 게이트, WASM/Preflight, 1/7/30/100/365/1000일 두 seed causal audit 및 snapshot replay.

과거 main과의 전체 snapshot hash는 경험 기억 추가에 따라 달라진다. 따라서 *같은 새 head* 반복 실행·저장 복원이 결정론적이어야 하며 생존 인원/출산 수를 고정 강제하지 않는다.

관련 설계: docs/COGNITIVE_AGENT_LAYER_v1.md, docs/HUMAN_REALISM_FOUNDATION_v1.md, Issue #660.

## 첫 장기감사에서 확인한 안전 게이트 실패와 교정

초기 후보는 일상 수면 점수까지 경험 선호를 곱했다. exact-head Core 전수검사/1000일 결정론은 성공했지만 365일 Weather Sleep/Era 감사에서 seed 874213954의 totalSleepSaturationResidentMinutes가 46,459로 허용 상한 45,402를 초과했다. 실제 1000일 후보의 사회활동·사망·출산 등 경로도 baseline과 크게 달라졌으므로 '돌봄/사회가 좋아졌다'고 단정해서는 안 된다.

**교정:** 비긴급 Wash/UseToilet 선택에만 ±10% 이력을 반영한다. Eat, Drink, Sleep은 실패 경험을 기록할 수 있지만 그 경험 때문에 신체적 필요 점수를 낮추지 않는다. 긴급 신체행동/위기 생존에서는 어떤 경험 편향도 적용하지 않는다. 동일한 테스트와 두 시드 1000일 감사를 새로운 exact head에서 다시 진행해야 한다. 설계 규칙(욕구 감소율, 수면 회복량, 질병 사망률, 사회 관계)은 수정하지 않았다.

## 회귀 감사 후 보정: 필수 필요 vs 행동 방법 구분

기존 실험에서 생리적 Goal 점수 자체를 경험에 따라 바꿔 365일 수면 포화 및 자동 문명 자원 채굴 테스트 회귀가 관찰되었다. Sleep만 보호하는 것도 충분하지 않음. 따라서 **모든 생리적 목표(Eat/Drink/Sleep/UseToilet/Wash)의 Utility 점수에 학습 가중치를 적용하지 않는다.** 실제 실행 결과는 기존 Character의 Belief/Memory에 계속 저장한다. 다음 PR #666은 목표가 아닌 수원 **선택 방법**을 과거 환경 직접 경험으로 바꾸며 필수 욕구와 설계 규칙은 그대로 둔다.
