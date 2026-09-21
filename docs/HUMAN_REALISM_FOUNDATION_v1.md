# LifeLens Human Realism Foundation v1

> Status: **CANONICAL DESIGN / IMPLEMENTATION HOLD**
>
> Date: 2026-09-21 KST
>
> Scope: perception, knowledge, place memory, habit, embodied state, personal space, ownership/reservation, sensory cues, action-to-motion truth and motion-asset policy.
>
> This document complements `docs/WORLD_ARCHITECTURE_v2.md` and `docs/CHARACTER_CONTEXT_MOTION_v1.md`.
>
> User decision: 인간적 행동을 강화하되, World v2 기반이 안정되기 전에는 Character 구현을 재개하지 않는다.

---

## 1. Human-first principle

LifeLens resident는 최적화 알고리즘처럼 행동하면 안 된다.

같은 세계 상태를 보더라도 각 resident는:
- 무엇을 실제로 지각했는지,
- 무엇을 기억하는지,
- 무엇을 믿는지,
- 얼마나 확신하는지,
- 무엇에 익숙한지,
- 누구를 좋아하거나 경계하는지,
- 몸 상태가 어떤지
에 따라 서로 다른 행동을 선택해야 한다.

Authoritative world truth와 resident knowledge는 분리한다.

```text
World Truth
-> Perception
-> Memory
-> Belief + Confidence
-> Preference / Habit / Relationship context
-> Goal
-> Action
-> Motion / Interaction
```

---

## 2. Perception / Knowledge Boundary

Core가 아는 사실을 resident가 자동으로 아는 것은 금지한다.

Knowledge source:
- 직접 시각.
- 소리.
- 냄새/연기/환경 흔적.
- 직접 경험.
- 다른 사람의 전달.
- 기록/표지/지도.
- 반복 학습.

Resident knowledge는 최소한 다음을 구분할 수 있어야 한다.

- known fact.
- remembered fact.
- reported fact.
- inferred belief.
- confidence.
- freshness / last-observed time.

예:
- 2km 밖 강이 World에 존재해도 본 적 없는 resident는 정확한 위치를 모를 수 있다.
- 오래된 물 위치 기억은 말랐거나 오염되었을 수 있다.
- 다른 사람이 잘못 전달한 정보도 belief로 존재할 수 있다.

---

## 3. Place Memory / Place Attachment

인간은 raw coordinate만 기억하지 않는다.

의미 있는 장소는 반복 행동과 사건으로 생성한다.

예:
- 처음 잠든 곳.
- 자주 물을 뜨는 강변.
- 처음 불을 피운 곳.
- 아이가 태어난 집.
- 배우자와 자주 머문 장소.
- 사고/죽음이 있었던 장소.
- 안전하다고 믿는 쉼터.
- 위험하다고 기억하는 계곡.

Place identity는 world coordinate를 참조하지만 의미는 resident/household/community마다 다를 수 있다.

Place memory는:
- navigation bias.
- goal selection.
- emotion.
- attachment.
- avoidance.
- migration reluctance.
에 영향을 준다.

---

## 4. Habit / Routine / Preference

Utility score가 항상 최고인 행동만 선택하지 않는다.

Human behavior에는 다음 bias를 추가할 수 있는 구조를 둔다.

- habitual action.
- familiar route.
- preferred seat / sleep spot.
- preferred food/tool/person.
- morning/evening routine.
- risk tolerance.
- inertia / reluctance to change.
- social routine.
- bad habit.

Habit은 절대 명령이 아니다.
강한 Need, 위험, 관계 사건, 새로운 학습이 habit을 깨뜨릴 수 있다.

---

## 5. Embodied Human

Needs/Health는 UI 숫자에서 끝나지 않고 움직임과 판단에 반영한다.

예:
- 피로 -> 속도/반응/자세 변화.
- 통증/부상 -> 절뚝임, 특정 동작 제한.
- 추위/더위 -> 자세, 이동 목적, 쉼터 탐색.
- 젖음 -> 체온/불/의복 행동.
- 굶주림/갈증 -> 판단 우선순위 변화.
- 임신/노화 -> 속도, endurance, interaction 제약.
- 수면 부족 -> 판단/기억/감정 영향.

Character Motion은 Core 상태를 소비해야 하며 Motion이 상태를 발명하지 않는다.

---

## 6. Personal Space / Social Distance

주민끼리 겹쳐 서는 것은 단순 collision bug가 아니라 human realism failure로 본다.

관계/상황에 따라 desired social distance가 달라질 수 있어야 한다.

- stranger.
- acquaintance.
- close friend.
- partner.
- parent/child.
- group conversation.
- work collaboration.
- conflict/fear.

필요 시스템:
- local separation.
- destination reservation.
- interaction slot.
- queue/turn-taking where appropriate.
- shared target approach offsets.

같은 목표 좌표를 모든 resident에게 그대로 주는 것을 피한다.

---

## 7. Ownership / Familiarity / Reservation

물건과 장소는 모두 anonymous target이면 안 된다.

확장 가능 상태:
- owner.
- household-owned.
- shared/community.
- borrowed.
- reserved/in-use.
- forbidden/private.
- preferred/familiar.

예:
- 자기 침상.
- 자주 쓰는 도구.
- household storage.
- 특정 작업 위치.
- 개인 소지품.

법/재산제도가 발전하기 전에도 "내가 계속 쓰던 것"이라는 familiarity는 존재할 수 있다.

---

## 8. Sensory World Affordances

Visual perception만으로 제한하지 않는다.

World가 후속 확장으로 제공할 수 있는 sensory event:

- audible water.
- thunder.
- animal calls.
- tool/construction noise.
- fire/cracking.
- human voice.
- smoke smell.
- food smell.
- waste/decay smell.
- fire/chemical hazard cues.

정밀 물리 음향/후각 시뮬레이션이 아니라도 spatial cue + intensity + persistence로 추상화할 수 있다.

---

## 9. Action -> Context -> Motion truth

Animation은 행동을 결정하지 않는다.

항상:

```text
Authoritative Action
-> target/context validation
-> interaction slot / facing / distance
-> motion selection
-> IK / warp / contact correction
-> completion
```

순서를 지킨다.

금지:
- 빈 공간에 앉기.
- 대상 없이 작업 모션.
- 멀리 떨어진 채 먹기/사용하기.
- target 반대 방향으로 작업.
- 동일 위치에서 여러 resident 겹침.
- motion 종료만으로 Core action success를 발명.

---

## 10. Motion library strategy

행동 범위가 커질수록 모든 animation을 직접 제작하지 않는다.

무료이며 배포 가능한 motion asset이 적합하면 적극 재사용한다.

우선 필요한 semantic categories:

- idle / look / shift-weight.
- walk / jog / run / turn / stop.
- crouch / kneel.
- sit / stand.
- lie down / get up / sleep.
- eat / drink.
- pick up / put down.
- carry light / heavy.
- gather / forage.
- chop / cut.
- hammer / build.
- dig.
- cook / tend fire.
- wash / hygiene.
- social gesture / conversation.
- embrace / family interaction when context supports it.
- injury / fatigue locomotion variants.

자산이 없다는 이유로 무조건 generic idle을 재사용하지 않는다.
동시에 animation을 보여주기 위해 존재하지 않는 Core 행동을 추가하지 않는다.

---

## 11. Zero-cost motion asset acceptance

외부 motion을 사용하려면 모두 충족해야 한다.

### License
- 상업적 게임 배포 허용.
- 패키지 포함 허용.
- attribution 요구 시 실제 기록 가능.
- NC / personal-only / editorial-only / ripped animation 금지.

### Technical
- source skeleton 확인.
- retarget 가능성 검증.
- root motion / in-place 여부 기록.
- loop 여부 기록.
- contact frame / foot plant 품질 확인.
- Unreal 5.6 retarget 검증.
- Android cook/runtime 비용 확인.
- 불필요한 source mesh/texture는 패키지에 넣지 않음.

### Context quality
- 행동 의미가 실제 LifeLens action과 맞아야 함.
- 손/발 접촉이 중요한 동작은 IK/Motion Warp 대상으로 분류.
- 앉기/눕기/도구 사용은 target affordance와 alignment contract가 있어야 함.
- 과장된 combat/game animation을 일상생활에 억지로 사용하지 않음.

### Provenance
최소 기록:
- asset/pack name.
- creator/provider.
- original source.
- source URL.
- license + checked date.
- attribution requirement.
- imported animation names.
- source skeleton.
- LifeLens semantic action mapping.
- modifications / retarget notes.
- platform classification.

---

## 12. Ecological / historical human connection

Human realism은 World와 분리하지 않는다.

주민은:
- 숲이 다시 자라는 것을 경험하고,
- 사냥감이 줄어드는 것을 경험하고,
- 계절/날씨를 학습하고,
- 오래된 길/폐허/무덤/집을 기억하고,
- 이전 세대가 남긴 장소에서 새로운 삶을 이어갈 수 있어야 한다.

그래서 Place Memory와 Persistent World History는 동일 world identity를 참조한다.

---

## 13. Implementation order

World v2 visual/physical foundation 이후:

1. perception/knowledge boundary.
2. place memory.
3. personal-space / target reservation / separation.
4. embodied motion inputs.
5. habit/routine bias.
6. action-context-motion alignment.
7. external zero-cost motion gap fill.
8. sensory affordances.
9. ownership/familiarity expansion.
10. long-run place attachment/history.

모든 항목을 한 번에 구현하지 않는다.
기존 Core truth를 보존하며 독립적인 contract부터 추가한다.

---

## 14. Acceptance examples

Human-realism pass는 단순 animation 재생으로 판단하지 않는다.

예:
- 네 명이 같은 목적지를 가져도 서로 겹치지 않고 자연스러운 주변 위치를 선택한다.
- 큰 바위가 있으면 그대로 관통하지 않는다.
- 자주 쓰는 길/장소가 반복 선택에 영향을 준다.
- 모르는 물 위치로 resident가 곧장 직선 이동하지 않는다.
- 피곤/다친 resident가 건강한 resident와 동일하게 움직이지 않는다.
- 앉기/잠/작업 모션이 실제 target과 정렬된다.
- 필요한 무료 motion asset을 사용했더라도 Core action truth와 provenance가 유지된다.

---

## 15. Execution lock

현재는 **DESIGN COMPLETE / IMPLEMENTATION HOLD**.

사용자의 World v2 시작 사인 전에는:
- 이 문서를 근거로 Character 구현을 시작하지 않는다.
- 외부 animation을 대량 다운로드/commit하지 않는다.
- current flat-world workaround를 추가하지 않는다.

구현 시작 후에는 실제 gap을 확인한 행동에 대해서만 무료 motion을 선별한다.
