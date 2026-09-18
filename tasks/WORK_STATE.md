# LifeLens Canonical Work State

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
>
> Canonical roadmap: `docs/DEVELOPMENT_MILESTONES.md`.
>
> Long-range civilization direction: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`.
>
> Ownership / locks / IR: `tasks/TEAM_BOARD.md`.
>
> Historical audit: `docs/INTEGRATED_AUDIT_2026-09-17.md` (point-in-time only; do not treat as live status).

Last reconciled: **2026-09-18 KST after PR #152 merge**.

---

## 1. Current main checkpoint

Current main before this docs-only reconciliation:

- main SHA: `6cceafca5233e9ba472a2fee04139ebf41887aa4`.
- #145 Character Context Motion v2 — **MERGED**.
- #149 Visual Catch-up v3 packaged rain/snow fallback — **MERGED**.
- #150 Lifecycle Presentation v2 — **MERGED**.
- #151 Observer Adaptive Information Density v1 — **MERGED**.
- #152 C1 Settlement Facility Authority Foundation v1 — **MERGED**.

#152 exact-head validation:
- Preflight #774 — PASS.
- Core Tests #707 — PASS.
- Unreal Linux Compile #235 — PASS.

Authority rule remains:

> Core / World owns simulation truth. UI / Character / Environment / WorldPresentation consumes authoritative contracts and must not invent outcomes.

---

## 2. Recent completed chain

### Time / environment
- #135 Simulation Time Authority & Variable Speed.
- #136 Calendar + Day/Night.
- #137 Seasons + Dynamic Weather Core.
- #138 Environmental Consequences.
- #139 Dynamic Environment Presentation Foundation.
- #140 Visual Catch-up v2.
- #142 Runtime Chrome.
- #147 Observer time/weather/speed controls.
- #148 Dynamic Observer Canopy Visibility.
- #149 Visual Catch-up v3 rain/snow packaged fallback.

### Character / Observer / lifecycle
- #143 Character Context Motion v2a.
- #144 Detail Scrolling v1.
- #145 Character Context Motion v2.
- #146 lifecycle live event feed.
- #150 selected-resident persistent family/lifecycle card + observed timeline.
- #151 adaptive information density.

### C1 settlement
- #152 C1-A authority:
  - WorkSurface is constructible.
  - SleepingPlace is constructible.
  - Shelter is constructible.
  - all require real materials + real work.
  - deterministic sites avoid existing facility/sanitation/resource collisions.
  - weather friction affects construction work.
  - operational facility state survives snapshot roundtrip.
  - NEW GAME still starts without those facilities.

---

## 3. Active priority

### C1-B — Autonomous settlement need recognition

Next Core-owned implementation target:

1. SleepingPlace utility from sleep/outdoor-rest pressure.
2. Shelter utility from repeated environmental exposure.
3. WorkSurface utility from repeated crafting/build demand.
4. missing project materials feed back into Gather decisions.
5. Plan / DeliverMaterial / Work actions resolve to real Core facility positions.
6. WorldDirector / Character presentation only executes the authoritative pending directive.
7. same seed / snapshot continuation remains deterministic.

Do **not** create a generic "advance settlement era" switch.

---

## 4. C1 continuation after C1-B

### C1-C — Facility effects / maintenance
- SleepingPlace sleep recovery/comfort benefit.
- Shelter weather-protection benefit.
- WorkSurface work/craft efficiency.
- facility durability / maintenance pressure.
- operational vs ruined state affects actual utility.

### C1-D — Water / food persistence
- water carrying/storage.
- food storage/spoilage.
- renewable/cultivated food.
- season/moisture/fertility dependency.
- local scarcity drives search/movement.

### C1-E — Emergent settlement geometry
- repeated use creates activity centers.
- household living space can differentiate.
- sanitation stays outside dense living space.
- storage/fire/work/sleep clusters emerge from use and constraints.
- no hard-coded "town center" authority object is required.

---

## 5. Roadmap after C1

- **C2** — long-run scale/history fast-forward/cleanup.
- **C3** — open-ended Capability / Technology / Transformation framework.
- **C4** — health/disease/population resilience.
- **C5** — education/recording/specialization/economy/institutions.
- **C6** — migration/multiple settlements/trade networks.
- **F1~F8** — historical, industrial, modern, digital, AI/robotics, advanced energy/biotech, space, open future.

F1~F8 are observer/development bands, never time-based Core era gates.

---

## 6. Presentation state

The Presentation Catch-up sprint is no longer the primary blocker.

Completed:
- time/weather/speed chrome and controls.
- detail scrolling on PC and Android.
- lifecycle event visibility.
- persistent selected-resident family/lifecycle presentation.
- adaptive HUD density.
- context motion expansion.
- dynamic canopy readability.
- packaged rain/snow fallback.

Remaining quality work is incremental:
- authored Niagara/material assets.
- higher-quality child/body/lifecycle animation.
- panel opacity salvage from stale #98 where still useful.
- device-specific layout/LOD tuning.
- camera/readability QA.

These should not block C1 Core progression unless a real provider/consumer contract gap is found.

---

## 7. Collaboration / merge rule

- Correct responsibility beats artificial file separation.
- Do not create duplicate classes/sources merely to avoid another person's file.
- Same responsibility -> same canonical file.
- Coordinate, rebase and resolve overlaps instead of bending architecture.

Standing Jjun merge rule:
- refresh PR exact head.
- required exact-head CI must be green.
- refresh Dagyeom active PRs and changed filenames.
- if no unresolved overlap/conflict, merge without another confirmation.
- if overlap/ownership is unresolved, coordinate first.

Current Dagyeom open item:
- #98 Observer readability + QA view — **STALE / SELECTIVE SALVAGE ONLY**.
- do not wholesale merge its old HUD/controller snapshots over current main.

---

## 8. Android state

Android Gate B remains **PAUSED BY USER**.

Known recovery result:
- PR #141 probe confirmed a prebuilt UE 5.6 development host contains Linux compile tooling plus Android platform payload.
- therefore routine Android recovery should use the verified prebuilt-host strategy rather than repeatedly compiling the full Unreal Editor from source.
- #141 itself is stale and should be reimplemented from fresh main if/when Android work resumes.

No long Android build should be launched until the user explicitly resumes it.

---

## 9. Validation policy

Core:
- Core Tests.
- deterministic regression/harness where relevant.
- Structural Preflight.

Unreal C++:
- Structural Preflight.
- Unreal Linux Compile.

Save/time/environment:
- snapshot roundtrip and deterministic continuation as applicable.

Docs-only:
- no heavy Unreal compile intentionally required solely for documentation.

Exact-head validation is mandatory before functional merge.

---

## 10. Immediate next implementation target

> **C1-B — Autonomous Settlement Need Recognition**

Do not jump directly to agriculture, bronze, cities or future technology before residents can autonomously recognize, construct and use the first durable settlement facilities.
