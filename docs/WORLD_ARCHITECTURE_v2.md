# LifeLens World Architecture v2 — World-first Continuous Earth

> Status: **CANONICAL DESIGN / IMPLEMENTATION HOLD**
>
> Date: 2026-09-21 KST
>
> User decision: 설계는 확정하되 **사용자의 별도 시작 사인 전에는 구현에 들어가지 않는다.**
>
> Scope: Earth-scale world identity, coordinates, terrain, hydrology, biome/ecology, chunk streaming, visual LOD, human environmental change, observer-scale continuity, persistence and platform budgets.
>
> Truth order: **actual GitHub main / PR / Actions > this document > older world-generation / world-visual wording**.
>
> 이 문서는 기존 Core를 버리는 재작성안이 아니다. 이미 구현된 deterministic WorldSeed / Planet / Surface Region / Chunk / Local Surface / terrain observation / hydrology / materialization / Observer scale 계약을 최대한 보존하고, 현재의 "InitialChunk 중심 작은 테스트 월드" 표현을 Earth-scale streaming 구조로 교체하는 마이그레이션 설계다.

---

## 0. 최상위 제품 결정

LifeLens의 세계는 **캐릭터보다 먼저 존재한다.**

캐릭터 4명은 "시작 생활권"에 배치되는 것이 아니라, 이미 존재하는 지구의 한 지점에 등장한다. 그 위치가 숲인지, 초원인지, 계곡인지, 강가인지, 해안인지, 건조 지형인지는 WorldSeed와 지구 좌표가 결정한다.

정착지/생활권은 사전 생성되는 월드 속성이 아니다.

```text
Planet
  -> Natural Environment
  -> Characters appear at one surface location
  -> repeated activity
  -> temporary camp
  -> lived-in area
  -> settlement
  -> village / society / civilization
```

따라서 다음 원칙은 불변이다.

- 자연환경을 시작 캐릭터에게 맞춰 비우지 않는다.
- 시작점 때문에 강/호수/숲을 인위적으로 생성하지 않는다.
- 캐릭터가 존재하지 않아도 terrain / water / biome / resources는 동일해야 한다.
- 인간 활동이 시작된 뒤에만 길, 벌목, 평탄화, 시설, 오염, 자원 고갈 같은 human delta가 자연 위에 누적된다.
- 카메라가 이동해도 월드 끝이나 큰 빈 평면이 나타나지 않는다.

---

## 1. 기존 구현에서 반드시 보존할 것

다음은 v2에서 그대로 살린다.

### World identity / determinism

- `WorldSeed`.
- `PopulationSeed` 분리.
- `WorldGenerationVersion`.
- deterministic Planet identity.
- deterministic Surface Region identity.
- signed Chunk coordinates.
- untouched chunk baseline이 탐색 순서와 무관한 구조.
- 동일 Seed + 동일 좌표 -> 동일 자연 baseline.

### Hierarchy

```text
Planet
  -> Surface Region
    -> Chunk
      -> Local Surface
```

이 계층은 이미 방향이 맞다. v2에서는 실제 streaming / rendering / persistence가 이 계층을 끝까지 소비하도록 연결한다.

### Existing world data contracts

- terrain center/corner elevation observations.
- relief / moisture / temperature / fertility / traversal / biome inputs.
- explicit hydrology kinds:
  - Spring
  - Stream
  - River
  - Lake / area water
  - Coast
  - Ocean
- downstream relation.
- freshwater / marine distinction.
- natural obstacles.
- resource patches.
- materialized chunk concept.
- Core/World authority, Presentation read-only 원칙.

### Observer scale

```text
Local Surface
-> Regional
-> Planetary
-> Orbital
-> Interplanetary
```

enum/identity를 다시 만들지 않는다. v2 작업은 **실제 consumer wiring과 representation transition**에 집중한다.

### Platform separation

- World/Core truth는 Android / Windows / macOS에서 동일.
- 차이는 visual radius, mesh/texture/foliage/VFX/LOD budget뿐이다.
- 기존 platform cook boundary와 zero-cost asset policy를 유지한다.

---

## 2. 폐기 / 축소 / 역할 변경할 것

### 2.1 "InitialChunk = 생활권 중심" 해석 폐기

`InitialChunkX/Y`, `InitialCenterGridX/Y`는 **초기 좌표 anchor**로만 남긴다.

금지:
- 시작 chunk를 정착지로 간주.
- 시작 좌표를 기준으로 자연환경을 비우기.
- 시작 좌표를 기준으로 숲/지형을 artificial recovery 시키기.

### 2.2 시작점 전용 자연 평탄화 폐기

현재 의미의:

- `SettlementFlattenRadiusUU`
- `SettlementBlendBandUU`
- 시작점 기반 terrain flatten
- 시작점 기반 vegetation clear

는 v2 자연 baseline에서 제거한다.

향후 평탄화는 **실제 인간 행위**에 의해서만 발생한다.

예:
- 건물 foundation grading.
- 반복된 보행으로 생긴 path wear.
- 농경지 정리.
- 채석/굴착.
- 도로/시설 공사.

### 2.3 CachedSettlementReference 기반 생태 억제 폐기

`CachedSettlementReferenceUU` 같은 start-centred presentation suppression은 자연환경 생성에 사용하지 않는다.

Observer 가독성을 위해 나무가 카메라-주민 사이를 잠시 투명/축소하는 것은 **렌더링-only visibility policy**로만 허용한다. 생태계 밀도 자체를 바꾸지 않는다.

### 2.4 FarGround를 최종 월드로 사용하지 않음

거대한 단색/평면 `FarGround`는 최종 지구 continuity 수단이 아니다.

전환 기간에는 안전망으로 남길 수 있지만, streaming terrain이 안정되면:
- 플레이 화면에서 보이지 않게 하거나
- emergency fallback으로만 유지한다.

최종 시야 끝은 **regional terrain / biome silhouette**가 담당한다.

### 2.5 count 기반 materialized chunk 추론 폐기

`MaterializedChunkCount`로 주변 ring 좌표를 추정하지 않는다.

반드시 authoritative coordinate set을 사용한다.

### 2.6 Production primitive 노출 금지

Engine Cube / Plane / Cylinder / white wire-like proxy / giant gray surface는 production local view의 정상 표현으로 인정하지 않는다.

허용:
- editor/debug visualization.
- 명시적 diagnostic mode.

Production fallback은:
- 승인된 실제 asset,
- 플랫폼 경량 asset,
- 또는 숨김 + 오류 로그
중 하나를 사용한다.

---

## 3. v2 권한 구조

월드 관련 책임을 다음처럼 고정한다.

### Core / World Truth

소유:
- WorldSeed / generation version.
- planet / region / chunk identity.
- deterministic terrain field.
- climate / moisture / biome field.
- hydrology graph.
- natural obstacles/resources.
- untouched natural baseline.
- persistent human/environmental deltas.
- simulation-interest materialization.
- current world state.

금지:
- Presentation이 독자 terrain/water/biome truth를 생성.

### Unreal World Runtime

소유:
- active interest-set orchestration.
- Core world coordinate -> current Unreal local coordinate mapping.
- terrain collision proxy.
- nav-relevant surface proxy.
- streamed actor/component lifetime.
- platform runtime budgets.

### Presentation

소유:
- mesh/material/foliage/water/VFX representation.
- local/regional/planetary LOD.
- HISM/HLOD/impostor selection.
- observer occlusion/readability.
- platform quality tier.

금지:
- 자원 존재 여부, 수계 존재 여부, settlement 여부를 임의로 생성.

### Save

소유:
- WorldSeed / GenerationVersion.
- persistent entities.
- changed chunk deltas.
- human-made world changes.
- historical consequences.

Untouched world는 저장하지 않고 Seed + coordinate로 재생성한다.

---

## 4. 좌표와 스트리밍 — 핵심 최적화

### 4.1 Logical World Address

모든 위치는 장기적으로 다음 논리 주소를 가진다.

```text
PlanetId
SurfaceRegionCoord
ChunkCoord
LocalCoord
```

월드 진실은 Unreal floating transform이 아니라 이 주소에 묶인다.

### 4.2 Presentation Anchor

Unreal에는 현재 관찰/활성 구역 근처만 적당한 로컬 좌표로 투영한다.

즉:
- 논리 지구 좌표는 계속 증가/감소 가능.
- 화면상의 Unreal 좌표는 현재 anchor를 기준으로 유지.
- anchor 변경은 world identity를 바꾸지 않는다.

이 구조는 장거리 이동에서 정밀도 문제와 거대 absolute coordinate 의존을 줄인다.

### 4.3 두 종류의 Interest Set

Streaming은 **카메라 하나만 따라가면 안 된다.**

#### Simulation Interest

Core가 full-detail로 유지해야 하는 곳:
- 살아 있는 resident cluster.
- active facility/action area.
- 중요한 진행 중 사건.
- 지속 물리 상호작용이 필요한 곳.

#### Observer Interest

사용자가 보고 싶은 곳:
- free camera 주변.
- selected resident.
- event focus.
- regional/planetary focus.

Observer가 멀리 이동했다고 그곳의 모든 simulation을 full-detail로 승격시키지 않는다.

필요하면 **read-only deterministic preview**를 생성한다.

### 4.4 Runtime rings

정확한 chunk 개수는 플랫폼/프로파일링으로 조정하고 코드에 영구 하드코딩하지 않는다.

개념상:

- **R0 Physical Local**  
  resident movement / collision / interaction이 필요한 고해상도 영역.
- **R1 Near Visual**  
  full terrain + nearby vegetation + water + visible facilities.
- **R2 Regional Visual**  
  simplified terrain, forest mass, river/coast silhouette.
- **R3 Planetary representation**  
  continent/ocean/climate/major terrain fields.
- **R4 Orbital/Interplanetary**  
  celestial representation.

카메라 이동 시 ring이 재센터링되고 앞쪽 data가 준비되며 뒤쪽 presentation은 해제된다.

---

## 5. Terrain v2

### 5.1 하나의 surface truth

현재 "Core는 평면 이동 / Presentation만 relief" 상태는 장기적으로 폐기한다.

v2의 목표는 동일한 authoritative surface sample을:
- terrain render,
- collision,
- navigation,
- water placement,
- object placement
가 함께 사용하는 것이다.

Character/navigation 소비 연결은 character phase에서 수행하더라도 **월드 데이터 계약은 먼저 하나로 만든다.**

### 5.2 Surface sample contract

권장 개념:

```text
FLLSurfaceSample
- Height
- Normal
- Slope
- SurfaceType
- Moisture
- Soil/Substrate
- WaterDepth or WaterSurfaceRef
- Traversability
```

현재 center/corner elevation observation은 compatibility / coarse regional input으로 유지할 수 있다.

### 5.3 Multi-scale terrain field

지형은 chunk마다 독립 random shape를 만들지 않는다.

```text
continental / regional low-frequency field
+ ridge / basin field
+ medium hills & valleys
+ local detail / erosion approximation
+ persistent human delta
= Current Surface
```

동일한 absolute world coordinate를 sample하므로 chunk seam에서 지형이 끊기지 않는다.

### 5.4 자연 상태에서 보장할 시각적 결과

- 평원도 완전한 기하 평면이 아니다.
- 언덕/골짜기/능선이 여러 chunk에 걸쳐 이어진다.
- 산악지역은 더 큰 elevation range.
- 강은 저지대를 따라간다.
- 생활권이 생기기 전까지 중앙 평탄화 없음.

---

## 6. Hydrology v2

### 6.1 물은 terrain의 결과

순서:

```text
terrain elevation
-> precipitation/climate potential
-> flow direction
-> accumulation
-> drainage network
-> stream/river
-> lake/wetland
-> coast/ocean
```

정밀 유체 시뮬레이션은 필요 없지만 인과 관계는 유지한다.

### 6.2 Cross-chunk continuity

River/stream은 chunk decorative prop가 아니다.

필수:
- upstream/downstream continuity.
- shared border crossing.
- lake outlet.
- ocean/coast termination.
- deterministic ID.

### 6.3 Spawn에 물을 강제 생성하지 않음

캐릭터 옆에 물을 새로 만들지 않는다.

초기 spawn 정책은 별도다:
- 바다 한가운데 같은 불가능 좌표는 제외 가능.
- 완전히 비정상적인 slope/geometry는 제외 가능.
- 필요하면 broad survivability candidate를 선택 가능.
- 단, "화면 안에 강이 보여야 한다"는 이유로 geography를 조작하지 않는다.

물이 멀면 캐릭터가 찾아야 한다.

---

## 7. Biome / Ecology Presentation v2

### 7.1 개체부터 뿌리지 말고 coverage부터 정한다

기존의 단순:
`fertility * moisture -> TreeCount`
의존을 축소한다.

먼저 continuous coverage field를 만든다.

```text
climate
+ moisture
+ elevation
+ slope
+ soil/substrate
+ hydrology distance
-> biome / vegetation coverage
-> cluster structure
-> individual instances
```

### 7.2 표현 계층

#### Near
- individual tree.
- shrub.
- grass.
- rock.
- deadwood.
- local ground cover.

#### Mid
- canopy clusters.
- tree groups.
- forest edge.
- riparian vegetation.
- rock fields.

#### Far
- forest silhouette.
- ridge tree line.
- biome color/mass.
- no heavyweight individual hero tree spam.

### 7.3 Natural distribution

- 풀을 균일 white-noise로 도배하지 않는다.
- 빈 공간과 군락이 함께 존재.
- 수변 식생은 수계와 연동.
- 암석은 geology/slope와 연동.
- 산림은 여러 chunk에 걸친 mass로 보인다.
- desert/tundra/grassland 등에서는 나무가 적어도 그 이유가 biome으로 설명된다.

---

## 8. Human-caused world change

월드는 다음 합성으로 정의한다.

```text
NaturalBaseline(WorldSeed, Coord)
+ PersistentNaturalChange
+ PersistentHumanDelta
= CurrentWorldState
```

### HumanDelta examples

- path wear.
- vegetation clearing.
- tree cutting.
- crop field.
- foundation grading.
- excavation.
- road.
- bridge.
- irrigation.
- waste/pollution.
- fire damage.
- resource depletion.
- restoration/regrowth.

Settlement는 이 delta와 반복 행동의 결과로 **발견/분류**되는 것이지 자연환경 생성 input이 아니다.

---

## 9. Settlement 정의 변경

"게임 시작 시 존재하는 settlement center"를 폐기한다.

Settlement는 아래 신호의 누적 결과로 정의한다.

- repeated sleep.
- repeated food/water handling.
- storage concentration.
- recurring work.
- built facilities.
- path network.
- household presence.
- long-term occupancy.

초기에는 settlement ID가 없어도 된다.

필요 시 Core가 threshold 이후 emergent settlement entity를 생성한다.

이 결정은 기존 family/civilization 시스템을 버리는 것이 아니라 **그 시작 조건을 자연스럽게 바꾸는 것**이다.

---

## 10. Observer continuity

같은 세계를 해상도만 바꿔 본다.

### Local
- 주민 / 시설 / 나무 / 돌 / 하천.
- exact interaction.

### Regional
- 산맥 / 계곡 / 큰 숲 / 강 / 호수 / 여러 거주지.
- individual foliage는 축약.

### Planetary
- 구형 planet.
- 대륙 / 해양 / 큰 산계 / 기후·biome mass.

### Orbital
- atmosphere / planet / moons / artificial orbital entities.

### Interplanetary
- multiple celestial bodies.

중요:
- 서로 다른 가짜 맵으로 순간 교체하는 것이 아니라 같은 `PlanetId + world address`를 다른 representation으로 보여준다.
- Local/Regional 전환에서 visible geography가 위치적으로 대응해야 한다.

---

## 11. Presentation 구조 최적화

현재 `LLWorldPresentationActor.cpp`가 너무 많은 책임을 가진다.

v2 최종 목표:

```text
LLWorldPresentationCoordinator        (thin orchestrator)
  -> LLTerrainPresentation
  -> LLVegetationPresentation
  -> LLWaterPresentation
  -> LLFacilityPresentation
  -> LLRegionalPresentation
```

Streaming/lifetime은 별도 runtime subsystem이 소유한다.

```text
LLWorldStreamingSubsystem
  -> interest set
  -> chunk preview/materialization requests
  -> presentation load/unload
  -> platform budgets
```

기존 actor는 즉시 삭제하지 않는다.
기능을 한 번에 뜯지 않고 responsibility 단위로 이동시킨다.

---

## 12. Collision / Navigation 준비

현재 스크린샷에서 확인된 "바위를 통과함", "주민이 겹침"은 character phase에서 별도 수정하지만 World v2가 먼저 다음 기반을 제공해야 한다.

- visible obstacle XY/extent와 collision proxy가 같은 authoritative obstacle observation을 소비.
- terrain collision은 rendered terrain과 같은 surface sample 사용.
- 물/절벽/큰 암석이 traversability 데이터와 연결.
- Presentation mesh size가 Core obstacle size와 크게 다르면 collision envelope도 production scale로 보정.

Character phase에서는 이 world contract 위에:
- nav.
- avoidance.
- separation.
- destination reservation.
을 연결한다.

즉 지금 character 코드를 먼저 고쳐서 임시 평면에 맞추지 않는다.

---

## 13. Save / persistence 최적화

전체 지구 chunk를 저장하지 않는다.

### 항상 저장

- WorldSeed.
- GenerationVersion.
- Planet identity.
- resident/facility/entity identities.
- simulation time/history.

### 변경된 곳만 저장

- generated/materialized history where needed.
- resource depletion.
- built facilities.
- terrain grading.
- vegetation removal/regrowth state.
- paths/roads.
- pollution/fire/flood aftermath.
- other persistent deltas.

### 저장하지 않는 것

- untouched grass instances.
- untouched tree transforms that can be regenerated.
- untouched terrain meshes.
- visual LOD caches.
- observer-only preview cache.

이 방식으로 Earth-scale logical world와 save size를 분리한다.

---

## 14. Platform strategy

### Shared truth

Android / Windows / macOS 모두:
- same WorldSeed.
- same terrain/hydrology/biome identities.
- same resident/facility/resource truth.
- same save.

### Desktop

- wider near/mid visual radius.
- denser canopy.
- photoreal high-tier assets.
- higher terrain subdivision.
- stronger atmosphere/shadows.
- sparse hero assets.

### Android

- smaller near radius.
- aggressive HISM/HLOD.
- lower foliage density.
- simpler material/shadow.
- distant biome/forest impostor.
- no desktop-only heavyweight assets.

**Android 최적화를 위해 월드 자체를 작게 만들지 않는다.**
표현 budget만 줄인다.

---

## 15. Production visual acceptance

다음은 소스 존재/컴파일 PASS만으로 완료 처리하지 않는다.

World v2 local/regional visual acceptance 최소 조건:

1. 시작 지점 주변이 인위적인 원형/사각형 "꾸민 구역"으로 보이지 않는다.
2. 카메라를 어느 방향으로 이동해도 자연환경이 계속 이어진다.
3. chunk 경계에서 terrain seam이 보이지 않는다.
4. 적어도 terrain type에 따라 언덕/골짜기/능선이 육안으로 읽힌다.
5. forest biome에서는 근/중/원거리 canopy가 이어져 숲 mass가 보인다.
6. river가 chunk를 넘을 때 끊기지 않는다.
7. lake/coast/ocean이 terrain과 일치한다.
8. 큰 gray plane / white primitive / debug proxy가 일반 플레이 화면에 없다.
9. grass/rock가 균일하게 도배되지 않는다.
10. 같은 Seed/좌표로 돌아왔을 때 동일 natural baseline이 나온다.
11. 다른 Seed는 의미 있게 다른 geography를 만든다.
12. camera preview가 simulation authority를 임의로 변경하지 않는다.
13. 인간이 아무것도 하지 않은 곳은 reload 후 자연 baseline이 유지된다.
14. 인간이 변경한 곳은 reload 후 delta가 유지된다.

최종 닫기 전 실제 스크린샷/실기기 확인이 필수다.

---

## 16. 단계별 마이그레이션 — 최소 재작업 순서

### W2-0 — Contract / cleanup map
목표:
- existing contract inventory 고정.
- deprecated start-centred semantics 표시.
- materialized coordinate enumeration을 명시 contract로 승격.
- 구현 전 regression targets 정의.

코드 리스크: 낮음.

### W2-1 — Continuous world fields
목표:
- absolute coordinate terrain sample.
- climate/moisture/biome field.
- seam-safe surface query.
- current center/corner observations는 compatibility adapter로 유지.

코드 리스크: 중간.

### W2-2 — Hydrology graph
목표:
- deterministic drainage.
- cross-chunk stream/river.
- basin lake/wetland.
- coast/ocean relation.
- freshwater observation continuity.

코드 리스크: 중간~높음.

### W2-3 — Streaming / interest sets
목표:
- authoritative materialized coords.
- SimulationInterest와 ObserverInterest 분리.
- observer scroll preview.
- load/unload/revisit deterministic.

코드 리스크: 높음.

### W2-4 — Terrain & water presentation migration
목표:
- visible local hills/valleys.
- regional terrain continuity.
- FarGround dependence 축소.
- cross-chunk water rendering.
- no giant empty plane.

코드 리스크: 높음.

### W2-5 — Biome / vegetation migration
목표:
- coverage/cluster-driven vegetation.
- big tree + forest silhouette.
- near/mid/far vegetation LOD.
- primitive/placeholder cleanup.

코드 리스크: 중간.

### W2-6 — Persistent human world delta
목표:
- actual clearing/grading/path/building impact.
- emergent settlement detection.
- save/load delta.

코드 리스크: 중간~높음.

### W2-7 — Observer scale consumer wiring
목표:
- Local <-> Regional <-> Planetary representation continuity.
- camera/culling/input transition.
- same world-address focus.

코드 리스크: 중간~높음.

### W2-8 — Character/world physical integration
**World visual acceptance 뒤에 진행.**
목표:
- terrain nav.
- obstacle collision alignment.
- resident separation/avoidance.
- target reservation.
- no rock clipping / resident pile-up.

### W2-9 — Platform/native QA
목표:
- Android visual budgets.
- Windows/macOS quality/performance.
- save/load.
- long-run.
- camera scroll stress.
- crash/memory.

---

## 17. 무엇을 지금 당장 하지 않는가

v2 첫 구현에서 하지 않는다.

- 실제 지구 GIS 데이터 복제.
- full tectonic plate simulator.
- real-time Navier-Stokes water.
- 모든 chunk의 permanent Actor화.
- planet 전체 foliage instance 생성.
- 캐릭터 옆에 강/호수 강제 생성.
- 시작점 강제 settlement.
- Android 때문에 world truth 축소.
- visual screenshot 하나를 위해 Core truth와 다른 fake geography 생성.
- 캐릭터 코드를 world v2 전에 평면 전제에 맞춰 추가 보정.

---

## 18. 기존 문서와의 관계

이 문서는 월드 구조에 한해 다음 문서보다 우선한다.

- `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`
- `docs/WORLD_VISUAL_ENVIRONMENT_v1.md`
- `docs/EARTH_AND_HUMAN_FOUNDATION.md`의 world implementation wording
- `docs/LIFELENS_SPEC_v1.1.md`의 기존 small/local wording

그 문서의 인간/문명/요구사항은 폐기하지 않는다.

충돌 시:
**WORLD_ARCHITECTURE_v2의 world-first / continuous Earth / no-start-settlement / interest-set streaming 규칙이 우선**한다.

---

## 19. 기존 소스의 migration map

### Keep / adapt
- `FLLCoreWorldGenerationObservation`
- `FLLCoreNaturalChunkObservation`
- `FLLCoreTerrainPresentationObservation`
- `FLLCoreSurfaceWaterPresentationObservation`
- `LLWorldSpatialContract`
- `LLWaterPresentationActor`
- `LLDesktopTerrainPresentationActor`
- deterministic chunk seed/hash utilities.
- platform cook policy.

### Refactor
- `LLTerrainPresentationContract`
  - start-settlement flatten 제거.
  - single surface sample adapter로 전환.
- `LLWorldPresentationActor`
  - monolithic responsibility 축소.
  - terrain/vegetation/facility/regional 역할 분리.
- materialized chunk access
  - count/ring inference -> explicit coordinate set.
- far environment
  - ring decoration -> regional field-driven representation.

### Deprecate after replacement
- start-centred settlement ecology clearing.
- start-centred sightline ecology deletion.
- giant FarGround as normal horizon.
- production BasicShape structural proxies.
- flat gameplay terrain authority.

---

## 20. 성공 판정

World v2 foundation이 성공했다고 말할 수 있는 최소 기준:

> 카메라가 시작점에서 멀어져도 "작은 테스트 맵의 가장자리"가 나타나지 않고, 같은 지구의 terrain / water / biome이 계속 이어진다.

그리고:

> 캐릭터들이 아무것도 하기 전에는 자연이 먼저 존재하며, 시간이 지나 인간 행동이 그 자연 위에 흔적과 생활권을 만든다.

이 두 문장이 runtime에서 보이면 v2 방향이 제대로 구현된 것이다.

---

## 21. 실행 잠금

현재 상태는 **DESIGN COMPLETE / IMPLEMENTATION HOLD**.

사용자가 명시적으로 시작 사인을 주기 전에는:
- World v2 C++ 구현 시작 금지.
- Character audit 재개 금지.
- World v2를 이유로 대규모 asset 변경 금지.

허용:
- 문서 정리.
- 기존 CI 결과 확인.
- 설계 질의/수정.
- 사용자가 요청한 브리핑.

## 22. Canonical future hooks — 살아 있는 지구

World v2 foundation이 향후 재설계 없이 확장할 수 있도록 다음 hook을 canonical로 예약한다.

### Ecological succession / decay
- 벌목지의 regrowth.
- 화재/홍수 후 천이.
- 버려진 농경지의 자연 회복.
- 인간 활동이 줄어든 지역의 vegetation recovery.
- 시설/도로/폐허의 decay.

### Fauna Population LOD
- 종/생태군별 habitat suitability.
- regional population density.
- reproduction / mortality / migration.
- hunting/fishing/predation pressure.
- local observer/simulation interest에서만 individual actor materialization.
- 지구 전체 동물을 Actor로 유지하지 않는다.

### Regional Weather Cells
- 지구 전체 단일 날씨 금지.
- 지역별 temperature / humidity / precipitation / wind state.
- 이동하는 weather cell.
- elevation / coast / mountain 영향.
- Local presentation은 해당 region/weather cell을 소비.

### Persistent Historical Traces
- path / road.
- abandoned camp.
- ruin.
- grave / memorial place.
- old field.
- logged forest.
- drained/altered land.
- historic infrastructure.

World history는 UI event log에만 남지 않고 필요할 때 환경에도 흔적을 남긴다.

### Sensory environment
- sound/smell/smoke 등 spatial affordance를 후속 contract로 지원.
- perception/knowledge 연결은 `docs/HUMAN_REALISM_FOUNDATION_v1.md`가 canonical이다.

이 hook들은 첫 World v2 pass에서 모두 구현하는 요구사항이 아니다.
단, Core/world identity와 persistence 구조가 나중에 이 기능들을 막지 않도록 한다.

## 23. Implementation ownership update

2026-09-21 사용자 결정으로 World v2는 human ownership handoff를 최소화한다.

- Jjun이 Core provider부터 WorldPresentation/Environment integration까지 end-to-end로 진행 가능.
- 별도 Dagyeom owner 승인을 기다리는 구조는 사용하지 않는다.
- Dagyeom은 실제 화면을 보며 visual QA/polish를 돕거나 사용자가 명시적으로 넘긴 시각 작업을 수행할 수 있다.
- 단, Core/World truth vs Presentation consumer라는 소프트웨어 authority 분리는 그대로 유지한다.
- active same-file branch가 존재하면 충돌 조정 후 진행한다.
