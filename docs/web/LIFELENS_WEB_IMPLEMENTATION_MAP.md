# LifeLens Web Observer — Implementation Map

Last reconciled: 2026-09-27 KST  
Canonical main baseline: `523dccd8d1f72d0607e70a94926d676a649fafba` (#465)

This document maps the current Web Observer source to the live architecture. It is an implementation map, not a visual-acceptance claim.

## 1. Authority and runtime boundary

### `src/runtime/core-types.ts`

Browser-side TypeScript contracts for authoritative LifeLensCore observations.

Current resident contracts include:
- identity and world position,
- needs/emotion/personality/relationships,
- current activity,
- authoritative presentation directive,
- action phase,
- physical/social/civilization/parenting intent,
- resident/object/grid targets,
- civilization material/facility action facts.

Terrain contracts include authoritative hydrology fields such as downstream coordinates, flow potential and drainage accumulation.

### `src/runtime/core-bridge.ts`

Typed WASM boundary.

Responsibilities:
- instantiate the pinned LifeLensCore Web runtime,
- create/reset worlds,
- advance time,
- read world/resident/terrain observations,
- normalize unavailable/malformed payloads safely.

React/render code does not own simulation truth.

### `src/runtime/world-session.ts`

Owns one observer session:
- world creation/reset,
- simulation stepping,
- observer center,
- resident group follow,
- terrain query window,
- last-valid snapshot continuity.

### `src/runtime/resident-continuity.ts`

Presentation cache only.

It may retain a recently authoritative resident/position through transient refresh gaps, but may not create residents or durable facts.

### `src/runtime/simulation-clock.ts`

Separates wall-clock simulation progression from render/UI cadence.

### `src/runtime/runtime-diagnostics.ts`

Development diagnostics for Core/render timing and observed counts.

## 2. State and observer UI

### `src/state/observer-store.ts`

Single external store for:
- world snapshot,
- terrain/residents,
- observer/camera state,
- selected resident,
- runtime status.

### `src/state/use-observer-snapshot.ts`

React adapter using `useSyncExternalStore`.

### `src/state/observation-feed.ts`

#460 factual event visibility.

Derives compact observation cues only from differences between consecutive authoritative Core snapshots. It does not create new simulation events.

### `src/state/human-traces.ts`

#463 Human Trace projection.

Consumes the authoritative Core Human Trace read model and prepares bounded observer-facing trace state.

### `src/ui/observer-readout.tsx`

Selected-resident/world information surface.

### `src/ui/observation-feed.tsx`

Compact event feed with resident/event focus affordances.

### `src/ui/human-traces.tsx`

Human Trace detail/readout surface.

### `src/App.tsx`

React shell for the observer client.

## 3. Input and camera

### `src/input/camera-input.ts`

Owns pointer pan/orbit, tap slop, pinch zoom and multi-touch transitions.

The #450 touch-direction/gesture fixes remain part of the current baseline.

### `src/render/world-scene.ts`

Owns the one active 3D world camera and composes terrain, water, ecology, atmosphere, residents and Human Trace presentation.

Camera repairs through #458 keep focus grounded to terrain and preserve streamed-origin continuity.

## 4. Unified Three.js world renderer

### `src/render/world-renderer.ts`

Renderer lifecycle and animation-frame wrapper.

### `src/render/terrain-geometry.ts`

Core-driven terrain mesh generation and deterministic presentation shading.

### `src/render/water-layer.ts`
### `src/render/water-geometry.ts`

Current water path.

Important invariant after #459:
- Core owns downstream hydrology.
- Web consumes `hasDownstream` / downstream coordinates.
- browser-side neighbor guessing is forbidden.
- uncertain channels are hidden rather than fabricated.

### `src/render/vegetation-layer.ts`
### `src/render/tree-asset-layer.ts`
### `src/render/ground-detail-layer.ts`

Presentation Layer v2 ecology/density.

Current tree variants use pinned zero-cost assets with recorded provenance. Ground micro-detail is deterministic and presentation-only.

### `src/render/weather-layer.ts`
### `src/render/atmosphere-layer.ts`

Weather/daylight presentation.

Snow uses soft screen-space flakes rather than default square points. Weather state remains Core-driven.

## 5. Resident presentation

### `src/render/resident-world-layer.ts`

Persistent resident actors keyed by resident GUID.

Responsibilities:
- load the real resident GLB,
- deterministic appearance variation,
- interpolate authoritative positions,
- smooth heading/gait,
- display action-context cue sprites,
- consume UAL1 + fail-soft UAL2 animation libraries,
- select only semantically permitted motion.

### `src/render/resident-action-context.ts`

Formats factual action wording from the authoritative resident presentation directive.

It may translate enum values and resolve known resident/object names. It may not infer intent from needs, memories or relationship values.

### `src/render/resident-semantic-motion.ts`

Current semantic-motion policy.

Motion ownership rules:
- moving resident -> locomotion path,
- Social / KnowledgeTeaching -> Talk only with a real nearby resident,
- Parenting Educate/Discipline -> Talk only with a real nearby resident,
- Eat/Drink -> UAL2 `Consume`,
- designated/real toilet use -> crouch,
- Sink Wash -> Interact,
- PlantFood Gather -> UAL2 `Farm_Harvest`,
- facility DeliverMaterial while moving -> UAL2 `Walk_Carry_Loop`,
- facility Work/Repair and Craft/Experiment -> reviewed kneeling work clip,
- missing clip or ambiguous semantics -> Idle/Walk fallback.

Explicit non-mappings:
- Sleep remains neutral until a reviewed lie-down/sleep/wake sequence exists.
- `TreeChopping_Loop` is not selected from `Wood` alone.
- `Sword_Attack` is not a chopping/hammering substitute.
- `PickUp_Table` is not a ground-gathering substitute.

### `src/render/resident-appearance.ts`

Deterministic identity-based appearance profile:
- height/body proportion range,
- upper/lower garments,
- footwear,
- skin,
- actual skinned scalp coloring,
- gait variation.

Current limitation: the Web runtime still relies on the existing base body model; appearance variation is not a full demographic body-topology system.

### `src/render/resident-world-coordinates.ts`

Maps authoritative resident grid positions into active world coordinates.

## 6. Human Trace

### Core
- `Source/LifeLensCore/include/lifelens/HumanTraceReadModel.h`
- Web bridge projection in `Source/LifeLensCore/src/WebClientBridge.cpp`

### Web
- `src/state/human-traces.ts`
- `src/render/human-trace-layer.ts`
- `src/ui/human-traces.tsx`

Current v1 traces are limited to facts that already exist durably in Core:
- resource depletion/use,
- environmental residue,
- started facility work/material progress.

The browser must not fabricate:
- worn paths,
- settlements,
- recurring-use places,
- activity centers.

Those require authoritative accumulated Core facts first.

## 7. Current observation-content milestone

Completed:
1. authoritative hydrology recovery (#459).
2. Event Visibility (#460).
3. Action Context (#462).
4. Human Trace v1 (#463).
5. conservative semantic motion v1 (#464).
6. UAL2 authoritative action mappings (#465).

Next:
1. motion tranche 2 — Sleep/Dig/Chop/Ground Gather/Carry variants only where both clip semantics and Core facts are adequate.
2. Social Cue.
3. Observer Director.
4. deeper Human Trace / lived-space accumulation.

## 8. Asset policy

Every external runtime asset must have:
- source,
- license,
- immutable version/commit,
- consumed file scope,
- intended use,
- fallback behavior.

Current motion libraries:
- Quaternius UAL1 Standard — CC0.
- Quaternius UAL2 Standard — CC0, Web fail-soft secondary library, pinned mirror commit recorded in `Content/Characters/Quaternius/PROVENANCE.md`.

Do not consume unrelated assets from a mirror repository merely because one permitted file is hosted there.

## 9. Validation

Key current gates include:
- Web Typecheck,
- Web Runtime Resilience,
- Web Runtime Release,
- Web WASM,
- LifeLens Core Tests,
- LifeLens Preflight,
- Unreal Linux Compile,
- Web Preview.

Current #465 baseline passed all of the above.

Automated success does **not** establish visual acceptance. Actual mobile/browser screenshots and interaction remain required.

## 10. Deployment boundary

Canonical Web source: GitHub repository.  
Canonical checkable preview: GitHub Pages.

Current preview pattern:

`https://sjlim91.github.io/lifelens-ue/?v=<main-short-sha>`

AppDeploy is not part of the LifeLens Web workflow.

## 11. Reliability invariants

- Never invent simulation truth in browser presentation.
- Never replace missing Core action semantics with a visually exciting but false motion.
- Never destroy a resident because of one incomplete refresh.
- Never couple simulation advancement to successful rendering.
- Never derive hydrology direction from browser adjacency when Core exposes downstream truth.
- Never treat CI success as runtime visual acceptance.
- A user-reported visual regression outranks planned polish work.

## 12. Current practical definition of done

The current Web observation slice is complete only when:
- important authoritative life changes can be discovered,
- resident action reason/target can be understood,
- major actions are visually distinguishable when an honest motion exists,
- human activity leaves authoritative inspectable traces,
- social change becomes observable without reading raw numeric state,
- important events can be followed without forced camera theft,
- the world remains visually stable on actual mobile devices.
