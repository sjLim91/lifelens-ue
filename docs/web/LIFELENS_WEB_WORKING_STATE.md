# LifeLens Web Observer — Working State

Last updated: 2026-09-22

## Production baseline

The current production AppDeploy preview runs React 19 + TypeScript, Canvas2D terrain, Three.js residents, and LifeLensCore WASM delivered through a backend proxy.

Production fixes already applied before source/deployment separation include resident continuity caching, last-known resident positions, transient terrain retention, independent simulation and refresh timers, elapsed wall-time catch-up, and protection against resume-frame resident flicker.

## Canonical development source

The repository now contains `web/` as the canonical browser development tree.

Production AppDeploy is no longer treated as the only source copy. New work is committed to GitHub first.

A GitHub Actions workflow at `.github/workflows/web-typecheck.yml` runs the TypeScript typecheck for changes under `web/`. This is a validation workflow only; it does not deploy.

## Current un-deployed development state

The GitHub web source now contains both the zoom-coherence fix and a large architecture split. None of these changes have been deployed.

Completed in GitHub development source:

- Resident height no longer uses fixed 43/51 screen pixels.
- Resident size, spacing, and label anchor follow current terrain scale.
- Screen-edge resident magnet/clamp behavior is removed.
- Core/WASM loading and typed access moved to `runtime/core-bridge.ts`.
- Simulation timing moved to `runtime/simulation-clock.ts`.
- Resident continuity moved to `runtime/resident-continuity.ts`.
- World query, observer tracking, and stable terrain retention moved to `runtime/world-session.ts`.
- Camera gestures moved to `input/camera-input.ts`.
- React readouts now subscribe to `observer-store.ts` via `useSyncExternalStore`.
- Legacy Canvas2D drawing moved out of `observer-engine.ts` into `render/legacy-canvas-world-renderer.ts`.
- Shared projection and terrain presentation math are isolated.
- A non-production-connected unified Three.js foundation now exists: WorldScene, WorldRenderer, WaterLayer, and VegetationLayer.
- `observer-engine.ts` has been reduced to an orchestration layer of roughly 200 lines instead of holding all Core/query/input/render/readout behavior.

Deployment still requires an explicit user request.

## Transitional technical debt

Canvas2D terrain and Three.js residents still use separate render pipelines.  
World controls still use temporary DOM lookup/click binding instead of React actions.  
LifeLensCore execution still shares the browser main thread.  
The unified Three.js WorldScene exists but is not yet connected to the visible viewport.  
Residents still render in the legacy transparent CharacterLayer rather than as actors inside WorldScene.  
Resident model/animation assets remain remote runtime dependencies.  
Labels remain part of the legacy Canvas renderer.

## Validation state

Static/source-boundary review has been performed after the refactor.

A real TypeScript dependency install/typecheck has not yet run in this session because the local sandbox cannot resolve external GitHub/npm DNS, and connector-originated commits have not shown a GitHub Actions workflow run. Treat typecheck as a gate before any future deployment.

## Next parallel work order

1. Convert WorldSeed/new-world/time/chunk controls to React actions and remove remaining control DOM queries from `observer-engine.ts`.
2. Connect the new WorldRenderer behind a development-only render-mode boundary without replacing the legacy renderer yet.
3. Improve WorldScene terrain geometry from one flat chunk plane to continuous corner-height geometry.
4. Improve WaterLayer topology for rivers/streams/lakes instead of one water plane per wet chunk.
5. Move resident actor/model logic toward a WorldScene ResidentLayer.
6. Define Worker snapshot protocol and move Core stepping/query work off the main thread.
7. Add development diagnostics UI/frame/query counters.
8. Resume environment density, human motion, day/night/weather, structures/tools, and society/life presentation.

## Deployment gate

Do not perform AppDeploy deployment/update during normal development.

Only deploy after the user explicitly requests deployment. Before deploying, compare GitHub `web/` against the live AppDeploy snapshot, sync reviewed changes only, run QA, and report the production result.
