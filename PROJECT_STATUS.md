# LifeLens Project Status

Date: 2026-09-29  
Active architecture: **LifeLensCore -> WASM -> Web Observer**  
Current pre-removal main checkpoint: `0ee1e161be22ede0a9f7ef854a1ca34597b3c6a6` (#496)

## Runtime decision

The Unreal client has been retired from active development and is being removed from `main`.

Reasons:
- visual/product verification now happens through the Web Observer;
- Unreal UHT/UBT and platform pipelines added maintenance/CI cost without being used for product review;
- LifeLensCore is already platform-neutral and the Web client consumes the same authority through WASM.

The final Unreal state is preserved at `archive/unreal-final-20260929`.

PR #497, which only repaired the Unreal external physical sleep path, was closed without merge. Timed sleep for the authoritative headless/Web path already landed through #494.

## Current authority

- LifeLensCore C++ is simulation truth.
- Web uses Core via WASM.
- React/TypeScript/Three.js is the primary observer/presentation client.
- Browser presentation must not fabricate resources, actions, facilities, geography, relationships, or future state.

## Recent P0 baseline

- #491 closer mobile zoom / readable resident labels.
- #492 ruined facility restoration before wasteful rebuilding.
- #493 provisions required before consumption.
- #494 real bedding travel, timed sleep, paid washing.
- #495 canonical GitHub Pages preview probe.
- #496 spatial causality guard for gather/store/retrieve/facility work.

## Validation target after Unreal removal

Required:
- LifeLens Core Tests
- deterministic harness
- LifeLens Web WASM
- Web Typecheck/build
- Web Runtime Release
- GitHub Pages Preview
- External Preview Probe
- Web/Core structural Preflight

No Unreal compile or Android Unreal APK gate remains on active `main`.

## Canonical documents

- Runtime architecture: `docs/RUNTIME_ARCHITECTURE_WEB_CORE_v1.md`
- Product/domain master spec: `docs/LIFELENS_SPEC_v1.1.md` (engine-specific implementation sections are historical where they conflict with the runtime architecture document)
- Web architecture: `docs/WEB_CLIENT_ARCHITECTURE_v1.md`
- Web working state: `docs/web/LIFELENS_WEB_WORKING_STATE.md`
- World architecture: `docs/WORLD_ARCHITECTURE_v2.md`
- Open-ended civilization: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`
