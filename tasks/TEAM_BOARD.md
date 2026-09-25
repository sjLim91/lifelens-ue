# LifeLens Team Board

이 파일은 **ownership / active locks / Integration Requests / cross-lane coordination**만 기록한다.

- live execution state: `tasks/WORK_STATE.md`
- canonical roadmap: `docs/DEVELOPMENT_MILESTONES.md`
- time / speed / environment contract: `docs/TIME_AND_DYNAMIC_ENVIRONMENT.md`
- long-range civilization direction: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`
- integrated audit: `docs/INTEGRATED_AUDIT_2026-09-17.md` — historical point-in-time evidence

---

## Ownership

| Responsibility | Primary | Collaboration |
|---|---|---|
| Integrated implementation across Core / AI / Simulation / World / WorldPresentation / Character / UI / Content / platform | Jjun | Dagyeom visual review/polish as needed |
| Runtime visual QA / screenshot-driven polish feedback | Dagyeom | Jjun |
| Build / CI / Android / Windows / macOS | Jjun | — |

Authority rule remains:

> Core / World owns simulation truth. Presentation and Character consume authoritative contracts and never invent resources, facilities, outcomes, lifecycle state, emotion, knowledge, time, weather, technology or movement authority.

### 2026-09-21 integrated ownership update

- previous hard split between Jjun and Dagyeom implementation lanes is retired.
- Jjun may modify Presentation/Character/UI/Content when required by an integrated milestone.
- Dagyeom is primarily a visual QA / targeted polish collaborator unless the user explicitly delegates a concrete implementation task.
- active branches still require same-file conflict coordination; this change is not permission to overwrite another live branch blindly.

## Collaboration model

LifeLens uses **one integrated implementation roadmap with optional visual collaboration**.

Rules:

- Jjun owns end-to-end milestone implementation by default.
- Core/World authority and Presentation/Character consumer boundaries remain architectural boundaries, not human ownership barriers.
- Dagyeom may review screenshots/runtime and take explicitly delegated visual polish work.
- same-file active work is coordinated before edits; do not overwrite live branches blindly.
- Jjun does not push directly to `dagyeom/*` branches.
- stale branches are not merged wholesale; valid missing ideas are reconstructed from latest `main`.
- provider contracts should still exist before consumers depend on them, even when one implementer handles both sides.
- conflict resolution compares both sides and preserves current-main truth.

### Cross-lane request checkpoint cadence

Jjun checks Dagyeom-side open PRs, new comments/review requests and new Integration Requests:

- before starting a new Jjun functional work unit;
- after completing or merging a Jjun work unit;
- when entering a long CI / Unreal build wait;
- when returning to act on a long CI / Unreal result;
- before a main-changing merge or rebase.

A new blocker is triaged before unrelated follow-up work. This does **not** mean repeatedly polling or restarting a healthy long build.

---

## Released Jjun Presentation Assist Locks (historical)

> All Jjun presentation assist locks below are **released by user direction as of 2026-09-19**.
> They remain here only as historical scope/evidence. New cross-lane edits require a fresh Integration Request or explicit user direction.


### ASSIST_LOCK-GFX-PLATFORM-COOK-2 — Jjun scoped platform packaging assist

- explicit user direction: keep desktop payload out of Android APK now.
- helper: Jjun.
- owner/reviewer for presentation source: Dagyeom.
- branch: `integration/platform-content-cook-boundary-v2`.
- cross-owner file:
  - `Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp`.
- scope:
  - Android loads lightweight Quaternius nature.
  - Windows/macOS load desktop photoreal nature.
  - Android cook excludes desktop nature/PCG payload.
  - desktop packages exclude future `/Game/Mobile`.
  - no simulation authority changes.
- compact firepit/basket/axe remain a documented shared exception until mobile replacements exist.
- release after merge/close and board reconciliation.


### ASSIST_LOCK-GFX-PCG-RUNTIME-2 — Jjun scoped PCG presentation assist

- helper: Jjun.
- owner/reviewer: Dagyeom.
- branch: `integration/desktop-pcg-runtime-dressing-v2`.
- integration status: platform cook boundary #269 is merged to `main`; #271 is retargeted to `main` for fresh CI validation.
- cross-owner files:
  - `Source/LifeLens/WorldPresentation/LLPCGGroundCoverPresentationActor.cpp`
  - `Source/LifeLens/WorldPresentation/LLPCGGroundCoverPresentationActor.h`.
- scope:
  - consume `PCG_LL_GroundCover` on Windows/macOS desktop runtime.
  - seed from authoritative initial-chunk `VisualSeed`.
  - decorative-only; no resource/facility authority.
  - Android does not spawn/link the PCG runtime path.
- release after merge/close and board reconciliation.

### ASSIST_LOCK-GFX-CHARACTER-PROPS-1 — Jjun scoped character presentation assist

- explicit user direction: push visible graphics through finalization.
- helper: Jjun.
- owner/reviewer: Dagyeom.
- branch: `integration/character-held-props-production-v1`.
- cross-owner file:
  - `Source/LifeLens/Characters/LLResidentMotionComponent.cpp`.
- scope:
  - replace visible Engine primitive daily-life props with approved CC0 basket/bowl art.
  - fail closed for primitive tools that still lack semantically correct art.
  - no action/tool/technology authority changes.
- dependency: wooden bowl asset PR #268 must merge before this consumer.
- release after merge/close and board reconciliation.

### ASSIST_LOCK-GFX-WATER-ALIGN-3 — Jjun scoped Water terrain alignment assist

- helper: Jjun.
- owner/reviewer: Dagyeom.
- branch: `integration/water-terrain-alignment-v3`.
- supersedes: conflicted #272 / v2 after #271 merged.
- cross-owner files:
  - `Source/LifeLens/WorldPresentation/LLWaterPresentationActor.cpp`
  - `Source/LifeLens/WorldPresentation/LLWaterPresentationActor.h`.
- scope:
  - project authoritative Water surfaces onto the same Core terrain relief used by WorldPresentation.
  - preserve the flat initial settlement baseline.
  - no hydrology/gameplay authority changes.
- release after merge/close and board reconciliation.

### ASSIST_LOCK-GFX-SMOOTH-TERRAIN-2 — Jjun scoped smooth-terrain assist

- helper: Jjun.
- owner/reviewer: Dagyeom.
- branch: `integration/desktop-smooth-terrain-v2`.
- integration status: retargeted to `main` to validate the full #276 Water + smooth-terrain stack; merge #276 first.
- supersedes: #274 / v1 after Water was reconstructed as #276.
- cross-owner files:
  - `Source/LifeLens/WorldPresentation/LLDesktopTerrainPresentationActor.cpp`
  - `Source/LifeLens/WorldPresentation/LLDesktopTerrainPresentationActor.h`.
- scope:
  - replace visible desktop chunk-flatness with a smooth procedural overlay.
  - authoritative Core terrain corners/center remain the only elevation input.
  - flatten around settlement/facilities for readability and existing interaction geometry.
  - visual-only: no collision/navigation/terrain gameplay authority.
  - Android keeps its existing lightweight ground path.
- plugin is desktop-only and treated as a presentation implementation detail.
- release after merge/close and board reconciliation.

### ASSIST_LOCK-GFX-FACILITY-COMPLETE-2 — Jjun scoped completed-facility visual assist

- helper: Jjun.
- owner/reviewer: Dagyeom.
- branch: `integration/photoreal-facility-completion-v2`.
- supersedes: conflicted #279 / v1 after #278 merged.
- scope:
  - completed SleepingPlace desktop frame uses approved photoreal timber.
  - completed Furnace desktop shell uses the already-approved photoreal boulder asset.
  - completed FirePit desktop fuel uses approved photoreal timber.
  - Android preserves lightweight facility presentation and must not hard-reference the desktop timber/boulder completion path.
  - construction-progress truth and Core authority remain unchanged.
- release after merge/close and board reconciliation.

### ASSIST_LOCK-GFX-ENV-DENSITY-1 — Jjun whole-world environment density assist

- helper: Jjun.
- owner/reviewer: Dagyeom.
- branch: `integration/environment-density-v1`.
- stacked after: #279; validation runs independently while #279 compiles.
- scope:
  - increase Windows/macOS near/far decorative density without changing Core resources.
  - preserve Android visual budgets.
  - replace white-noise scattering with deterministic natural micro-clustering.
  - preserve settlement/facility readability envelopes and dynamic canopy visibility.
  - no gameplay, resource, collision or navigation authority changes.
- release after merge/close and board reconciliation.

### ASSIST_LOCK-RUNTIME-VISUAL-SANITY-1 — Jjun screenshot-driven runtime visual hotfix

- helper: Jjun.
- owner/reviewer: Dagyeom.
- branch: `hotfix/runtime-visual-sanity-v1`.
- evidence: local editor screenshots show sparse settlement dressing and visible stretched primitive facility blocks.
- scope:
  - hide Engine BasicShape facility structure proxies on desktop production.
  - show staged photoreal timber/stone for planned/in-progress desktop facilities.
  - reduce over-aggressive settlement-wide vegetation clearing.
  - improve visibility of the current sapling-heavy desktop canopy catalogue.
  - preserve Android lightweight presentation and Core gameplay authority.
- release after exact-head Preflight + Unreal compile and runtime recheck.

## Open Integration Requests

### IR-GFX-CINEMATIC-1 — Desktop cinematic presentation adoption (Windows + macOS)

- requester: Jjun.
- needed owner: Dagyeom.
- target: Windows + macOS desktop renderer integration after Config foundation.
- requested areas: `Content/Environment/**`, `Content/Maps/**`, `Content/WorldPresentation/**` and presentation-owned environment setup as needed.
- needed work:
  - opt approved high-detail static assets into Nanite on the desktop presentation path while preserving Android fallback LOD/mesh.
  - tune Sky Atmosphere / volumetric fog/cloud / exposure and dynamic environment lighting against authoritative time/weather state.
  - verify terrain/water/material presentation under the Windows/macOS desktop renderer paths.
  - adopt the enabled Unreal `PCG` plugin for natural dressing/biome placement without inventing resource authority.
  - project authoritative Hydrology into the enabled Unreal `Water` system for river/lake/coast/ocean visuals, with an Android-safe fallback path.
  - do not restore Engine Cube/Cone or visibly prototype local-view fallback art.
- why existing contract is insufficient: Config can enable the renderer, but content/map assets must explicitly use and visually validate it.
- blocking: **does not block the Jjun Config PR**, but it blocks claiming that the cinematic visual target itself is delivered.

Open a new Integration Request only when one owner needs another owner to change a file/API/config outside the requester's lane and the current contract is insufficient.

A request must contain:

- requester / needed owner;
- exact file/API/config needed;
- why the existing contract is insufficient;
- target branch/PR or asset path;
- whether it blocks the current milestone.

---

## Current owner work references

### Jjun — PR #133 Roadmap / documentation reconciliation — ACTIVE

Purpose:

- reconcile #132/#134 completion.
- close original #100 as superseded.
- add canonical simulation time / variable speed / dynamic environment contract.
- reorder next implementation priorities.

This PR is docs-only. Do not start a heavy Unreal compile solely because documentation changed.

After #133 merges, Jjun starts:

> **T1 — Simulation Time Authority & Variable Speed**

Then:

> **E1 Calendar + Day/Night Authority**
> **E2 Seasons + Dynamic Weather Core v1**
> **E3 Environmental Consequences v1**

### Dagyeom — PR #100 World Readability — SUPERSEDED / CLOSED BY #134

Original branch:
`dagyeom/world-visual-readability-envelope`.

The old PR is no longer the integration vehicle.
Validated WorldPresentation source was reconstructed on current main through #134.

#134 evidence:

- Preflight #723 PASS.
- Unreal Linux Compile #207 PASS.
- merged to main as `1195fddbaf3341ac9508347d07fab69740f02482`.

Therefore Dagyeom no longer waits for #100 merge.

### Dagyeom — PR #98 Observer Readability / QA View — STALE

- do not merge as-is.
- selectively reimplement useful ideas on current main.
- true Observer scrolling remains a live task.

---

## Provider contracts available to Dagyeom

### ContextAction / KnowledgeTeaching

`KnowledgeTeaching` is authoritative:

- Core identifies teacher, learner and technique.
- Bridge exposes target resident + technique + token.
- World moves teacher to learner and ACKs only after completion conditions.
- Core revalidates the real meeting.
- Save/Load does not persist stale pending teaching.

Use for richer teaching/social motion and Observer labels without changing simulation authority.

### Emotion runtime

#132 is merged.
Dagyeom may consume authoritative resident emotion projection.

Rules:

- UI/Character must not calculate a second emotional truth.
- animation/facial/UI state may summarize Core emotion but must not mutate it.
- missing cause/history metadata requires provider read-model extension instead of visual inference.

### Lifecycle

Core lifecycle/family/history DTOs remain authority for Lifecycle Event Presentation v2.
Presentation may show birth/growth/death/history but must not infer cause or create grave/corpse state unless Core exposes it.

### Time / Speed — provider pending T1

Target presets:

- Pause 0x.
- 1x = 8 real minutes / LifeLens day.
- 4x = 2 minutes/day.
- 16x = 30 seconds/day.
- 64x = 7.5 seconds/day.
- History = adaptive long-run mode.

Dagyeom Observer may implement the control surface after T1 exposes the authoritative runtime contract.
Do not directly scale Core truth from UI tick rate.

### Hydrology -> Water presentation projection — provider ready

Jjun bridge now exposes deterministic, read-only local-surface water presentation observations for materialized chunks:

- authoritative `SurfaceWaterId`, kind and salinity.
- stable chunk-center grid anchor.
- downstream center target for spring/stream/river where Core exposes downstream.
- deterministic suggested channel width / area radius derived from authoritative kind, availability and flow.

These are **presentation hints**, not new water simulation truth. Dagyeom may use them to build Unreal Water splines/bodies without inventing a second hydrology layout. Gameplay drinking, resource availability, flow and salinity remain Core authority.

### Calendar / Environment / Sky presentation projection — provider ready

Available provider state:
- authoritative time of day / day / year / annual phase / season.
- authoritative daylight/night summary.
- deterministic local temperature / precipitation / cloud / wind / humidity / visibility / wetness.
- `FLLCoreSkyPresentationObservation` rendering hints for initial-region SkyAtmosphere consumers:
  - sun elevation / azimuth.
  - sun intensity.
  - sky brightness.
  - cloud / fog / wind / surface wetness.

The sky DTO is a deterministic **presentation projection** of Core time + weather, not a second astronomy/weather simulation. Dagyeom should drive sun/sky/fog/material parameters from it rather than reconstructing a competing clock or weather model.

---

# Next work by lane

## Jjun Core / World lane

Active after the 2026-09-19 Presentation handoff:

1. **C1-D Durable Subsistence** — water carrying/storage, food storage/spoilage, cultivation and renewable food production.
2. **C1-E Emergent Settlement Form**.
3. **C1-F Early Material Progression**.
4. **Stage D Long-Run Reliability + Open-Ended Civilization Engine**.
5. Health / education / economy / institutions / migration / trade.
6. Historical -> industrial -> modern -> digital -> AI -> advanced energy/biotech -> space -> open future.

T1/E1/E2/E3 foundations are already integrated; do not restart them as if pending.

Canonical architecture:

- `docs/DEVELOPMENT_MILESTONES.md`
- `docs/TIME_AND_DYNAMIC_ENVIRONMENT.md`
- `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`

## Dagyeom Presentation lane

Presentation is explicitly handed back to Dagyeom from the frozen #282 main baseline.

Immediate review set:
1. #281 Character animation polish.
2. #283 Lighting / atmosphere polish.
3. #284 Observer UI / camera polish.
4. runtime screenshot review of the latest main before accepting further visual changes.
5. future environment/character/UI quality work as provider contracts permit.

Parallel execution is expected. Dagyeom work does not block unrelated Jjun Core work unless an actual Integration Request is opened.

## Android / device lane

**Gate B remains PAUSED BY USER.**

Do not start Android build/seed because roadmap docs changed.
When explicitly resumed, prefer cached/fast smoke paths before long engine builds.

---

## Recently completed coordination items

### PR #251 — Authoritative Unreal Water consumer v1 — DONE

- merged after exact-head Structural Preflight + Unreal Linux Compile PASS.
- Core Hydrology presentation DTO now drives runtime Unreal Water river/lake/wetland projection.
- spawned water remains presentation-only; collision/navigation and gameplay authority stay outside Unreal Water.
- coast/ocean remains deferred until planetary coastline geometry is sufficiently explicit.
- ASSIST_LOCK-GFX-WATER-CONSUMER-1 released.


### PR #134 — PR #100 WorldPresentation current-main reapplication — DONE

- owner/integration: Jjun.
- changed files: only current WorldPresentation source pair.
- stale coordination docs intentionally excluded.
- Preflight #723 PASS.
- Unreal Linux Compile #207 PASS.
- merged `1195fddbaf3341ac9508347d07fab69740f02482`.
- original #100 superseded.

### PR #132 — Emotion Runtime Integration v1 — DONE

- Need pressure/resolution/failure/civilization outcomes integrated.
- production external physical relief path included.
- authoritative outcome semantics preserved.

### PR #130 — Knowledge Transmission Spatial Authority v1 — DONE

- merged `fb1842ad60c25f0054eb040f46d757340f65991c`.
- closed remote witness/teaching telepathy.

### PR #128 — Mac editor build unblocker — DONE

- minimal `-Wshadow` rename only.

### ASSIST_LOCK-LIFECYCLE-PRESENTATION-1 — RELEASED

- Character ownership returned to Dagyeom after #125.

---

## Recently resolved Integration Requests

- IR-D typed Context Action consumer — RESOLVED through #103/#111/#116/#117 and extended by #130 teaching contract.
- IR-B character facing — RESOLVED by #102.
- IR-A WorldPresentation owner path — RESOLVED.
- IR-C production map + observer framing — RESOLVED by #96.


## 2026-09-21 Active World v2 coordination

### Jjun — W2-0 provider contract foundation — ACTIVE

- branch: `jjun/world-v2-contract-foundation`.
- owner scope:
  - `Source/LifeLens/Simulation/**`
  - `.github/workflows/preflight.yml`
  - canonical docs/state.
- purpose:
  - generic Planet/SurfaceRegion/Chunk logical address projection.
  - observer-centered deterministic terrain preview without Core materialization.
  - legacy initial-region preview remains a compatibility wrapper.
- Presentation/Environment-owned source/content is not modified in W2-0.
- before W2-4 terrain/water/vegetation presentation migration, open a fresh Integration Request / scoped Assist Lock for Dagyeom-owned `Source/LifeLens/WorldPresentation/**` and Content paths.

## 2026-09-22 Work Mode — Web runtime resilience scope (released: #422 merged)

- Owner: Jjun / this Work Mode session.
- Branch: `work/web-runtime-resilience-20260922`.
- Reserved files: `web/src/runtime/core-bridge.ts`, new `core-response.ts`, new
  `runtime-loading.ts`, `web/tests/runtime-resilience/**`, and new
  `.github/workflows/web-runtime-resilience.yml`.
- Scope: bounded runtime loading, initialization fallback, mandatory response validation.
- Active PR #421 camera/clock/Core locomotion/actor files and #417 release workflows excluded.
- No automatic merge or deployment. Handoff: `tasks/WEB_RUNTIME_RESILIENCE_2026-09-22.md`.

## 2026-09-25 Work Mode — Resident continuity / review pending

- Owner: Jjun / Work Mode.
- Branch: `work/web-resident-continuity-20260925`.
- Scope: `web/src/runtime/resident-continuity.ts`, new
  `web/tests/runtime-resilience/continuity.mjs`, `.github/workflows/web-runtime-resilience.yml`.
- Excluded: Chat actor/weather/tree rendering, camera, clock, Core simulation and runtime release lanes.
- Checkpoint and resumption: `tasks/WEB_CONTINUITY_HANDOFF_2026-09-25.md`.
- No automatic main merge or deployment.
