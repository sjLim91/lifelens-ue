# PROJECT LIFELENS — WEB OBSERVER MASTER SPEC

Version: 1.0  
Baseline date: 2026-09-22  
Status: Active

## 1. Purpose

LifeLens Web Observer is a first-class observation client for the same LifeLens simulation, not a separate browser remake.

LifeLensCore remains authoritative for WorldSeed, simulation time, residents, GUIDs, needs, relationships, life events, terrain/ecology truth, and future family/society systems. The web client owns presentation, observer interaction, camera, interpolation, diagnostics, and efficient visualization.

The web path exists to provide fast iteration, phone/desktop access, easy visual verification, and a shorter feedback loop than packaged native builds.

## 2. Non-negotiable rules

- Core truth has priority over browser convenience. The browser may retain the last valid snapshot through transient errors but may not fabricate durable simulation truth.
- WorldSeed and compatible generation versions must remain deterministic.
- The viewport is observer-first. Detailed information belongs in selection/detail UI instead of covering the world.
- Terrain, water, vegetation, humans, resources, structures, weather, day/night, and later planetary scale must form one coherent spatial system.
- Human actions and animation must be driven by actual activity/state rather than unrelated random poses.
- Runtime dependencies and assets should remain free unless the user explicitly changes that constraint.
- Android web is a primary verification target; desktop receives more visual headroom, not a different simulation.
- GitHub is the development source of truth. Production deployment occurs only after an explicit user deployment request.

## 3. Canonical stack

UI: React 19 + TypeScript  
Bundler: Vite  
3D renderer: Three.js/WebGL2, with future capability-based WebGPU evaluation  
Simulation authority: LifeLensCore C++ compiled to WASM  
Runtime proxy: backend route serving pinned Core JS/WASM artifacts  
Assets: free-license assets pinned by immutable commit/version

React component state must not become a second simulation database.

## 4. Current implementation

The current browser prototype has four main pieces.

`App.tsx` renders the observer shell.  
`observer-engine.ts` loads WASM, advances simulation time, queries Core state, updates UI, projects terrain, and computes resident presentation positions.  
Canvas2D renders terrain/hydrology/vegetation markers, labels, and fallback resident markers.  
`character-layer.ts` renders skinned 3D residents using a transparent Three.js canvas.

This got a working observer on screen quickly, but it created a structural flaw: terrain and residents live in separate renderers with separate scale logic. The user-visible zoom bug came directly from that split.

The current P0 patch makes resident visual height and spacing follow terrain tile scale. This is a compatibility fix, not the final architecture.

## 5. Target runtime architecture

Target flow:

LifeLensCore WASM  
→ typed Core bridge / Web Worker  
→ immutable versioned WorldSnapshot  
→ render state + observer UI state  
→ one Three.js WorldScene  
→ React observer interface

### 5.1 Core bridge

Create one typed boundary around WASM. It owns world creation, time stepping, terrain queries, resident queries, events, save/load bindings, and error normalization.

Snapshots should carry at least simulation minute and monotonically increasing sequence/version information. Stale responses must never overwrite newer state.

### 5.2 Timing

Simulation stepping is independent from rendering.

Use wall-clock accumulation for the requested simulation rate. Three.js animation/interpolation runs separately. Heavy UI panels can update at a lower cadence. A failed renderer/query refresh must not stop the simulation clock.

Transient missing payloads retain the last valid presentation snapshot long enough to avoid blink/disappear/recreate behavior.

### 5.3 Worker isolation

After stabilization, move LifeLensCore WASM to a Web Worker. The Worker owns Core execution and publishes typed snapshots. The main thread owns React, input, camera, and rendering.

This removes simulation/terrain query stalls from pinch, scroll, animation, and UI interaction.

### 5.4 State architecture

Phase out `querySelector`, direct `innerHTML`, and UI mutation from `observer-engine.ts`.

Use a small typed observable store, preferably based on `useSyncExternalStore` or another low-dependency mechanism, with distinct channels for:

simulation snapshot, camera/observer state, selected resident/UI state, and runtime diagnostics.

React subscribes to UI state. Three.js consumes render snapshots. Neither one owns authoritative life simulation state.

## 6. Rendering architecture

### 6.1 One camera, one world coordinate system

Terrain, residents, water, vegetation, buildings/props, effects, and pickable objects must share a single Three.js coordinate system and one camera.

Resident size belongs to world units. Zoom changes the camera view, not an arbitrary fixed pixel resident height.

Nameplates are observer UI and may clamp text size for readability, but their anchor comes from the resident's projected world head position.

### 6.2 Terrain

Replace per-frame Canvas2D polygon painting with reusable chunk meshes.

Use BufferGeometry, geometry/material caches, chunk pooling, consistent edge vertices, and normals. Terrain truth comes from Core; presentation may smooth/interpolate geometry but may not alter authoritative biome/elevation/resource classifications.

### 6.3 Water

Streams, rivers, lakes, wetlands, coasts, and ocean become world-space geometry.

Use lightweight strips/curves for narrow flow and meshes for larger water surfaces. Mobile may simplify reflections/materials while preserving topology.

### 6.4 Vegetation and repeated props

Use InstancedMesh for trees, rocks, grass clusters, debris, and repeated props when possible.

Placement presentation should remain deterministic from Core ecology plus seed-derived presentation hashes. Density and LOD adapt to device capability and zoom.

### 6.5 Residents

Residents are persistent render actors keyed by stable GUID.

A transient incomplete resident payload does not destroy the actor. Authoritative positions become interpolation targets. Presentation state owns facing, animation blend, interpolation, LOD, and visual variant only.

Activity/state drives animation. Unsupported states fall back to neutral idle instead of an unrelated sitting/interaction pose.

### 6.6 Camera

Pinch, wheel, observer orbit, resident follow, event focus, and future cinematic modes all operate the same world camera.

No world renderer may maintain an independent zoom that can make terrain recede while humans stay fixed-size.

Future modes include observer orbit/isometric, resident follow, event focus, cinematic track, and eventually globe/planet transition.

## 7. Visual direction

The target is a living world, not a debug map.

Priority order is spatial correctness and camera stability, then terrain relief and water continuity, vegetation/prop density, human scale and contextual animation, lighting/day-night/weather/atmosphere, settlement/tools/traces, and finally cinematic polish.

Visual upgrades must not replace simulation truth with decorative randomness.

## 8. Human and society observation

The browser architecture must be able to observe the full LifeLens system, not only four avatars.

Expected domains include needs, emotion, personality, memory/belief, relationship dimensions, utility goals/actions, multi-turn conversations, conflict, rumor, factions, leadership competition, and noncompliance.

Life progression includes dating, partnership, marriage, cohabitation, pregnancy, birth, parenting, childhood, aging, death, inheritance, households, genealogy, and generational replacement.

The main viewport stays visually clean. Selecting a resident exposes deeper state.

## 9. Asset policy

Every external runtime asset must record source, license, immutable version/commit, intended use, and fallback behavior.

Avoid mutable `latest` asset dependencies. Before production hardening, permitted assets should move into a controlled project asset pipeline when licensing allows.

Prefer CC0, public domain, MIT, Apache-2.0, or other compatible free licenses. Attribution obligations must be documented.

## 10. Performance budgets

These are engineering targets, not guaranteed device claims.

Target responsive interaction at 30 FPS minimum on supported mobile hardware and 60 FPS where possible. Keep Core stepping off the render path. Reuse terrain meshes and resident actors. Use instancing for repeated environment objects. Cap DPR when necessary. Apply zoom/distance LOD to characters, vegetation, shadows, and effects.

Development diagnostics should track frame time, Core query time, snapshot size, resident count, visible chunk count, and draw calls.

## 11. Reliability invariants

Never clear residents because one terrain query fails.  
Never destroy a resident actor because one residents payload is incomplete.  
Never couple simulation clock progression to render success.  
Never let an older snapshot replace newer simulation state.  
Never silently replace unavailable Core truth with fake browser residents/world state.  
New World is the explicit reset boundary for resident presentation continuity.

## 12. Required regression coverage

Core WASM load and world creation.  
Automatic time progression without button interaction.  
Resident continuity through incomplete/transient payloads.  
Mobile and desktop zoom coherence between terrain and residents.  
Terrain navigation with Core biome/ecology truth.  
WorldSeed validation.  
Visible failure state when runtime loading fails.

Every user-visible fixed bug gets regression coverage before deployment.

## 13. Development and deployment workflow

Normal flow:

GitHub `web/` edit/refactor → test update → working-state document update → source review/compile validation → wait → explicit user deployment request → compare GitHub with live AppDeploy snapshot → sync reviewed changes → production QA.

AppDeploy production source must never be the only copy of a fix.

## 14. Roadmap

### P0 — Stabilize current observer

Canonical GitHub web source, continuous residents, independent clock, temporary zoom-coupled resident scale, mobile pinch stability, regression tests.

### P1 — Unified Three.js world

Move terrain, water, vegetation, and residents into one scene/camera and delete dual-render coordinate hacks.

### P2 — React/state cleanup

Typed observer stores, React-owned UI state, renderer modules, removal of direct DOM mutation.

### P3 — Worker/Core isolation

Run Core/WASM off-main-thread, version snapshots, interpolation buffers, robust lifecycle/error recovery.

### P4 — Earth-like environment

Continuous terrain, rivers/lakes/coasts/ocean, vegetation density, weather, day/night, wind, material variation, natural resources.

### P5 — Human presentation

Better free resident assets, LOD, contextual locomotion/interactions, tools/objects, grounded poses, improved body/appearance variation.

### P6 — Life and society observation

Relationships, family lifecycle, memory/belief, conversations, conflicts, households, factions, leadership, settlements and generational change.

### P7 — Scale and polish

Persistence, larger populations, streaming, profiling, cinematic observer modes, and future globe/space transition architecture.

## 15. Current stabilization definition of done

The current slice is not complete until time advances automatically, founders remain continuously represented, zoom never leaves characters visually fixed while terrain changes scale, pinch/wheel does not magnet residents to screen edges, mobile UI remains usable, Core remains the sole simulation authority, and reviewed source/documentation exist in GitHub before any production deployment.
