# LifeLens Web Client Architecture v1

> Status: **CANONICAL / IMPLEMENTATION ACTIVE**
>
> Updated: 2026-09-29 KST
>
> Decision: the active LifeLens product path is **LifeLensCore -> WASM -> Web Observer**. The former Unreal client is archived and is not part of active `main`.

## 1. Product shape

```text
                    LifeLensCore
          World + Human simulation truth
                         |
              explicit read/action contracts
                         |
                  Emscripten / WASM
                         |
                         v
                 Web Observer / PWA
          React + TypeScript + Three.js
```

The renderer is replaceable. LifeLensCore is not.

## 2. Authority

### LifeLensCore owns

- WorldSeed / PopulationSeed / WorldGenerationVersion.
- world generation, terrain, hydrology and ecology truth.
- resident identity/state.
- needs/emotion/personality/memory/belief.
- relationships/family/lifecycle.
- civilization/resources/facilities/history.
- authoritative save state and deterministic progression.

### Web Observer owns

- browser/PWA lifecycle.
- WebGL/Three.js presentation.
- browser input/camera.
- glTF/GLB character/prop presentation.
- browser-local persistence transport.
- HTML/CSS/React observer UI.
- visual interpolation, LOD and effects.

The Web client never forks or reimplements simulation rules.

## 3. Identity and transport

64-bit IDs/seeds cross the JavaScript boundary as lossless strings unless a BigInt-specific contract is explicitly introduced. They must not be rounded through JavaScript Number.

Browser-facing state is exported through explicit bridge/DTO contracts rather than direct C++ memory layout coupling.

## 4. Web runtime bridge

The bridge exposes authoritative Core operations and observation snapshots across the WASM ABI.

Current/future contract areas include:

- new game and deterministic replay.
- simulation time progression and bounded fast-forward.
- world/resident observation.
- terrain/hydrology/ecology windows.
- event/history feed.
- selected resident detail and genealogy.
- civilization resources/storage/facilities.
- save snapshot import/export.
- observer-interest queries.
- persistent human/world traces.
- action/motion presentation DTOs.

## 5. WASM build

`LifeLensCore` stays C++17.

Emscripten is an additional build target, not a separate Core fork.

```bash
python Tools/build_web_client.py
```

Generated JS/WASM binaries are release/build artifacts.

## 6. Presentation progression

- truth shell: actual Core world/residents, fail-closed runtime loading.
- world surface: continuous terrain, hydrology, water, free observer camera.
- ecology: biome/vegetation/ground cover from authority facts.
- residents: stable identity, semantic action/motion mapping and contextual targets.
- observer product: detail, relationships, family/genealogy, events, time controls and persistence.

## 7. No fake fallback

If Core WASM fails to load:

- do not spawn fake residents.
- do not generate a JavaScript-only authoritative terrain.
- do not synthesize events/resources/facilities.
- show an explicit Core-unavailable state.

Presentation-only placeholders must never look like durable LifeLens truth.

## 8. Performance model

The Web client may reduce mesh density, vegetation density, shadows, materials, animation complexity and effect budgets. It may not make the logical world smaller or alter simulation outcomes to fit rendering performance.

## 9. Persistence

Long-term Web persistence uses the same Core save codec:
- OPFS preferred when supported.
- IndexedDB fallback.
- user-controlled import/export where practical.

No incompatible browser-only simulation save format.

## 10. Deployment

The Web client is static-hostable and has no required paid backend.

The zero-cost publication path builds LifeLensCore with Emscripten and publishes the stable browser runtime. GitHub Pages is the canonical checkable preview.

## 11. Future native clients

A future Unreal or other native client can be introduced as another presentation adapter around the then-current LifeLensCore contracts. Core must remain free of renderer dependencies so that reintroduction does not require rebuilding the simulation model.

## 12. Acceptance

Web foundation is healthy when:
1. Core tests and deterministic harness pass.
2. WASM builds from the same LifeLensCore.
3. browser UI fails closed without WASM.
4. browser displays actual Core residents/world state.
5. terrain/hydrology presentation derives from Core truth.
6. TypeScript and production Web build pass.
7. the product remains functional without a paid backend.
