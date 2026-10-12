# COG-AUDIT B1 — 자율성 Core 진단 기준선 (2026-10-12)

참조: [자율 인공생명 권한·책임·인과 경계 v1](architecture/AUTONOMOUS_LIFE_AUTHORITY_BOUNDARIES_v1_2026-10-12.md).

## 목적

현재 AI가 실제로 무엇을 반복하고, Core 저장 상태에 확인 가능한 학습/지식 전파 흔적이 얼마나 남는지 **추측 없이** 수치화한다. AI·World·Needs·실제 행동·자원·RNG·저장 codec는 **변경하지 않는다**.

## 구현

- \`AutonomyAuditReadModel.h\`: \`const SimulationStateSnapshot&\` 전용 순수 읽기; 실제 Core 주민/런타임/Memory/Belief/SocialKnowledge에서 checkpoint 수치를 생성.
- \`AutonomyAuditSeries\`: 동일 시드·세대·PopulationSeed, **증가하는 시뮬레이션 분**만 누적. 동일 시간/역행/교차 월드/잘못된 snapshot version은 **메트릭까지 원자 불변**으로 거부.
- 주민의 해당 시점 Goal 분포(Eat/Drink/Sleep/UseToilet/Wash/Idle), 살아 있는 인구·Runtime 관찰, 현재 미완료 context action 유형(사회/문명/교육/교역), **샘플 간 관찰된** Goal 변동, 경험 Belief의 성공/실패 근거 누적 차분, 직접/전언/추론 기억, 마지막 SocialFact/직접·전파 KnowledgeReceipt 카운트.
- \`ll_autonomy_audit --seed 874213954 --days 1 --sample-minutes 60\`: 현행 Core 그대로 실행하고 한 행 JSON 출력. \`--days\` 상한 1000, \`--sample-minutes\` 1..1440.
- 동일 JSON 순서 보존: actor ID 정렬, field 이름 고정, 무한/자유 텍스트 포함하지 않음.

## 중요한 해석 한계

**이 도구는 분 단위 실행 이벤트를 계측하지 않는다.** 60분 checkpoint는 중간에 실행된 빠른 Goal 전환을 놓칠 수 있다. \`sampledPhysicalGoals\`는 '실제로 완료된 행동 횟수'가 아니라 '그 시점에 Core runtime이 선택 중인 Goal'의 표본 빈도다.

- Belief evidence delta는 저장된 물리 경험의 관찰된 차분이며 **실제 가설 생성**이나 '학습이 행동에 인과적으로 영향을 줬다'는 증거가 아니다.
- \`socialFacts\` / \`transmittedKnowledgeReceipts\`는 관찰된 사회적 사실/지식 전달 누적량이지 문명 창발 자체는 아니다.
- \`notInstrumented\`에 \`novelGoalCreation\`, \`hypothesisGeneration\`, \`composedPrimitivePlans\`, \`acceptedCognitionEvents\`, \`causalMethodLearning\`, \`emergentCivilization\`을 **측정 불가**로 명시. 해당 기능이 없는 것으로 속이거나, 숫자 0으로 표시하지 않는다.
- 새로운 목표 합성·전파 인과·행동 결과를 실제로 측정하려면 Core-authorized \`Decision/Outcome\` 계층에 식별자를 추가하는 후속 별도 PR이 필요하다. COG-3 #667/#668의 accepted journal과 중복 구현 금지.

## 검증

- \`test_autonomy_audit\`: synthetic actor 순서/물리 Goal 전환/경험 차분/중복·역행·교차 Seed 무효화.
- 실제 Seed 874213954, 4242001 각각 \`setupNewGame()\`→1440분 실행에서 Audit ON/OFF 경로의 **Core snapshot binary와 전체 로그의 정확 일치**.
- 추가 CI: Core C++17 compile, 1일 명시적 CLI JSON 작성/검증; 기존 Core / WASM / Preflight / 장기 회귀도 병합 조건.
- 주민 숫자가 작더라도 모든 시점의 무제한 Snapshot 저장은 하지 않는다. 메트릭만 actor별 O(N)으로 누적하고 각 샘플 상태를 영속화하지 않는다.

## 다음 개선 (B2 이후)

- 이벤트 기반 원인 ID: \`Perception -> Goal -> Hypothesis -> ActionAttempt -> Outcome -> LearnedMethod -> SocialFact\`.
- Core-validated accepted cognition journal 및 중복 없는 저장·재생 먼저; 이후 \`novelGoalCreation\` 측정을 실제로 구현.
- 100/365/1000/3000일 + 다중 시드/지역의 행동 다양성·비강제 창발 패턴 비교. 현재 B1에서 값이 없으면 '아직 측정하지 않음'으로 표시한다.

이 문서의 각 지표는 **현재 가능한 관찰 범위만** 기술하며 미래 기능의 구현 완료를 주장하지 않는다.
