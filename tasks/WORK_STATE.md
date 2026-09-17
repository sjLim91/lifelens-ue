# LifeLens Canonical Work State

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
> Canonical roadmap: `docs/DEVELOPMENT_MILESTONES.md`.
> Ownership / locks / IR: `tasks/TEAM_BOARD.md`.
> Point-in-time audit: `docs/INTEGRATED_AUDIT_2026-09-17.md`.

Last reconciled: **2026-09-17 KST after PR #130 merge**.

## Current main baseline

Current `main`: `fb1842ad60c25f0054eb040f46d757340f65991c`.

Recent completed checkpoints:
- #115 Lifecycle Core Correctness v1 — DONE.
- #116 Action Completion Unification v1 — DONE.
- #117 Social Communication & Localization v1 — DONE.
- #118~#122 Facilities / Tools / Fire / Furnace / Copper — DONE.
- #123 World / Facility / Obstacle Authority Normalization — DONE.
- #124 Legacy Authority Removal v1 — DONE.
- #125 Lifecycle Presentation v1 — DONE.
- #126 Social speech-bubble readability — DONE, merge `32ef9e31eb3af9c47d08ac78f591c0bf60f3421a`.
- #127 Integrated audit / canonical roadmap reconciliation — DONE, merge `44fe94efaed01ca5fd4250d5f17d9bcbf27b2699`.
- #128 Mac editor `-Wshadow` unblocker — DONE, merge `7aaac203af35ab806ee8dcebf49bacc13a22b08a`.
- #130 Knowledge Transmission Spatial Authority v1 — DONE, merge `fb1842ad60c25f0054eb040f46d757340f65991c`.

Authority rule:

> Core / World owns simulation truth. UI / Character / WorldPresentation consumes authoritative contracts and must not invent outcomes.

## #130 closed — Knowledge Transmission Spatial Authority v1

Delivered:
- technique witness eligibility is limited to residents within 2 Core grid cells of the completed civilization work position;
- hourly remote `teachTechnique()` application is removed;
- `ContextActionKind::KnowledgeTeaching` carries teacher, learner and technique through the same completion-aware interaction chain;
- external physical execution requires teacher movement to the learner, talking presentation and Core ACK before teaching applies;
- World follows the live learner actor while moving, then Core revalidates runtime proximity at ACK;
- wrong token and far ACK are rejected;
- target and technique are revalidated at completion;
- pending teaching is not persisted, so Save/Load cannot replay stale teaching;
- headless simulation still completes through the same ContextAction completion path for deterministic tests.

Exact-head validation for `3d42c407140266557ba52256af28e4ccb82dca93`:
- Preflight #715 — PASS.
- Core Tests #664 — PASS.
- Unreal Linux Compile #203 — PASS.

## Dagyeom work

### PR #100 — World Readability Envelope — ACTIVE / FINAL CLOSEOUT

Branch: `dagyeom/world-visual-readability-envelope`.
Current head: `62b191a6afb2c0c5ae32ddbb1e0ae7876ae00556`.

Current status:
- mergeable with current repository state at last check;
- exact-head Preflight #707 — PASS;
- exact-head Unreal Linux Compile #200 — PASS;
- Mac editor rebuild blocker was removed by merged PR #128;
- remaining closeout is Dagyeom Mac PIE visual confirmation / final merge decision;
- WorldPresentation-only; Core resource/action authority is unchanged.

Do not merge merely to clear the queue if Dagyeom still needs the final visual check.

### PR #98 — STALE / SELECTIVE SALVAGE ONLY

Do **not** merge this old branch wholesale.
Still-useful ideas may be reimplemented on latest main:
- Observer panel readability tuning;
- QA-only resident framing helpers;
- true Observer Detail scrolling remains a current gap.

## Administrative state

Canonical document reconciliation is complete through #130.
- `ASSIST_LOCK-LIFECYCLE-PRESENTATION-1` — RELEASED.
- Current Assist Lock count — 0.
- Open Integration Requests — 0 at last checkpoint.
- Android Gate B — **PAUSED BY USER**; keep it in the roadmap but do not run Android builds until explicitly resumed.
- old PR #129 is superseded by the post-#130 live-state reconciliation branch and should not be merged independently.

## Unified next work order

One roadmap, parallel ownership lanes:

1. **Dagyeom #100 closeout** — final Mac PIE confirmation, then merge if visually acceptable.
2. **Character Context Motion v2 — Dagyeom** — consume authoritative ContextAction targets; improve PickUp/Carry, Dig/Strike, Craft/Build/Fire/Smelt, Parenting, sanitation, sitting/lying only where real facilities support it.
3. **Emotion Runtime Integration v1 — Jjun** — wire existing Core emotion model to real Needs, outcome, environment, family/loss and civilization events; bounded utility effects only.
4. **Observer Readability + Real Scrolling — Dagyeom** — current-main implementation, selective #98 salvage only.
5. **Lifecycle Event Presentation v2 — Dagyeom** — birth/growth/death/history visibility from Core DTOs.
6. **Civilization Phase 2 — Jjun -> Dagyeom** — SleepingPlace, Shelter, WorkSurface, facility effects, Tin/Bronze progression; no free infrastructure.
7. **Cleanup + long-run performance — Jjun** — legacy source/check cleanup, catch-up frame budget, residue/HISM profiling.
8. **Health / disease / premature mortality**.
9. **Migration / multiple settlements / economy / society**.
10. **Android Gate B — PAUSED** until explicit user resume.

## Cross-lane coordination checkpoints

Jjun checks Dagyeom-side open PRs, new comments/review requests and `TEAM_BOARD` Integration Requests:
- before starting a new Jjun work unit;
- after completing or merging a Jjun work unit;
- when entering a long Unreal/CI wait;
- when returning to handle a long Unreal/CI result;
- before a main-changing merge or rebase.

A new Dagyeom blocker is triaged before unrelated follow-up work. This is independent from build polling: never repeatedly query or restart a healthy long build merely to perform coordination checks.

## Open audit findings

### P1 correctness / integration
- Character Context Motion is still generic for many concrete actions;
- Observer Detail overflow is not true scrolling;
- Emotion exists but is not yet causally wired across the full runtime;
- lifecycle birth/growth/death events need richer observer presentation.

Knowledge transmission remote-telepathy defect is **closed by #130**.

### P2 cleanup / performance
- environmental residue presentation refresh cost needs long-run profiling;
- long catch-up stepping needs an explicit per-frame budget;
- `SimulationSnapshotCodecLegacy.cpp` remains dead/uncompiled source;
- structural Preflight still contains some compatibility markers for removed legacy surfaces.

### P3 later depth
- health/disease/pathogens;
- richer facilities/resources/material progression;
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

Exact-head validation remains mandatory before functional merge.
