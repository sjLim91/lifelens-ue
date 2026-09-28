# LifeLens Web Observer — Working State

## 2026-09-28 CANONICAL CURRENT — fast-forward + local growth + cultivation (#481–#483)

This section supersedes older Web-current snapshots below while preserving them as historical context.

### Baseline
- Current main: `77b4dceb39eade6111e153682f235025ad2cfb35` (#483).
- Canonical preview: https://sjlim91.github.io/lifelens-ue/?v=77b4dceb
- GitHub Pages branch provenance: `preview: 77b4dceb...`.
- Web remains a first-class presentation client for the same authoritative LifeLensCore; no browser-side future-state fabrication was introduced.

### Long-run visible development
- #481 authoritative day fast-forward executes the normal Core timeline in causal order and compares authoritative before/after snapshots.
- #482 local resident demand allows settlement foundation capacity to grow instead of treating one global facility as permanent completion.
- #483 adds authoritative cultivation:
  - `Cultivation` is discovered through Core knowledge/experiment prerequisites.
  - `CultivatedPlot` construction requires real materials/work.
  - planting consumes PlantFood seed stock, watering consumes Water, tending is resident work.
  - growth uses Core ecology/climate/season inputs and produces PlantFood only on harvest.
  - crop state survives Save/Load via snapshot v6.
  - Web receives cropPlanted/growth/moisture/care/harvest facts through Core DTO/HumanTrace contracts.
  - Three.js facility rendering shows the plot and crop growth directly from those authority facts.
  - Korean-only UI registry includes Cultivation/CultivatedPlot and Plant/Water/Tend/Harvest.

### Validation
- #483 exact-head: Preflight, Web Typecheck, Runtime Resilience, Web WASM, Core Tests, Unreal Linux Compile — PASS.
- main Web Runtime Release / Web Preview / Pages publish — PASS.
- External Preview Probe remains a separate known failure and is not evidence of failed Pages publication.

### Remaining durable-subsistence gap
- C-S3 is still ACTIVE: explicit **local scarcity -> resource search/exploration/movement pressure** is not yet complete.
- The browser must not invent search targets, farms, settlement centers or projected yields. The next step belongs in Core decision/search authority first.


## 2026-09-27 CANONICAL CURRENT — code baseline `c2e7a766` (#476)

This section supersedes older Web-current snapshots below while preserving them as historical context.

### Product/runtime
- Web Observer is a first-class LifeLensCore client, not a separate simulation.
- GitHub `main` is source truth; GitHub Pages is the checkable Web preview.
- Current preview pattern: https://sjlim91.github.io/lifelens-ue/?v=<current-main-short-sha>
- AppDeploy is not used for LifeLens Web.
- Core / World remains the sole authority for terrain, hydrology, residents, goals, targets, life events, facilities and persistent human traces.
- Presentation may interpolate, LOD, label and animate authoritative facts; it may not invent durable world state or action meaning.

### Korean-only presentation contract (#477)
- Every user-visible Web Observer string is Korean. Core enum names and internal authority tokens remain internal transport values only.
- All Web-exposed Core enums and structured resident-detail keys must be registered in `web/src/localization/korean.ts` before merge.
- `Tools/validate_web_korean_ui.py` compares the current Core enum definitions against the Korean registry. Adding a new enum value without its Korean label fails Preflight with the exact missing value.
- Already-Korean simulation display text may pass through unchanged; an unregistered English authority value is never intentionally exposed as fallback text.
- Runtime version-skew fallback remains Korean and logs the exact missing token to the developer console so the omission is visible rather than silently hidden.
- New panels/features must consume the shared localization layer instead of defining ad-hoc raw enum fallbacks.

### Current renderer/presentation
- Unified Three.js `WorldScene` is the active Web world path for terrain, water, vegetation, atmosphere and resident actors.
- Resident actors are stable by GUID and retain presentation continuity through transient refresh gaps.
- Camera/touch fixes through #458 remain part of the baseline.
- Hydrology consumes Core downstream facts (#459); the browser does not rebuild a fake drainage graph.
- Environment density, three deterministic CC0 tree variants, weather/snow recovery, resident appearance diversity and water repairs from #452–#459 remain integrated.

### Observation-content chain
- **Event Visibility (#460):** compact observation feed derives only from changes between authoritative Core snapshots.
- **Action Context (#462):** Core emits presentation kind/phase, resident/object/grid targets and action metadata; in-world cues expose factual intent.
- **Human Trace v1 (#463):** Web inspects authoritative resource depletion, environmental residue and started facility work with bounded world markers and focus.
- **Semantic Motion (#464/#465):**
  - UAL1 remains the proven locomotion/action baseline.
  - UAL2 Standard is a pinned CC0 fail-soft secondary library.
  - Eat/Drink -> `Consume`.
  - PlantFood Gather -> `Farm_Harvest`.
  - facility `DeliverMaterial` while moving -> `Walk_Carry_Loop`.
  - Work/Repair and Craft/Experiment use reviewed kneeling work motion.
  - social/teaching motion requires a real nearby resident target.
  - unsupported or ambiguous action semantics fail closed to Idle/Walk.

### Social Cue v1 (#466)
- active authoritative `Interacting` Social / KnowledgeTeaching / Parenting pairs can render a subtle bounded world-space connector.
- `Avoid`, missing targets and duplicate reciprocal pairs do not create extra connectors.
- cue density is capped and connectors render only while the smoothed actors are visually near.
- the browser does not infer a relationship from relationship scores, emotions or memories.

### Observer Director / resident truth / world activity (#469–#471)
- **Observer Director (#469):** important authoritative events may be surfaced for observation without forcibly stealing camera control.
- **Focused resident truth (#470):** genetics, life condition, development, family/kinship, life history, memories, beliefs and civilization knowledge/inventory are exposed in focused inspection.
- **World activity observation (#471):** civilization resources, storages, facilities, discoveries and sanitation sites are sampled on a bounded low-rate path and exposed as inspectable factual activity.
- the first heavy world payload does not replay old history as new events.

### Spawn-independent lived world (#474/#475)
- **Core emergent activity anchor (#474):** storage, fire, furnace and settlement facility placement follows authoritative resident/activity positions rather than the NEW GAME spawn coordinate.
- **Unreal natural presentation (#475):** spawn/camera readability no longer creates a permanent ecology clearing; vegetation/resource thinning is facility-local only.
- #474 and #475 supersede the stale conflicting #472/#473 attempts. #472/#473 were closed without merge.

### Household / pregnancy truth (#476)
- focused resident inspection now carries authoritative household shared resources/assets, member contribution/responsibility facts, and active pregnancy role/stage/timing/health/fatigue/stress/nutrition.
- residents may legitimately have `household: null` or `pregnancy: null`; presentation must not invent either state.

### Authoritative day fast-forward (#481)
- Observer accepts a user-entered day count and advances the existing LifeLensCore simulation through the normal minute-by-minute `runMinutes` / `step()` path.
- This is **not** a direct clock jump and does not synthesize future state. Needs, action decisions, facilities, resources, knowledge, family progression, pregnancy/birth, aging/mortality and every other currently implemented Core rule execute in causal order.
- Web pauses the normal wall-clock simulation timer, runs Core in bounded 360-minute chunks, yields to the browser between chunks for mobile progress UI, and does not render intermediate world frames.
- After completion Web forces fresh heavy authority payloads, recenters to current residents, renders the final world, and compares pre/post authoritative snapshots.
- The completion summary may report only facts present in those snapshots: population, deaths/births, households, couples, pregnancies, facilities/state changes, knowledge/discoveries, life events, resources, storage and sanitation.
- Current Web input guard is 1–3650 days. Larger-scale history acceleration would need a separate Core optimization contract; it must not be faked by directly changing `world.minute`.

### Deliberate gaps
- Sleep has no accepted lie-down / sleep-loop / wake sequence yet.
- TreeChopping_Loop is present in UAL2 but is not bound from `Wood` alone; Core must expose enough source/tool semantics first.
- Ground gather, digging and more carry/build variants require reviewed clips plus authoritative action facts.
- Human Trace v1 does not yet fabricate worn paths, settlements or activity centers from browser heuristics.
- Full romance history, extended genealogy/birth-record inspection and deeper lived-space accumulation remain follow-up observer expansions; they must come from Core facts rather than browser inference.
- Actual Android/mobile visual acceptance remains open; automated tests/CPU geometry checks do not replace device review.

### Current validated baseline
For code baseline `c2e7a766` after #476:
- Preflight — PASS.
- Typecheck — PASS.
- Web Runtime Release — PASS.
- Web WASM — PASS.
- Core Tests — PASS.
- Unreal Linux Compile — PASS.
- Web Preview / GitHub Pages — PASS.
- External Preview Probe — known independent failure, not evidence that Pages publish failed.

### Immediate next work
1. review UAL2 / compatible zero-cost clips for Sleep, Dig, Chop, Ground Gather and carry variants.
2. bind only actions for which Core exposes enough authoritative semantics.
3. Social Cue v1 — DONE (#466).
4. Observer Director — DONE (#469); keep camera follow opt-in rather than forced.
5. motion tranche 2 can continue only where clip semantics and Core authority are both sufficient.
6. expand Human Trace only from persistent/accumulated Core facts.

Canonical motion handoff: `tasks/WEB_MOTION_HANDOFF_2026-09-27.md`.

---

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
- Core-backed focused resident detail exposes needs, emotion, personality/traits, genetics, life condition, development, directional relationships, family/kinship, household facts, pregnancy state, life history, memories, beliefs and civilization knowledge.
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
- no +10 minute / +1 hour direct-clock-jump controls
- user-entered day fast-forward is allowed only when it executes the authoritative Core timeline in causal order; it may skip rendering but never skip simulation rules
- catch-up is bounded; render refresh failure must not stop Core time
- History mode is not faked as a large multiplier; future coarse/adaptive history stepping requires a separate authoritative Core contract

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
- The initial spawn coordinate is not a settlement or permanent living-area authority. Core facility placement follows lived activity (#474), and Unreal ecology readability is facility-local rather than spawn-centered (#475).

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

6. **Rich object-bound motion alignment**
   - The authoritative Action Context DTO now exists and drives current semantic motion.
   - Rich object-bound actions still need stronger slot/alignment/tool/source facts before clips such as chopping, digging, ground pickup and sleep transitions can be bound safely.
   - Until those facts and reviewed clips exist, neutral fallback is intentional and canonical.

7. **World v2 visual completeness**
   - Near/mid/far ecology, coast/ocean material polish and richer structures/tools remain incomplete.
   - Human Trace v1 is present, but lived paths/activity centers require deeper authoritative accumulation.
   - Runtime screenshots/device QA remain the acceptance source; source/CI success alone is not visual completion.

## Deployment rule

GitHub is source truth and GitHub Pages is the canonical checkable Web preview. A merge may publish the Pages preview through the repository workflow. AppDeploy is not used for LifeLens Web.
