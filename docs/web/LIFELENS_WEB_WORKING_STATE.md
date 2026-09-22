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
- WorldSeed/new-world/time/chunk navigation controls now use typed React actions instead of DOM click bindings.
- Legacy Canvas2D drawing moved out of `observer-engine.ts` into `render/legacy-canvas-world-renderer.ts`.
- Shared projection and terrain presentation math are isolated.
- A development-only unified Three.js mode is now wired through `WorldRenderer` and can be selected locally without changing the default Legacy mode.
- Three World terrain uses shared corner heights across adjacent chunks rather than one flat elevation plane per chunk.
- Water geometry now builds connected river/stream arms from neighboring water topology instead of treating every flowing-water chunk as a full square.
- Vegetation uses an instanced tree layer.
- Terrain and water builders reuse one indexed terrain window per refresh instead of rebuilding a full lookup map for every chunk.
- A future Core Web Worker command/event protocol is defined.
- `observer-engine.ts` has been reduced to a small orchestration layer rather than holding Core/query/input/render/readout behavior.

Deployment still requires an explicit user request.

## Transitional technical debt

The production-compatible Legacy path still uses Canvas2D terrain plus a separate Three.js CharacterLayer.  
LifeLensCore execution still shares the browser main thread.  
Three World is connected only as a development render mode and is not the default path.  
Residents are intentionally hidden in Three World until the real resident actor/model pipeline moves into WorldScene, avoiding misleading low-quality placeholder humans.  
Resident model/animation assets remain remote runtime dependencies.  
Name labels remain part of the Legacy Canvas renderer.  
Three World water/vegetation materials are still first-pass presentation and need LOD/material/shoreline refinement.

## Validation state

Static/source-boundary review has been performed after the refactor.

A real TypeScript dependency install/typecheck has not yet run in this session because the local sandbox cannot resolve external GitHub/npm DNS, and connector-originated commits have not shown a GitHub Actions workflow run. Treat typecheck as a gate before any future deployment.

## Next parallel work order

1. Move the real resident actor/model pipeline into a WorldScene ResidentLayer without introducing placeholder production humans.
2. Add terrain mesh pooling/dirty-chunk updates so unchanged geometry is not recreated every snapshot.
3. Refine river continuity, lake shoreline shapes, coast/ocean transitions, and water materials.
4. Add atmosphere/day-night lighting and weather hooks to Three World.
5. Implement the defined Worker protocol and move Core stepping/query work off the main thread.
6. Add snapshot sequence rejection so stale responses can never overwrite newer state.
7. Add structures/tools/traces layers and resident-object interaction anchors.
8. Continue human animation/behavior presentation and society/life observation UI.

## Deployment gate

Do not perform AppDeploy deployment/update during normal development.

Only deploy after the user explicitly requests deployment. Before deploying, compare GitHub `web/` against the live AppDeploy snapshot, sync reviewed changes only, run QA, and report the production result.
