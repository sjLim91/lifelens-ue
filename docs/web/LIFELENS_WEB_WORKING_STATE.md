# LifeLens Web Observer — Working State

Last updated: 2026-09-22

## Production baseline

The current production AppDeploy preview runs React 19 + TypeScript, Canvas2D terrain, Three.js residents, and LifeLensCore WASM delivered through a backend proxy.

Production fixes already applied before source/deployment separation include resident continuity caching, last-known resident positions, transient terrain retention, independent simulation and refresh timers, elapsed wall-time catch-up, and protection against resume-frame resident flicker.

## Canonical development source

The repository now contains `web/` as the canonical browser development tree.

Production AppDeploy is no longer treated as the only source copy. New work is committed to GitHub first.

A GitHub Actions workflow at `.github/workflows/web-typecheck.yml` runs the TypeScript typecheck for changes under `web/`. This is a validation workflow only; it does not deploy.

## Current un-deployed patch

The GitHub web source includes a zoom-coherence patch that is intentionally not deployed.

Changes:

- Resident height is no longer hard-coded to 43/51 screen pixels.
- Resident visual height derives from the current terrain tile scale.
- Resident separation radius follows the same zoom-dependent scale.
- Screen-edge character clamping is removed so characters behave as projected world objects rather than HUD markers.
- Name label text remains screen-readable, while its vertical anchor follows current resident height.
- Regression coverage now explicitly checks pinch/wheel zoom coherence.

Deployment requires an explicit user request.

## Transitional technical debt

Canvas2D terrain and Three.js residents still use separate render pipelines.  
`observer-engine.ts` still performs direct DOM updates.  
WASM/Core execution still shares the main browser thread.  
Resident model/animation assets are remote runtime dependencies.  
Labels are still rendered through the terrain canvas.  
Terrain is still rebuilt as Canvas2D presentation instead of reusable world geometry.

## Next work order

1. Keep the GitHub canonical web source type-clean and finish current zoom regression cleanup.
2. Split Core bridge, render state, camera state, and UI state out of `observer-engine.ts`.
3. Replace direct DOM mutation with typed React subscriptions.
4. Introduce a unified Three.js `WorldScene` and move terrain/camera projection into it.
5. Move water and vegetation into world-space geometry with pooling/instancing.
6. Bind residents directly to world coordinates and delete temporary pixel/tile scale coupling.
7. Move Core stepping and queries into a Web Worker.
8. Resume environment density, human motion, day/night/weather, structures/tools, and society/life presentation.

## Deployment gate

Do not perform AppDeploy deployment/update during normal development.

Only deploy after the user explicitly requests deployment. Before deploying, compare GitHub `web/` against the live AppDeploy snapshot, sync reviewed changes only, run QA, and report the production result.
