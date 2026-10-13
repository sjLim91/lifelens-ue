# COG-B4c-3 — Core 목표 상태 변화의 범용 비교 계약

## 목적

#708의 social goal → pending action → 실제 결과 학습은 주민 자신의 신뢰 목표 하나에 한정된다. 다음 가설·대안 계획·문명 창발의 공통 평가 기준으로, **현재 Core가 측정할 수 있는 4종 목표 metric의 진실 상태 차이**를 분류한다.

이 코드는 실행/자원/AI 권한을 갖지 않는 **Core 전용 읽기 평가기**다. 타인의 신뢰 등 관찰자가 직접 알 수 없는 Core truth를 개인의 Memory/Belief로 몰래 복사하지 않는다.

## 구현

`compareAutonomousGoalCoreTruth(baseline World+RelationshipBook, observed World+RelationshipBook, validatedGoal)`:

- GoalSpec actor, 시작/만료 시각, WorldSeed/PopulationSeed/GenerationVersion, 시간 역행, enum/threshold/target/자원 enum/조건 개수를 검증.
- `ActorNeed` 5종, `ActorRawMaterialCount`, **방향성이 있는** `RelationshipTrust`, `KnownResidentAlive`를 기존 `autonomousGoalTruthValue`의 Core 실측값으로 검사.
- 각 조건의 시작/끝 실측값·정규화된 임계값까지의 거리·만족 여부 반환. `AtMost`와 `AtLeast` 비교 모두 처리. 복합 목표의 조건별 독립 진전/퇴보를 보존.
- `already-satisfied`, `reached-since-baseline-not-attributed`, `progress-but-unmet`, `mixed-changes`, `regressed`, `stalled`, `deadline-expired`, `unverifiable`, `invalid-context` 명시적으로 구분.
- 이미 만족한 목표가 이후 불만족으로 바뀌면 `regressed`. 신뢰 관계 기록이 없거나 상대가 사망한 경우 신뢰 진전을 상상하지 않는다.
- 원래 World·RelationshipBook·RNG·저장 포맷·주민 기억/행동에 영향이 전혀 없다.

## 이 단계가 아직 하지 않는 일

이 비교기는 임의 두 시점 사이 목표 상태가 달라졌다는 **사실**을 분석하지만, **어떤 특정 행동이 그 결과를 만들었다는 인과를 주장하지 않는다**. 중간에 다른 주민의 도움/오염/기후/시간 경과가 있었을 수 있기 때문이다. 특정 행동에 대한 credit은 #708처럼 동일 Core 실행 토큰과 실행 직전·직후 결과를 결속해야 한다.

주민에게 학습으로 넣을 수 있는 경험은 실제로 그 주민이 지각할 수 있는 정보만 사용한다. 부정확한 전언·추론과 Core 실제 상태를 구별한다.

## 다음 실행 단계

- B5: 다른 행동을 시도할 수 있을 때 제한된 재료·시간·위험 예산 안에서 계획 후보를 만들고, 실행 토큰별 *진짜 결과*와 비교한다.
- 개인 가설의 신뢰도 업데이트, 실패 원인 구분, 기술/생활 방법의 전파, 다른 환경에서의 검증.
- 100/365/1000일 다중 시드에서 가설 시도·성공·미달·퇴보·반복 루프·세대별 전승을 별도 지표로 계측.
