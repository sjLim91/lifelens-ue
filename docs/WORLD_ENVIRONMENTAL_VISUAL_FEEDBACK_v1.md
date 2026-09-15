# LifeLens World Environmental Visual Feedback v1

## 목적

이 문서는 LifeLens에서 **주민의 행동과 세계 변화가 실제 환경 상태를 만들고, 그 결과가 Unreal 그래픽으로 다시 보이는 과정**의 canonical 기준이다.

LifeLens의 환경은 정적인 배경이 아니다.
주민이 먹고, 자고, 배변하고, 물을 사용하고, 자원을 채취하고, 불을 피우고, 길을 만들고, 시설을 짓고, 폐기물을 남기면 그 결과가 세계에 남고 다른 주민이 다시 그 환경을 경험해야 한다.

핵심 루프:

`Resident Action → Core/World Environmental Consequence → Saved Authoritative State → Unreal Visual Expression → Resident Perception/Exposure → Changed Decision/Behavior`

그래픽은 이 루프의 **표현 계층**이며 시뮬레이션 결과를 스스로 결정하지 않는다.

---

## 최우선 원칙

### 1. 환경 consequence는 Core/World authority다

- 오염량, 위치, 자원량, 시설 상태, 화재 상태, 토양/수질 상태 등 simulation 의미를 가진 값은 Core/authoritative World가 소유한다.
- Unreal presentation은 이 상태를 읽어 mesh / material / decal / particle / lighting / foliage 등의 형태로 표현한다.
- 그래픽 오브젝트의 존재 자체를 근거로 Core 상태를 추론하거나 생성하지 않는다.

예:

`Core: HumanWaste residue @ Grid(7,-4), intensity 0.72`
→ `Unreal: 해당 위치의 오염 흔적 + 범위/강도에 맞는 VFX`

반대 방향인

`Unreal decal이 있으므로 오염됨`

은 금지한다.

### 2. 물리적으로 의미 있는 환경 변화에는 시각화 경로가 있어야 한다

**공간적/물리적으로 세계에 영향을 주는 authoritative consequence는 최소 하나의 visual representation path를 가진다.**

단, 모든 내부 scalar를 그대로 HUD 숫자로 노출한다는 뜻은 아니다.
현상에 맞는 그래픽 표현을 사용한다.

- 오염 → 지면 흔적 / material variation / decal / 벌레·악취 암시 VFX
- 자원 감소 → mesh/instance 수량·크기·상태 감소
- 불 → flame / light / smoke / scorched ground
- 젖음 → wet material / puddle / darker ground
- 반복 보행 → grass flatten / dirt trail
- 시설 노후 → 손상 material / debris / 변형
- 건축 진행 → 단계별 construction state

### 3. Visual state는 authoritative state와 함께 생성·변화·소멸한다

- Core state 생성 → visual 생성.
- intensity/amount/radius 변화 → visual 강도/범위 변화.
- decay/cleanup/removal → visual도 감소/제거.
- Save/Load 후 authoritative state 복원 → 동일한 환경 visual 재구성.
- 화면에만 남는 ghost effect 또는 Core에는 남아 있는데 화면에서는 사라지는 split-brain을 허용하지 않는다.

### 4. Observer readability를 유지한다

환경 결과를 모두 표현하되 Observer 화면을 정보 쓰레기로 만들지 않는다.

- 멀리서: 큰 상태 변화만 자연스럽게 읽힘.
- 중간 거리: 오염/훼손/건설/자원변화가 구분됨.
- 가까이/선택 시: 세부 흔적과 강도 표현이 명확해짐.
- 이름 라벨, 선택 링, 주민 silhouette 가독성을 침해하지 않는다.
- simulation debug overlay와 실제 world visual은 분리한다.

### 5. Android-first budget을 지킨다

환경 consequence가 많아져도 residue마다 무거운 Actor를 무한 생성하지 않는다.

권장 방식:
- instancing / pooling
- decal/material parameter batching
- distance LOD
- far-distance aggregate representation
- Niagara/VFX 수 제한
- effect culling
- texture/material budget 제한

시뮬레이션 residue 수와 렌더링 객체 수는 반드시 1:1일 필요가 없다.
여러 Core record를 하나의 시각 cluster로 표현할 수 있지만, 표시 의미는 authoritative state와 일치해야 한다.

---

## 환경 변화별 기본 시각 계약

| Authoritative consequence | 기본 그래픽 표현 | 시간에 따른 변화 | 주민 행동과 재연결 |
|---|---|---|---|
| Human waste / sanitation residue | ground stain/decal, 오염 material, 필요 시 flies/odor hint VFX | intensity/radius decay와 함께 약화/제거 | 불쾌감, hygiene burden, 회피, 기억 |
| 일반 폐기물 / 쓰레기 | debris/cluster mesh | 누적/청소에 따라 증가·감소 | 회피, 청소 행동, 위생 문제 인식 |
| 물/토양 오염 | 물색/탁도, shoreline/soil material 변화 | 확산/희석/정화 상태 반영 | 음용 회피, 건강위험, 대체 수원 탐색 |
| Wet ground / rain effect | wetness parameter, puddle, darker terrain | 건조 시 감소 | 이동/쾌적성/경로 비용과 연결 가능 |
| 반복 보행 | flattened grass, dirt/worn trail | 사용 빈도 증가 시 강화, 장기 미사용 시 회복 가능 | 실제 선호 경로가 자연스럽게 보임 |
| vegetation/resource harvest | instance 제거/크기 변화/그루터기/빈 영역 | regeneration 시 복구 | 자원 부족 인지, 탐색 범위 변화 |
| mining/soil extraction | exposed soil/rock, pit/depletion visual | 채취량에 따라 단계 변화 | 채굴 위치/위험/자원 고갈 인지 |
| fire | flame/light/smoke | 연료/강도에 따라 변화, 종료 후 제거 | 난방/조리/위험 회피 |
| burnt/scorched area | char/scorch material, dead vegetation | 회복/정화에 따라 약화 | 해당 장소 기억/회피/재활용 |
| construction | foundation → primitive structure → completed facility | authoritative build progress와 동기화 | 새 affordance 사용 가능성 변화 |
| facility damage/aging | damaged material/mesh state/debris | 수리/노후 진행 반영 | 수리/대체/회피 판단 |
| civilization growth | 실제 구조물/길/작업장/저장소 변화 | 발견/건설/개선 단계와 동기화 | 새 생활 패턴과 이동/관계 변화 |

이 표는 v1 최소 계약이며 새로운 environmental consequence가 추가되면 **Core state + visual mapping + perception 영향**을 함께 정의한다.

---

## Environmental Residue v1 시각화 계약

현재 첫 구현 대상은 `HumanWaste` residue다.

### Core authoritative input

최소 입력:
- residue id
- kind
- GridPos
- amount
- intensity
- radiusTiles
- createdMinute / lastUpdatedMinute

### Unreal presentation

초기 단계에서는 다음 중 모바일 비용이 낮은 조합을 사용한다.

1. 지면 decal 또는 terrain/material overlay.
2. intensity에 따른 opacity/size 변화.
3. amount/radius 증가 시 범위 확대 또는 cluster 강화.
4. 가까운 거리에서만 저비용 secondary cue.
   - 작은 insect VFX
   - 미세한 색/표면 차이
   - 필요 시 선택/디버그 시에만 영향 범위 표시

### 금지

- 실제 화장실이 없는데 toilet mesh를 생성해 오염을 숨기는 것.
- residue가 decay했는데 visual만 영구적으로 남는 것.
- visual effect가 Core hygiene/health 값을 직접 수정하는 것.
- 모든 residue마다 고비용 Actor/Niagara를 무제한 생성하는 것.

---

## Resident Perception과의 연결

환경 visual은 장식이 아니라 같은 authoritative environment를 사람이 다시 경험하는 결과다.

다음 단계의 기본 흐름:

1. Core resident position과 환경 residue 위치를 비교한다.
2. exposure / proximity / intensity를 계산한다.
3. 주민에게 discomfort / hygiene burden / health risk input을 준다.
4. 반복 경험은 Memory/Belief/Problem Recognition 후보가 된다.
5. 행동 선택에서 오염 지역 회피 또는 청소/개선 필요가 발생한다.
6. 결과적으로 지정 배변장소, 구덩이, primitive latrine 같은 문명 발전의 원인이 된다.
7. 그 새 시설 역시 World Visual Environment에 실제 구조물로 나타난다.

즉:

`야외 배변 → 오염이 보임 → 주민이 겪음 → 피함/문제 인식 → 해결 시도 → 시설 발생 → 월드 외형 변화`

까지 하나의 causal loop로 유지한다.

---

## 시간 / 날씨 / 자연 상태와의 결합

향후 환경 시스템이 확장되면 visual feedback도 같은 contract를 사용한다.

### 비
- Core/World: 강수/젖음/세척/확산 결과.
- Unreal: rain VFX, wet material, puddle, residue 변화 표현.

### 물
- Core/World: 수량/오염/접근 가능성.
- Unreal: 수면 상태, 탁도, 주변 wetness.

### 토양
- Core/World: 오염/건조/채취/경작 상태.
- Unreal: terrain material/vegetation density 변화.

### 계절/기후
- Core/World: authoritative climate/season state.
- Unreal: foliage/material/lighting/atmosphere presentation.

날씨 VFX 자체가 simulation 결과를 결정하지 않는다.

---

## Civilization visual progression 계약

문명 시설은 arbitrary decoration으로 나타나지 않는다.

예시 sanitation progression:

`No facility`
→ `Repeated outdoor sanitation problem`
→ `designated area`
→ `pit / primitive latrine`
→ `improved latrine`
→ 이후 더 발전된 sanitation infrastructure

각 단계는:

- Core discovery/knowledge/resource/build state가 먼저 존재하고
- authoritative affordance가 생성된 뒤
- Unreal에 해당 구조물/공사 상태가 표현된다.

월드 아트 작업이 문명 단계를 선행해서는 안 된다.

---

## Save / Load 요구사항

- persistent environmental consequence는 Core snapshot/authoritative save state에 포함한다.
- Load 직후 Unreal visual layer는 저장된 state를 읽어 재구성한다.
- visual-only temporary FX는 저장하지 않아도 되지만, authoritative state에서 다시 생성 가능해야 한다.
- 같은 save를 load했을 때 residue/자원/시설의 의미 있는 위치·단계·강도가 동일해야 한다.

---

## 구현 ownership

### 쭌 / Core-World lane

기본 소유:
- authoritative environmental state
- consequence generation/decay/cleanup
- exposure/perception input
- Core Grid / World execution contracts
- Save/Load
- read-only Bridge DTO/API

### 다겸 / Presentation lane

기본 소유:
- `Content/Environment/**`
- `Content/WorldPresentation/**`
- environmental mesh/decal/material/VFX
- distance/readability/LOD tuning
- Observer-friendly visual presentation

Presentation에서 추가 authoritative 정보가 필요하면 Core 상태를 복제하지 말고 Integration Request를 통해 Bridge read API를 확장한다.

---

## v1 완료 기준

Environmental Visual Feedback v1은 최소 다음을 만족해야 한다.

- `HumanWaste` authoritative residue가 Unreal 월드 위치에 보인다.
- amount/intensity/radius 변화가 visual 강도/범위에 반영된다.
- decay/removal 후 visual도 감소/제거된다.
- Save/Load 후 같은 environmental state에서 visual이 재구성된다.
- residue visual이 주민/선택 링/라벨 가독성을 심하게 해치지 않는다.
- visual effect가 simulation authority를 갖지 않는다.
- Android baseline에서 pooling/LOD/culling을 사용하여 무제한 actor/VFX 증가를 막는다.
- automated compile + 실제 runtime smoke에서 상태/표현 불일치가 없는지 확인한다.

---

## 향후 우선 확장

1. HumanWaste visual feedback.
2. Environment Exposure → Discomfort/Hygiene/Avoidance/Memory.
3. sanitation problem recognition.
4. primitive sanitation facility progression + visual construction stages.
5. resource depletion/regrowth visual.
6. fire/smoke/scorch consequence.
7. repeated-foot-traffic trail formation.
8. water/soil contamination presentation.
9. weather-driven environment feedback.
10. settlement/civilization growth visualization.

---

## Canonical 관계

- 환경 simulation / affordance: `docs/WORLD_AFFORDANCE_ENVIRONMENT_v1.md`
- 배경/자연환경 presentation: `docs/WORLD_VISUAL_ENVIRONMENT_v1.md`
- **행동 결과의 환경 시각 피드백: 이 문서**

세 문서의 authority boundary는 동일하다:

**Core/World decides reality. Unreal presents reality. Residents perceive the same reality and act on it again.**
