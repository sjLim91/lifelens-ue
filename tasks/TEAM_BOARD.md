# LifeLens Team Board

이 파일은 **ownership / active locks / Integration Requests / cross-lane coordination**만 기록한다.

- live execution state: `tasks/WORK_STATE.md`
- canonical roadmap: `docs/DEVELOPMENT_MILESTONES.md`
- time / speed / environment contract: `docs/TIME_AND_DYNAMIC_ENVIRONMENT.md`
- long-range civilization direction: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`
- integrated audit: `docs/INTEGRATED_AUDIT_2026-09-17.md` — historical point-in-time evidence

---

## Ownership

| Lane | Owner | Default scope |
|---|---|---|
| Core / AI / Simulation / World / Save / Bridge | Jjun | `Source/LifeLensCore/**`, `Source/LifeLens/AI/**`, `Source/LifeLens/Simulation/**`, `Source/LifeLens/World/**` |
| Build / CI / Android / Config | Jjun | `.github/workflows/**`, build pipeline, `Config/**`, startup/default map, project integration |
| UI / Observer | Dagyeom | `Source/LifeLens/UI/**`, `Content/UI/**` |
| Character presentation | Dagyeom | `Source/LifeLens/Characters/**`, `Content/Characters/**` |
| Environment visual / maps / WorldPresentation | Dagyeom | `Source/LifeLens/WorldPresentation/**`, `Content/Environment/**`, `Content/Maps/**`, `Content/WorldPresentation/**` |

Authority rule:

> Core / World owns simulation truth. Presentation consumes authoritative read/action contracts and never invents resources, facilities, outcomes, lifecycle state, emotion, knowledge, time, weather or technology.

Time/environment ownership clarification:

- Jjun owns authoritative simulation time, calendar, season/weather state and gameplay effects.
- Dagyeom owns sun/sky/weather VFX, environment readability and Observer controls/readouts.
- Dagyeom must consume provider contracts rather than calculate a second time/weather truth.

---

## Collaboration model

LifeLens uses **one integrated roadmap with parallel ownership lanes**.

Rules:

- Jjun defines Core/World truth and provider contracts.
- Dagyeom consumes those contracts for Character/UI/WorldPresentation.
- each owner stays in their lane by default.
- direct cross-owner edits require an Integration Request or explicit scoped Assist Lock.
- Jjun does not push directly to `dagyeom/*` branches.
- stale branches are not merged wholesale; valid missing ideas are reconstructed from latest `main`.
- new civilization/time/environment systems expose provider contracts before presentation depends on them.
- Dagyeom-owned work may merge independently after CI if it does not conflict with active provider ownership.
- conflict resolution compares both sides; never overwrite current main blindly.

### Cross-lane request checkpoint cadence

Jjun checks Dagyeom-side open PRs, new comments/review requests and new Integration Requests:

- before starting a new Jjun functional work unit;
- after completing or merging a Jjun work unit;
- when entering a long CI / Unreal build wait;
- when returning to act on a long CI / Unreal result;
- before a main-changing merge or rebase.

A new blocker is triaged before unrelated follow-up work. This does **not** mean repeatedly polling or restarting a healthy long build.

---

## Current Assist Locks

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
- cross-owner files:
  - `Source/LifeLens/WorldPresentation/LLPCGGroundCoverPresentationActor.cpp`
  - `Source/LifeLens/WorldPresentation/LLPCGGroundCoverPresentationActor.h`.
- scope:
  - consume `PCG_LL_GroundCover` on Windows/macOS desktop runtime.
  - seed from authoritative initial-chunk `VisualSeed`.
  - decorative-only; no resource/facility authority.
  - Android does not spawn/link the PCG runtime path.
- release after merge/close and board reconciliation.

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

1. **T1 Simulation Time Authority & Variable Speed**.
2. **E1 Calendar + Day/Night Authority**.
3. **E2 Seasons + Dynamic Weather Core v1**.
4. **E3 Environmental Consequences v1**.
5. **C1 Settlement & Subsistence Foundation**.
6. **C2 Long-Run Scale + History Fast-Forward**.
7. **C3 Open-Ended Civilization Framework v1**.
8. Health / Disease / Population Resilience.
9. Education / Recording / Specialization / Economy / Institutions.
10. Migration / Multiple Settlements / Trade Networks.
11. Historical -> industrial -> modern -> digital -> AI -> advanced energy/biotech -> space -> open future.

Canonical architecture:

- `docs/DEVELOPMENT_MILESTONES.md`
- `docs/TIME_AND_DYNAMIC_ENVIRONMENT.md`
- `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`

## Dagyeom Presentation lane

1. **Character Context Motion v2**.
2. **Observer Readability + Real Scrolling**.
3. **Time Controls / date-time readout** after T1/E1 provider lands.
4. **Dynamic Environment Presentation v1** after E1/E2 provider lands.
5. **Lifecycle Event Presentation v2**.
6. settlement/civilization/future presentation consumers as provider contracts land.

Parallel execution is expected. Dagyeom work does not block unrelated Jjun provider work unless an actual IR is opened.

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
