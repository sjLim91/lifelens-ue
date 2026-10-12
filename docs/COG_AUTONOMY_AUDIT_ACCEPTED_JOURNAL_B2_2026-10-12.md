# COG-AUDIT B2 — Core 승인 인지 제안 저널 읽기 전용 계측

> 2026-10-12 / `COG-3 #693` 병합 후 구현. `COG-AUDIT B1/B1b` 확장.

## 구현 내용

- `AutonomyAuditReadModel.h`가 Core가 실제로 보유한 `SimulationStateSnapshot.acceptedCognitiveDecisions`의 **수락된 입력**을 읽는다.
- `acceptedCognitiveDecisionsStoredLast`: 마지막 checkpoint에 보존된 ledger 크기. 1024개 상한과 오래된 항목 제거로 전체 역사 수는 아니다.
- `observedAcceptedCognitiveDecisions`: 첫 checkpoint 이후 새롭게 관찰된 수락 이벤트 수. actor ID와 분 단위 unique Core decision 계약으로 주민별 마지막 기록 시간을 사용한다. 같은 journal을 반복 관찰해도 중복 증가하지 않는다.
- 주민별 `actors[].observedAcceptedCognitiveDecisions`: 위 전체 지표의 주민별 분포. 이미 존재하지 않거나 사망한 주민의 과거 ledger 항목도 고정 actor ID로 유지할 수 있다.
- JSON의 `notInstrumented`에 있던 `acceptedCognitionEvents` 대신 `acceptedCognitionExecutionEffects`를 명시한다. Core의 제안 수락은 측정할 수 있게 됐지만, **그 제안이 실제 Goal/Action을 바꿨는지는 아직 계측하거나 실행하지 않는다**.

## 읽기 전용 / 결정론 경계

- Core World, Simulation, accepted-input journal, Needs, 실제 AI 선택·자원·건강·생태 상태는 바꾸지 않는다.
- 수락 이력은 Shadow Mode 후보나 실제 실행 결과와 혼동하지 않는다.
- 첫 샘플에 이미 들어 있던 결정은 count=0 기준선으로 처리한다. 이미 지난 history를 새로운 AI 수락으로 꾸미지 않는다.
- 저장/재시작 시 감사 인스턴스도 다시 생성하면 새로 시작한 checkpoint부터 차분 집계한다. 감사 자체는 authoritative state를 저장하지 않는다.
- 코어가 보존할 수 있는 ledger 크기(최대 1024)를 넘는 이벤트가 두 샘플 사이에 만들어지면 오래된 이벤트 일부를 놓칠 수 있다. 수락 이력 전체를 복원한 숫자로 해석하지 않는다.
- API 결과는 안전한 구조화된 숫자와 고정 키로만 출력한다.

## 테스트

1. actor 7/13의 실제 타입 `AcceptedCognitiveDecision`를 분 단위로 순차 삽입하여 주민별·전체 증분이 정확한지 검사.
2. 같은 checkpoint 중복 관찰 거부 및 집계 불변 검사.
3. 2개의 과거 기록이 있는 상태에서 신규 감사 시작 시 0 기준선으로 처리, 새 이벤트 1건만 집계하는지 검사.
4. 기존 2개 Seed의 1440분 Core snapshot 바이너리 및 전체 로그 audit ON/OFF 정확 일치 유지.
5. 전용 Core Audit 및 Core Tests, Preflight/WASM/장기 회귀 exact-head 성공 후 병합.

## 다음 연결

`COG-GOAL`에서 실제 GoalSpec 후보를 도입하더라도 **후보 -> Core 수락 -> Core action attempt -> 실제 outcome -> 개인 학습**은 다른 이벤트로 구분한다. 무허가 자동 기술 해금이나 시대별 발전 강제는 없다.
