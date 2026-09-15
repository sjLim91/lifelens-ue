# LifeLens — World Genesis / Chunk / Migration v1

Status: **CANONICAL DESIGN v1**  
Date: 2026-09-15 KST  
Scope: initial natural world generation, scalable map topology, chunk lifecycle, persistent world history, population pressure, migration and multi-settlement expansion.

This document extends `docs/LIFELENS_SPEC_v1.1.md`, `docs/WORLD_AFFORDANCE_ENVIRONMENT_v1.md`, `docs/WORLD_VISUAL_ENVIRONMENT_v1.md`, `docs/CIVILIZATION_PROGRESSION_v1.md`, and `docs/STATE_MANAGEMENT.md`.

## Implementation status — 2026-09-15

- **WG-1 DONE** via PR #83, merge `f5c8cbab3aa41c6a37c3bae06838eeb583749771`.
- merged runtime contract includes `WorldSeed`, separate `PopulationSeed`, `WorldGenerationVersion`, stable `ChunkCoord`, negative-safe grid/chunk mapping, and order-independent untouched chunk baseline/substream seeds.
- validation: Preflight `34933128958` PASS, Core `34933128953` **46/46 PASS** + deterministic harness, Unreal Linux Compile `34933128950` PASS including UE 5.6 UHT/UBT/link.
- **WG-2 READY_NOW:** Macro World facts + viable initial start-site selector.
- WG-3+ detailed chunks/persistence/streaming/migration remain future implementation and must not be described as already running.

---

## 1. Product decision

LifeLens must **not** be built as a small fixed arena whose usable land stays constant while generations and population grow.

The canonical world model is:

`WorldSeed -> Macro World -> deterministic lazy chunks -> persistent human/environmental change -> carrying-capacity pressure -> migration -> additional settlements -> regional society/civilization`

The current flat/gray test stage is only a bootstrap presentation surface. It is not the final map architecture.

The player observes an expanding living world. Population growth should create pressure to intensify land use, explore, migrate, split households/groups, found new settlements, exchange, compete, merge, decline, and recolonize rather than merely crowd a fixed plate.

---

## 2. Core principles

1. **Large logical world, limited active rendering.**
   - The simulation may know about a much larger world than Unreal renders at once.
   - Only relevant/nearby chunks and actors need high-fidelity presentation.

2. **Seeded and deterministic natural baseline.**
   - A `WorldSeed` defines the initial natural geography and resource baseline.
   - A chunk's untouched natural state must be derivable from `WorldSeed + ChunkCoord`, independent of exploration order.

3. **History overrides generation.**
   - Procedural generation creates the initial natural baseline only.
   - Once humans or natural processes alter a generated region, persistent state becomes authoritative.
   - Reloading must never silently reroll a previously generated/modified region.

4. **No starting civilization infrastructure.**
   - NEW GAME starts with nature plus the initial residents.
   - No house, road, toilet, farm, storage shed, workshop, plumbing, or modern utility may be silently pre-spawned as simulation truth.
   - Terrain/natural water/vegetation/stone/wild food are environment; civilization facilities must emerge through the causal simulation loop.

5. **Simulation authority remains in Core.**
   - Core owns logical world/chunk state, resident positions, resources, environment changes, settlement facts and migration intent.
   - Unreal World/Presentation materializes the authoritative state and reports physical execution facts.

6. **Android-first scalability.**
   - World size must not imply rendering or simulating every 3D object at full fidelity.
   - Streaming, pooling, instancing, culling, HLOD/LOD and simulation/presentation distance tiers are required.

---

## 3. Seed model

### 3.1 Separate world and population randomness

Use separate deterministic seeds:

- `WorldSeed`
  - macro terrain
  - biome/climate baseline
  - water topology
  - natural resource distribution
  - deterministic chunk baseline

- `PopulationSeed`
  - initial four resident names
  - initial appearance/genetics/personality variation
  - other initial-population-only randomization

Default NEW GAME may generate both seeds fresh.

This preserves the existing product rule that the initial 2 male + 2 female residents receive fresh names/traits on each ordinary new game while still allowing a world to be replayed.

Optional future modes:
- **Same world, new people:** pin `WorldSeed`, regenerate `PopulationSeed`.
- **Exact replay/debug:** pin both seeds.

### 3.2 Order-independent chunk derivation

Untouched chunk generation must use a stable derivation such as:

`ChunkSeed = Hash(WorldSeed, ChunkX, ChunkY, GenerationVersion)`

Never consume one global random stream in exploration order. Otherwise walking east before west could change what exists in the west, breaking determinism and Save/Load reproducibility.

---

## 4. Macro World Genesis

At NEW GAME, create a lightweight macro representation before high-detail chunks are needed.

The macro layer may contain:
- elevation field / mountain and valley regions
- drainage / major rivers / lakes / coast where applicable
- temperature and moisture bands
- biome regions
- broad soil/fertility potential
- broad natural-resource provinces
- traversal difficulty / cliffs / major barriers
- broad hazard tendency
- candidate habitable basins

The macro layer is **data**, not thousands of rendered Actors.

A possible logical view:

```text
+--------------------------------------------------+
| mountain | forest | ridge   | unexplored        |
|----------+--------+---------+-------------------|
| river    | start  | meadow  | forest            |
|          | basin  |         |                   |
|----------+--------+---------+-------------------|
| plains   | lake   | hills   | future settlement |
+--------------------------------------------------+
```

The world may be implemented as a very large bounded region initially rather than claiming literal infinity. The architectural requirement is that normal demographic growth is not constrained by the current bootstrap stage bounds.

---

## 5. Initial start-site selection

The first four residents should not be placed at an arbitrary impossible coordinate.

World Genesis selects candidate start cells/chunks using viability scores such as:
- reachable fresh water
- wild food potential
- basic wood/stone/fiber availability
- traversable terrain / slope
- moderate environmental risk
- enough open space for early movement and experimentation

The target is **survivable but not pre-solved**.

The start-site selector must not fabricate shelter, sanitation, storage, agriculture, roads or tools simply to make survival easier.

Starting state:

`natural environment + 2 male residents + 2 female residents + zero civilization infrastructure`

Residents must solve missing affordances through the existing fallback/progression model.

---

## 6. Chunk model

### 6.1 Logical chunk

A chunk is a stable world region identified by coordinates, for example:

```text
ChunkCoord = (17, 22)
BaseBiome = TemperateForest
ElevationMean = 0.42
Moisture = 0.71
Fertility = 0.65
WaterAccess = true
```

Exact dimensions are an implementation choice to be profiled on Android. Do not freeze meter size in design until traversal/render/resource density tests exist.

### 6.2 Chunk lifecycle

Recommended states:

- `Unmaterialized`
  - macro facts known; detailed baseline not yet instantiated.
- `GeneratedNatural`
  - deterministic natural detail generated.
- `ActivePhysical`
  - loaded around camera/residents and presented in Unreal.
- `DormantPersistent`
  - not physically loaded, but persistent deltas/history exist.

A chunk can leave `ActivePhysical` without losing simulation history.

### 6.3 Generate on demand

Detailed chunk generation occurs when required by:
- exploration/migration route
- resident interaction range
- camera observation
- nearby simulation dependency

Do not generate the whole high-detail world at NEW GAME.

---

## 7. Base state plus persistent deltas

Each chunk conceptually consists of:

`DeterministicNaturalBaseline(WorldSeed, Coord) + PersistentWorldDelta`

Persistent delta examples:
- resource depletion/regrowth state
- felled/grown vegetation
- HumanWaste and other environmental residues
- dug sanitation pits / facilities
- structures
- storage and crafted objects
- excavation / terrain modification
- cultivated ground
- foot-traffic trail/path development
- fire/scorch/damage state
- abandoned settlement remains
- ownership/social/settlement facts where applicable

For untouched chunks, baseline regeneration from the seed is sufficient.
For visited or changed chunks, authoritative delta/state must be serialized.

A generated chunk must never reroll because it was unloaded from presentation.

---

## 8. Environmental time and history

Nature is not static scenery.

Long-lived world processes may include, incrementally:
- vegetation growth/death/regrowth
- renewable-resource regeneration
- depletion of finite resources
- weather/season influence
- erosion/water/soil effects later
- contamination accumulation and decay
- abandoned-area succession back toward nature

Human history changes the map:

```text
initial forest
-> repeated traffic
-> informal path
-> resource clearing
-> camp
-> designated sanitation area
-> dug pit
-> shelters/storage/work sites
-> cultivated land
-> settlement
-> possible abandonment / regrowth
```

Generated nature is therefore the **starting condition**, not the permanent final layout.

---

## 9. Population growth and carrying capacity

A settlement/region must have dynamic carrying capacity rather than a fixed hard resident count.

Pressure inputs can include:
- available food and water
- production and storage capacity
- shelter capacity
- sanitation burden / contamination
- accessible land
- local resource depletion
- travel cost
- congestion
- household crowding
- social conflict / safety
- technology and production efficiency

Conceptually:

`SettlementPressure = Demand(population) - EffectiveCapacity(environment + infrastructure + knowledge)`

Population growth does not directly enlarge terrain. It increases pressure on the current settlement and creates incentives for land intensification, exploration and migration.

Do not hard-code thresholds such as "population 30 => found village B" as a magic rule. Decisions should come from resident/household/group utility and real environmental constraints.

---

## 10. Migration and settlement expansion

Migration is a causal behavior, not a map teleport mechanic.

Possible causes:
- food/water shortage
- overcrowding
- resource depletion
- contamination or environmental hazard
- household formation
- relationship/family pull
- conflict or avoidance
- discovery of a better location
- work/trade opportunity later

Expected sequence:

`pressure/opportunity -> migration intent -> destination knowledge/search -> route/travel -> temporary presence/camp -> sustained use -> settlement emergence`

Long-term world shape may become:

```text
Settlement A ---- trail/route ---- Settlement B
     |                                  |
 households                         households
 production                         production
     |                                  |
     +---------- exchange --------------+
```

Later systems can support:
- inter-settlement marriage
- household relocation
- trade/exchange
- faction/group divergence
- conflict
- alliance
- settlement merge
- settlement abandonment

All of these must emerge from simulation state rather than a prewritten historical timeline.

---

## 11. Settlement identity

A settlement should eventually become a Core-owned emergent entity/fact, not merely a cluster of Unreal meshes.

Candidate derived conditions may involve:
- persistent population concentration
- repeated sleeping/food/storage/work locations
- shared facilities
- durable paths/structures
- household co-location
- sustained occupancy over time

Do not force a settlement label on the initial four residents immediately. The label should emerge when persistent spatial/social behavior justifies it.

Exact settlement-entity implementation is deferred, but the world architecture must leave room for it.

---

## 12. Simulation and presentation distance tiers

To scale to larger populations and Android, logical life must be separated from visual fidelity.

Suggested presentation tiers:

### Tier A — observed / near camera
- full resident Actor
- animation
- detailed interaction props
- local VFX/decals
- high-frequency presentation updates

### Tier B — nearby active region
- reduced detail/LOD
- simplified presentation updates
- instanced vegetation/props

### Tier C — distant simulated region
- no full resident 3D Actor required
- Core residents/households/world state continue logically
- events/state changes recorded without rendering every frame

### Tier D — untouched world
- macro data only
- detailed natural chunk generated deterministically when first required

Presentation unloading must never pause or erase authoritative history unless a deliberate future simulation-LOD policy says so.

---

## 13. Unreal implementation direction

Use Unreal-native systems where they fit, but do not let engine objects become simulation authority.

Likely presentation tools:
- World Partition / level streaming or an equivalent chunk streaming layer
- PCG/procedural generation for visual/natural instantiation where appropriate
- Hierarchical Instanced Static Meshes for vegetation/rocks/props
- HLOD / LOD / culling
- pooled dynamic environmental visuals
- Navigation data scoped/rebuilt for active relevant regions

The exact Unreal technology choice must be validated against UE 5.6 and Android constraints before being frozen.

Do not create one UObject/Actor for every tree in the logical world.

---

## 14. Core data direction

A future minimal contract may resemble:

```text
WorldGenesisState
- worldSeed
- generationVersion
- macroWorldDescriptor
- initialStartRegion

ChunkKey
- x
- y

ChunkState
- key
- generated
- biome/environment facts
- resource state
- persistent deltas
- settlement/facility references
- lastSimulatedMinute

MigrationIntent
- resident/household/group
- origin region
- destination candidate
- reason/utility
- status
```

Names are illustrative, not implementation commitments.

All cross-layer coordinates need one authoritative conversion contract. The existing Core Grid <-> Unreal world conversion is the starting precedent; chunk/world-region coordinates must not introduce competing position authorities.

---

## 15. Save / Load rules

Save state must preserve:
- `WorldSeed`
- generation/version compatibility info
- macro world facts that cannot safely be regenerated across generator-version changes, or an explicit migration strategy
- generated/visited chunk registry
- persistent chunk deltas
- resident authoritative positions
- facilities/resources/environment residues
- settlement/migration state when implemented

Key requirement:

**Loading a save must reproduce the same world history, not merely a similar regenerated landscape.**

Generator version changes require explicit compatibility/migration policy. Never silently apply a new generator to old modified chunks in a way that moves rivers/resources beneath existing settlements.

---

## 16. New Game behavior

Canonical NEW GAME flow:

1. Generate/accept `WorldSeed`.
2. Generate `PopulationSeed` independently.
3. Create macro natural world.
4. Rank viable start regions.
5. Select a survivable but unsolved start region.
6. Generate only the necessary initial detailed chunks.
7. Spawn 2 male + 2 female residents from `PopulationSeed`.
8. Spawn **no civilization infrastructure**.
9. Begin autonomous simulation.
10. Lazily generate new chunks as exploration/migration/camera requires.
11. Persist all meaningful modifications as history.

---

## 17. Relation to current sanitation progression

The sanitation work in PRs #66, #68, #75–#80 is the prototype for the larger world-history rule:

`missing facility -> fallback behavior -> residue -> exposure/memory -> problem recognition -> experiment/knowledge -> designated area -> persistent site -> dug-pit improvement`

When chunked world generation is implemented:
- HumanWaste residue belongs to its authoritative chunk/world coordinate.
- sanitation-site identity and GridPos survive streaming.
- unloading a chunk must not delete residue or pit progress.
- migration can reduce/increase sanitation pressure in different settlements.

This same pattern later applies to shelters, storage, paths, agriculture, workshops and other civilization-created affordances.

---

## 18. Implementation sequence

This document remains the architecture gate. WG-1 is now implemented; WG-2 and later runtime phases remain incomplete until separately merged and validated.

Recommended phases:

### WG-1 — deterministic world coordinates and chunk keys — **DONE #83**
- merged as `f5c8cbab3aa41c6a37c3bae06838eeb583749771`.
- chunk coordinate contract.
- stable deterministic derivation from `WorldSeed + ChunkCoord + GenerationVersion`.
- negative-coordinate floor semantics.
- PopulationSeed separated from natural-world identity.
- order-independence test and Core 46/46 validation.

### WG-2 — macro world + initial start-site selector — **READY_NOW**
- lightweight terrain/biome/water/resource potential
- start viability scoring
- no infrastructure spawn

### WG-3 — lazy natural chunk baseline
- deterministic local environment/resource generation
- generated-chunk registry
- Save/Load continuity

### WG-4 — persistent world deltas
- resource/vegetation/environment/facility modifications survive unload/load
- reuse existing Core environmental residue/facility authority

### WG-5 — Unreal streaming presentation
- active-chunk materialization
- vegetation/rocks/water presentation
- Android culling/LOD/instancing profiling

### WG-6 — carrying capacity and migration
- region/settlement pressure facts
- resident/household migration decisions
- travel and new-settlement emergence

### WG-7 — multi-settlement society
- routes/exchange/family migration
- settlement identity and history
- later politics/trade/conflict integration

Do not jump directly to WG-6/7 before world coordinates, persistence and deterministic chunk generation are stable.

---

## 19. Acceptance criteria

The architecture is acceptable when all of the following are true:

- ordinary population growth is not trapped by the bootstrap stage boundary.
- same `WorldSeed + GenerationVersion + ChunkCoord` yields the same untouched natural chunk regardless of discovery order.
- ordinary NEW GAME can still produce new initial resident names/traits through a separate PopulationSeed.
- initial game has natural resources but no silently prebuilt civilization infrastructure.
- unvisited regions need not exist as full rendered Actors.
- visited/modified regions never reroll when unloaded/reloaded.
- human modifications visibly and logically accumulate over generations.
- a growing population can create migration pressure and found additional persistent settlement areas.
- distant populations can remain logically alive without requiring all residents to render at full fidelity.
- Save/Load reproduces world history.
- Core remains authoritative; Unreal streaming/presentation cannot invent simulation state.
- design remains viable for Android-first profiling.

---

## 20. Priority / scheduling rule

This architecture must be respected **before LifeLens commits to a production-sized fixed map or permanent hand-authored settlement layout**.

It does not block small bootstrap/test maps used for current Core/Bridge/animation verification.

Before `World Visual Environment v1` evolves from a presentation prototype into the production world, WG-1/WG-2 boundaries should be implemented or explicitly accounted for so environment work does not lock the project into a small static arena.

---

## 21. Non-goals for v1

This document does not yet implement:
- infinite terrain
- exact chunk dimensions
- full climate simulation
- full hydrology/erosion
- animals/ecology
- disease/pathogens
- trade economy
- political borders
- war
- explicit settlement government
- procedural building architecture

Those systems may build on this world foundation later.

The immediate goal is to guarantee that LifeLens can begin with four people in nature and grow across generations without the world architecture becoming the limiting factor.
