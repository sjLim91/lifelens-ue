
# LifeLens Earth & Multi-Origin World Architecture v1

> Status: CANONICAL DESIGN CANDIDATE
>
> Date: 2026-10-05 KST
>
> Active product path: LifeLensCore -> WASM -> React/TypeScript/Three.js Web Observer
>
> Extends: docs/WORLD_ARCHITECTURE_v2.md, docs/EARTH_AND_HUMAN_FOUNDATION.md

## 0. Product decision

LifeLens must not remain a simulation that merely happens on a very large rectangular board.

The long-term product is:

~~~text
One Earth
-> multiple geographically separated human origin groups
-> independent survival / family / knowledge / culture / civilization histories
-> exploration / migration
-> indirect evidence of others
-> first contact
-> trade / conflict / marriage / disease / knowledge exchange
-> regional civilizations
-> continental / global civilization
-> planetary / orbital / interplanetary civilization
~~~

The observer does not direct the optimal route. The observer watches human history emerge on one shared planet.

## 1. Non-negotiable authority rules

- Core / World remains the sole simulation authority.
- Three.js/Web never fabricates terrain truth, resources, settlements, weather, contact, culture, or civilization state.
- Earth support must preserve existing Needs / Utility / ContextAction / Relationship / Emotion / Memory / Belief / Lifecycle / Household / Facility / Resource / Civilization / Trade contracts.
- Existing Chunk and LocalSurface concepts are retained. They become local tangent patches on Earth rather than the whole world.
- Camera position must never decide whether a population continues to exist or advance.
- Remote populations continue to simulate while the observer watches another region.
- No paid map, terrain, climate, or cloud API is required by the default runtime path.

## 2. Canonical world hierarchy

~~~text
Planet
└─ Global Surface
   └─ Surface Region
      └─ Chunk
         └─ Local Surface
~~~

Current rectangular local terrain is reinterpreted as one local patch of a spherical Earth.

Canonical observer scales:

~~~text
Resident
-> Settlement
-> Local Surface
-> Regional
-> Continental
-> Planetary
-> Orbital
-> Interplanetary
~~~

These are different representations of the same world, not unrelated maps.

## 3. Logical world address

Authoritative simulation positions are not raw Three.js globe coordinates.

~~~text
WorldAddress
- PlanetId
- SurfaceRegionId
- ChunkCoord
- LocalCoord
~~~

Observer/read models may additionally expose:

~~~text
GeoCoordinate
- Latitude
- Longitude
- Elevation
~~~

Resident movement, pathing, facility placement, resource interaction and collision should remain local-coordinate hot paths wherever possible.

## 4. Planet surface topology

Preferred direction: hierarchical cube-sphere or equivalent low-distortion spherical subdivision.

Goals:

- stable integer region identity
- deterministic subdivision
- no longitude singularity at the poles
- efficient hierarchical LOD
- clean globe rendering
- reusable model for Moon / Mars / future planets

Exact encoding is finalized in EARTH-0. Presentation mesh topology must not become world authority.

## 5. Local tangent frame

Each active Surface Region exposes:

~~~text
Region Origin
+ East
+ North
+ Up
~~~

Residents operate in metric local space. Presentation maps this space onto the globe when needed.

This preserves most existing:

- A*
- movement
- facility placement
- resource distance
- obstacle/collision logic
- ContextAction travel
- settlement geometry

## 6. Earth natural geography

Earth baseline means natural geography, not modern civilization.

Use as baseline:

- land / ocean
- coastline
- macro elevation
- bathymetry
- latitude
- large-scale terrain / basin constraints

Do not initialize:

- modern nations
- modern cities
- current roads
- present-day buildings
- modern farms
- modern industry
- current population
- current political identity

The intended world is recognizable Earth natural geography with civilization starting from zero.

A location near present-day Busan may inherit coast, mountains, watershed and latitude, but not Busan city.

## 7. Geography composition

~~~text
Real Earth macro geography
+ deterministic LifeLens regional refinement
+ deterministic local refinement
+ dynamic natural change
+ persistent human delta
= current world
~~~

The whole Earth is never stored at LocalSurface resolution.

Planetary representation uses coarse truth. Detailed geometry is materialized only where simulation requires it.

## 8. Earth data pipeline

Runtime must not require live map APIs.

~~~text
redistributable public Earth source data
-> offline preprocessing
-> versioned LifeLens Earth tile pack
-> Core geographic baseline
~~~

Persist version identities:

- EarthDataVersion
- WorldGenerationVersion
- HydrologyModelVersion
- ClimateModelVersion

A source-data update must not silently change old saves.

## 9. Determinism and RNG domains

Adding a distant human group must not reshuffle another group's unrelated history through a shared sequential RNG.

Use domain-separated deterministic randomness.

Examples:

~~~text
Geography      = hash(WorldSeed, RegionId)
Origin group   = hash(PopulationSeed, OriginId)
Resident       = hash(PopulationSeed, ResidentGuid)
Regional event = hash(WorldSeed, RegionId, EventKind, TimeWindow)
Weather        = hash(WorldSeed, RegionId, WeatherWindow)
~~~

Required regression:

Adding Origin B must not alter Origin A geography or causally-independent stochastic history before contact.

## 10. Global time, local solar time and season

LifeLens keeps one authoritative SimulationInstant.

Local environmental state derives from:

~~~text
SimulationInstant
+ longitude
+ latitude
+ axial tilt
+ orbital position
-> local solar time / sun state / season
~~~

Different origins may therefore experience day/night simultaneously at different local times.

Northern and southern hemisphere seasonal phase may differ.

## 11. Regional weather

Weather becomes region-local.

~~~text
Earth
├─ Region A weather
├─ Region B weather
└─ Region C weather
~~~

Needs, health, crops and movement consume actual resident-local weather. Camera location never changes weather truth.

## 12. Hydrology

Water remains authoritative simulation state.

~~~text
Elevation
+ precipitation potential
+ terrain drainage
-> flow direction
-> accumulation
-> watershed
-> stream
-> river
-> lake / wetland
-> coast / ocean
~~~

Spawn never fabricates nearby freshwater merely for founder convenience.

## 13. Ecology

~~~text
Latitude
+ temperature
+ moisture
+ elevation
+ hydrology
+ soil / substrate
-> biome
-> vegetation
-> animal ecology
~~~

Future ecology can add plant competition, herbivores, predators, fish, hunting, fishing and human ecological pressure without replacing the Earth address model.

## 14. EarthScenario and Multi-Origin population

New world configuration becomes scenario-driven.

~~~text
EarthScenario
- WorldSeed
- PopulationSeed
- EarthDataVersion
- OriginGroups[]
~~~

~~~text
OriginGroupSpec
- OriginId
- InitialPopulation
- SpawnLocation
- SpawnPolicy
- PopulationSeedDomain
~~~

Example:

~~~text
Origin A
4 founders
East Asian coast

Origin B
4 founders
continental interior
~~~

All residents live inside one Core world and one simulation clock.

## 15. Origin is not faction, nation or ethnicity

Origin means historical founder provenance only.

Separate concepts:

- OriginLineage: immutable founder provenance
- Household: family/economic unit
- Settlement: lived spatial cluster
- Community: social grouping
- Institution: organization
- Culture: emergent transmitted patterns
- Civilization: capability/social aggregation

Origins may mix. Settlements may contain many ancestries. Civilizations may merge, split, collapse or reform.

## 16. Initial human equality

Default origins begin from broadly equal human capability.

Do not hardcode geographic stereotypes or technology bonuses.

~~~text
Environment
-> repeated behavior
-> experience
-> knowledge
-> belief
-> practice
-> social transmission
-> culture
~~~

Geography creates different pressures; history creates different societies.

## 17. Spawn policy

Supported modes:

- Manual Geo
- Biased Random survivable-land candidate
- Harsh Random physically valid land

Validator may reject open ocean, impossible cliffs or invalid generated geometry.

It must not create free water, food or flat terrain.

## 18. Multiple simulation interest centers

This is mandatory for multi-origin Earth.

~~~text
Simulation Interest
├─ Origin A resident cluster
├─ Origin B resident cluster
├─ migrating household
└─ active expedition / trade journey

Observer Interest
└─ current camera / selected event
~~~

Watching A must not pause B.

## 19. Remote simulation and Simulation LOD

Eight founders may initially remain exact residents.

Later population scale needs deterministic LOD:

~~~text
L0 Exact Resident
L1 Settlement-detail population
L2 Regional coarse population
L3 Historical aggregate
~~~

Simulation LOD may depend on simulation relevance, never on whether the observer is looking.

Observer attention must not change history.

## 20. Global movement

Long-distance travel is not teleportation.

~~~text
regional route choice
-> local physical travel
-> region boundary transition
-> next regional segment
-> destination local travel
~~~

Routes consume terrain, known geography, freshwater, coast/ocean, mountains, carried supplies and available transport technology.

Residents cannot path through geography they do not know as if they had omniscient maps.

## 21. Geographic knowledge

Core truth and resident knowledge are distinct.

A river may exist while a resident does not know it exists.

Knowledge may come from:

- exploration
- teaching
- oral description
- maps/records
- migration
- trade
- landmarks

Known world becomes part of resident/community Knowledge.

## 22. Indirect contact

Groups can discover human evidence before direct contact.

Examples:

- trails
- tree cutting
- smoke
- farms
- abandoned facilities
- waste
- stored objects
- tools
- records
- burned sites

These are authoritative human traces and may become Memory/Belief evidence.

## 23. First Contact

FIRST_CONTACT is not a scripted date.

It occurs through real spatial/perception conditions.

Possible event model:

~~~text
FirstContactEvent
- participants
- location
- simulation minute
- prior indirect knowledge
~~~

Contact grants no automatic friendship, hostility, trade or knowledge sharing.

## 24. Post-contact outcomes

Potential results include:

- observe
- avoid
- approach
- help
- gift
- barter
- teaching
- migration
- intermarriage
- competition
- theft
- threats
- violence
- pathogen exposure

Selection remains grounded in existing Needs, Personality, Emotion, Memory, Belief, Relationship, Utility and Cognitive Agent systems.

## 25. Language and culture

Initial versions may start founders with a common proto-language.

Long-term isolation may create dialect/language divergence and contact may create learning, translation or hybridization.

Language is not hardcoded from geography.

Culture is not a single enum. It is transmitted social practice and belief, including examples such as storage norms, property/sharing, dwelling, food, burial, cooperation, education, outsider trust, mobility, celebration, taboo and recordkeeping.

Culture may split, merge, disappear or hybridize.

## 26. Disease between populations

Separated populations may develop different pathogen and immunity histories.

After contact, disease pressure follows actual exposure, pathogen load, immunity, sanitation and care.

First contact does not automatically generate an epidemic.

## 27. Genetics, ancestry and identity

OriginLineage may support mixed ancestry history.

Ancestry must never be treated as culture, citizenship, belief or personality.

Mixed descendants may belong to entirely new communities/cultures.

## 28. Globe Observer

Planetary view renders a real spherical Earth representation.

Possible read-only overlays:

- terrain / ocean
- mountain systems
- biome
- snow/ice
- weather/clouds
- settlements
- populations
- exploration traces
- migration
- trade routes
- major historical events

Presentation only consumes Core facts.

## 29. Globe -> Regional -> Local continuity

~~~text
Globe
-> Regional terrain
-> Local tangent surface
~~~

These representations must correspond spatially.

A coastline, mountain, river or settlement seen locally must map to the correct planetary location.

## 30. Multi-Origin observer comparison

Observer may compare societies even if residents themselves have no knowledge of each other.

The useful question is not just what differs, but why their histories diverged.

Compare candidates:

- population
- health
- food/water stability
- family structure
- known geography
- settlements
- knowledge/capability
- institutions
- activity budget
- environmental pressure

## 31. Global history

Potential global events:

- origin creation
- first settlement
- first birth
- settlement split
- migration
- settlement collapse
- first evidence of another group
- first contact
- first inter-community trade
- first conflict
- first mixed-origin child
- major technology diffusion
- epidemic
- regional collapse
- global network
- orbital launch

All arise from simulation truth, never era timers.

## 32. Human Delta

~~~text
Natural baseline
+ natural change
+ human delta
= current Earth surface
~~~

Human delta examples:

- logging
- trails
- farms
- buildings
- roads
- bridges
- mines
- excavation
- waste
- pollution
- irrigation
- fire
- urbanization

## 33. Persistence

Persist:

- WorldSeed
- PopulationSeed
- EarthDataVersion
- WorldGenerationVersion
- residents
- origin/ancestry provenance
- settlements
- facilities
- known geography
- changed/materialized region history when needed
- resource depletion
- human/environmental deltas
- relevant global history

Do not persist untouched terrain meshes, untouched vegetation transforms, globe rendering caches or observer-only LOD caches.

Untouched nature is regenerated from versioned geography + seed + coordinate.

## 34. Web architecture

Long-term presentation separation:

~~~text
EarthGlobeRenderer
RegionalWorldRenderer
LocalWorldRenderer
~~~

All consume Core read models. Three.js remains presentation only.

## 35. Earth data packaging

Large source geography assets are preprocessing inputs, not direct browser runtime datasets.

~~~text
raw Earth data
-> preprocessing
-> LOD pyramid / compact LifeLens tiles
-> packaged/self-hosted static data
~~~

Global view reads coarse data. Active simulation regions load required regional data. Local detail is deterministic refinement.

## 36. C7 integration

C7 Population / Settlement Maturation consumes this foundation.

Instead of only:

~~~text
4 founders -> one mature village
~~~

support:

~~~text
Origin A: 4 -> families -> settlement -> expansion/migration
Origin B: 4 -> families -> settlement -> expansion/migration
~~~

C7 authority remains Household / Settlement / Population pressure, not Origin identity.

## 37. Cognitive Agent integration

Future bounded Core cognition context may include:

- known geography
- known foreign residents
- known communities
- contact memories
- travel experience
- beliefs about outsiders
- scarcity and migration pressure

Cognitive proposals remain non-authoritative until accepted-event persistence/replay and Core validation rules allow behavior influence.

## 38. Performance architecture

Two origins do not mean materializing all geography between them.

~~~text
Origin A active simulation island

[large untouched deterministic macro world]

Origin B active simulation island
~~~

The untouched middle exists as coarse seeded geographic truth until exploration/materialization reaches it.

## 39. Implementation packages

### EARTH-0 — Contract Foundation

Before large C7 expansion:

- Earth identity
- EarthDataVersion
- stable SurfaceRegion addressing
- global/local coordinate adapter
- deterministic RNG domain separation
- multiple simulation-interest contract
- compatibility regression tests

No gameplay rebalance.

### EARTH-1 — Real Geography Baseline

- land/ocean
- macro elevation
- bathymetry
- coastline
- Earth tile preprocessing
- region geographic read model

Minimize changes to current local simulation.

### EARTH-2 — Globe Observer

- spherical Earth
- planetary camera
- region selection
- Globe -> Regional -> Local mapping
- accurate marker for active local simulation

This is the first stage where LifeLens visibly stops feeling like a rectangular board.

### EARTH-3 — Multi-Origin Foundation

- EarthScenario
- OriginGroupSpec
- 2 x 4 founders
- multiple active resident clusters
- observer-independent simulation
- deterministic origin-isolation tests

Recommended before full C7 expansion.

### EARTH-4 — Local Geography Conditioning

- macro Earth terrain -> local terrain
- regional climate
- local hydrology
- biome
- soil/geology

Different Earth positions must produce meaningfully different environmental pressures.

### EARTH-5 — Earth Time / Climate

- longitude-based local time
- latitude solar angle
- hemisphere seasonality
- regional weather
- resident-local Needs / crop / health pressure

### EARTH-6 — Global Exploration / Migration

- region knowledge
- global/regional route
- region transition
- exploration corridor materialization
- group/household migration

### EARTH-7 — Contact

- indirect evidence
- foreign resident detection
- FirstContactEvent
- cross-community relationships
- exchange/conflict foundations

### EARTH-8 — Culture / Language Divergence

- cultural transmission
- cultural drift
- language drift
- mutual intelligibility
- hybridization / divergence

### EARTH-9 — Global Civilization Observer

- settlements
- population
- migration
- trade
- technology diffusion
- culture
- world-history timeline

### EARTH-10 — Scale

- Core Web Worker
- deterministic remote Simulation LOD
- population LOD
- region LOD
- snapshot delta compaction
- 100 / 300 / 1000+ resident validation

### EARTH-11 — Orbital Bridge

After civilization has the actual capabilities:

~~~text
Planetary
-> Orbital
-> Moon / other bodies
-> Interplanetary
~~~

Reuse Planet/Region architecture for other celestial bodies.

## 40. Recommended execution order from current project state

~~~text
#642 C6 trade persistence closeout
-> current-main long-run balance/causal audit
-> EARTH-0
-> EARTH-1
-> EARTH-2
-> EARTH-3
-> C7 Population / Settlement Maturation
-> EARTH-4~7 in coordinated slices
-> cognition persistence/behavior influence
-> global civilization expansion
~~~

Do not wait for all Earth work before C7.

Pre-C7 priority is structural: Earth addressing, determinism, globe mapping and multiple active simulation centers.

## 41. Critical regression contracts

### Geography invariance
Same WorldSeed + EarthDataVersion + coordinate -> same untouched natural baseline.

### Camera invariance
Watching Origin A or Origin B -> same Core history.

### Origin isolation
Adding a distant B -> no unrelated RNG-driven mutation of A before causal contact.

### Observer isolation
Globe pan/zoom/region visits -> no Core outcome changes.

### Save continuity
Multi-origin save/load -> deterministic continuation.

### Coordinate roundtrip
Local -> Earth -> Local -> within defined tolerance.

### Region crossing
Long-distance travel crosses region boundaries without teleportation.

### Contact truth
Separated populations do not magically know each other.

### Earth locality
Distant regions may have different local time, season and weather.

## 42. Earth Foundation Definition of Done

Earth foundation is not complete until:

1. zooming out reveals an actual sphere representation.
2. the active local simulation maps to a valid Earth location.
3. Local Surface has no conceptual rectangular world edge.
4. at least two origins can live simultaneously in one Earth simulation.
5. observing one origin does not freeze another.
6. separated origins do not automatically know one another.
7. natural conditions differ meaningfully by geographic location.
8. local time/weather can differ by location.
9. groups can physically expand and migrate across regions.
10. contact can arise from real movement/perception.
11. contact feeds existing social/cognitive systems.
12. natural baseline remains independent of observer/population placement.
13. current C1~C6 authority and determinism remain intact.

## 43. Final product picture

~~~text
                       EARTH

          Origin A
          4 humans


                                       Origin B
                                       4 humans
~~~

They begin with no knowledge of one another.

Geography pressures their lives differently. Families, settlements, knowledge and practices emerge independently. Exploration expands their known worlds. They may discover traces before people. They may eventually meet.

What follows is not predetermined.

They may cooperate, trade, intermarry, compete, fight, merge, separate, exchange disease and knowledge, collapse, recover, or remain independent for centuries.

Much later, the observer may pull away from the same Earth and see orbital infrastructure surrounding a planet whose history began with those small founder groups.

That is the canonical long-term Earth/Multi-Origin direction for LifeLens.
