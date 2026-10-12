# LifeLens COG-GOAL B3 — 조합 가능한 목표·가설의 Core 읽기 전용 계약

> 기준: 2026-10-12 자율 인공생명 권한·인과 경계 v1 및 COG-3 #693.
>
> 단계 상태: **형식 정의 및 검증/관찰만 구현. 현재 실제 행동에는 연결하지 않음.**

## 문제

기존 Utility Goal과 CognitiveIntentKind는 고정 목록으로 존재하며 주민이 아직 지정되지 않은 목표를 스스로 만드는 데 한계가 있다. 반면 "자연을 연구하다가 우연히 도구를 만드는" 등 특정 문명의 날짜/레시피 해금을 강제하는 접근도 허용되지 않는다.

B3는 특정 문명 결과 이름을 만드는 대신 **Core에서 평가할 수 있는 기초 상태 조건을 자유롭게 조합한 목표 후보**와 **그 목표를 달성할 수 있다는 확률적 가설 후보**를 표현한다.

## 구현

- 새 Core 헤더: Source/LifeLensCore/include/lifelens/AutonomousGoalContract.h
- 새 단위 테스트: Source/LifeLensCore/tests/test_autonomous_goal_contract.cpp
- 목표 제안: actor, 현재 Core 시각, 최대 30일 유효기간, priority, 1~6개의 typed desired-state 조건, 1~8개의 개인 CognitionRequest 근거 참조.
- 초기 typed 상태 측정: 본인의 Needs, 본인의 물질 재고, 인지 문맥에 관계가 있는 주민의 trust, 인지 문맥에 관계가 있는 주민의 alive 여부.
- 비교 연산자: AtMost/AtLeast. 주민은 서로 다른 항목과 범위/대상을 결합해 기존 "기술/의도" enum에 없는 목표를 **후보로** 표현할 수 있다.
- Hypothesis candidate: 같은 actor/minute와 bounded probability/estimatedCost, 예측한 Core 상태 변화의 typed predicate 목록, 근거 참조.
- Core 읽기 전용 평가기: 실제 World/Relationship truth로 목표 조건이 충족되었는지를 계산한다. 이 판정은 **AI 입력 문맥으로 노출하지 않는다**.

## 거부 계약

- 다른 actor, 생존 불가/직접 보살핌 중인 주민, 과거/미래 문맥 불일치, 유효기간 초과, NaN·무한대·범위 초과.
- 조건 없는 목표, 6개 초과 조건, 동일 조건 중복, 존재하지 않거나 개인 인지 관계 문맥에 없는 타 주민.
- 잘못된 MaterialKind, 정수가 아닌 재고 수량 임계, 잘못된 enum, 존재하지 않는 기억·믿음·관계 인덱스.
- 가설의 확률·비용·예측 조건·근거가 유효하지 않으면 수락하지 않는다.

## 반드시 오해하지 말 것

1. **Goal 후보 검증은 실행 권한이 아니다.** 이 모듈은 신규 행동 primitive, 시설, 자원, 생리 수치, 관계 값을 변경하거나 기술을 해금하지 않는다. 목표를 달성하는 계획/행위/실제 outcome 생성은 별도 Core executor 계약이 필요하다.
2. 이 단계에서 모델은 주어진 기초 상태 변수와 조건만 조합할 수 있다. 동물 먹이/서식지/지역 재료 특성 등 더 많은 Core 사실은 개별 주민 인식 범위를 나타내는 typed PerceptionFact 확장 이후에만 도입한다.
3. 현재 CognitiveRequest의 memory evidence에는 직접 목격인지, 전언인지, 추론인지 구분이 빠져 있다. 따라서 인덱스가 유효하다고 해서 사실성을 증명하지 못하며 **B3의 근거는 provenance authentication이 아니다**. 미래 수락 게이트에서 반드시 현재 Core 문맥 재생성, witness provenance 및 accepted decision 기록이 필요하다.
4. 목표는 AI가 새롭게 조합한 **제안 데이터**다. 이 PR을 "이제 AI가 문명을 자율 발전시킨다"는 완료 보고서로 해석하지 않는다.
5. 기존 COG-3 입력 저널에 GoalSpec을 억지로 저장하거나 SnapshotVersion=3 / binary format=8을 변경하지 않는다. persistence 확장은 후속 PR에서 호환/재현을 설계한다.

## 검증

- Core 기본 goal+경험학습·장기 생존·WASM 회귀를 그대로 유지한다.
- 혼합 필요/물질/사회 목표의 조합, 실제 World truth 평가 및 Core RNG 비변경.
- NaN priority, 목표 위조 actor, ghost target, 잘못된 evidence, 중복/빈 조건, 과도한 유효기간, 비정수 재고 요구, 부정확한 가설 확률 및 비용을 fail-closed.
- 신규 GoalSpec을 아직 행동에 연결하지 않았으므로 Audit UI나 주민 실제 행동을 변경했다고 주장하지 않는다.

## 후속 개발 (설계 경계 유지)

1. 각 주민이 실제로 볼 수 있는 환경/물체/자원의 typed PerceptionFact 및 직접 목격/전언/추론 출처 보존.
2. GoalSpec 후보의 현재 Core 문맥 검증 및 acceptance journal 확장. COG-3과 단일 authority 유지.
3. 기초 affordance 후보 조합 및 실제 World 물리 비용/이동/재고 검증.
4. 실제 시도 결과와 가설 예측 오차를 개인 Experience/Belief에 반영. Needs에 학습된 부정적 점수를 부여하지 않는다.
