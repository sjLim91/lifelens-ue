# LifeLens Web Observer

`web/` is the canonical development source for the LifeLens browser observer.

## Role

The browser is a first-class LifeLens observer client. It uses the same authoritative LifeLensCore compiled to WASM. Browser code may interpolate or cache presentation state, but it must not invent persistent residents, terrain truth, relationships, life events, or simulation results.

## Development vs deployment

GitHub source and production deployment are deliberately separated.

Development, refactoring, tests, and documentation happen here first. AppDeploy is a deployment target, not the source of truth. Production deployment is performed only after an explicit user instruction to deploy.

## Current stack

React 19 + TypeScript + Vite for UI, LifeLensCore C++/WASM for simulation authority, Three.js for 3D residents, and an AppDeploy backend proxy for pinned Core runtime artifacts.

The current Canvas2D terrain + Three.js resident overlay is transitional. The target is a single Three.js world scene/camera so terrain, water, vegetation, structures, residents, effects, selection, and zoom all share one coordinate system.

See:
- ../docs/web/LIFELENS_WEB_MASTER_SPEC.md
- ../docs/web/LIFELENS_WEB_WORKING_STATE.md

## Current implementation map

See [LIFELENS_WEB_IMPLEMENTATION_MAP.md](../docs/web/LIFELENS_WEB_IMPLEMENTATION_MAP.md) for the module-by-module development state and migration boundary.
