# LifeLens Web Observer — Working State

Last updated: 2026-09-22

## Authority

The Web/PWA client is a presentation/observer client for the same `LifeLensCore` truth used by native clients.

Canonical priority for current web work:

1. `docs/LIFELENS_SPEC_v1.1.md`
2. domain canonical companions such as `WORLD_ARCHITECTURE_v2.md`, `TIME_AND_DYNAMIC_ENVIRONMENT.md`, `HUMAN_REALISM_FOUNDATION_v1.md`, and `OBSERVER_CAMERA_CONTROL_v1.md`
3. `docs/WEB_CLIENT_ARCHITECTURE_v1.md`
4. this working-state document

This file records implementation status only. It must not override canonical design contracts.

## Current implementation baseline

- React 19 + TypeScript observer UI.
- LifeLensCore C++ WASM remains simulation authority.
- Three World is the normal integrated terrain/water/vegetation/resident presentation path; Legacy Canvas remains an emergency presentation fallback only.
- One Three.js world coordinate system is used for Three World terrain, water, vegetation and residents.
- Core-backed focused resident detail exposes needs, emotion, personality/traits, directional relationships, family, memories and beliefs.
- Dynamic weather presentation consumes Core weather state.
- Resident continuity prevents transient payload gaps from destroying actors.
- Normal NEW GAME generates a WorldSeed automatically. Explicit Seed entry is secondary deterministic replay UI.
- Normal user-facing Observer UI defaults to Korean; diagnostics and renderer switching remain development-only.
- Mobile observer chrome uses safe-area padding and a 48 logical-pixel touch-target baseline.

## Canonical time contract

The web clock follows `docs/TIME_AND_DYNAMIC_ENVIRONMENT.md`.

- Pause: 0x
- Observe: 1x
- Fast: 4x
- Faster: 16x
- Rapid: 64x
- 1x target: 8 real minutes per LifeLens day
- no +10 minute / +1 hour product time-jump controls
- catch-up is bounded; render refresh failure must not stop Core time
- History mode is not faked as a large multiplier and remains a separate future adaptive/coarse-step system

## Canonical camera contract

The web camera follows `docs/OBSERVER_CAMERA_CONTROL_v1.md` while respecting the newer World v2 no-start-settlement rule.

Android:
- short one-finger tap: resident selection
- one-finger drag: orbit yaw/elevation
- two-finger movement together: pan
- pinch: zoom

Desktop:
- left click: resident selection
- right drag: orbit
- middle drag: pan
- wheel: zoom

Orbit elevation and distance are bounded and presentation motion is smoothed. Panning changes ObserverInterest only; it must not materialize simulation state or redefine Core world truth. The recenter affordance means current resident group, not a pre-authored settlement/living-area center.

## World v2 presentation alignment

Web presentation must not manufacture geography.

- Terrain subdivision may interpolate Core-provided elevation, but presentation-only synthetic ridge/relief height is forbidden.
- Vegetation placement may use deterministic presentation hashes from Core ecology coverage, but tree ground height uses the same authoritative elevation sampler as terrain/residents.
- Hydrology topology comes from Core observations.
- Observer movement requests different deterministic Core windows instead of revealing a decorated finite board.
- The initial spawn coordinate is not a settlement or permanent living-area authority.

More detailed continuous surface sampling, regional/planetary representation and persistent human world deltas remain World v2 follow-up work.

## Human motion truth

The browser must follow `docs/HUMAN_REALISM_FOUNDATION_v1.md`.

- movement may use locomotion animation
- an activity label alone is not enough to play sit/use/tool interaction motion
- Sleep/UseToilet/Eat/Drink/Wash/Repair do not invent chairs, beds, toilets, tools or interaction slots
- unsupported/contextless object-bound actions fall back to neutral idle
- target-backed social motion may be shown only when the target/context is actually present
- future action/motion DTO work must carry authoritative target, slot, facing/distance and alignment context

## Reliability

- Core failure is explicit; no fake browser residents/world truth.
- Older runtime optional-feature skew may disable that presentation feature but must not fabricate simulation state.
- Runtime release/preview validation must keep JS/WASM assets version-compatible.
- stale async snapshots must not overwrite newer truth once the Worker path is active.

## Known canonical gaps / not complete yet

These are not approved deviations; they are outstanding implementation work.

1. **Permanent resident GUID contract**
   - Core currently uses `CharacterId = uint64_t` with sequential founder IDs.
   - Master Spec calls for permanent GUID identity.
   - This requires a deliberate Core/Save/Relationship/Family migration, not a browser-only patch.

2. **Experience-based place memory / living area**
   - World v2 correctly treats spawn as an initial coordinate only.
   - Human Realism defines place memory, attachment, routine and familiar routes.
   - A complete resident/household learned home-range model is not finished yet.

3. **Web Worker execution**
   - Worker protocol and `SnapshotSequencer` exist, but LifeLensCore still executes on the browser main thread.
   - Moving Core/query work into the Worker remains required by the web architecture roadmap.

4. **Web save/load persistence**
   - OPFS/IndexedDB transport around the shared Core save codec is not complete.

5. **Observer scale transitions**
   - Local/Regional/Planetary/Orbital/Interplanetary representation continuity is not complete.

6. **Action/motion presentation DTO**
   - Web currently lacks the authoritative interaction-slot/alignment payload needed for rich object-bound motion.
   - Until that exists, neutral fallback is intentional and canonical.

7. **World v2 visual completeness**
   - Near/mid/far ecology, coast/ocean material polish, structures/tools/traces and persistent human deltas remain incomplete.
   - Runtime screenshots/device QA remain the acceptance source; source/CI success alone is not visual completion.

## Deployment rule

Normal source work does not trigger AppDeploy. Deployment/public-preview changes occur only when explicitly requested. Source, tests and working-state documentation are updated first.
