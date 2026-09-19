# PROJECT LIFELENS — MASTER DESIGN SPEC v1.1

**Unreal Native Autonomous Life & Society Simulation**

> 문서 정보
> - 이 파일은 MASTER DESIGN SPEC v1.1 원문을 Markdown으로 재정리한 것이다. 내용·의도·항목은 원문과 동일하며, 형식만 바꿨다.
> - 원문의 섹션 번호를 그대로 유지한다. 다른 문서가 "31절", "72~76절"처럼 번호로 참조한다.
> - 빌드/검증 방식(72~76절)은 `docs/BUILD_STRATEGY_v1.2.md`가 우선한다. 그 외 모든 섹션은 이 문서가 기준이다.
> - 확장형 세계 생성/청크/이주 구조는 `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`가 canonical companion이며, 고정 소형 arena를 제품 월드 구조로 사용하지 않는다.
> - 문명이 원시 정착에서 현대를 지나 미지의 미래까지 자율적으로 발전하는 장기 방향은 `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`가 canonical companion이다. 시대 라벨은 Core unlock timer가 아니다.
> - PC cinematic / Android mobile rendering 분리는 `docs/CINEMATIC_RENDERING_STRATEGY_v1.md`가 canonical companion이다.
> - 실제 구현 순서는 `docs/DEVELOPMENT_MILESTONES.md`, 현재 상태는 `tasks/WORK_STATE.md`를 따른다.

---

## 0. 문서 목적 / 새 세션 작업 원칙

이 문서는 신규 프로젝트 **LIFELENS**의 최상위 기준 설계서다.

기존 LOCAL OBSERVER를 계속 개조하는 것이 아니라, LOCAL OBSERVER에서 검증했던 아이디어와 사용자 요구사항을 취합하여 **처음부터 Unreal Engine에 맞는 구조로 새 프로젝트를 개발**한다.

새 세션에서는 사용자가 요구사항을 다시 설명하게 하지 말고, 이 문서를 기준으로 설계와 구현을 계속 진행한다.

### 최우선 작업 원칙

- 계획만 설명하지 말고 가능한 작업은 실제 수행한다.
- 처음부터 Unreal-native 구조로 설계한다.
- 기존 웹 구조를 억지로 Unreal에 이식하지 않는다.
- 기존 LOCAL OBSERVER는 요구사항/아이디어 참고자료로 사용한다.
- HTML/JS/Vercel 기반 게임 런타임은 제거한다.
- 기능 수보다 안정적인 기반이 우선이다.
- Android를 가장 먼저 실제 실행 가능 상태로 만든다. 이후 Windows PC를 지원한다.
- 유료 API / 유료 클라우드 / 유료 런타임에 의존하지 않는다. 무료 범위에서 개발/빌드가 가능해야 한다.
- 하나의 단순 오류 때문에 몇 시간짜리 전체 빌드를 반복하지 않는다.
- Engine Build와 LifeLens Project Build를 분리한다. 변경되지 않은 Unreal Engine은 매번 다시 컴파일하지 않는다.
- 무거운 작업 전에 짧은 Preflight 검증을 수행한다.
- 각 Phase는 실제 실행 가능한 빌드가 존재해야 완료로 본다.
- 기능 추가 후 기존 기능이 깨지는 regression을 최소화한다.
- Android → Windows 순서로 안정화한다.

---

## 1. 프로젝트 이름

| 항목 | 값 |
|---|---|
| 정식 프로젝트명 | LIFELENS |
| 표기 | Project LIFELENS |
| Unreal Project Name | LifeLens |
| GitHub Repository (권장) | lifelens-ue |
| 내부 Prefix | `LL_` |
| 패키지명 예시 | `com.lifelens.sim` |

---

## 2. 프로젝트 정의

LIFELENS는 극사실적 인간들이 플레이어의 직접 명령 없이 스스로 생각하고 생활하고 관계를 맺으며, **연애하고, 결혼하고, 가족을 이루고, 아이를 낳고, 아이를 키우고, 늙고, 죽고, 다음 세대로 삶을 이어가는 과정**을 플레이어가 관찰하는 Unreal 기반 자율 인생·사회 시뮬레이션이다.

---

## 3. 핵심 철학

LIFELENS의 중심은 플레이어가 아니다. 캐릭터들과 세계는 플레이어가 보고 있지 않아도 계속 살아가고 변화해야 한다.

- 플레이어의 기본 역할: **Controller가 아니라 Observer.**
- 목표 경험: "게임을 조종한다"보다 **"살아 있는 사회를 들여다본다"**에 가깝게 만든다.

핵심 개념: Autonomous Life, Human-like AI, Emergent Story, Memory, Emotion, Relationship, Romance, Family, Generation, Realistic Interaction, Photorealism, Observer, Cinematic Simulation

---

## 4. 기존 LOCAL OBSERVER에서 제거할 요소

LIFELENS의 핵심 런타임에서 제거: HTML, JavaScript 중심 구조, Three.js, DOM UI, 브라우저 저장 구조, 브라우저 입력 구조, Vercel 게임 런타임, GLTFLoader 중심 캐릭터 시스템, 웹 화면을 Unreal처럼 보이게 만드는 방식

**중요:** 코드를 옮기는 것이 아니라 아이디어와 요구사항만 가져온다.

---

## 5. 기존 프로젝트에서 계승할 요구사항

- **월드/캐릭터:** 실시간 3D 월드, 극사실 인간 캐릭터, 생활 AI
- **생존 요소:** 욕구, 위생, 방광, 수면, 배고픔, 갈증, 피로, 건강
- **내면:** 감정, 성격, 기억, 관계, 대화
- **인생 주기:** 연애, 결혼, 동거, 임신, 출산, 육아, 성장, 노화, 죽음, 세대교체
- **가족:** 가족, 가계도
- **생활/경제:** 생활 스케줄, 직업, 경제 확장, 자원, 건물, 시설 사용, 생존, 건설 확장
- **사회적 사건:** 목격, 진술, 증거, 신뢰도, 기억 오류, 추론
- **환경:** 시간, 날씨, 낮/밤, 동적 환경
- **확장성:** 사회 AI, 경제 AI, 정치/집단 AI 확장성
- **이동/공간:** 경로 탐색, 시선 처리, 거리 유지, 충돌 회피
- **현실적인 동작:** 앉기, 눕기, 잡기, 사용하기, 대화하기
- **관찰:** 관찰 카메라, 캐릭터 Follow, 시네마틱 카메라, 사건 자동 추적
- **UI:** Observer-first UI
- **플랫폼:** Android, Desktop PC (Windows + macOS)

---

## 6. 플랫폼 전략

개발 우선순위: **1. Android → 2. Windows PC**

Core Simulation은 두 플랫폼에서 동일하게 유지한다. PC와 Android는 그래픽 품질과 일부 성능 예산만 다르게 한다.

```
Core Simulation
    + AI
    + Memory
    + Relationship
    + Family
    + LifeCycle
    + World
    + Events
    |
    +--------------------------+
    |                          |
PC Presentation        Android Presentation
```

모바일이라고 캐릭터의 기본 AI 철학을 바꾸지 않는다. 단, Simulation LOD를 이용하여 계산 비용을 줄일 수 있다.

**그래픽 품질의 상한은 플랫폼별로 분리한다.** Android 우선 개발은 Windows PC의 시각 품질을 모바일 수준으로 제한한다는 뜻이 아니다.

- Windows PC: Unreal의 cinematic real-time stack(Lumen GI/Reflections, Nanite eligible assets, Virtual Shadow Maps, TSR, 고품질 PBR/atmosphere)을 적극 활용한다.
- Android: 동일한 World/Character/Facility identity를 유지하면서 conventional LOD/HLOD, 경량 material/shadow/foliage/texture 정책으로 다운스케일한다.
- 그래픽 LOD 차이는 presentation 차이일 뿐이며 Core/World/Save truth를 바꾸지 않는다.
- PC 고품질 자산은 Android용 fallback mesh/LOD 또는 별도 경량 presentation path를 가져야 한다.
- 세부 기준은 `docs/CINEMATIC_RENDERING_STRATEGY_v1.md`를 따른다.

---

## 7. 전체 아키텍처

```
LifeLens
├── Core
├── Character
│   ├── Identity
│   ├── Genetics
│   ├── Appearance
│   ├── Personality
│   ├── Traits
│   ├── Health
│   └── Skills
├── Needs
├── Emotion
├── AI
│   ├── Utility AI
│   ├── Goal System
│   ├── StateTree
│   ├── Planner
│   └── Perception
├── Interaction
│   ├── Smart Objects
│   ├── Reservation
│   └── Contextual Interaction
├── Memory
│   ├── Episodic Memory
│   ├── Belief
│   └── Knowledge
├── Relationship
│   ├── Friendship
│   ├── Conflict
│   └── Romance
├── Family
│   ├── Partnership
│   ├── Marriage
│   ├── Household
│   ├── Pregnancy
│   ├── Parenting
│   └── Genealogy
├── LifeCycle
│   ├── Birth
│   ├── Growth
│   ├── Aging
│   └── Death
├── World
│   ├── Time
│   ├── Weather
│   ├── Temperature
│   ├── Buildings
│   ├── Resources
│   └── Economy
├── Events
│   ├── Social Event
│   ├── Witness
│   ├── Evidence
│   └── Consequence
├── Observer
│   ├── Camera
│   ├── Cinematic Director
│   └── Event Director
├── Save
├── UI
└── Platform
```

---

## 8. 캐릭터 Identity

**캐릭터 이름을 시스템 Primary Key로 절대 사용하지 않는다.** 모든 인간은 영구 GUID를 가진다.

기본 구조 예:

| 그룹 | 필드 |
|---|---|
| 식별 | CharacterGuid |
| 기본 정보 | DisplayName, Sex, GenderPresentation, BirthDate, Age, Occupation |
| 소속 | HouseholdId |
| 가족 연결 | ParentIds, SiblingIds, PartnerIds, ChildrenIds |
| 신체/기질 | Genetics, Appearance, Personality, Traits, Skills, Health |
| 상태 | Needs, Emotion |
| 인지 | Memory, Beliefs, Relationships |
| 현재 | CurrentGoal, CurrentAction, CurrentLocation, CurrentThought |
| 이력 | Schedule, LifeHistory |

이름이 바뀌거나, 결혼하거나, 가족관계가 바뀌어도 CharacterGuid는 유지한다.

---

## 9. 정식 게임의 초기 인구 — 핵심 요구사항

새로운 LIFELENS World는 기본적으로 **남성 2명 + 여성 2명, 총 4명의 성인**으로 시작한다.

**중요:** 서윤 / 하린 / 민재 / 태훈 같은 고정 캐릭터를 정식 게임의 기본 시작 인구로 사용하지 않는다. 기존 이름은 개발 테스트용으로 사용할 수 있지만, 실제 NEW GAME에서는 Procedural Generation으로 생성한다.

---

## 10. 초기 캐릭터 생성 시점

캐릭터 랜덤 생성은 **"NEW GAME으로 새로운 World를 생성하는 최초 1회"**에만 발생한다.

```
NEW GAME
→ WorldSeed 생성
→ 초기 인간 4명 생성
→ 각 Character GUID 생성
→ 이름 생성
→ 유전정보 생성
→ 외형 생성
→ 성격 생성
→ Traits 생성
→ 능력 생성
→ 선호 생성
→ 배경 생성
→ 초기 상태 생성
→ Save 생성
```

이후 해당 World에서는 캐릭터를 다시 랜덤 생성하지 않는다. 게임을 종료하고 다시 실행해도 동일한 네 사람이 그대로 존재한다.

---

## 11. New Game마다 다른 사회

새로운 게임을 시작하면 새로운 WorldSeed를 만든다. 따라서 World A와 World B의 시작 인물은 완전히 다를 수 있다.

| | 남성 | 여성 |
|---|---|---|
| WORLD A | 준혁, 현우 | 소연, 지우 |
| WORLD B | 도윤, 민석 | 서아, 유진 |

단순 이름만 바뀌는 것이 아니다. 외모, 성격, 능력, 욕구 성향, 연애 성향, 가족관, 직업, 취미, 배경 등도 새롭게 생성한다. 따라서 NEW GAME마다 전혀 다른 사회적 결과가 나올 수 있어야 한다.

---

## 12. 초기 생성 대상

| 범주 | 항목 |
|---|---|
| Identity | GUID, 이름, 성별, 나이, 생년월일 |
| Appearance / Genetics | 얼굴 특징, 피부, 눈, 머리, 체형, 키, 신체적 특징 |
| Personality | Empathy, Sociability, Introversion, Aggression, Honesty, Jealousy, Curiosity, RiskTolerance, Conscientiousness, Romanticism, FamilyOrientation, Patience, Ambition, Loyalty |
| Physical Traits | 체력, 건강, 신진대사, 수면 성향, 활동성 |
| Mental Traits | StressSensitivity, EmotionalResilience, LonelinessSensitivity, SocialNeed, RiskSensitivity |
| Skills / Potential | Cooking, Social, Fitness, Intelligence, Learning, Crafting, Survival, Leadership, 기타 |
| Life Preferences | Romance Preference, Marriage Intent, Child Preference, Family Orientation, Career Ambition, Lifestyle Preference |
| Background | 직업, 교육, 관심사, 취미, 과거 경험 일부 |

---

## 13. 랜덤 생성은 완전 난수로 하지 않는다

모든 특성을 단순 0~100 Uniform Random으로 만들지 않는다. 현실적인 인간 분포를 사용한다.

권장: **Normal Distribution + Trait Correlation + Archetype Bias + Controlled Randomness**

- 대부분 사람은 중간값 부근.
- 극단값은 상대적으로 희귀하게 발생.
- 하지만 캐릭터들이 지나치게 비슷해지지 않도록 Variation을 유지한다.

---

## 14. 특성 간 상관관계

일부 특성은 다른 행동 경향에 영향을 준다.

| 특성 | 영향 |
|---|---|
| 높은 Empathy | 타인 위로 가능성 상승 |
| 높은 Jealousy | 연애 갈등 Utility 상승 가능 |
| 높은 FamilyOrientation | 결혼/출산 관련 Goal 가중치 상승 |
| 높은 Ambition | Career Goal 강화 |
| 높은 Introversion | 혼자 있는 활동 선호 가능 |

하지만 Personality는 행동을 강제하지 않는다. **Utility Score와 probability를 조정한다.**

---

## 15. 초기 관계

초기 4명은 기본적으로 완성된 연인이나 부부로 시작하지 않는다.

- 기본 상태: **Stranger** 또는 **Low Familiarity** 정도.
- 누가 누구와 친해지고, 누가 연인이 되고, 누가 결혼하고, 누가 싸울지는 **시뮬레이션 결과가 결정**한다.
- 특정 남녀를 미리 커플로 지정하지 않는다.

---

## 16. WorldSeed

모든 NEW GAME은 WorldSeed를 가진다. (예: `WorldSeed = 874213954`) Seed를 Save에 저장한다.

사용 목적: 초기 캐릭터 생성, 일부 초기 환경 생성, 디버깅 재현

개발 과정에서는 특정 Seed를 입력하여 동일한 초기 조건을 재현할 수 있게 한다. 일반 사용자 NEW GAME에서는 자동 생성한다.

---

## 17. 생성 이후 영구 고정 데이터

**초기 생성 후 저장해야 하는 값 (Load 시 다시 랜덤 생성하면 안 됨):** Character GUID, Birth Identity, Genetic Base, 출생 정보, 초기 Personality, 초기 Background

**살면서 변화할 수 있는 값:** Skill, Health, Relationship, Emotion, Memory, Belief, Preferences 일부, Personality 일부, Appearance, Family State, Career

핵심 철학: **"태어난 사람은 동일하지만, 살아온 경험을 통해 사람이 변한다."**

---

## 18. 시작 다양성 목표

| World | 결과 예 |
|---|---|
| A | A ↔ B 연애, C ↔ D 친구, A ↔ C 갈등 |
| B | A ↔ D 연애, B ↔ C 갈등, D는 결혼에 관심이 낮음 |
| C | 수년 동안 연애가 거의 생기지 않을 수도 있음 |
| D | 한 사람을 두 명이 좋아해 질투/갈등 가능 |

이런 결과를 허용한다. **LIFELENS는 정해진 Story Route를 강제하지 않는다.**

---

## 19. 개발 프로토타입과 정식 시작의 구분

기술 개발 초기는 1명으로 검증할 수 있다. (예: 서윤 1명 + 작은 집)

목적: AI, Navigation, Smart Object, Needs, Animation, Observer UI 등의 기술 검증.

하지만 정식 New Game의 완성 기준은 **남성 2명 + 여성 2명 Procedural Generation**이다.

---

## 20. 극사실 캐릭터

LIFELENS는 실사형 인간 표현을 목표로 한다.

목표 요소: 현실적인 얼굴, 피부, 눈, 머리카락, 체형, 나이 변화, 표정, 입 움직임, 시선, 자세, 보행, 상호작용

| 플랫폼 | 구성 |
|---|---|
| PC | 고품질 Character |
| Android | 동일 Character Identity + LOD + Hair Cards + Texture 축소 + 경량 Material + Animation 최적화 |

PC와 모바일 캐릭터가 전혀 다른 인물처럼 보이면 안 된다.

---

## 21. 인간다운 움직임

중요 기술: Motion Matching, IK, Motion Warping, Control Rig, Contextual Animation, Smart Objects, NavMesh, Avoidance, Look-at / Gaze

**잘못된 방식:** 의자 발견 → Sit Animation

**권장 방식:**

```
앉고 싶은 Goal 발생
→ 의자 탐색
→ 사용 가능 확인
→ 예약
→ Interaction Point 확인
→ Navigation
→ 방향 조정
→ Motion Warp
→ IK
→ 실제 위치에 앉기
→ 사용 상태 유지
→ 종료 후 자리 반환
```

---

## 22. Smart Object

월드 Object는 장식이 아니라 **AI가 이해하고 사용할 수 있는 객체**다.

예: Chair, Table, Bed, Toilet, Sink, Shower, Fridge, Stove, Sofa, TV, Computer, Door, Storage, WorkStation

각 Smart Object가 가지는 것: Available State, Interaction Point, Orientation, Reservation, Allowed User, Required Skill, Action, Animation Context, Needs Effect

---

## 23. Needs 시스템

| 구분 | 항목 |
|---|---|
| Physical Needs | Hunger, Thirst, Sleep, Bladder, Hygiene, Energy, Temperature, Pain, Health |
| Psychological | Loneliness, Stress, Fear, Safety, Comfort, Boredom, Curiosity, Happiness, Anger |

Needs는 화면에 보여주기 위한 값이 아니라 **AI 의사결정에 사용**한다.

---

## 24. Personality

다차원 Personality를 사용한다.

Empathy, Sociability, Introversion, Aggression, Honesty, Jealousy, Curiosity, RiskTolerance, Conscientiousness, Romanticism, FamilyOrientation, Patience, Ambition, Loyalty

같은 상황에서도 캐릭터별 반응이 달라야 한다.

---

## 25. Emotion

예: Joy, Sadness, Anger, Fear, Embarrassment, Pride, Jealousy, Affection, Anxiety, Relief, Grief

- **감정 입력:** 사건, 관계, 기억, 성격, 건강, Needs, 환경
- **감정 출력:** 표정, 행동, 대화, Decision, Relationship 변화

---

## 26. AI 핵심 아키텍처

```
Needs + Emotion + Memory + Personality + Relationship + Environment + Schedule
        ↓
    Utility AI          ← 무엇을 할 것인가?
        ↓
      Goal
        ↓
    StateTree           ← 어떻게 수행할 것인가?
        ↓
   Action / Plan
        ↓
Smart Object / Navigation   ← 세상의 무엇을 사용할 것인가?
        ↓
Animation / Interaction
```

---

## 27. AI 예시

현재 상태: Hunger = 86, Sleep = 42, Loneliness = 57

| Goal 후보 | Utility |
|---|---|
| Eat | 0.91 |
| Talk | 0.60 |
| Sleep | 0.44 |

선택된 Goal: **Eat**

Plan:

```
음식 확인
→ 냉장고 이동
→ 음식 획득
→ 조리 여부 판단
→ 식사 위치 검색
→ 자리 예약
→ 식사
→ Hunger 감소
→ Memory 생성
```

---

## 28. 완벽하게 합리적인 NPC 금지

NPC가 항상 가장 효율적인 행동만 하면 사람처럼 보이지 않는다.

행동에 영향을 주는 것: 습관, 취향, 기분, 충동, 스트레스, 두려움, 게으름, 관계, 과거 경험, 성격

예:
- 배가 고파도 싸운 사람이 식탁에 있으면 피할 수 있다.
- 매우 피곤해도 사랑하는 사람이 힘들어하면 대화를 선택할 수 있다.

---

## 29. Memory

MemoryRecord 필드: Who, What, Where, When, Emotion, Importance, Confidence, Witnessed, Source, Decay, Tags

예:

| 필드 | 값 |
|---|---|
| When | 14:32 |
| What | 태훈이 식량창고에서 음식을 가져감 |
| 목격 | 서윤 |
| Confidence | 0.93 |
| Emotion | Suspicion |

---

## 30. Memory → Belief

```
Memory → Belief → Relationship → Decision
```

NPC는 세계의 모든 사실을 자동으로 알면 안 된다. **직접 목격 / 다른 사람에게 들음 / 추측**을 구분한다. 기억은 틀릴 수도 있다.

---

## 31. Relationship

**단일 친밀도 값 하나 사용 금지.**

관계 차원: Affection, Trust, Respect, Comfort, Familiarity, Attraction, RomanticInterest, SexualAttraction, Commitment, Conflict, Jealousy, Fear, Grudge

따라서 다음이 가능하다:
- 좋아하지만 신뢰하지 않음
- 친하지만 연애감정 없음
- 사랑하지만 자주 싸움
- 헤어졌지만 애정이 남음

---

## 32. Romance

연애는 Script로 강제하지 않는다.

입력: Personality Compatibility, Attraction, Familiarity, Trust, Affection, Shared Experience, Emotion, Past Relationship, Life Goal, Availability

가능한 흐름:

```
만남 → 친분 → 관심 → 호감 → 데이트 → 고백 → 연애 → 동거 → 약혼 → 결혼
```

고백한다고 자동 수락하지 않는다. **상대 AI가 독립적으로 판단한다.**

---

## 33. 연애 행동

대화, 함께 식사, 산책, 데이트, 선물, 도움, 위로, 고백, 스킨십, 관계 확인, 동거 제안, 청혼 등.

모든 행동은 현재 관계, 성격, 상황, 감정, 기억으로 결정한다.

---

## 34. 결혼

Relationship State 예: Single, Dating, Engaged, Married, Separated, Divorced, Widowed

결혼은 Badge가 아니다. 실제 생활에 영향을 준다.

영향: 주거, Household, 공동 자원, 재정, 역할 분담, 생활 Schedule, 가족 관계, 출산 계획, 육아, 갈등

---

## 35. Household

가족과 Household는 별도로 관리 가능하게 한다.

Household 필드: HouseholdGuid, Members, Home, Resources, SharedMoney, SharedObjects, Responsibilities

가능한 형태: 연인 동거, 부부, 부부+자녀, 친구 공동거주, 다세대 가족

---

## 36. Pregnancy

임신 가능성은 단순 랜덤 이벤트가 아니다.

- 고려 요소: Age, Health, Fertility, Relationship, PregnancyIntent, LifeSituation, HouseholdCondition
- Pregnancy State: Stage, Health, Fatigue, Stress, Nutrition, DueDate

임신은 실제 AI 행동/Needs에 영향을 준다.

---

## 37. Birth

```
새 Character 생성
→ 새 GUID
→ 부모 연결
→ Household 연결
→ Genetics 생성
→ LifeHistory 시작
```

---

## 38. Genetics

Child Genetics = **Parent A + Parent B + Genetic Variation + Random Variation**

영향 가능: 얼굴, 눈, 머리, 피부, 체형, 키 성향, 일부 건강 특성, 일부 능력 Potential, 일부 Temperament

**성격 전체는 유전으로 결정하지 않는다.**

---

## 39. 성장

Life Stage: Baby → Toddler → Child → Teen → Young Adult → Adult → Middle Age → Elderly

각 단계에서 신체, Needs, 가능 행동, Animation, Skill, Education, Responsibility, Relationship 등이 바뀐다.

---

## 40. Parenting

부모가 실제로 행동한다: 먹이기, 재우기, 씻기기, 안기, 놀아주기, 보호, 교육, 훈육, 위로, 건강관리

양육 경험은 아이에게 영향을 준다: Attachment, Trust, Confidence, Stress, SocialSkill, Personality Development

---

## 41. 가족 관계 / Genealogy

관리하는 관계: Parent, Child, Sibling, HalfSibling, Spouse, Grandparent, Grandchild, InLaw 등

가계도 UI 확장 가능.

---

## 42. Aging

시간이 흐르면 캐릭터는 늙는다.

영향: Appearance, Health, Energy, Movement, Fertility, Occupation, Life Goal, Family Role

**불로불사 NPC 구조 금지.**

---

## 43. Death

캐릭터는 죽을 수 있다.

- Death 이후에도 남기는 것: LifeHistory, Memory, Family, Genealogy, Social Impact
- 주변 사람에게 발생하는 것: Grief, Memory, Relationship Change

세대교체를 실제로 구현한다.

---
## 44. Life History

캐릭터별 Timeline 기록: Birth, Childhood, Education, Friendship, First Love, Dating, Marriage, Birth of Child, Career, Conflict, Accident, Divorce, Achievement, Family Death, Own Death 등

---

## 45. World

World State: Time, Date, Weather, Temperature, Resources, Buildings, Locations, Population, Economy, Events, Social State

---

## 46. 시간 시스템

시간에 따라 변하는 것: 태양, 조명, 온도, 생활 패턴, 출근, 수면, 상점, 야외활동, AI Schedule

---

## 47. Weather

날씨는 그래픽 효과만이 아니다.

```
비
→ 야외 활동 감소
→ 실내 인구 증가
→ 사회적 상호작용 증가
→ 갈등/친밀 사건 확률 변화
```

---

## 48. 직업 / 경제 확장

장기적으로 지원 가능하게 한다: 직업, 근무, 소득, 소비, 저축, 실업, 교육, 직업 만족, 주거비, 재산, 사회적 지위

**초기 MVP에서는 후순위.**

---

## 49. 사회 / 정치 확장

향후 확장 가능: Group, Leadership, Power, Rule, Election, Policy, Resource Allocation, Social Conflict

**초기 Core와 강하게 묶지 않는다.**

---

## 50. 생존 / 자원 / 건설

기존 LOCAL OBSERVER의 생존 요소를 버리지 않는다.

향후 확장: Food, Water, Energy, Shelter, Tools, Resources, Building, Repair, Environmental Risk

그러나 초기 LIFELENS에서는 **인간과 관계 시뮬레이션이 더 높은 우선순위**다.

---

## 51. Event

Event 예: Accident, Illness, Argument, Fight, Confession, Breakup, Engagement, Marriage, Pregnancy, Birth, Divorce, Death, Crime, Witness, Rescue, Disappearance, Achievement

Event가 영향을 주는 것: Memory, Emotion, Relationship, LifeHistory, WorldState

---

## 52. Witness / Evidence / Statement

```
Event → Witness → Memory → Statement → Evidence → Belief → Social Reaction
```

NPC는 자신이 모르는 사건에 전지적 관점으로 반응하면 안 된다.

---

## 53. Observer-first UI

핵심 UI 원칙: **"평소에는 세계를 본다. 필요할 때만 정보를 연다."**

- 기본 화면은 최대한 깨끗하게 유지한다.
- 수많은 막대와 숫자가 월드를 가리지 않게 한다.
- 게임 화면만 캡처하면 전략게임 Dashboard보다는 영화 / 다큐멘터리 / 실제 가상세계에 가까워야 한다.

---

## 54. Main Observer View

- **상시 노출 후보:** 날짜, 시간, 날씨, 기온, 인구, Simulation Speed, 중요 Event Notification
- **기본 화면에 항상 띄우지 않는 것:** 욕구 수치, 관계 수치, 감정 그래프 등

---

## 55. UI 단계

| Level | 상태 | 표시 |
|---|---|---|
| LEVEL 0 | 아무것도 선택하지 않음 | 월드 관찰 |
| LEVEL 1 | 캐릭터 선택 | Quick Inspector |
| LEVEL 2 | 캐릭터 상세 | Full Character Information |

---

## 56. Character Inspector

예:

```
유진
27세
연구원

현재 행동: 지민과 대화 중
현재 감정: 편안함
현재 목표: 저녁 식사
상태: 조금 피곤함 / 배고픔 양호

[자세히]
```

Quick Inspector는 `73/100`보다 **양호 / 낮음 / 높음 / 매우 피곤함** 등 사람이 읽기 쉬운 표현을 우선한다.

---

## 57. Character Detail

탭 구성: Overview, Needs, Emotion, Personality, Traits, Health, Skills, Memory, Relationships, Romance, Family, Career, Life History

---

## 58. Relationship UI

예:

```
유진 ↔ 도윤

현재 관계: 연인
애정: 매우 높음
신뢰: 높음
갈등: 낮음

최근 변화:
+ 함께 저녁식사
+ 상대를 위로함
- 의견 충돌
```

Relationship History: 첫 만남, 첫 데이트, 연애 시작, 동거, 약혼, 결혼, 주요 갈등, 화해

---

## 59. Family UI

Partner, Parents, Children, Siblings, Extended Family. 가계도 확장 가능.

---

## 60. Pregnancy UI

- **일반 Inspector:** 임신 24주 / 상태 안정 / 예정일 xxxx.xx.xx
- **상세:** Health, Fatigue, Nutrition, Stress, Stage, DueDate

---

## 61. World Overview

필요할 때만 연다. 예: Population, Households, Couples, Married Couples, Pregnancies, Babies, Children, Adults, Elderly, Major Events

---

## 62. 중요 사건 알림

- **알림하지 않는 일상:** 식사, 잠, 화장실, 세면 등
- **알림하는 중요 사건:** 고백, 연애 시작, 약혼, 결혼, 임신, 출산, 큰 갈등, 이별, 사고, 죽음 등

Notification에는 `[보기]` 버튼을 두고, 선택 시 해당 Event로 카메라 이동 가능.

---

## 63. Observer Camera

Free Observer, Character Follow, Shoulder Follow, Interior Observation, Event Camera, Cinematic Camera

---

## 64. Cinematic Observer

LIFELENS의 대표 기능 후보. 플레이어가 조작하지 않아도 **AI Director가 흥미로운 상황을 찾아 보여준다.**

예:

```
두 사람의 중요한 대화
→ 밖에 있는 다른 캐릭터 행동
→ 비 시작
→ 연애 관련 Event 발생
→ 카메라 전환
```

목표: **"AI 인간 사회 다큐멘터리"**

---

## 65. Event Director

모든 사건을 추적하지 않는다. Importance Score 사용.

| Event | Importance |
|---|---|
| Eating | 0.05 |
| Sleeping | 0.05 |
| Argument | 0.4 |
| Confession | 0.7 |
| Pregnancy | 0.8 |
| Marriage | 0.9 |
| Birth | 1.0 |
| Death | 1.0 |

Importance + 현재 Camera + 사용자 관심 대상 등으로 Notification / Camera 전환을 판단한다.

---

## 66. 성능 원칙

모든 NPC가 매 프레임 모든 시스템을 계산하지 않는다.

| 시스템 | 갱신 주기 (예시) |
|---|---|
| Rendering | 30~60Hz |
| Movement | Frame Based |
| Near Animation | 30~60Hz |
| Far Animation | Reduced |
| AI Decision | 약 2~4Hz |
| Needs | 약 1Hz |
| Relationship | Event Driven |
| Memory | Event Driven |
| Schedule | Low Frequency |
| World AI | 약 0.2~1Hz |

실제 값은 Profiling으로 결정한다.

---

## 67. Simulation LOD

| 거리 | 처리 |
|---|---|
| Observed / Near | Full Animation, 높은 AI Frequency, Perception 활성 |
| Nearby | 일부 Update 감소 |
| Far / Off-screen | Abstract Simulation, 낮은 AI Frequency, Schedule/Event 중심 |

**화면 밖 캐릭터도 삶 자체가 멈추면 안 된다.**

---

## 68. C++ Source 구조

```
Source/
└── LifeLens/
    ├── Core/
    ├── Character/
    ├── Genetics/
    ├── Needs/
    ├── Emotion/
    ├── AI/
    ├── Interaction/
    ├── Memory/
    ├── Relationship/
    ├── Romance/
    ├── Family/
    ├── LifeCycle/
    ├── World/
    ├── Events/
    ├── Observer/
    ├── Save/
    ├── UI/
    └── Platform/
```

---

## 69. C++ / Blueprint 역할

| 담당 | 영역 |
|---|---|
| C++ | Core Simulation, AI, Needs, Memory, Relationship, Romance Logic, Family, Life Cycle, Save, Data, 성능 중요 시스템 |
| Blueprint | Level, Animation, VFX, Camera Tuning, UI Layout, Content Setup, Visual Interaction |

핵심 시스템을 Blueprint Spaghetti로 만들지 않는다. 반대로 콘텐츠를 전부 C++로 하드코딩하지 않는다.

---

## 70. Data Driven

Data Asset / Data Table 활용: Character Name Pool, Character Archetype, Personality Distribution, Trait, Needs Tuning, Relationship Tuning, Smart Object, Occupation, Life Stage, Events, Animation Context, World Setting

가능한 한 코드 변경 없이 콘텐츠를 늘릴 수 있게 한다.

---

## 71. Save System

저장 대상:

| 그룹 | 항목 |
|---|---|
| World | WorldSeed, World Time, Weather |
| Characters | GUID, Genetics, Appearance, Personality, Traits, Skills, Health, Needs, Emotion, Memory, Beliefs, Relationships, Romance |
| 가족 | Households, Families, Genealogy, Pregnancy, Life History |
| 월드 상태 | World Object State, Resources, Events |

Reference는 GUID 기반으로 연결한다.

---

## 72. 빌드 전략

> ⚠️ 72~76절은 `docs/BUILD_STRATEGY_v1.2.md`가 대체한다. 원문 보존을 위해 남겨둔다.

현재 Smoke Pipeline에서 얻은 가장 중요한 교훈: **Unreal Engine 전체를 프로젝트 변경마다 다시 빌드하면 안 된다.**

목표:

```
UE Engine Build
→ Reusable Engine Cache / Artifact
→ LifeLens Project Compile
→ Cook
→ Package
→ APK
```

Engine이 변하지 않았으면 Engine 전체 Compile을 반복하지 않는다.

---

## 73. Android Build Pipeline

```
Preflight
→ Java 확인
→ SDK 확인
→ NDK 확인
→ Engine Cache 확인
→ LifeLens Compile
→ Cook
→ Package
→ APK 존재 확인
→ Artifact Upload
→ Release / Download
```

가능한 오류는 무거운 compile 전에 차단한다.

---

## 74. FAST TEST / FULL BUILD

| 구분 | 구성 |
|---|---|
| FAST TEST | 작은 Map, 최소 Character, 낮은 Graphic, AI 검증, 빠른 APK |
| FULL BUILD | 고품질 Character, 전체 Environment, 고품질 Lighting, 최종 Asset |

개발 대부분은 FAST TEST. Milestone에서 FULL BUILD.

---

## 75. 개발 Phase

| Phase | 내용 |
|---|---|
| P0 | Unreal-native 프로젝트 생성, Android APK 성공 |
| P1 | 작은 주거 공간, 개발용 Character 1명, Observer Camera |
| P2 | Navigation, Walking, Turning, Chair / Bed / Toilet 등 Interaction |
| P3 | Basic Needs (Hunger, Sleep, Bladder, Hygiene) |
| P4 | Utility AI, StateTree — 개발 캐릭터 1명이 자율적으로 하루 생활 |
| P5 | Personality, Emotion |
| P6 | Memory, Belief |
| P7 | Procedural Character Generator |
| P8 | 정식 NEW GAME — Male 2 / Female 2, 각각 새 이름·외모·성격·Trait·Skill·Background |
| P9 | 4인 독립 생활 |
| P10 | Relationship |
| P11 | Romance |
| P12 | Household / Cohabitation |
| P13 | Marriage |
| P14 | Pregnancy |
| P15 | Birth / Genetics |
| P16 | Baby / Child / Teen |
| P17 | Parenting |
| P18 | Aging / Death |
| P19 | Generation Simulation |
| P20 | World System (Time, Weather, Resources) |
| P21 | Social Event, Witness, Evidence, Belief |
| P22 | Observer UI |
| P23 | Cinematic / Event Director |
| P24 | Economy / Advanced Society |
| P25 | Graphics Polish |
| P26 | Android Optimization |
| P27 | Desktop High Quality (Windows + macOS) |

---

## 76. Phase 완료 조건

다음을 만족한 후 다음 Phase로 이동한다:

- 기능 실제 작동
- Crash 없음
- Android Build 성공
- Save/Load 확인
- 기존 기능 Regression 최소
- Smoke Test 통과

---

## 77. 첫 기술 프로토타입

정식 게임 시작은 4명이지만 개발 초기는 1명으로 시작한다. **작은 집 + 개발용 캐릭터 1명.**

자동으로 수행해야 하는 하루:

```
기상 → 화장실 → 세면 → 식사 → 활동 → 휴식 → 저녁 → 취침
```

시간표로 강제하지 않고 **Needs + Utility AI + StateTree + Smart Object**로 만들어야 한다.

---

## 78. 정식 NEW GAME 첫 목표

```
NEW GAME 버튼
→ 새 WorldSeed 생성
→ 남성 2명 / 여성 2명 생성
   (각각 새 이름, 새 얼굴, 새 체형, 새 성격, 새 특성, 새 능력, 새 배경, 새 선호)
→ 4명은 대부분 Stranger 상태
→ 시뮬레이션 시작
→ 누가 친해지고, 누가 싸우고, 누가 연애하고, 누가 결혼할지는 AI와 경험이 결정
```

---

## 79. 현실감 Acceptance Criteria

줄여야 하는 문제:

- 허공에 앉음
- 벽 통과
- 캐릭터 겹침
- 같은 위치에서 떨림
- 대화 중 상대를 보지 않음
- 상호작용 물체와 몸 위치 불일치
- 상황과 무관한 Animation
- 행동 무한 반복
- 목적 없는 왕복
- 같은 의자를 여러 명이 사용
- Navigation 실패 후 멈춤
- 감정과 표정 불일치

---

## 80. Human-like AI Acceptance Criteria

같은 캐릭터도 매일 완전히 같은 행동을 하지 않는다. 같은 Hunger라도 성격, 상황, 관계, 기억, 기분에 따라 행동이 달라질 수 있다.

NPC가 할 수 있어야 하는 것:

```
Goal 생성 → Goal 선택 → Plan → 실행 → 실패 대응 → 대안 탐색 → 행동 종료 → Memory 생성
```

---

## 81. Observer Acceptance Criteria

- 아무것도 선택하지 않은 기본 화면은 매우 깨끗해야 한다.
- 캐릭터를 클릭했을 때만 정보가 열려야 한다.
- 더 자세한 정보는 추가 선택 후 표시한다.
- 플레이어가 명령하지 않아도 세계에서 흥미로운 상황이 발생해야 한다.
- 단순히 캐릭터 4명이 욕구만 채우다 하루가 끝나는 게임이 되어서는 안 된다.

---

## 82. Graphics Acceptance Criteria

- **최종 PC 목표 (Windows + macOS):** Photorealistic / Cinematic
- **Android 목표:** 동일한 World와 Character Identity를 유지하면서 안정적인 성능 범위 내 최대 현실감.

모바일은 "낮은 FPS의 과도한 그래픽"보다 **"안정적인 30fps의 높은 현실감"**을 우선한다.

---

## 83. 반드시 피할 것

- 웹 프로토타입 구조 그대로 이식
- 모든 기능 한꺼번에 개발
- Engine 매번 전체 Rebuild
- NPC 모든 시스템 매 Frame 계산
- Name을 ID로 사용
- Relationship 하나의 숫자로 표현
- NPC가 모든 사건을 전지적으로 앎
- Smart Object 없이 Animation만 실행
- 기본 화면 UI 도배
- Android와 PC를 별도 게임으로 개발
- 빌드 성공 전 기능 대량 추가
- 유료 서비스 필수 의존
- Random Character를 Load 때마다 재생성
- 초기 캐릭터 Couple 강제 지정
- 특정 Story Route 강제

---

## 84. 장기적 세계 확장

LifeLens의 세계는 가족·인구 증가에서 끝나지 않는다.

```
4명
→ Relationship / Couple / Family / Children / Generation
→ Households / Settlement / Population
→ Specialization / Exchange / Economy / Institutions
→ Agriculture / Metallurgy / Cities
→ Mechanical / Industrial civilization
→ Electrical / Modern infrastructure
→ Digital / Network civilization
→ AI / Robotics / Advanced automation
→ Advanced energy / materials / biotechnology
→ Planetary / Space civilization
→ Unknown future civilization
```

이 순서는 강제 Tech Tree가 아니다. 실제 자원, 지식, Capability, 시설, 에너지, 사회조건과 문제 해결의 결과로 각 경로가 열려야 한다.

- 시간만 흘렀다고 시대가 자동으로 상승하지 않는다.
- 어떤 사회는 발전이 느리거나 붕괴할 수 있다.
- 기술과 지식은 소실·재발견될 수 있다.
- 같은 seed/ruleset/snapshot에서는 결정론적 재현을 유지한다.
- Observer의 시대/문명 단계 표시는 실제 Capability와 사회 상태를 요약하는 결과일 뿐이다.

세부 원칙은 `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`를 따른다.

---

## 85. LIFELENS의 핵심 차별점

이 프로젝트는 **"플레이어가 이야기를 만들고 NPC가 그것을 연기하는 게임"**이 아니다.

목표는 **"NPC가 살아가면서 스스로 이야기를 만들고, 플레이어는 그것을 발견하고 관찰하는 세계"**다.

---

## 86. 절대 빠지면 안 되는 핵심 요구사항

- **아키텍처:** Unreal-native Architecture, 무료 개발/빌드 원칙, 빠르고 반복 가능한 Build Pipeline
- **캐릭터:** Extreme Realistic Human Character, Natural Human Animation, Autonomous AI
- **초기 인구:** Procedural Initial Population, NEW GAME마다 Male 2 / Female 2 생성, NEW GAME마다 새로운 이름 / 외모 / Personality / Traits / Skills, WorldSeed, Character GUID
- **내면 시스템:** Needs, Personality, Emotion, Memory, Belief
- **관계/가족:** Relationship, Romance, Marriage, Cohabitation, Household, Pregnancy, Birth, Genetics, Parenting, Growth, Aging, Death, Generations, Genealogy
- **세계/사건:** Time, Weather, Events, Witness, Evidence, Statements
- **관찰:** Observer-first UI, Character Inspector, World Overview, Cinematic Observer, Event Director
- **플랫폼:** Android, Windows PC

---

## 87. LIFELENS 최종 정의

LIFELENS는 새로운 세계를 생성할 때마다 서로 다른 이름, 서로 다른 외모, 서로 다른 성격, 서로 다른 능력, 서로 다른 삶의 목표를 가진 남성 2명과 여성 2명으로 시작한다.

이 네 사람은 처음부터 정해진 커플이나 정해진 스토리를 갖지 않는다. 그들은 스스로 생활하고, 기억하고, 느끼고, 친해지고, 다투고, 사랑하고, 헤어지고, 결혼하고, 아이를 낳고, 아이를 키우고, 늙고, 죽는다.

그들의 아이들 역시 성장하여 새로운 인간관계를 형성하고 새로운 세대를 만들어간다.

플레이어는 이 모든 삶에 직접 명령을 내리는 대신, 가능한 한 방해받지 않는 화면에서 그들의 인생과 사회의 변화를 관찰한다.

그 사회는 단순히 몇 세대를 이어가는 데서 멈추지 않는다. 주민들이 생존 문제를 해결하고 지식을 전승하며 도구·시설·생산·제도·기술을 만들어가면, 원시 정착에서 현대 수준을 지나 현실에 아직 존재하지 않는 미래 문명까지 발전할 가능성을 가진다. 그 발전은 날짜나 고정 시대 스크립트가 아니라 실제 시뮬레이션 상태의 결과여야 한다.

매번 새로운 게임을 시작할 때마다 새로운 네 사람과 새로운 세계의 역사가 시작된다.

---

## 88. 새 세션에서 즉시 진행할 작업

초기 bootstrap 단계는 이미 완료되었다. 새 세션에서 이 오래된 초기 체크리스트를 다시 수행하지 않는다.

새 세션은 다음 순서로 현재 상태를 복구한다.

1. 실제 GitHub `main` / open PR / exact-head Actions 확인.
2. `PROJECT_STATUS.md` 확인.
3. `tasks/WORK_STATE.md`에서 현재 ACTIVE / NEXT 확인.
4. `docs/DEVELOPMENT_MILESTONES.md`에서 canonical execution order 확인.
5. `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`에서 장기 방향 확인.
6. `tasks/TEAM_BOARD.md`에서 ownership / lock / Integration Request 확인.
7. 현재 milestone에서 실제 구현 가능한 작업을 계속한다.

2026-09-18 기준 초기 Unreal 프로젝트, 4인 NEW GAME, Needs/Utility, 관계/가족/lifecycle, primitive civilization, 시간/날씨, Observer catch-up은 이미 main에 존재한다.

현재 기능 개발 축은 **C1 Settlement & Subsistence**다.

- C1-A Settlement Facility Authority — 완료.
- C1-B Autonomous Settlement Need Recognition — 다음.
- 이후 Facility Effects/Maintenance -> Water/Food/Cultivation -> Emergent Settlement -> 후속 재료/기술 progression.

Android APK Gate B는 제품 우선순위에서 삭제된 것이 아니라 사용자 요청으로 일시 정지 상태다. 재개 시 현재 main에서 검증된 prebuilt-host 방향으로 이어간다.

**현재 첫 목표가 아니라 유지해야 할 제품 기준:**
"NEW GAME을 누르면 서로 다른 남성 2명과 여성 2명이 생성되고, 플레이어 명령 없이 생존·관계·가족·지식·정착·문명을 스스로 만들어가며 장기적으로 미지의 미래까지 발전할 수 있다."

---

*END OF PROJECT LIFELENS MASTER DESIGN SPEC v1.1*
