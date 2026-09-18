# LifeLens Canonical Work State

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
>
> Whole-source audit / current P0 fixes: `docs/SOURCE_AUDIT_2026-09-18.md`.
>
> Canonical roadmap: `docs/DEVELOPMENT_MILESTONES.md`.
>
> Long-range civilization direction: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`.
>
> Ownership / locks / IR: `tasks/TEAM_BOARD.md`.
>
> Dagyeom presentation assist handoff: `docs/DAGYEOM_PRESENTATION_HANDOFF_2026-09-18.md`.
>
> Historical audit: `docs/INTEGRATED_AUDIT_2026-09-17.md` (point-in-time only; do not treat as live status).

Last reconciled: **2026-09-18 KST after whole-source audit promotion**.

---

## 1. Current main checkpoint

Current main before this docs-only audit-priority sync:

- main SHA: `87020ec85787f1dfb3d03c30dd8fafaeb7b2aafe`.
- #145 Character Context Motion v2 — MERGED.
- #149 Visual Catch-up v3 packaged rain/snow fallback — MERGED.
- #150 Lifecycle Presentation v2 — MERGED.
- #151 Observer Adaptive Information Density v1 — MERGED.
- #152 C1 Settlement Facility Authority Foundation v1 — MERGED.
- #153 docs reconciliation — MERGED.
- #154 explicit Dagyeom presentation assist handoff — MERGED.

Recent validated functional baseline:
- Core Tests #707 — PASS.
- 67/67 Core tests — PASS.
- deterministic harness — PASS.
- Preflight #774 — PASS.
- Unreal Linux Compile #235 — PASS.

Authority rule remains:

> Core / World owns simulation truth. UI / Character / Environment / WorldPresentation consumes authoritative contracts and must not invent outcomes.

---

## 2. STOP-THE-LINE ACTIVE PRIORITY

### AUDIT-0A — Time / Weather / Speed UI consolidation

Status: **ACTIVE / FIRST**

Problem:
- `ALLRuntimeObserverHUD::DrawRuntimeChrome()` already renders time/weather/speed controls.
- `ULLObserverTimeWeatherOverlay` is also auto-created and renders the same information/control set.
- both can call `ULLSimulationSubsystem::SetSimulationSpeedPreset()`.

Required:
1. choose one canonical production control surface.
2. remove/absorb the duplicate path.
3. keep one input/hit-testing path.
4. keep Core time authority untouched.
5. verify PC + Android interaction.
6. PIE smoke: exactly one time/weather/speed surface.

### AUDIT-0B — Resident-local environmental Need pressure

Status: **NEXT P0, immediately after 0A**

Problem:
- per-minute `applyStartRegionEnvironmentalNeedPressure(world)` applies one start-region environment to every living resident.
- travel/work systems already use actual spatial location, so environment causality is inconsistent.

Required:
1. authoritative resident runtime GridPos.
2. GridPos -> chunk.
3. per-resident DynamicEnvironment.
4. per-resident environmental Need pressure.
5. regression with two residents in different climates.
6. deterministic Save/Load continuation.

### AUDIT-0C — Full regression gate

Status: **BLOCKS C1-B**

After 0A/0B:
- Core Tests.
- deterministic harness.
- Structural Preflight.
- Unreal Linux Compile.
- PIE visual/input smoke checklist.

**Do not start/merge C1-B before AUDIT-0A/B/C are green.**

---

## 3. P1 audit follow-up

### AUDIT-1A — Explicit materialized chunk enumeration

Current WorldPresentation and obstacle collision proxy infer materialized chunk coordinates from:
- `MaterializedChunkCount`.
- a ring around the initial chunk.

This can miss distant/non-contiguous materialized chunks later.

Required before migration/multi-settlement:
- Core bridge exposes authoritative materialized chunk coordinate list.
- WorldPresentation consumes the list.
- obstacle collision proxy consumes the list.
- no count-based coordinate guessing.

This is not ahead of P0 and does not replace C1-B after stabilization.

---

## 4. Recent completed chain

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
- #149 packaged rain/snow fallback.

### Character / Observer / lifecycle
- #143 Character Context Motion v2a.
- #144 Detail Scrolling v1.
- #145 Character Context Motion v2.
- #146 lifecycle live event feed.
- #150 selected-resident persistent family/lifecycle card + observed timeline.
- #151 adaptive information density.

### C1 settlement
- #152 C1-A authority:
  - WorkSurface constructible.
  - SleepingPlace constructible.
  - Shelter constructible.
  - real materials + real work.
  - deterministic sites.
  - weather friction on construction work.
  - snapshot persistence.
  - no free NEW GAME settlement facility.

---

## 5. C1 continuation after audit P0

### C1-B — Autonomous settlement need recognition
1. SleepingPlace utility from sleep/outdoor-rest pressure.
2. Shelter utility from local environmental exposure.
3. WorkSurface utility from repeated craft/build demand.
4. missing project materials -> Gather demand.
5. Plan / DeliverMaterial / Work -> authoritative spatial ContextAction.
6. presentation only executes/visualizes Core directive.
7. same-seed / snapshot continuation deterministic.

### C1-C — Facility effects / maintenance
- sleep benefit.
- shelter environmental protection.
- work/craft benefit.
- durability / maintenance.
- ruined/inactive facilities stop providing benefit.

### C1-D — Water / food persistence
- water handling/storage.
- food storage/spoilage.
- cultivation/renewable production.
- season/moisture/fertility dependency.
- scarcity -> search/movement.

### C1-E — Emergent settlement geometry
- activity centers emerge from use.
- household space differentiation.
- sanitation away from dense living.
- storage/fire/work/sleep clusters emerge.
- no hard-coded town-center authority.

### C1-F — Early material expansion
- Tin/Bronze only after actual prerequisites.

---

## 6. Roadmap after C1

- C2 — long-run scale/history fast-forward/cleanup.
- C3 — open-ended Capability / Technology / Transformation framework.
- C4 — health/disease/population resilience.
- C5 — education/recording/specialization/economy/institutions.
- C6 — migration/multiple settlements/trade networks.
- F1~F8 — historical -> industrial -> modern -> digital -> AI/robotics -> advanced energy/biotech -> space -> open future.

No forced era timer.

---

## 7. Runtime QA not covered by green compile/tests

Still requires runtime/device validation:
- actual HUD z-order/layout/safe area.
- touch gesture conflicts.
- camera feel.
- animation transitions / hand-tool alignment.
- Niagara/material visual quality.
- Android GPU/performance.
- long Unreal session memory/performance.
- APK packaging/device launch.

Android Gate B remains PAUSED BY USER.

---

## 8. Collaboration / merge rule

- Review source as one LifeLens product; ownership is for coordination, not quality silos.
- Correct responsibility beats artificial file separation.
- Same responsibility -> same canonical file.
- coordinate/rebase actual overlaps.
- stale #98 remains selective-salvage only.

Standing Jjun merge rule remains:
- refresh exact head.
- required CI green.
- refresh active PRs/files.
- no unresolved overlap/conflict.
- expected_head_sha on merge.

---

## 9. Immediate next implementation target

> **AUDIT-0A — consolidate duplicate Observer time/weather/speed UI.**

Then:
**AUDIT-0B -> AUDIT-0C -> C1-B**.
