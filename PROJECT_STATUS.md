# LifeLens Project Status

Date: 2026-09-30  
Active architecture: **LifeLensCore -> WASM -> Web Observer**  
Current main checkpoint: `257c225eff48f26cbde80e3f901c0ded69f40f3a` (#511)

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

## Recent P0/P1 baseline

- #491 closer mobile zoom / readable resident labels.
- #492 ruined facility restoration before wasteful rebuilding.
- #493 provisions required before consumption.
- #494 real bedding travel, timed sleep, paid washing.
- #495 canonical GitHub Pages preview probe.
- #496 spatial causality guard for gather/store/retrieve/facility work.
- #503 canonical observation speed rebased to the former 4x pace, with real
  portable/natural-water authority for washing.
- #504 Web hydrology visibility aligned with Core direct-water authority;
  drinking/washing interaction waits for visible arrival and carried-water
  actions are labeled separately from natural-water use.
- #505 resident locomotion/action animation now follows the full global
  1x/4x/16x simulation speed contract instead of an independent visual cap.
- #506 timed Core sleep is presented as a truthful lying/resting posture only
  after authoritative arrival; movement, collision and Need authority stay in
  Core.
- #507 Observer feed now exposes factual Need -> travel -> interaction
  milestones for Eat/Drink/Sleep/UseToilet/Wash without replay/spam.
- #509 16x observer hot path split from heavyweight snapshots: resident
  movement/Needs/presentation use a compact runtime DTO while relationships,
  memories, beliefs, family and civilization detail refresh at a lower cadence;
  deterministic terrain is cached until the observer window changes.
- #510 dynamic human-trace refresh avoids rescanning static terrain while still
  rebuilding grass/rocks/trees once when authoritative facility footprints
  actually change.
- #511 social observation now exposes factual approach -> interaction phases for
  Approach/Comfort/Repair/Avoid plus KnowledgeTeaching and Parenting, with exact
  Core social outcomes taking priority over duplicate inferred activity rows.

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
