# LifeLens Runtime Architecture

Status: **CANONICAL**  
Effective: **2026-09-29**

## 1. Authority

`LifeLensCore` is the single simulation authority. Rendering technology is replaceable; simulation truth is not.

Core owns:

- deterministic seeds and simulation time
- resident identity and state
- needs, emotion, personality, memory and belief
- relationships, family, pregnancy, lifecycle and generations
- goals, decisions, navigation intent and action causality
- civilization, knowledge, resources, inventory and facilities
- terrain, hydrology, ecology, weather and environmental consequences
- persistence and deterministic replay contracts

## 2. Active product path

```text
LifeLensCore C++17
      |
      | Emscripten / explicit bridge contracts
      v
WASM runtime
      |
      v
React + TypeScript + Three.js Web Observer
```

The browser owns presentation, input, camera, UI, interpolation, LOD and visual effects. It may never resolve simulation outcomes independently of Core.

## 3. Boundary rule

The dependency direction is one-way:

```text
Web -> WASM adapter -> LifeLensCore
```

LifeLensCore must not depend on React, Three.js, DOM APIs, browser persistence APIs, or a specific renderer. This keeps a future native client or another engine possible without rewriting the simulation.

## 4. Verification

Core changes:
- CMake configure/build
- CTest suite
- deterministic harness smoke

Web runtime changes:
- Emscripten/WASM build
- runtime contract checks
- TypeScript typecheck
- Vite production build
- Web structural regressions
- GitHub Pages preview/probe

## 5. Retired Unreal client

Unreal is not an active `main` runtime or CI target.

The final pre-removal state is preserved at `archive/unreal-final-20260929` from commit `0ee1e161be22ede0a9f7ef854a1ca34597b3c6a6`.

If Unreal is reintroduced later, treat it as a new optional presentation client around the then-current LifeLensCore contracts. Restore useful native presentation code/assets from the archive and adapt the bridge to current Core APIs rather than restoring old engine coupling into Core.

## 6. Product invariant

A renderer may disappear without deleting LifeLens. The simulation model, world history, resident lives and save semantics remain owned by LifeLensCore.
