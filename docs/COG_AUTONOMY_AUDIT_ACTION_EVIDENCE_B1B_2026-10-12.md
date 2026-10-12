# COG-AUDIT B1b — 주민·행동별 경험 증거 진단

## 변경 목적

기존 B1은 특정 주민의 경험 성공/실패 근거를 합산했지만, 반복 실패의 **행동 종류**를 구별하기 어려웠다. B1b는 기존 읽기 전용 Core Snapshot 감사에서 경험 Belief의 `experienced-action-outcome:<action>` 이름을 고정된 분류로 집계한다.

## 실제 구현

- 기존 `AutonomyAuditReadModel.h` 내부의 `const SimulationStateSnapshot&`만 관찰. World, Simulation, Needs, 생존/사회/인지 선택 로직, Snapshot codec, RNG를 변경하지 않는다.
- 경험 분류: `Eat`, `Drink`, `Sleep`, `UseToilet`, `Wash`, `Idle`, `Other` (미등록 행동). 알 수 없는 proposition 접미사를 그대로 출력하지 않는다.
- 주민별 `successEvidenceDeltaByAction` / `failureEvidenceDeltaByAction`, 전체 합계 동명의 두 객체를 JSON에 추가한다. 기존 `physicalSuccessEvidenceDelta`, `physicalFailureEvidenceDelta` 등과 다른 수치를 만들어 내지 않는다.
- 같은 분을 중복 관찰하면 거절, 서로 다른 Seed/PopulationSeed/GenerationVersion/Invalid snapshot도 기존 B1대로 거절한다.
- 관찰 간 증거가 감소한 경우 감소량을 음수 성공/실패로 잘못 기록하지 않는다. 이후 증가 부분만 측정한다.

## 데이터 해석 및 제한

**이는 실제 행동 횟수가 아니다.** 저장된 주민 Belief에 반영된 근거 개수의 checkpoint 간 증가량이며, 측정 시점 사이에 벌어진 완료·실패 시도 전부를 복원할 수 없다. 첫 checkpoint의 이전 경험도 새로운 증가량으로 집계하지 않는다.

`Other`는 개인이 이미 알려진 Core 함수에 전달한 비표준 action 문자열을 안전하게 묶을 뿐 새로운 물리 행동을 만들거나 허용하지 않는다.

JSON `evidenceCaveat`에 이러한 제한을 추가했고, B1의 `notInstrumented`를 유지한다. 본 변경으로 가설 생성·행동 후보에 의한 인과 학습이 구현되었다고 주장하지 않는다.

## 검사

- 실제 `rememberActionOutcome` 증거가 `Drink` 실패, `Sleep` 성공으로 각각 주민별/전체 분류되는지 단위 테스트.
- 미등록 `UnlistedWorldMethod`가 `Other`로 이동하고 자유 텍스트는 JSON에 포함되지 않는지 검사.
- 동일 분 중복 관찰 시 집계 변하지 않는지 검사.
- 기존 2개 Seed의 1440분 Core Snapshot 바이너리 및 로그 audit ON/OFF 동등성 테스트 유지.
- 전용 `LifeLens Core Autonomy Read-only Audit`, `LifeLens Core Tests`, `LifeLens Preflight`, `LifeLens Web WASM` 확인 후 병합한다.

## 다음 단계

실제 `action_attempt/outcome`의 event ID 기반 계측과 COG-3 accepted-input journal (#667/#668) 중복 정리는 별도 PR로 진행한다. 직접 행동 실행/가설 생성 코드는 이 진단 PR에 섞지 않는다.
