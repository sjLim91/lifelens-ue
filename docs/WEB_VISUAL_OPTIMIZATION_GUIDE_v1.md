# LifeLens Web Visual Optimization Guide v1

## 1. Purpose

This is the implementation guide for Web/PWA visual work in LifeLens.

It does not replace canonical product/world documents. It translates those
documents into practical Web rendering rules and budgets so future visual work
does not regress into prototype-looking geometry, duplicate Core authority, or
unbounded mobile cost.

Canonical priority remains:

1. docs/LIFELENS_SPEC_v1.1.md
2. docs/WORLD_ARCHITECTURE_v2.md and domain companions
3. docs/WEB_CLIENT_ARCHITECTURE_v1.md / docs/web/LIFELENS_WEB_MASTER_SPEC.md
4. this guide

## 2. Non-negotiable authority rules

- LifeLensCore owns terrain, hydrology, ecology coverage, resource existence and
  quantity, facilities, residues, residents, time and weather.
- Web may interpolate, cluster, shade, animate, LOD, pool and emphasize that
  truth, but must not invent simulation state.
- Initial spawn is a coordinate entry point, never a permanent living-zone
  authority.
- Resident selection must not force a camera jump. Automatic framing is allowed
  only for initial observer framing, explicit recenter/event-focus modes, and
  future opt-in follow/cinematic modes.
- Visual decoration and authoritative resource nodes remain distinct.
- Human-caused visual change must be derived from authoritative state or
  observed authoritative movement/actions.

## 3. Current source audit — 2026-09-23

### Strengths already present

- One Three.js world/camera for terrain, water, vegetation, residents and world
  consequences.
- Terrain/resident height share the same authoritative elevation sampler.
- Vegetation and repeated nature props use InstancedMesh.
- Resource depletion is projected to Web and now drives visible tree/shrub
  reduction and depletion scars.
- Facilities, sanitation, waste, construction progress, fire, smoke, weather,
  social/action presentation and observer details consume Core truth.
- Weather presentation consumes authoritative precipitation, wetness, wind and
  visibility.
- Camera pan/orbit/zoom are smoothed and bounded.

### Main visual quality bottlenecks

1. Ground middle-detail gap
   - terrain mesh -> large trees/shrubs/rocks has too large a visual jump.
   - missing near-ground grass clusters, pebbles, small stones, deadwood and
     litter makes the world read as a colored board.

2. Terrain material depth
   - current Web terrain is mostly vertex-color presentation.
   - it needs world-space micro-variation and slope/moisture/coverage response,
     but presentation must not alter authoritative terrain height.

3. Vegetation repetition
   - current instancing is efficient, but a small geometry vocabulary is still
     visible at local observer distance.
   - near/mid/far representation and biome-aware variants must be separated.

4. Consequence-layer scaling
   - small facility/resource counts are fine as individual meshes, but future
     populations must use batching/pooling for repeated debris, residue,
     pebbles, footprints and dust.

5. Camera contract drift
   - recent auto-centering work accidentally made resident selection eligible to
     move the camera. This conflicts with OBSERVER_CAMERA_CONTROL_v1 and must be
     corrected.

6. Asset quality boundary
   - Unreal already has approved photoreal Poly Haven assets, but Web has no
     controlled GLB/KTX2 nature asset library yet.
   - until a licensed Web asset pipeline exists, Web should use non-debug
     procedural/instanced fallback geometry and avoid obvious Cube/Cone hero
     placeholders. Photoreal asset import is a separate controlled task.

## 4. Visual layer architecture

### Layer A — Authoritative terrain surface

Inputs:
- elevation
- water kind
- biome
- moisture
- temperature
- forest/grass/shrub/rock/wetland coverage

Rules:
- no synthetic relief height that changes geography.
- geometry may interpolate authoritative samples.
- material/color/roughness micro-variation may use deterministic presentation
  noise because it does not alter topology or gameplay truth.

### Layer B — Near ground detail

Near-only, deterministic, instanced:
- short grass clumps
- sparse weeds
- pebbles
- small stones
- deadwood/twigs
- leaf/forest-floor clusters

Distribution must use:
- Core coverage
- biome/moisture
- sampled slope
- hydrology/water kind
- deterministic seed
- human/resource clearing masks

Never uniform white-noise carpet.

### Layer C — Mid vegetation / rocks

- trees
- shrubs
- medium rocks
- stumps
- resource-linked depletion visuals

Use clustered distribution and empty space.
Keep InstancedMesh/HISM-equivalent batching.

### Layer D — Far mass

At far zoom:
- suppress tiny ground instances.
- preserve forest mass, tree line, biome tone and major water/terrain shape.
- no hero-detail spam.

### Layer E — Persistent/observable human deltas

Examples:
- repeated travel wear
- vegetation clearing
- resource depletion
- excavation scars
- construction footprints/material piles
- sanitation/waste
- fire/scorch
- completed facilities

Only render what can be justified by Core state or authoritative observed action.

## 5. LOD and culling contract

### Near
- ground detail ON
- facility labels contextual
- individual vegetation visible
- footprints/wear visible
- full resident animation

### Mid
- reduce ground-detail density
- hide low-priority labels
- keep tree/shrub/rock masses
- aggregate minor traces

### Far
- ground detail OFF
- most labels OFF
- keep terrain, water, forest silhouette and major facilities/events only

Camera zoom controls presentation LOD only; it never changes simulation truth.

## 6. Web mobile budgets

Engineering targets, not device guarantees:

- DPR capped by existing camera/render contract.
- repeated natural props use InstancedMesh.
- avoid one Mesh/Actor per grass blade, pebble or residue record.
- near-ground detail should target <= 6 additional draw calls for the first
  implementation wave.
- pooled/capped transient effects:
  - footprints <= 120 visible marks
  - worn-ground cells <= 140 current compatibility cap
  - construction dust capped by active facilities
  - precipitation uses shared geometry buffers
- rebuild deterministic nature buffers only when terrain/presentation signature
  actually changes, not every render frame.
- shader animation for wind/water preferred over CPU per-instance transforms.

## 7. Ground-detail distribution rules

For each deterministic candidate:

Grass probability:
- increases with grassCoverage
- reduced by rockCoverage and wetland extremes
- suppressed around active facilities and depleted resource clearings
- reduced on steep sampled slopes

Pebble probability:
- increases with rockCoverage
- increases near Coast/River/Stream
- increases on moderate slope
- clustered, not uniform

Deadwood/litter probability:
- increases with forestCoverage
- moderate near forest edge
- reduced in open wetland/water

Wetland ground cover:
- derived from wetlandCoverage/moisture
- sparse taller clumps rather than standard dry grass

## 8. Terrain material rules

Web first wave:
- keep authoritative geometry.
- add deterministic world-space micro color/roughness variation.
- blend by coverage + moisture + slope.
- rain darkens/reduces roughness.
- snow brightens exposed ground.
- avoid visible chunk borders by sampling absolute/chunk-continuous coordinates.

Future asset wave:
- controlled licensed texture set with KTX2/Web-friendly compression.
- preserve Android/mobile fallback.

## 9. Tree / vegetation quality rules

- at least several silhouette families, not one repeated crown.
- vary height, width, crown asymmetry, rotation and maturity deterministically.
- tree scale must remain human-scale plausible.
- riparian vegetation follows hydrology/moisture.
- forest edge differs from forest interior.
- depleted Wood reduces trees and leaves stumps/clearings.
- renewable resource recovery visually regrows from Core quantity recovery.

## 10. Rock / pebble quality rules

- separate boulder / medium rock / pebble scale classes.
- partially bury medium/large rocks.
- cluster by slope/geology proxy/rock coverage.
- river/coast pebbles are flatter and denser than upland boulders.
- depleted mineral/clay nodes show exposed soil/pit/scar and fewer remaining
  resource pieces.

## 11. Weather rules

Rain:
- elongated streaks around active camera interest.
- wet terrain/facilities/vegetation.
- stronger water ripple energy.
- visibility/fog remains Core-driven.

Snow:
- larger drifting flakes.
- mild surface/vegetation/facility brightening.
- no fake gameplay snow-depth authority until Core exposes it.

Wind:
- GPU foliage sway.
- water motion energy.
- no CPU transform loop across every vegetation instance.

## 12. Camera/readability rules

- initial/recenter framing may use current residents + real facilities.
- selected resident does not move camera by itself.
- explicit follow/event-focus modes may move camera.
- activity framing uses actual terrain height for vertical centering.
- manual pan disables auto activity framing.
- manual zoom disables auto fit until explicit recenter.

## 13. Acceptance criteria

A visual slice is not complete from source/CI alone.

Required:
1. Typecheck/compile PASS.
2. Core authority tests PASS when bridge/contracts changed.
3. no visible debug primitive regressions.
4. no chunk-seam dominated ground color.
5. no uniform grass/rock carpet.
6. rain and snow clearly readable.
7. resource depletion visibly changes nature.
8. residents remain readable against environment.
9. manual camera control remains stable.
10. screenshot/device QA before calling the slice visually complete.

## 14. Implementation order

### VOPT-1 — Ground middle detail
- instanced grass / pebble / small rock / deadwood layer
- deterministic clustered distribution
- zoom LOD
- facility/resource clearing

### VOPT-2 — Terrain material depth
- slope/moisture/coverage microvariation
- wet/snow response
- chunk-continuous world-space shader/detail

### VOPT-3 — Vegetation silhouettes
- broaden tree/shrub families
- forest edge/interior/riparian logic
- maturity/regrowth presentation

### VOPT-4 — Rock/shore detail
- boulder/medium/pebble classes
- shoreline/river pebble clustering
- partial burial

### VOPT-5 — Human delta polish
- wear recovery when unused
- construction footprint staging
- extraction scars
- scorch/debris where authoritative data exists

### VOPT-6 — Asset wave
- build licensed Web GLB/KTX2 asset pipeline
- record provenance/version/fallback
- replace procedural local-view hero nature where approved assets exist

### VOPT-7 — Profiling / device QA
- draw calls
- frame time
- memory
- snapshot/update costs
- representative Android + desktop checks
