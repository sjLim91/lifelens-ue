# LifeLens Canonical Work State

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
> Long-term order: `docs/DEVELOPMENT_MILESTONES.md`.
> Durable design decisions: `docs/DECISION_LOG.md`.

Last reconciled: 2026-09-15 KST after PR #96 merge.

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

Status: `READY_NOW — HARDCODING CLEANUP B`

Primary next milestone:
- introduce pure-C++17 immutable/versioned `SimulationRuleset`.
- externalize Needs decay and UtilityAI tuning first.
- wire Simulation to consume the ruleset without introducing a global mutable singleton.
- align snapshot/current save structure as needed for the ruleset contract.

Pre-release cleanup rule:
- there is no shipped user save population yet.
- do not add backward-migration code for development-only save/snapshot formats.
- remove legacy save/snapshot compatibility code when it directly obstructs or would make the current ruleset/save architecture more expensive later.
- do not expand this milestone into unrelated projection/UI cleanup; Social/Fun compatibility belongs to its planned later cleanup unless it becomes a direct blocker.

## Dagyeom lane

Status: `SYNC FROM AGENTS.md / NO OPEN INTEGRATION REQUESTS`

Dagyeom/Claude should start by reading latest `AGENTS.md`, including `docs/DECISION_LOG.md`, then reconcile with actual GitHub before new work.

## Active blockers / locks

- Formal Integration Requests: 0 open after #90/#91/#96 resolution.
- Assist locks: 0 known active.
- #96 Dagyeom review is not required and must not be re-requested mechanically.

## Long compile rule

When a long UE compile/package is started, record HEAD + Run ID and continue safe independent work. Do not continuously poll. A successful compile is evidence for integration correctness, while visual quality still requires its own appropriate validation.