# LifeLens Canonical Work State

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
> Long-term order: `docs/DEVELOPMENT_MILESTONES.md`.
> Durable design decisions: `docs/DECISION_LOG.md`.

Last reconciled: 2026-09-15 KST after Hardcoding Cleanup B dispatch.

## Recently closed product checkpoints

### World Visual Milestone A — DONE
- Dagyeom presentation/content delivery merged through PR #90.
- Production map asset: `/Game/Maps/LifeLensWorld`.
- deterministic WorldPresentation consumes Core/World read contracts; it does not own simulation authority.
- start-region readability rule is presentation-only and does not hide authoritative resource patches.

### World visual/runtime integration — DONE
- supporting integration fixes merged through #91/#92/#96.
- PR #96 final product head: `2642e72efbc69a01c5f58cff08683b1cea793087`.
- Preflight #551 (`34970785808`): PASS.
- Unreal Linux Compile #103 (`34970785878`): PASS.
- PR #96 squash merge: `60432fd102fed327d9e7ae8b4c3ad8727aff476c`.
- production default/startup map now points to `/Game/Maps/LifeLensWorld`.
- observer camera tuning is Config-backed and defaults to 1.25 chunk distance / 2.0 chunk height / 100 UU target / 55 FOV.
- stale bootstrap validator expectation for `/Engine/Maps/Entry` was updated to the production map contract.

## Current validation risk

`OPEN VISUAL QA RISK — NOT A CODE/COMPILE BLOCKER`

The #96 compile proves C++/UHT/UBT integration, not camera aesthetics. The next actual PIE/APK visual pass should confirm:
- `LifeLensWorld` opens as production map.
- founders are visible and camera is not inside canopy.
- generated-world presentation appears correctly.
- authoritative resource patches remain readable.
- character facing/orientation remains correct.
- Android framing is acceptable.

If only framing is poor, tune `Config/DefaultGame.ini` first rather than changing Core spatial authority.

## Jjun lane

Status: `ACTIVE — HARDCODING CLEANUP B / PR #99`

Branch: `jjun/hardcoding-cleanup-b-ruleset`
PR: #99 `[CORE] Hardcoding Cleanup B — immutable SimulationRuleset`
Validation at dispatch:
- Core Tests #469 (`34974317432`): queued.
- Preflight #558 (`34974317413`): queued.
- Unreal Linux Compile #105 (`34974317541`): queued because Save/Bridge C++ interfaces changed.

Milestone scope:
- introduce pure-C++17 immutable/versioned `SimulationRuleset`.
- externalize Needs decay and UtilityAI tuning first while preserving existing default behavior.
- wire Simulation to consume a const per-instance ruleset without a global mutable singleton.
- persist full ruleset values with Core snapshots and restore under the same immutable rules.
- simplify snapshot persistence to current pre-release format only.
- remove Unreal SaveGame v1 replay migration and legacy payloads because no product build has been shipped to users.

Explicitly deferred from this milestone:
- Social/Fun projection compatibility cleanup remains Cleanup D.
- LifeStage / Relationship / Romance / Pregnancy / Family tuning migration remains Cleanup C.
- Character/UI/WorldPresentation presentation behavior is not part of #99.

Pre-release cleanup rule:
- there is no shipped user save population yet.
- do not add backward-migration code for development-only save/snapshot formats.
- remove legacy save/snapshot compatibility code when it directly obstructs or would make the current ruleset/save architecture more expensive later.
- do not expand this milestone into unrelated projection/UI cleanup.

## Tracked implementation gaps — DO NOT DROP

### Emotion runtime integration gap

Observed during PIE review on 2026-09-15: the resident Emotion detail UI can legitimately show all `0%` values because founders currently begin with neutral `EmotionState` and many ordinary life events do not yet drive emotion changes.

This is **not currently treated as a UI rendering defect**. The Core read/bridge/UI path exposes the values; the simulation-side event coverage is incomplete.

Required follow-up:
- keep neutral-at-start semantics unless product design later decides otherwise; do not fill founder emotions with arbitrary random noise just to avoid zeros.
- connect ordinary life/survival events to emotion where causally appropriate: unresolved hunger/thirst/fatigue/bladder/hygiene pressure, need relief, environmental hazard/contamination exposure, successful gathering/crafting/work, repeated failure/frustration, threat/loss, and other meaningful outcomes.
- preserve event-driven causality: Emotion must reflect what happened to the resident, not UI-fabricated values.
- audit Master Spec emotion coverage against `EmotionState`: the code comment claims the Master Spec emotion model, but the current state exposes only the presently implemented dimensions. Close the gap to the canonical spec rather than silently accepting the partial set.
- add Core regression coverage showing non-social daily-life events can produce/decay emotion and that Observer projection reports the authoritative values.

Scheduling:
- do **not** expand PR #99 Cleanup B to implement this.
- keep this gap visible for a dedicated Emotion/Life-event integration slice after the current ruleset cleanup sequence, or pull it forward only if it becomes a direct blocker for the next simulation milestone.

## Dagyeom lane

Status: `SYNC FROM AGENTS.md / NO OPEN INTEGRATION REQUESTS`

Dagyeom/Claude should start by reading latest `AGENTS.md`, including `docs/DECISION_LOG.md`, then reconcile with actual GitHub before new work.

## Active blockers / locks

- Formal Integration Requests: 0 open after #90/#91/#96 resolution.
- Assist locks: 0 known active.
- #99 is Jjun Core/Simulation/Save/Bridge scope; no Dagyeom review is requested by default.

## Long compile rule

When a long UE compile/package is started, record HEAD + Run ID and continue safe independent work. Do not continuously poll. A successful compile is evidence for integration correctness, while visual quality still requires its own appropriate validation.