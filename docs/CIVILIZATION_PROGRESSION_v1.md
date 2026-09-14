# LifeLens Civilization Progression Design v1

> 상태: **Canonical product-direction extension**
>
> 이 문서는 `docs/LIFELENS_SPEC_v1.1.md`의 인간/사회/세대 시뮬레이션 철학을 확장한다. 기존 요구사항을 대체하지 않고, **자급자족·자원·발견·제작·지식 전파·세대 누적·문명 발전**을 LifeLens의 핵심 제품 방향으로 추가한다.

---

## 1. 한 줄 정의

**LifeLens는 몇 명의 인간이 최소한의 조건에서 스스로 생존하고, 자연을 관찰하고, 자원을 채집하고, 시행착오를 통해 도구와 기술을 발견하며, 그 지식을 다른 사람과 다음 세대에 전파해 하나의 문명을 만들어 가는 과정을 플레이어가 관찰하는 자율 인류·사회·문명 시뮬레이션이다.**

플레이어는 기술을 직접 연구하거나 NPC에게 제작 명령을 내리는 존재가 아니다. 인간들이 스스로 필요를 느끼고, 시도하고, 배우고, 가르치고, 전문화하고, 사회를 발전시키는 과정을 관찰한다.

---

## 2. 가장 중요한 원칙 — 고정 Tech Tree가 아니다

LifeLens의 기술 발전은 다음과 같은 단순 해금 구조를 기본 모델로 사용하지 않는다.

`Stone Age 완료 → Bronze Age 해금 → Iron Age 해금`

대신 다음의 인과 흐름을 기본으로 한다.

`Need / Curiosity / Environment`
`→ Observation`
`→ Resource acquisition`
`→ Experiment / Attempt`
`→ Failure or Discovery`
`→ Personal Knowledge`
`→ Reproduction of technique`
`→ Teaching / Demonstration / Rumor / Imitation`
`→ Shared Knowledge / Culture`
`→ Better tools and production`
`→ New possible discoveries`

시대 이름(석기, 청동기, 철기 등)은 **세계가 실제로 획득한 재료·기법·생산능력을 보고 사후적으로 분류하는 관찰용 레이블**일 수는 있지만, NPC의 행동을 강제로 잠그고 여는 중심 로직이 되어서는 안 된다.

---

## 3. 기본 세계 시작 방향

기본 Civilization 시뮬레이션은 **완성된 현대 생활 인프라를 전제로 시작하지 않는다.**

NEW GAME의 초기 4명은 인간으로서의 기초적인 생존 능력과 감각/학습 능력은 가지지만, 고급 도구·산업·전기·현대 가전이 자동 제공되거나 모든 제작법이 선행 지식으로 열려 있지 않는다.

초기 세계의 핵심은:

- 자연 자원
- 식용/비식용 가능성이 있는 동식물과 재료
- 돌, 나무, 섬유, 흙, 물 등 기초 물질
- 간단한 은신처/환경 조건
- 개인별 기초 능력과 학습 성향
- 아직 발견되지 않은 많은 material affordance와 technique

이어야 한다.

예: 불은 월드에 이미 존재 가능한 자연 현상일 수 있지만, `불을 안정적으로 만들고 유지하고 활용하는 방법`은 개인 또는 사회가 아직 모를 수 있다.

---

## 4. 자원과 물질

자원은 단순 아이템 이름이 아니라 최소한 다음 속성을 가진다.

- Material / Resource type
- 양 / 질량 / 부피
- 상태: raw / processed / component / finished
- 물성: hardness, sharpness, brittleness, flexibility, combustibility, heat resistance 등
- 획득 난이도
- 가공 가능성
- 사용 가능한 affordance
- 보존/부패/소모 가능성
- 위치와 접근성

초기 예시:

- Stone
- Flint-like stone
- Wood / Branch
- Fiber
- Clay
- Water
- Edible plant / Fruit
- Bone
- Hide
- Copper-bearing ore
- Tin-bearing ore
- Iron-bearing ore

실제 데이터 모델은 특정 역사 재현에만 묶이지 않도록 data-driven으로 설계한다.

---

## 5. Gathering / Harvesting

캐릭터는 Needs, 환경, 기억, 지식, 도구 보유 여부에 따라 자원을 획득한다.

예:

`Hunger → 주변 식생 탐색 → 이전에 먹어본 식물 기억 → 채집 → 운반 → 먹기 / 저장`

`Tool need → 단단한 돌 탐색 → 채집 → 보관 → 가공 시도`

중요:

- 채집은 무한 생성이 아니다.
- 자원 노드는 수량/재생/계절/환경 영향을 받을 수 있다.
- 좋은 도구는 채집 효율과 획득 가능한 자원의 범위를 변화시킨다.
- 자원의 고갈은 이동, 저장, 교환, 농업, 전문화를 유발할 수 있다.

---

## 6. Inventory / Storage / Ownership

지식 발전 전에 실제 물질 흐름이 존재해야 한다.

지원 방향:

- Personal inventory
- Carried item/tool
- Ground item / world resource
- Household/shared storage
- Community storage
- Ownership / permission
- Quantity / durability / condition

초기에는 개인이 들고 다니거나 단순 더미에 저장하다가, 이후 바구니·용기·창고 등의 발견으로 저장 효율이 개선될 수 있다.

---

## 7. Crafting은 Recipe List 클릭이 아니다

캐릭터가 모든 Recipe를 처음부터 알고 있지 않는다.

제작 가능성은 최소한 다음 요소의 결합으로 결정한다.

- 개인이 알고 있는 technique / concept
- 재료에 대해 알고 있는 property
- 현재 보유 자원
- 필요한 tool / work surface / heat source
- 개인 Skill
- 과거 성공/실패 경험
- Personality: Curiosity, RiskTolerance, Patience 등

한 캐릭터가 특정 제작법을 발견해도 다른 캐릭터에게 자동 전역 해금되지 않는다.

---

## 8. Experiment / Discovery

새로운 지식은 필요 또는 호기심에서 발생한다.

예:

`Cutting need`
`→ 날카로운 무언가 필요`
`→ 돌끼리 충돌`
`→ 우연히 날카로운 파편 생성`
`→ 절삭 성공`
`→ "특정 돌은 깨뜨리면 날카로운 면이 생긴다" 지식 형성`
`→ 반복 성공`
`→ 의도적 타격 기술 발전`
`→ 더 정교한 석기`

또 다른 예:

`Cold / raw food / darkness`
`→ 자연 불 관찰 또는 마찰/충돌 실험`
`→ 불씨 발생`
`→ 연료 조건 학습`
`→ Fire-making knowledge`
`→ 난방 / 조리`
`→ 이후 도자기 / 금속가공 가능성 확대`

Discovery는 고정 순서가 아닐 수 있다. 단, 물리적 prerequisite는 지킨다.

---

## 9. Knowledge는 개인 상태다

다음은 서로 다르다.

- 세계에 어떤 사실이 존재한다.
- 한 캐릭터가 그것을 직접 발견했다.
- 다른 캐릭터가 그것을 목격했다.
- 누군가 설명을 들었다.
- 설명을 믿었다.
- 실제로 재현할 수 있을 만큼 숙련됐다.

따라서 기술 상태는 단순 `WorldTechUnlocked=true`가 아니다.

예시 계층:

- Unknown
- Observed
- Hypothesized
- Understood
- Reproducible
- Practiced
- Mastered

각 캐릭터마다 수준이 다를 수 있다.

기존 LifeLens의 `Memory`, `Belief`, `Witness/Rumor` 계층은 이 시스템의 기반으로 재사용한다.

---

## 10. Knowledge transmission / Culture

기술은 다음 경로로 퍼질 수 있다.

- 직접 목격
- 모방
- 대화/설명
- 시범
- 공동 작업
- 부모 → 자녀 교육
- 스승 → 학습자
- 사회적 소문/정보 전달

전달 과정에서 일부 내용이 누락되거나 잘못 전달될 수 있다.

사람들이 반복적으로 공유하고 재현하는 지식은 점차 개인 지식을 넘어 **가구/집단/문화의 지식**이 된다.

---

## 11. Tool / Technology progression 예시

다음은 강제 tech tree가 아니라 가능한 emergent chain의 예시다.

`Raw stone`
`→ sharp flake 발견`
`→ chipped stone technique`
`→ hafting / composite tool`
`→ grinding / polished stone`
`→ digging / cutting efficiency 증가`
`→ mining 가능성 확대`
`→ heat + ore observation`
`→ copper processing`
`→ alloy experimentation`
`→ bronze`
`→ furnace / higher temperature control`
`→ iron processing`
`→ improved blade/tool`
`→ specialized production`

장기적으로는:

- Agriculture
- Animal domestication
- Pottery
- Textile
- Construction
- Metallurgy
- Mechanical power
- Electricity
- Industrial production
- Modern technology

등으로 확장 가능하다.

어떤 세계가 정확히 같은 순서를 거칠 필요는 없다.

---

## 12. Specialization / Economy / Society

문명이 커지면 모든 사람이 모든 것을 직접 하지 않는다.

Skill, Personality, Knowledge, Reputation, Need, Resource availability에 따라 자연스럽게 역할이 갈릴 수 있다.

예:

- Gatherer
- Hunter
- Tool maker
- Builder
- Farmer
- Healer
- Teacher
- Metal worker
- Trader
- Leader

잉여 생산이 생기면:

`Specialization → Exchange → Ownership → Household economy → Community economy → Institutions`

으로 발전할 수 있다.

직업은 초기부터 현대식 직업 목록을 강제로 배정하기보다 실제 반복 행동과 사회적 역할에서 emergent하게 생길 수 있어야 한다.

---

## 13. Family / Generation과 문명 발전

기존 LifeLens 가족·출산·성장·노화·죽음 시스템은 문명 진행의 핵심이다.

1세대가 발견한 지식이 자동으로 다음 세대에게 들어가는 것이 아니라:

- 부모가 가르친다.
- 아이가 관찰한다.
- 공동체가 가르친다.
- 기록/교육 시스템이 이후 발명된다.

는 식으로 전달된다.

어떤 핵심 기술을 알고 있던 사람이 후계자에게 전하지 못하고 죽으면 사회가 해당 기술을 잃거나 품질이 퇴보할 수도 있다.

따라서 LifeLens의 역사에는 **발전뿐 아니라 지식 손실·재발견·문화적 분화**도 가능하다.

---

## 14. AI 의사결정에 추가되는 입력

기존:

`Needs + Emotion + Memory + Personality + Relationship + Environment + Schedule`

확장:

`+ Inventory + Available Resources + Known Material Properties + Personal Knowledge + Skill + Tool Availability + Storage + Community Knowledge + Production Need`

결과적으로 AI는 다음을 스스로 선택할 수 있다.

- 먹기
- 쉬기
- 채집하기
- 운반하기
- 저장하기
- 도구 사용하기
- 도구 제작하기
- 실험하기
- 건설하기
- 누군가에게 배우기
- 누군가에게 가르치기
- 교환하기

---

## 15. World Affordance 방향

현재 Unreal `ActivityAnchor / Smart Object` 실행 기반은 현대 가구 전용 시스템이 아니다.

향후 동일한 실행 계층이 다음을 포함하도록 일반화한다.

- Natural resource node
- Harvest point
- Fire / heat source
- Work surface
- Crafting station
- Storage
- Tool
- Building component
- Machine
- Furniture

Core는 **무엇을 할지** 결정하고, Unreal은 **어떤 실제 World affordance에서 어떻게 수행할지** 표현한다.

현재 개발용 Eat/Drink/Sleep/Toilet/Hygiene bootstrap anchor는 테스트를 위한 임시 실행 affordance이며, 정식 Civilization 시작 월드에 냉장고/현대 가구가 기본 제공된다는 제품 요구사항이 아니다.

---

## 16. Observer UI 방향

메인 Observer 화면은 여전히 단순해야 한다.

문명 시스템이 추가된다고 메인 HUD에 자원/기술 수십 개를 항상 표시하지 않는다.

Level 0:
- 세계와 인간 중심
- 중요한 발견/발명/고갈/건설 사건만 가볍게 알림

Resident detail:
- 개인 Inventory
- Known techniques
- Skills
- Current experiment / craft goal

World / Civilization detail:
- 알려진 주요 기술
- 핵심 자원 재고/부족
- 문화/집단별 지식 차이
- 역사적 주요 발견

예:
- `하린이 처음으로 불을 안정적으로 재현했습니다.`
- `민재가 날카로운 석편을 절삭 도구로 사용하기 시작했습니다.`
- `2세대 아이들이 석기 제작법을 부모에게서 배우고 있습니다.`

UI는 관찰/설명 역할만 하며 기술을 강제로 unlock하는 Controller가 아니다.

---

## 17. 구현 우선순위

문명 발전 전체를 한 번에 구현하지 않는다.

### Civilization Foundation v1
1. Resource / Material definitions
2. Resource node + finite quantity
3. Personal Inventory
4. Gather / Carry / Store primitives
5. Tool / item representation
6. Personal Knowledge state
7. Simple Experiment / Discovery result
8. Simple Crafting requiring resources + known technique
9. Memory/Belief/Witness integration hook
10. deterministic save/load support

### v2 이후
- teaching / imitation
- skill practice
- material-property inference
- multi-step production chains
- storage/building
- agriculture
- specialization/exchange
- culture/community knowledge
- metallurgy
- long-term civilization eras

---

## 18. Acceptance criteria

Civilization 방향의 첫 성공 기준은 다음이다.

> **캐릭터가 플레이어 지시 없이 필요에 따라 자연 자원을 찾아 채집하고, 개인 Inventory에 보관하며, 아직 모르는 간단한 도구 제작을 시행착오로 발견하고, 발견한 지식을 기억한 뒤 다른 캐릭터가 관찰/학습을 통해 그 기술을 습득할 수 있다.**

추가 기준:

- 다른 NEW GAME seed에서는 발견자/발견시점/진행 순서가 달라질 수 있다.
- 같은 seed에서는 결정론적 재현이 가능해야 한다.
- 모든 Recipe가 시작부터 알려져 있지 않아야 한다.
- 개인 지식과 세계 사실을 구분한다.
- 지식이 자동 전역 공유되지 않는다.
- 발전은 Needs/환경/성격/기억/재료 조건에 의해 emergent하게 일어난다.
- Save/Load 후 Inventory/Knowledge/Discovery 진행이 보존된다.

---

## 19. 제품 정의 갱신

기존 정의:

> NPC들이 살아가면서 스스로 이야기를 만들고, 플레이어는 그 이야기를 발견한다.

확장된 정의:

> **NPC들이 스스로 살아가고, 사랑하고, 가족을 이루는 동시에 자연에서 자원을 얻고, 실패하고, 발견하고, 만들고, 가르치며 세대에 걸쳐 하나의 문명과 역사를 만들어간다. 플레이어는 그 과정을 지시하는 것이 아니라 발견하고 관찰한다.**

이 방향은 LifeLens의 기존 Human-like AI / Relationship / Family / Generation / Observer 철학과 동등한 최상위 제품 요구사항이다.
