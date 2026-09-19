# LifeLens World Visual Environment v1

## 목적

이 문서는 LifeLens의 **배경/자연환경 시각 표현**을 위한 canonical presentation 기준이다.

배경은 단순 장식용 맵이 아니라 Observer가 살아 있는 세계를 읽을 수 있게 만드는 시각 레이어다. 다만 환경의 실제 시뮬레이션 상태와 자원/시설 존재 여부의 authority는 Core/World에 남는다.

행동 이후 발생하는 오염, 자원 감소, 불/그을음, 반복 보행, 건설/노후화 등 **authoritative environmental consequence의 시각 피드백 기준은 `docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md`를 따른다.**

## Authority boundary

- Core / `Source/LifeLens/World/**`가 실제 세계 상태, 환경 consequence, affordance, 자원/시설 존재 여부의 authority다.
- World Visual Environment는 이를 표현하는 **presentation layer**다.
- 배경 작업이 침대, 화장실, 집, 도로, 현대 시설 등을 몰래 생성해서는 안 된다.
- 자연물도 상호작용/채집 가능한 오브젝트라면 이후 Core resource/affordance state와 연결해야 한다.
- 연결 전의 자연물은 visual-only decor로 취급한다.
- Character/UI 쪽과 마찬가지로 presentation이 독자적인 simulation authority를 만들지 않는다.
- 공간적/물리적으로 의미 있는 authoritative environment change는 최소 하나의 visual representation path를 가져야 한다.
- visual은 Core state의 생성/강도/감쇠/제거/SaveLoad 복원과 동기화되어야 한다.

## 시작 월드 방향

초기 LifeLens 세계는 문명 인프라가 이미 완성된 공간이 아니다.

허용되는 기본 시각 요소:
- 자연 지형 / 흙 / 바위 / 풀 / 나무
- 하늘 / 태양 / 달 / 구름 / 안개 / 대기
- 자연 수역 또는 지형적 물 표현
- 원거리 실루엣 / 산 / 숲 / 자연 배경

초기부터 자동 배치하지 않는 것:
- 현대식 주택
- 침대 / 화장실 / 샤워 시설
- 도로 / 가로등 / 현대 도시 시설
- 문명 발전으로 획득해야 하는 구조물

## v1 목표

### 1. Ground / Terrain
- 기본 지형이 placeholder plane처럼 보이지 않게 한다.
- 주민 4명이 걷고 생활할 수 있는 충분한 평탄 구역과 자연스러운 고저차를 함께 둔다.
- Observer 카메라에서 지형이 캐릭터를 가리지 않도록 가독성을 우선한다.

### 2. Sky / Lighting / Atmosphere
- 시간대에 따라 읽히는 하늘과 조명 구조를 만든다.
- 낮/밤 표현은 authoritative simulation minute / weather provider에 연결한다.
- Windows PC는 `docs/CINEMATIC_RENDERING_STRATEGY_v1.md`의 Lumen/VSM/TSR/atmosphere cinematic tier를 사용한다.
- Android는 동일한 환경 truth를 mobile-safe light/shadow/fog/material path로 표현한다.
- Android 우선 검증이 Windows 시각 품질의 상한이 되어서는 안 된다.

### 3. Natural Dressing
- 나무 / 풀 / 바위 / 지면 디테일로 자연환경을 구성한다.
- 같은 에셋 반복이 눈에 띄지 않도록 scale/rotation/variant를 사용한다.
- Unreal PCG Framework를 기본 procedural placement tool로 사용할 수 있다.
- `FLLCoreNaturalChunkObservation.VisualSeed`와 각 resource patch의 `VisualSeed`를 deterministic PCG seed로 사용해 같은 WorldSeed/World state에서 장식 배치가 불필요하게 흔들리지 않게 한다.
- PCG가 생성한 위치/밀도는 visual decoration authority만 가진다. 채집 가능한 나무/광물/식량 등 gameplay resource 존재 여부는 반드시 Core/World read contract를 따른다.
- Windows에서는 고밀도/고품질 PCG 결과를 허용하고, Android에서는 partition/bake/LOD/HISM/density scaling으로 비용을 낮춘다.
- 초반에는 visual-only decoration과 authoritative resource node를 명확히 구분한다.
- project-authored baseline graph `/Game/Environment/PCG/PCG_LL_GroundCover`는 승인된 `weed_plant_02` CC0 mesh만 사용해 **decorative ground cover**를 배치한다. 이 graph는 CPU path, collision/navigation off, distance culling/density scaling을 기본값으로 하며 gameplay 자원을 생성하지 않는다.
- PCG graph asset authoring은 headless Unreal workflow로 재현 가능해야 하며, binary `.uasset`만 수동 편집한 상태를 canonical source로 두지 않는다.

### 4. Observer Readability
- 캐릭터가 배경에 묻히지 않도록 명암/밀도/높이를 조절한다.
- 선택 링, 이름 라벨, Level 0 Observer 정보가 배경 때문에 읽기 어려워지지 않아야 한다.
- 너무 조잡한 HUD 요소를 배경에 추가하지 않는다.

### 5. Water / Hydrology Presentation
- Unreal Water System은 강/호수/해안/바다의 **렌더링·메시·수면 표현 도구**로 사용한다.
- 물의 존재/종류/염도/흐름/가용성은 Core Hydrology가 authority다.
- Presentation이 보기 좋은 위치에 임의의 강/호수를 만들거나 Core 수계를 재추측하지 않는다.
- Water spline/body/mesh가 필요하면 authoritative hydrology read model을 projection하여 생성한다.
- Bridge의 `FLLCoreSurfaceWaterPresentationObservation`은 materialized water의 stable center/downstream target과 deterministic width/radius **rendering hint**를 제공한다. 이 값은 Unreal Water spline/body 제작용이며 gameplay 수량/깊이/유량의 새 authority가 아니다.
- Windows는 고품질 반사/수면/대기 상호작용을 사용할 수 있고 Android는 경량 water material/mesh/LOD path를 유지한다.
- 수영/음용/채집/홍수 같은 gameplay consequence는 Water plugin 자체가 아니라 Core/World 계약이 결정한다.

### 6. Mobile-first Performance
- Android가 첫 실제 검증 플랫폼이다.
- vegetation/rocks는 instancing/LOD 중심으로 구성한다.
- 머티리얼 수와 texture memory를 제한한다.
- 과도한 투명 foliage overdraw를 피한다.
- 4 residents + Observer + Core + Environment를 함께 실행하는 것을 baseline으로 본다.
- residue/흔적/VFX도 pooling, culling, aggregate representation을 우선하며 simulation record 수만큼 고비용 Actor를 생성하지 않는다.

## 환경 consequence visual rule

배경은 정적인 장식으로 끝나지 않는다.

기본 causal presentation:

`Action → authoritative environment state → visual change → resident perception → next action`

예:
- 야외 배변 → 지면 오염 흔적 / 강도 변화 / decay 시 약화.
- 자원 채취 → 나무/식생/자원 mesh 감소 또는 depleted state.
- 불 사용 → flame/light/smoke, 종료 뒤 필요하면 scorch consequence.
- 반복 보행 → grass flatten / dirt trail.
- 건설 → authoritative build progress에 따른 구조물 단계 변화.
- 시설 노후/파손 → 상태에 맞는 외관 변화.

세부 계약과 완료 기준은 `docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md`가 canonical이다.

## 에셋 정책

- 유료 runtime/API dependency 금지.
- 무료 사용 가능한 에셋만 사용한다.
- 외부 에셋은 정확한 pack/version/license/source provenance를 기록한다.
- publisher 이름만 보고 CC0라고 추정하지 않는다.
- 이후 asset 교체가 쉽도록 gameplay authority에 vendor asset id를 넣지 않는다.

## 다겸 소유 범위

기본 허용 경로:
- `Content/Environment/**`
- `Content/Maps/**`
- `Content/WorldPresentation/**`

다겸은 기본적으로 다음을 수정하지 않는다:
- `Source/LifeLensCore/**`
- `Source/LifeLens/World/**`
- authoritative Save/Load / Simulation state

위 영역 변경이 필요하면 `tasks/TEAM_BOARD.md`에 Integration Request를 남긴다.

## 우선순위

현재 캐릭터 Appearance를 끝낸 직후, 현재 보이는 `Idle 상태로 미끄러지는 이동`만 최소 locomotion으로 제거한 다음 **World Visual Environment v1을 높은 우선순위로 진행**한다.

권장 순서:

1. Character Appearance v1 PR #67 closeout / merge
2. Motion bootstrap — Core directive 기반 Idle / Walk / Jog 최소 연결
3. **World Visual Environment v1**
4. 나머지 Character Motion & Context — sit / lie / gaze / IK / interaction transitions
5. Observer UX Polish

이렇게 하면 배경 작업을 너무 뒤로 미루지 않으면서도 사람이 서서 미끄러지는 상태의 어색함을 먼저 제거할 수 있다.

## Unreal-native environment tools

Project plugins:
- `PCG` — procedural vegetation/rocks/ground-cover/biome dressing.
- `Water` — river/lake/ocean surface presentation driven by authoritative Hydrology.
- `Niagara` — weather/fire/smoke/ambient VFX where appropriate.

사용 원칙:
- 엔진 기능은 Presentation 생산성을 높이는 도구다. Core authority를 대신하지 않는다.
- Experimental PCG extensions/GPU features는 별도 검증 없이 production 필수 의존성으로 만들지 않는다.
- Runtime PCG가 비싸면 Android에서 반드시 runtime generation을 고집하지 않고 deterministic seed 결과를 bake/cache하거나 더 단순한 instancing path를 사용할 수 있다.

## v1 완료 기준

- placeholder 느낌이 아닌 자연 지형/하늘/조명/vegetation baseline
- 초기 월드에 현대식 인프라 자동 생성 없음
- 4명의 resident가 배경 속에서 명확히 보임
- selection ring / label 가독성 유지
- visual-only 자연물과 authoritative interactable world state 경계 명확
- Android 친화적인 LOD/instancing/material 구성
- Windows cinematic tier에서 동일 세계의 고품질 environment path 확인
- PCG/Water를 사용할 때 Core authority와 presentation projection 경계 확인
- 사용 외부 에셋 provenance 기록
- 실제 Unreal 실행/컴파일 검증

## 후속 확장

- authoritative time 기반 day/night
- weather presentation
- Environmental Residue 시각화 → `docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md`
- 물/토양 상태 표현
- 계절/기후
- 건축/정착지 성장 시각화
- Civilization progression에 따른 구조물 변화
- biome / procedural environment

후속 확장도 Core state를 읽어 표현하며 presentation 자체가 simulation 결과를 결정하지 않는다.


### Headless PCG authoring evidence

The project-authored `PCG_LL_GroundCover.uasset` was regenerated successfully in Unreal Engine 5.6 headless CI from `create_lifelens_groundcover_pcg.py`.

- generated graph size: ~48 KiB.
- source mesh: approved Poly Haven `SM_LL_weed_plant_02`.
- graph path: `/Game/Environment/PCG/PCG_LL_GroundCover`.
- authoring run completed after the required Unreal Water plugin collision profile was supplied.
- runtime use remains decorative-only until the consumer PR attaches the graph to materialized chunks.
