# LifeLens Canonical Work State

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
> Long-term order: `docs/DEVELOPMENT_MILESTONES.md`.
> Detailed 2026-09-17 audit: `docs/INTEGRATED_AUDIT_2026-09-17.md`.
> Ownership/locks/IR: `tasks/TEAM_BOARD.md`.
> Durable design decisions: `docs/DECISION_LOG.md`.

Last reconciled: **2026-09-17 KST after PR #126 merge**.

## Current main baseline

Completed functional checkpoints:
- #115 Lifecycle Core Correctness v1 — DONE.
- #116 Action Completion Unification v1 — DONE.
- #117 Social Communication & Localization v1 — DONE.
- #118 PrimitiveStorage / Facilities-Tools-Technology v1 — DONE.
- #119 Tool Effectiveness + held tool presentation — DONE.
- #120 DiggingStick / StoneHammer — DONE.
- #121 Fire / Heat / FirePit v1 — DONE.
- #122 Furnace / Copper Smelting v1 — DONE.
- #123 World / Facility / Obstacle Authority Normalization v1 — DONE.
- #124 Legacy Authority Removal v1 — DONE.
- #125 Lifecycle Presentation v1 — DONE.
- #126 Social speech-bubble readability — DONE, merged as `32ef9e31eb3af9c47d08ac78f591c0bf60f3421a`.
- #127 Integrated roadmap / audit reconciliation — DONE, merged as `44fe94efaed01ca5fd4250d5f17d9bcbf27b2699` before #126.

Authority rule remains unchanged:

> Core / World owns simulation truth. UI / Character / Environment / WorldPresentation is a read-only presentation consumer and must not create a second authority.

## Administrative state

Integrated documentation reconciliation is **DONE**.

- `docs/INTEGRATED_AUDIT_2026-09-17.md` contains the point-in-time Jjun + Dagyeom audit.
- `docs/DEVELOPMENT_MILESTONES.md` contains the unified roadmap.
- `tasks/TEAM_BOARD.md` contains current ownership and locks.
- `ASSIST_LOCK-LIFECYCLE-PRESENTATION-1` is **RELEASED**.
- Current Assist Lock count: **0**.
- Android Gate B remains in the roadmap but is **PAUSED BY USER**.

The audit document is historical evidence and should not be rewritten for every later PR. This file is the live execution snapshot.

## Dagyeom work

### PR #100 — World Readability Envelope — ACTIVE / CLOSEOUT

Branch: `dagyeom/world-visual-readability-envelope`.

Current status:
- rebased onto the post-#127 main baseline;
- mergeable after resolving the previous `TEAM_BOARD.md` collision;
- latest known exact-head Preflight #707: **PASS**;
- Unreal Linux Compile #200: **running at last reconciliation**;
- WorldPresentation-only; Core target/resource authority unchanged.

Rule:
- merge only after latest exact-head required CI passes;
- do not restart or repeatedly poll a healthy long Unreal compile;
- PIE/device visual feel is separate quality evidence.

### PR #98 — STALE / SELECTIVE SALVAGE ONLY

Branch: `dagyeom/observer-readability-and-qa-view`.

Do **not** merge the old branch wholesale.
Still-useful ideas may be re-applied on current main:
- denser Observer panel readability tuning;
- debug-only resident framing helpers.

Main UI gap still open:
- Observer Detail overflow is not true scrolling.

### PR #126 — Social bubble readability — DONE

- one-file UI change in `LLSocialObserverHUD.cpp`;
- background remains readable over generated vegetation while text retains age fade;
- exact-head Preflight #704 PASS;
- exact-head Unreal Linux Compile #199 PASS;
- approved and merged as `32ef9e31eb3af9c47d08ac78f591c0bf60f3421a`.

## Unified next work order

One roadmap, parallel ownership lanes.

1. **Dagyeom PR #100 closeout** — finish exact-head CI and merge if clean.
2. **Knowledge Transmission Spatial Authority v1 — Jjun** — no remote witness/teaching; require real proximity/meeting and deterministic tests.
3. **Character Context Motion v2 — Dagyeom** — Sit/Stand/Lie/Wake, PickUp/Carry/Use, Dig/Strike, Craft/Build/Fire/Smelt, Parenting, sanitation; consume Core ContextAction truth only.
4. **Observer Readability + Real Scrolling — Dagyeom** — true detail scrolling and selective #98 salvage.
5. **Emotion Runtime Integration v1 — Jjun provider + Dagyeom consumer** — causally wire existing emotion model to Needs, work success/failure, environment, family/loss and civilization outcomes.
6. **Lifecycle Event Presentation v2 — Dagyeom** — birth/growth/death event visibility and deceased/history inspection from Core DTOs.
7. **Civilization Phase 2 — Jjun -> Dagyeom** — SleepingPlace, Shelter, WorkSurface, facility efficiency, Tin/Bronze progression; no free starting infrastructure.
8. **Cleanup + long-run performance — Jjun** — remove dead legacy source/checks, catch-up frame budget, residue/HISM profiling.
9. **Health / disease / premature mortality** — pathogens, contaminated water/soil, illness/recovery, non-age death.
10. **Migration / settlements / economy / society** — exploration, migration, multiple settlements, jobs/trade, institutions/politics, multi-generation stability.
11. **Android Gate B — PAUSED** — resume only on explicit user request.

## Ownership model

### Jjun lane
- `Source/LifeLensCore/**`
- `Source/LifeLens/AI/**`
- `Source/LifeLens/Simulation/**`
- `Source/LifeLens/World/**`
- Save/Load / Bridge / authoritative read contracts
- Build / CI / Android / config

### Dagyeom lane
- `Source/LifeLens/UI/**`
- `Source/LifeLens/Characters/**`
- `Source/LifeLens/WorldPresentation/**`
- corresponding Content UI / Characters / Environment / Maps assets

Cross-owner direct implementation requires an Integration Request or scoped Assist Lock. A file originally authored by the other lane does not override current path ownership when the board clearly assigns that path.

## Open audit findings

### P1 correctness / integration
- civilization knowledge witness/teaching lacks physical meeting requirements;
- Character Context Motion remains generic for many concrete actions;
- Observer Detail overflow is not truly scrollable;
- existing Emotion model is not yet causally wired to the full survival/work/family/environment loop;
- lifecycle birth/growth/death events need richer observer presentation.

### P2 cleanup / performance
- environmental residue presentation rebuild cost needs long-run profiling;
- long catch-up stepping needs explicit per-frame budget;
- `SimulationSnapshotCodecLegacy.cpp` remains dead/uncompiled source;
- some structural Preflight compatibility checks still pin removed legacy names/paths.

### P3 later simulation depth
- health/disease/pathogens;
- richer facilities/resources;
- open-ended material/component artifact progression;
- migration/multiple settlements/economy/society.

## Validation strategy while Android is paused

Core changes:
- Core Tests;
- deterministic harness;
- Preflight.

Unreal C++ changes:
- Preflight;
- Unreal Linux Compile.

Docs-only changes:
- do not start a heavy Unreal compile solely for documentation.

Exact-head rule remains mandatory before merge.

## Long compile rule

When a long UE compile/package is started, record HEAD + Run ID and continue safe independent work. Do not repeatedly poll or restart a healthy long-running Unreal compile. A successful compile is integration evidence; visual/input quality still requires later PIE/device validation.
