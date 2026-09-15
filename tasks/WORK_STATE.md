# LifeLens Canonical Work State

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
> Long-term order: `docs/DEVELOPMENT_MILESTONES.md`.
> Durable design decisions: `docs/DECISION_LOG.md`.

Last reconciled: 2026-09-15 KST after Hardcoding Cleanup B merge, IR-B resolution/#102 merge, and Context Action Contract v1 PR dispatch.

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
- production default/startup map points to `/Game/Maps/LifeLensWorld`.

### Hardcoding Cleanup B — DONE
- PR #99 `[CORE] Hardcoding Cleanup B — immutable SimulationRuleset` merged.
- final PR head: `3d65f79a2d16837ad1cb1b6a944bb73725ea167a`.
- Core Tests #480: PASS.
- Preflight #574: PASS.
- Unreal Linux Compile #116 (`34977769413`): PASS.
- squash merge: `1ca6db8f276cf01211a8ce8c023d2e8d8107d6cd`.
- Needs / UtilityAI tuning now flows `DefaultGame.ini -> UE Config -> immutable Core SimulationRuleset -> Simulation`.
- current snapshot/save format is pre-release current-only; old development-only migration paths are not product requirements.

### Character facing / backwards-walk IR-B — DONE
- PR #102 `[Character] IR-B 후진 보행 수정 — 메시 정면 축 보정 +90 → -90` merged.
- imported skeleton measurement proved Quaternius visual forward is local `+Y`; presentation yaw correction therefore needs `-90` against Unreal +X actor/world forward.
- `ll.DebugMotion` A/B evidence: old `+90` -> `facing=-1.00`, corrected `-90` -> `facing=+1.00` on all sampled moving frames.
- PIE confirmed residents face the travel direction; the reproduced backwards-walk issue is resolved.
- Preflight #577: PASS.
- Unreal Linux Compile #117 (`34979601112`): PASS.
- squash merge: `d3c87996499b353c742ce97fd90978633b9b7ca0`.
- remaining stationary-target and male/female/outfit checks are ordinary visual regression QA, not an open IR-B blocker.

### Android Fast Pipeline recovery — MERGED / RUNTIME VALIDATION PENDING
- PR #101 recovered the `seed / fast / full` workflow and merged as `fe97884f3f2bcf71b5a508cdce2e2ca8f42d912c`.
- stale PR #2 was closed unmerged after its obsolete Core bridge was intentionally excluded.
- first manual `seed` Run #3 (`34979129395`) was `in_progress` at the last live check; do not infer success until the run is checked again.
- after a successful seed, normal APK validation should use `fast`; do not repeat a multi-hour seed after a failure without inspecting the failed stage.

## Current validation risk

`OPEN VISUAL QA RISK — NOT A CODE/COMPILE BLOCKER`

The compile gates prove C++/UHT/UBT integration, not every presentation variant. The next actual PIE/APK visual pass should confirm:
- `LifeLensWorld` opens as production map.
- founders and generated-world presentation are readable.
- authoritative resource patches remain visible.
- corrected facing remains valid for stationary interaction targets and male/female/outfit variants.
- Android framing is acceptable.

## Jjun lane

Status: `ACTIVE — CONTEXT ACTION CONTRACT v1 / PR #103`

Branch: `jjun/context-action-contract-v1`
PR: #103 `[CORE] Context action contract v1 — civilization presentation read data`
Head: `3c18f4b94ff906f857cebdeeca163b8e6242b3ae`

Scope:
- expose actual executed civilization work to presentation without inventing actions.
- current authoritative civilization action set is exactly `Gather / Store / Experiment / Craft`.
- keep the short-lived work context in `Simulation::Runtime`, intentionally outside snapshot persistence so save/load cannot replay stale work animations.
- project typed action/result data through `FLLCoreActionDirective`: material, item, technique, quantity, result, stable resource/storage IDs, action minute, and an authoritative spatial target only when Core really owns one.
- sanitation-site creation/improvement may expose its real grid position; ordinary resource/storage actions currently must not fabricate world positions.
- Character presentation files are not modified in this Jjun branch.

Validation at last live check:
- Core Tests #490 (`34986684474`): PASS.
- Preflight #587 (`34986684197`): PASS.
- Unreal Linux Compile #118 (`34986684115`): IN_PROGRESS.
- no Dagyeom review request; handoff is through typed contract + TEAM_BOARD IR-D.

## Tracked implementation gaps — DO NOT DROP

### Civilization resource/storage spatialization gap

Current Core `ResourceNode` and `StorageSite` records have stable IDs and authoritative inventory/material state but no authoritative `GridPos`.

Consequences / rule:
- Context Action Contract v1 may expose the stable resource/storage ID but sets no spatial target for ordinary Gather/Store.
- Character/World Presentation must not choose an arbitrary nearby tree, rock, or container and pretend it is the authoritative target.
- sanitation-site work is an exception when Core actually supplies the real site position.
- add resource/storage spatial authority in a later Core/World slice before scenery-specific gather/store approach/alignment is considered truthful.

### Emotion runtime integration gap

Resident Emotion can remain all `0%` because founders begin neutral and many ordinary life events still do not drive emotion changes.

Required follow-up:
- keep neutral-at-start semantics; do not random-fill emotions just to avoid zeros.
- causally connect meaningful survival/life outcomes: unresolved need pressure, relief, contamination/hazard, successful gathering/crafting/work, repeated failure/frustration, threat/loss, etc.
- audit the Master Spec emotion set against the currently implemented dimensions.
- add Core regression coverage for daily-life emotion generation/decay and Observer projection.

### Observer resident-detail data fidelity gap

PIE review showed several detail tabs are structurally present but still hide or bypass authoritative Core data.

Required follow-up:
- **Needs:** keep human labels but also show authoritative numeric value/bar.
- **Personality:** expose the full 14 Core dimensions rather than only five legacy axes.
- **Traits & Skills:** stop using unpopulated legacy DTO fields; use authoritative Core skills/traits. `None listed` must not falsely imply no ability.
- **Relationships:** keep bond/trust/romance summary if useful, but expose the underlying directional relationship dimensions in detail.
- **Emotion:** show authoritative dimensions/derived values once the separate runtime integration gap is addressed.
- **Family:** `None` is valid for unrelated/single founders; do not invent links.
- **Overview:** remain concise.
- **Knowledge & Gear:** keep the current direct-Core model as the pattern for other tabs.

Data-authority rule:
- Observer may format/summarize but must not create a second resident-state authority.

### Localization + social communication presentation gap

Canonical contract: `docs/SOCIAL_COMMUNICATION_LOCALIZATION_v1.md`.

Required follow-up:
- normal-user UI defaults to Korean; Core identifiers remain language-neutral.
- raw identifiers such as `UseToilet` must not leak into final user UI.
- actual Core social actions must be observable through approach/facing/gaze/context animation plus appropriately tiered bubbles/icons/Event Feed/history.
- dialogue baseline is deterministic/data-driven and must not require a paid LLM/API.
- Presentation must not invent social events that Core did not issue.

Ownership:
- Jjun/Core: authoritative social action/event/outcome/read context where needed.
- Dagyeom: Korean UI rendering and social presentation.

### Character context-motion gap

Canonical contract: `docs/CHARACTER_CONTEXT_MOTION_v1.md`.

Current runtime locomotion is primarily `Idle / Walk / Jog / Sprint`; imported context animation assets are not implementation until wired to authoritative actions.

Required follow-up in Character Presentation:
- Context Motion Router.
- social talking/facing/gaze.
- sitting enter/idle/exit where real sit affordances exist.
- generic interact/pickup/kneeling work mappings.
- truthful fallbacks for Eat/Drink/Sleep/Toilet/Hygiene.
- privacy-first Toilet/outdoor sanitation sequence with reusable local Privacy Mask.
- consume typed `Gather / Store / Experiment / Craft` data after PR #103 merges.

### Early-survival / all-needs-critical investigation

PIE capture showed Day 4 with all five needs `Very low` and repeated `UseToilet`. This is not yet classified as a confirmed simulation defect.

Required validation:
- run a deterministic New Game through at least the first 4 simulation days and record five needs, inventory, selected goals, civilization decisions, and physical completion ACKs per founder.
- prove food/water acquisition and consumption occur before urgent survival pressure can deadlock civilization acquisition.
- prove outdoor sleep/toilet/wash fallbacks relieve the matching need.
- if deadlocked, fix causal acquisition/decision logic rather than hiding it in labels or seeding fake modern facilities.

## Dagyeom lane

Status: `IR-B DONE / CONTEXT MOTION HANDOFF READY AFTER #103`

- IR-B is resolved and merged through #102; no further IR-B blocker is open.
- ordinary visual regression checks for stationary target-facing and male/female/outfit variants remain QA only.
- IR-D is the current open Character handoff: consume `FLLCoreActionDirective` through a Context Motion Router after provider PR #103 merges.
- no Jjun direct changes to `Source/LifeLens/Characters/**` unless an explicit assist lock is opened.

## Active blockers / locks

- Formal Integration Requests: **IR-D open** for Context Motion contract consumption.
- Assist locks: 0 known active.
- PR #103 is Jjun Core/Bridge scope; no Dagyeom review is requested by default.
- Android seed runtime validation is independent of this Core/Bridge work.

## Long compile rule

When a long UE compile/package is started, record HEAD + Run ID and continue safe independent work. Do not continuously poll. A successful compile is evidence for integration correctness; visual quality still requires PIE/APK validation.
