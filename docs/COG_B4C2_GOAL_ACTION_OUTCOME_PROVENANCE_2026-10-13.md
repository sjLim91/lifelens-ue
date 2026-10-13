# COG-B4c-2 — 자율 목표·실제 사회행동 토큰·결과 인과 결속

기준: #707 B4c-1 / `AUTONOMOUS_LIFE_AUTHORITY_BOUNDARIES_v1_2026-10-12.md`.

## 이유

주민의 개인 목표가 Core의 사회적 행동 상대 선택을 바꾸더라도, 그 행동이 실제로 완료되었는지와 목표 상태를 달성했는지는 다른 사실이다. 특히 `Approach`가 성공하면 **상대가 주인공을 어떻게 생각하는지**가 바뀔 수 있지만, **주인공이 상대를 신뢰하는 정도**는 그대로일 수 있다. 이는 명백히 별개의 방향성 상태다.

## 실제 인과 흐름

1. `preferAutonomousSocialGoalTargetWithGoal`이 Core 인증 후보 중 실제 reroute된 *원본 goal*, context fingerprint, 기존 SocialDecision을 함께 반환. fallback/social 미선택, 거리 차단, 긴급 생존은 목표 인과의 발급 조건이 아니다.
2. `Simulation::trySocialDecision`의 원래 `PendingContextAction`에 **같은 token/actor/target/intent/issueMinute/goal trust threshold/issue-time self trust/context fingerprint**를 `autonomousGoal`로 저장. 선택을 다시 생성하거나 다른 행동을 생성하지 않는다.
3. 이 링크는 기존 pending과 함께 이동·대기하고 `pending.clear()`를 통해 **취소·실패·중단 시 원자적으로 소멸**한다. action issued ≠ action completed, canceled ≠ failed goal.
4. `completeContextAction`은 실제 `executeSocialDecision` 성공 뒤에만 token과 actor·target·intent 일치 여부를 확인한다. 주인공 **자기 방향 신뢰값**을 실제 Core action 바로 전/후에 읽고 다음 결과 중 하나로 판정한다.
   - `achieved-during-action`: 실행 직전에는 목표 미달, 실행 직후 임계값 도달.
   - `progress-but-not-achieved`: 실제 자기 신뢰 향상, 아직 임계값 미달.
   - `not-achieved-no-progress`: 실행했지만 목표 개선 없음.
   - `already-reached-before-action`: 이동 중이나 다른 원인으로 실행 전에 이미 목표 달성. **현재 action에는 공로를 귀속하지 않음.**
   - `unverifiable`: 믿을 수 있는 실제 관측값이 없음. 성공/실패 학습을 하지 않음.
5. 직접 경험한 자기 신뢰의 목표 진척 여부만 기존 Character `MemoryState` 및 `BeliefState`에 개인별로 누적. memory는 상대/방법당 최신 하나, belief 근거는 bounded moving window. 다음 social 후보 선호에 최대 ±0.04의 소폭 타이브레이크만 반영. 타인의 비공개 신뢰/생각, 자원, 관계 값을 직접 바꾸지 않는다.
6. Core 로그 `COG_GOAL_OUTCOME`은 actionToken, issuedMinute, actor, target, result를 읽기 전용 감사 근거로 남긴다. Core 사회행동은 기존 유틸리티·Needs·실제 이동·Cooldown·물리 결과 규칙이 전부 통제한다.

## 영속성

- `PendingContextAction.autonomousGoal`에 엄격 타입을 두고, 기존 **v8 스냅샷 마지막 생태 확장 뒤**에 `LLGACT01` 선택적 확장을 덧붙인다.
- active link가 없으면 신규 바이트 **0**. 기존 v8 저장은 그대로 읽고, 과거 미완료 행동에 대해 존재하지 않았던 목표를 추측·부여하지 않는다.
- live pending 사회 토큰, actor, target, intent, issuedMinute 일치와 finite/범위·중복 actor 검사. 잘못된 확장은 원자적으로 거부하며 스냅샷의 기존 월드에 손대지 않는다.
- 저장/복원 후 새 링크를 통해 동일 결과를 재현하도록 테스트. 완료된 링크는 재사용 불가.

## 필수 검증

- 전용 CTest: 실제 Social `pending` 외부 ACK + 인과 링크의 binary roundtrip, 오토 복원, 잘못된 token/잘린 bytes 차단, 목표 미달과 action 성공 분리, 취소 시 no learning, 독립 주민, 반복 기억 성장 제한.
- 기존 124 Core 검사 + 추가 테스트; 정확한 PR HEAD의 WASM/Preflight/생태/자율 감사/365일 장기 테스트.
- 결과를 알 수 없을 때 **미검증**을 유지한다. 실험 가설·범용 사고·새 문명 생성까지 완료한 것으로 표시하지 않는다.

## 다음 단계 B4c-3 / B5

실행 전·후 predicate truth를 **객관적 Core 감사**와 **주민이 실제 알 수 있는 경험**으로 분리해, 필요(식수·음식·위생)/자원/신뢰 목표의 대안 행동을 비교한다. 이후 주민들이 경쟁 가설과 가용 affordance를 이용해 비용 제한 실험을 계획하고, 실제 결과에 따라 선택을 수정하도록 일반화한다. 특히 행동 실패·중단·경로 실패·목표 미달은 서로 다른 범주다.
