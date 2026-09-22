# LifeLens Web Observer — Implementation Map

Last updated: 2026-09-22

This document maps the current GitHub web source to the Web Observer master architecture. It describes development state only. It does not imply production deployment.

## Runtime authority

### src/runtime/core-types.ts

Canonical browser-side TypeScript contracts for residents, terrain, overview data, water categories, and the raw WASM client interface.

### src/runtime/core-bridge.ts

The only module that knows how the AppDeploy runtime payload is fetched and how the Emscripten LifeLensCore module is instantiated.

Responsibilities:
- fetch Core JS/WASM payload,
- create the Emscripten module,
- expose typed world overview, resident, terrain, world creation, and time-step methods,
- normalize malformed JSON to safe unavailable states.

Browser UI/render code should not call raw `worldOverviewJson()`, `residentsJson()`, or `terrainWindowJson()` directly.

### src/runtime/world-session.ts

Owns one active LifeLens world observation session.

Responsibilities:
- create/reset a world,
- advance Core time,
- track observer center chunk,
- resident group auto-follow,
- terrain query radius,
- retain the last valid terrain window through transient query gaps,
- combine Core overview/residents/terrain into one session snapshot.

### src/runtime/resident-continuity.ts

Owns short-lived presentation continuity for residents and last-known positions.

This module may retain authoritative values across transient incomplete refreshes, but it may not fabricate new residents or durable world state.

### src/runtime/simulation-clock.ts

Wall-clock accumulator and refresh scheduler.

Simulation advancement and render/UI refresh are deliberately separate. Browser timer throttling can catch up from elapsed wall time without coupling Core stepping to successful rendering.

### src/runtime/runtime-diagnostics.ts

Small in-memory development diagnostics for Core-query time, render time, resident/chunk counts, and failure counts.

## State/UI

### src/state/observer-store.ts

Typed external store for:
- world snapshot,
- residents,
- terrain,
- camera/observer state,
- runtime loading/error state.

This is the bridge between imperative runtime/render systems and React.

### src/state/use-observer-snapshot.ts

React `useSyncExternalStore` adapter.

### src/ui/observer-readout.tsx

React-rendered runtime badge, world overlay, observer metrics, and resident list.

These readouts previously lived as direct DOM mutations inside `observer-engine.ts`.

### src/ui/observer-format.ts

Pure display formatting helpers.

### src/App.tsx

React observer shell.

WorldSeed, world creation, time stepping, and chunk movement are now React events routed through `observer-actions.ts`. Read-only state comes from the typed observer store.

## Input

### src/input/camera-input.ts

Pointer drag, pinch zoom, and wheel zoom.

It owns gesture state and emits one camera state. The engine no longer implements pointer tracking itself.

## Current production-compatible renderer

### src/render/legacy-canvas-world-renderer.ts

Extracted transitional Canvas2D renderer.

It preserves the existing visual implementation while the project migrates to one Three.js world.

Current responsibilities:
- relief tile drawing,
- legacy water rendering,
- legacy vegetation markers,
- resident projection and separation,
- name labels,
- fallback resident markers,
- handoff of projected resident placements to CharacterLayer.

### src/character-layer.ts

Existing transparent Three.js skinned resident renderer.

Current zoom compatibility patch passes resident height derived from terrain tile scale rather than fixed 43/51 pixel sizing.

This layer is transitional and will eventually become a resident layer inside the unified WorldScene.

### src/render/world-projection.ts

Shared projection math extracted from the legacy renderer.

### src/render/terrain-presentation.ts

Deterministic presentation helpers for terrain color, shading, seeded visual hashes, and normalized coverage values.

## Target unified Three.js renderer

The following files form the development-only Three World path. It is connected behind a local render-mode switch but is not the default production-compatible renderer.

### src/render/world-scene.ts

One Three.js scene/camera foundation.

It composes continuous corner-height terrain meshes, WaterLayer, and instanced VegetationLayer.

### src/render/world-renderer.ts

Renderer lifecycle wrapper for WorldScene.

### src/render/water-layer.ts

World-space water mesh prototype.

### src/render/vegetation-layer.ts

Instanced vegetation prototype.

The target is to move terrain, water, vegetation, residents, structures, effects, and picking into this single scene and then delete the legacy Canvas2D renderer and transparent character overlay.

## Observer engine

### src/observer-engine.ts

The engine is now primarily an orchestration layer rather than the location of every implementation detail.

Current responsibilities:
- find the world/character canvases and control elements after React mount,
- create CharacterLayer, LegacyCanvasWorldRenderer, CameraInput, ResidentContinuity,
- connect LifeLensCoreBridge and WorldSession,
- publish session snapshots into observerStore,
- resize current canvases,
- bind temporary legacy buttons,
- coordinate the simulation clock.

The remaining engine DOM dependency is canvas acquisition after React mount. World controls/readouts no longer require engine DOM mutation.

## Validation

### tests/tests.json

User-visible regression contracts include Core world creation, resident continuity, automatic time progression, terrain navigation, mobile seed validation, and zoom-coupled resident scale/spacing.

### .github/workflows/web-typecheck.yml

TypeScript validation workflow only. It does not deploy.

Connector-originated commits have not produced a visible workflow run in the current session, and the local execution environment cannot resolve external npm/GitHub DNS. Therefore the latest refactor is structurally reviewed but still requires an actual `npm run typecheck`/CI pass before it is deployment-ready.

## Deployment boundary

GitHub `web/` is the development source of truth.

Nothing in this implementation map authorizes deployment. AppDeploy synchronization happens only after an explicit user request.
