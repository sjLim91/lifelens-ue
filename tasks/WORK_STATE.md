# LifeLens Canonical Work State

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
> Long-term order: `docs/DEVELOPMENT_MILESTONES.md`.
> Detailed 2026-09-17 audit: `docs/INTEGRATED_AUDIT_2026-09-17.md`.
> Ownership/locks/IR: `tasks/TEAM_BOARD.md`.
> Durable design decisions: `docs/DECISION_LOG.md`.

Last reconciled: **2026-09-17 KST after integrated Jjun + Dagyeom audit**.

## Current main baseline

Current functional baseline:
- PR #115 — Lifecycle Core Correctness v1 — DONE.
- PR #116 — Action Completion Unification v1 — DONE.
- PR #117 — Social Communication & Localization v1 — DONE.
- PR #118 — PrimitiveStorage / Facilities-Tools-Technology v1 — DONE.
- PR #119 — Tool Effectiveness + held tool presentation — DONE.
- PR #120 — DiggingStick / StoneHammer — DONE.
- PR #121 — Fire / Heat / FirePit v1 — DONE.
- PR #122 — Furnace / Copper Smelting v1 — DONE.
- PR #123 — World / Facility / Obstacle Authority Normalization v1 — DONE.
- PR #124 — Legacy Authority Removal v1 — DONE.
- PR #125 — Lifecycle Presentation v1 — DONE, merged as `e6e005ba1180a4152476ab9b7a19ad9953c07287`.

Authority rule remains unchanged:

> Core / World owns simulation truth. UI / Character / Environment / WorldPresentation is a read-only presentation consumer and must not create a second authority.

## What #125 closed

Lifecycle Presentation v1 now provides:
- living-only physical compatibility projection;
- deceased actor/runtime/reservation cleanup through existing WorldDirector reconciliation;
- LifeStage-driven body scale synchronization;
- capsule radius/half-height synchronization;
- ground-preserving capsule resize;
- identity/genetics/appearance seed preservation across growth.

Known v1 limitation:
- Quaternius adult skeleton is uniformly scaled for younger stages. This is functional stage presentation, not final child anatomy quality.

## Current active external work

### Dagyeom PR #100 — World Readability Envelope — ACTIVE

Branch: `dagyeom/world-visual-readability-envelope`.

Status:
- valid current-main work; base is #125 main.
- WorldPresentation-only.
- Core target/resource authority unchanged.
- replaces one simple start clear radius with a settlement readability envelope.

Rule:
- merge only after latest exact-head required CI passes.
- PIE/device visual feel is separate quality evidence, not a Core authority blocker.

### Dagyeom PR #98 — STALE / DO NOT MERGE AS-IS

Branch: `dagyeom/observer-readability-and-qa-view`.

Status:
- old base; do not merge the branch wholesale.
- selectively salvage valid ideas on current main only.

Still-useful ideas:
- denser Observer panel readability tuning.
- debug-only resident framing helpers.

Main gap that still remains:
- Observer Detail overflow is not real scrolling.

## Android / device lane

Status: **PAUSED BY USER — 2026-09-17**.

The Gate B roadmap remains valid, but no Android seed/full/fast build should be started until the user resumes this lane.

Do not delete the gate from the roadmap. Do not burn long CI time on it while paused.

## Unified next work order

This is one integrated roadmap. Jjun and Dagyeom keep separate ownership lanes but do not maintain separate schedules.

### 0. Canonical docs reconciliation — CURRENT ADMINISTRATIVE CLOSEOUT

- update `WORK_STATE` through #125;
- update `DEVELOPMENT_MILESTONES` through #125;
- release `ASSIST_LOCK-LIFECYCLE-PRESENTATION-1`;
- record integrated audit findings;
- establish one parallel execution order.

### 1. Dagyeom PR #100 closeout

- exact-head CI pass;
- review/merge if clean;
- no Core changes.

### 2. Knowledge Transmission Spatial Authority v1 — Jjun

Current defect:
- technique witness/teaching can occur without a real physical encounter.

Required:
- discovery witness requires spatial proximity/encounter;
- teaching requires real approach/meeting/context completion;
- no remote settlement telepathy;
- deterministic tests.

### 3. Character Context Motion v2 — Dagyeom

Current state:
- Core/World ContextAction target + ACK chain is authoritative and merged.
- presentation modes are still broad `Interact/Gather/Build` fallbacks.

Next:
- Sit/Stand/Lie/Wake;
- PickUp/Carry/Use;
- Cut/Chop/Dig/Strike;
- Craft/Build/Fire/Smelt;
- Parenting care;
- sanitation interactions;
- use existing Quaternius animations before adding new assets.

### 4. Observer Readability + Real Scrolling — Dagyeom

- true detail scrolling;
- generated-world panel readability;
- selective #98 salvage only;
- keep debug camera helpers separate from production observer framing.

### 5. Emotion Runtime Integration v1 — Jjun provider + Dagyeom consumer

Core already owns the emotion model. Connect real events:
- need pressure/relief;
- success/failure/frustration;
- environment threat/contamination;
- parenting/family;
- lifecycle loss/grief;
- civilization discovery/craft outcomes.

UI must not fabricate emotion.

### 6. Lifecycle Event Presentation v2 — Dagyeom

#125 already handles physical death removal and growth scale.

Still needed:
- birth/growth/death event visibility;
- deceased/history inspection via Core observer/history DTOs;
- optional truthful death animation presentation.

### 7. Civilization Phase 2 — Jjun -> Dagyeom

Existing progression is stable through PrimitiveStorage / FirePit / Furnace / Copper.

Next candidates:
- SleepingPlace;
- Shelter;
- WorkSurface;
- facility-driven Needs/work efficiency;
- TinOre;
- Bronze / bronze-tool progression.

No free starting infrastructure.

### 8. Cleanup + long-run performance — Jjun

- remove dead `SimulationSnapshotCodecLegacy.cpp` when compatibility checks are updated;
- remove obsolete Preflight legacy markers/assertions;
- add explicit per-frame catch-up simulation budget;
- profile residue/HISM refresh cost during long runs.

### 9. Health / disease / premature mortality

- pathogens;
- contaminated water/soil;
- sanitation-linked health;
- illness/recovery;
- non-age premature death.

### 10. Migration / settlements / economy / society

- exploration and carrying-capacity pressure;
- household migration;
- multiple settlements;
- jobs/economy/trade;
- social institutions/politics;
- multi-generation society stability.

### 11. Android Gate B — PAUSED

Resume only on explicit user request.

## Ownership model

### Jjun lane
- `Source/LifeLensCore/**`
- `Source/LifeLens/AI/**`
- `Source/LifeLens/Simulation/**`
- `Source/LifeLens/World/**`
- Save/Load / Bridge / read contracts
- Build/CI/Android/config

### Dagyeom lane
- `Source/LifeLens/UI/**`
- `Source/LifeLens/Characters/**`
- `Source/LifeLens/WorldPresentation/**`
- corresponding Content/UI/Characters/Environment/Maps assets

Cross-owner work requires an Integration Request or scoped Assist Lock before direct implementation.

## Full-source audit findings still open

### P1 correctness/integration

- civilization knowledge witness/teaching lacks physical meeting requirements;
- Character Context Motion remains generic for many concrete actions;
- Observer Detail overflow is not truly scrollable;
- existing Emotion model is not yet causally wired to the full survival/work/family/environment loop;
- lifecycle birth/growth/death events need richer observer presentation.

### P2 cleanup/performance

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
- do not start a new heavy Unreal compile solely for documentation.

Exact-head rule remains mandatory before merge.

## Long compile rule

When a long UE compile/package is started, record HEAD + Run ID and continue safe independent work. Do not repeatedly poll or restart a healthy long-running Unreal compile. A successful compile is integration evidence; visual/input quality still requires later PIE/device validation.
