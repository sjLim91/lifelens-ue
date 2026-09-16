# LifeLens Canonical Work State

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
> Long-term order: `docs/DEVELOPMENT_MILESTONES.md`.
> Durable design decisions: `docs/DECISION_LOG.md`.

Last reconciled: 2026-09-16 KST during Observer Resident Detail Data v1 work after PR #111 merge.

## Current main baseline

Latest functional checkpoint covered here:
- PR #108 — Android build pipeline split: **MERGED** as `0d3da3d7f69c2e3c2ad54b2b4f43d19822ee8760`.
- PR #109 — Observer Camera Control v1 refresh: **MERGED** as `47f75d2cb7fd93b26b8b2082bf9f30cfafe67bed`.
- PR #110 — Observer mobile UI polish refresh: **MERGED** as `e6443d693076791e3981047d827ff5a64e4858f2`.
- PR #111 — Civilization resource/storage spatial targets: **MERGED** as `dda35bac7cef4a88433f4f6a362b89c1b3ea2eee`.
- TEAM_BOARD reconciliation after #111: `cd8245378b87bcbb4603ed2dd2deccc1b6bea5d1`.

Authority rule remains unchanged: Core/World owns simulation truth. UI/Character/Environment/WorldPresentation presents that truth and must not create a second authority.

## Recently closed product checkpoints

### World Visual / production map integration — DONE
- World Visual delivery merged through PR #90 and follow-up integration through #91/#92/#96.
- production map: `/Game/Maps/LifeLensWorld`.
- default/startup map points to `/Game/Maps/LifeLensWorld`.
- WorldPresentation consumes authoritative Core/World read contracts only.

### Hardcoding Cleanup B — DONE
- PR #99 merged as `1ca6db8f276cf01211a8ce8c023d2e8d8107d6cd`.
- Needs / UtilityAI tuning flows `DefaultGame.ini -> UE Config -> immutable Core SimulationRuleset -> Simulation`.

### Character facing / backwards-walk IR-B — DONE
- PR #102 merged as `d3c87996499b353c742ce97fd90978633b9b7ca0`.
- Quaternius imported skeleton visual forward is local `+Y`; presentation yaw correction uses `-90` against Unreal +X actor/world forward.
- PIE/debug evidence confirmed residents face travel direction.

### Context Action Contract v1 — DONE
- PR #103 merged as `cba58803c67f971eb1273aaf4e35f5c22b980f01`.
- `FLLCoreActionDirective` exposes authoritative civilization presentation context for `Gather / Store / Experiment / Craft`.
- action/result/material/item/technique/quantity/time/stable entity IDs are presentation read data only.
- runtime work provenance is not persisted as stale animation replay state.
- Character Presentation consumer work remains owned by Dagyeom through IR-D.

### Early Survival Provisioning — DONE
- PR #106 merged as `4b8c938629cca18fafad00de0abcfb8cf41b36c9`.
- urgent Hunger/Thirst can promote only the real matching ResourceNode Gather when no matching carried provision exists.
- no resource is synthesized.
- deterministic regression covers four-founder/four-day production-like recovery, real Water/PlantFood acquisition, Drink/Eat completion, and outdoor sanitation residue.

### Android pipeline timeout/split recovery — DONE / DEVICE BASELINE VALIDATION UNDERWAY SEPARATELY
- PR #107 increased the old monolithic seed/full timeout budget and merged as `e58374a4c3a427ae47c2c507ae7c23b282d0078e`.
- PR #108 then split the workflow into two jobs and merged as `0d3da3d7f69c2e3c2ad54b2b4f43d19822ee8760`.
- `seed-engine` builds Linux editor/cook tools and writes the encrypted host cache.
- `build-android` is a separate job; a `seed` dispatch chains into it automatically after the seed job.
- `fast` reuses the encrypted Android engine cache and must fail clearly on cache miss rather than silently falling back to a full engine build.
- the historical seed Run #3 failure belongs to the pre-split workflow and is no longer the canonical current pipeline state.
- Android seed/device-baseline validation is being handled separately from the Observer data branch; **this Observer PR must not trigger a duplicate Android seed/full run**.
- after a genuinely successful seed/cache creation, normal APK validation should use `fast`.

### Observer Camera Control v1 — DONE / DEVICE FEEL QA REMAINS
- original PR #105 was superseded and closed.
- refreshed PR #109 merged as `47f75d2cb7fd93b26b8b2082bf9f30cfafe67bed`.
- PC: wheel zoom, right-drag orbit, middle-drag pan, left-click selection preserved.
- Android: short tap selection on release, one-finger orbit, pinch zoom, two-finger pan.
- distance/elevation clamps and smoothing are integrated.
- `ASSIST_LOCK-UI-CAMERA-1` is **RELEASED**.
- compile integration is complete; final gesture feel still belongs to real device QA.

### Observer mobile UI polish — DONE / DEVICE QA REMAINS
- refreshed PR #110 merged as `e6443d693076791e3981047d827ff5a64e4858f2`.
- Level2 -> Level1 Back flow, zero-resident empty state, resident-strip de-emphasis, safe-area/cutout handling, minimum touch targets, overflow indicators and short selection/focus feedback are integrated.

### Civilization resource/storage spatial authority — DONE
- PR #111 merged as `dda35bac7cef4a88433f4f6a362b89c1b3ea2eee`.
- validation: Core Tests #502 PASS, Preflight #617 PASS, Unreal Linux Compile #130 PASS.
- Core test suite at #111: **51/51 PASS**, including `test_civilization_spatial_targets`; deterministic harness smoke also PASS.
- `ResourceNode` and `StorageSite` own authoritative `GridPos`.
- production-generated ResourceNodes preserve the exact `NaturalResourcePatch.pos`.
- civilization snapshot extension v2 persists resource/storage positions and retains v1 read compatibility.
- `ULLCoreBridgeSubsystem::GetResidentActionDirective` resolves Gather/Store to actual Core-owned positions.
- the old spatialization gap is **RESOLVED**.

## Current active product work

### Observer Resident Detail Data v1 — ACTIVE

Canonical contract: `docs/OBSERVER_RESIDENT_DETAIL_DATA_v1.md`.

Branch: `jjun/observer-data-completeness-v1`.
Assist lock: `ASSIST_LOCK-UI-OBSERVER-DATA-1` — ACTIVE.

Purpose:
- finish the Level 2 Observer resident-detail fidelity end-to-end rather than splitting Core/UI handoffs;
- make detailed resident inspection consume authoritative Core read data wherever it exists;
- remove misleading dependence on empty legacy `FLLResidentData` Traits/Skills/Preferences fields.

Implemented on the active branch:
- **Needs:** direct `FLLCoreResidentObservation::Needs`; five physical needs show satisfaction percentage plus compact bar. Core deficit semantics remain `0=satisfied, 1=urgent`.
- **Personality:** direct Core personality, all 14 dimensions shown rather than five compatibility axes.
- **Traits:** no explicit named Core trait taxonomy currently exists; UI states that fact rather than showing `None listed` or inventing trait labels.
- **Skills:** direct authoritative civilization `Gathering / Crafting / Learning` skill values.
- **Preferences:** no authoritative Core preference model currently exists; legacy placeholders are hidden rather than presented as facts.
- **Relationships:** preserve SocialBond/RomancePotential summaries while exposing the underlying directional Core dimensions: Affection, Trust, Respect, Comfort, Familiarity, Attraction, RomanticInterest, SexualAttraction, Commitment, Conflict, Jealousy, Fear, Grudge.
- **Family:** existing direct-Core family observation remains authoritative.
- **Knowledge & Gear:** existing direct-Core civilization observation remains the reference pattern.
- **Emotion:** display remains direct-Core; ordinary life-event emotion causality is deliberately deferred to the separate Emotion Runtime Integration milestone.

Validation still required before merge:
- Preflight PASS.
- Unreal Linux Compile PASS.
- diff review confirms Level 0/1 behavior, camera/tap routing, safe-area/mobile polish, and selection feedback remain intact.
- no Android build is required for this PR because Android seed/device work is already being handled separately.

### Jjun lane

Status: `OBSERVER DATA COMPLETENESS ACTIVE / ANDROID DEVICE BASELINE RUNNING SEPARATELY`

Current facts:
- branch `jjun/observer-data-completeness-v1` is the active Jjun feature branch.
- `ASSIST_LOCK-UI-OBSERVER-DATA-1` covers only the Observer detail UI files needed for this milestone.
- Jjun is intentionally completing the whole Observer data slice; after merge/release, Dagyeom resumes normal Observer UI maintenance/styling under the canonical data-authority contract.
- #111 spatial provider work is complete.
- do not launch another Android seed/full job from this branch.

### Dagyeom lane

Status: `IR-B DONE / IR-D CONTEXT MOTION CONSUMER WORK / OBSERVER DATA ASSIST LOCK ACTIVE`

Current facts:
- IR-B is resolved and merged through #102.
- IR-D provider contract is merged through #103 and spatial target follow-up #111.
- Character Presentation can consume real Gather/Store target coordinates instead of scenery guesses.
- Dagyeom owns Context Motion Router/action-to-animation presentation.
- while `ASSIST_LOCK-UI-OBSERVER-DATA-1` is active, Jjun owns the explicitly locked Observer detail files; after release, normal UI maintenance returns to Dagyeom.
- no Jjun direct changes to `Source/LifeLens/Characters/**` unless a new explicit assist lock is opened.

## Current validation risk

`OPEN PIE/APK VISUAL + DEVICE QA RISK — NOT A CORE/COMPILE BLOCKER`

The compile gates prove C++/UHT/UBT integration, not every presentation/input variant. The next real PIE/APK pass should confirm:
- `LifeLensWorld` opens as the production map.
- founders and generated-world presentation remain readable.
- authoritative resource patches remain visible.
- corrected facing remains valid for stationary interaction targets and male/female/outfit variants.
- Android framing/safe area remains acceptable.
- Observer Camera gestures feel natural and do not trigger unintended resident selection.
- Gather residents approach the actual Core resource target.
- Store uses the actual storage target once storage exists.
- Sleep fallback visibly and causally relieves Energy through the UE execution/ACK path.
- outdoor Hygiene fallback visibly and causally relieves Hygiene through the UE execution/ACK path.
- the old food/water provisioning deadlock does not reappear.
- Level 2 Needs/Personality/Skills/Relationships/Family/Knowledge detail remains readable on the device after the current Observer milestone merges.

## Tracked implementation gaps — DO NOT DROP

### Emotion runtime integration gap

Resident Emotion can remain all `0%` because founders begin neutral and many ordinary life events still do not drive emotion changes.

Required follow-up:
- keep neutral-at-start semantics; do not random-fill emotions just to avoid zeros.
- causally connect meaningful survival/life outcomes: unresolved need pressure, relief, contamination/hazard, successful gathering/crafting/work, repeated failure/frustration, threat/loss, etc.
- audit the Master Spec emotion set against the currently implemented dimensions.
- add Core regression coverage for daily-life emotion generation/decay and Observer projection.

### Observer resident-detail data fidelity gap — IN PROGRESS

This gap is the active `Observer Resident Detail Data v1` milestone on `jjun/observer-data-completeness-v1`.

Resolved in the branch implementation once validation/merge completes:
- Needs direct-Core numeric value/bar.
- all 14 Core personality dimensions.
- authoritative civilization skills instead of legacy empty skill DTOs.
- truthful no-model state for Traits/Preferences rather than fake `None listed`.
- detailed directional relationship dimensions.
- authoritative Family and direct-Core Knowledge/Gear preserved.

Do not mark this gap DONE until the branch passes integration validation and merges.

### Explicit Core trait/preference model gap

The current Core has personality, genetics/life-condition data, civilization skills/knowledge, relationships, family, etc., but it does not yet own an explicit named resident Trait taxonomy or an authoritative Preference model.

Rule:
- Observer must not infer named traits/preferences from unrelated values just to populate UI.
- current Observer data milestone communicates model absence truthfully.
- a future product milestone may add explicit Core models when they have actual simulation meaning.

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
- Dagyeom: Korean UI rendering and social presentation after the current Observer assist lock is released.

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
- consume merged typed `Gather / Store / Experiment / Craft` data from #103.
- consume merged authoritative Gather/Store spatial targets from #111.

## Active blockers / locks

- Formal Integration Requests: **IR-D open**, provider fully DONE / presentation consumer work belongs to Dagyeom.
- Assist locks: **ASSIST_LOCK-UI-OBSERVER-DATA-1 ACTIVE**.
- Active Jjun branch: `jjun/observer-data-completeness-v1`.
- #111 Core/Preflight/Unreal compile blocker: **None**.
- Android device-baseline validation is separate; do not duplicate it from Observer work.
- Device visual/input QA remains open but is not a code/compile blocker.

## Android validation rule

For the split workflow:
1. Run `mode=seed` once when a fresh engine cache is required.
2. A successful `seed-engine` chains into `build-android` and APK production.
3. After a valid Android engine cache exists, use `mode=fast` for normal APK checks.
4. Use `full` only as an evidence-driven fallback.

Do not run repeated expensive seed/full jobs blindly. Capture the exact failing job/step/log first if a future run fails.

## Long compile rule

When a long UE compile/package is started, record HEAD + Run ID and continue safe independent work. Do not continuously poll. A successful compile is evidence for integration correctness, while visual/input quality still requires appropriate PIE/APK validation.
