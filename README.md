# LifeLens

LifeLens is an autonomous life, society, civilization, and world observation simulation built around one deterministic simulation authority.

## Current runtime

The active product runtime is:

```text
LifeLensCore (C++ simulation authority)
        |
        v
Emscripten / WASM bridge
        |
        v
Web Observer (React + TypeScript + Three.js)
```

`LifeLensCore` owns simulation truth: seed/time, residents, needs, relationships, family/lifecycle, civilization, resources, facilities, terrain, hydrology, ecology, environment, persistence, and deterministic progression.

The Web Observer presents that truth. It must not create a second browser-only simulation or fabricate durable world state.

## Repository structure

- `Source/LifeLensCore/` — authoritative C++ simulation and tests.
- `Clients/Web/` — WASM bridge/runtime staging support.
- `web/` — canonical React/TypeScript/Three.js observer.
- `Tools/build_web_client.py` — Emscripten build entry.
- `docs/RUNTIME_ARCHITECTURE_WEB_CORE_v1.md` — current runtime architecture.
- `docs/web/LIFELENS_WEB_WORKING_STATE.md` — current Web implementation state.

## Unreal archive

The former Unreal client was retired from active `main` on 2026-09-29 because the product is now developed and visually verified through the Web Observer.

The final pre-removal Unreal state is preserved at:

`archive/unreal-final-20260929`

That archive contains the former `.uproject`, Unreal C++ bridge/client, Config, Content assets, native build workflows, and associated platform work. It can be used later as a reintroduction reference without keeping Unreal maintenance cost in the active product branch.

## Development policy

GitHub `main` is source truth. Core logic changes must pass native Core tests and deterministic harness checks. Browser-facing changes must pass WASM, TypeScript/build, and Web structural checks.

GitHub Pages is the canonical checkable Web preview. AppDeploy is not used for LifeLens Web.

No paid runtime API/cloud dependency is required for the baseline product.
