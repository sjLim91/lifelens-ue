# LifeLens Canonical Work State

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
> Long-term order: `docs/DEVELOPMENT_MILESTONES.md`.
> Durable design decisions: `docs/DECISION_LOG.md`.

Last reconciled: 2026-09-16 KST during World Obstacle Collision v1 / PR #114.

## Current main baseline

Latest functional checkpoints:
- PR #108 — Android build pipeline split: **MERGED** as `0d3da3d7f69c2e3c2ad54b2b4f43d19822ee8760`.
- PR #109 — Observer Camera Control v1 refresh: **MERGED** as `47f75d2cb7fd93b26b8b2082bf9f30cfafe67bed`.
- PR #110 — Observer mobile UI polish refresh: **MERGED** as `e6443d693076791e3981047d827ff5a64e4858f2`.
- PR #111 — Civilization resource/storage spatial targets: **MERGED** as `dda35bac7cef4a88433f4f6a362b89c1b3ea2eee`.
- PR #112 — Observer Resident Detail Data v1 + authoritative Traits/Preferences: **MERGED** as `27ba0aa147fc38ad05cf388e9390dd2dcaccdf30`.

Authority rule remains unchanged: Core/World owns simulation truth. UI/Character/Environment/WorldPresentation presents that truth and must not create a second authority.

## Active branch checkpoint

### World Obstacle Collision v1 — ACTIVE / PR #114

Canonical contract: `docs/WORLD_OBSTACLE_COLLISION_v1.md`.

Branch: `jjun/world-obstacle-collision-v1`.

Purpose:
- stop residents visibly clipping through generated tree trunks and non-trivial rocks;
- preserve Core action/target/resource authority;
- avoid changing Dagyeom-owned WorldPresentation render/culling source.

Implementation:
- hidden query-only HISM collision proxies mirror actual generated tree/rock presentation instances;
- proxies block `ECC_Pawn` only and do not affect navigation generation;
- tree trunks and meaningful rocks block; shrubs, grass and explicit `Pebble_*` meshes remain traversable;
- `ALLResidentCharacter` preserves its existing `VInterpConstantTo` constant-speed frame step, sweeps that step, and spends only the **remaining** frame-distance budget on obstacle slide/side-step;
- obstacle avoidance faces the direction actually travelled and does not alter Core target selection or action outcome authority.

Ownership:
- `ASSIST_LOCK-CHARACTER-OBSTACLE-1` is ACTIVE for `Source/LifeLens/Characters/LLResidentCharacter.cpp` only;
- supporting collision adapter/runtime spawn lives in Jjun-owned World/Core paths;
- `Source/LifeLens/WorldPresentation/**` remains unchanged.

Validation status:
- PR #114 Preflight #632 passed before the final movement-budget/pebble/document refinements;
- final branch head requires fresh Preflight/Unreal Linux validation after those refinements;
- this PR does not touch `Source/LifeLensCore/**`, so Core Tests are not path-triggered and are not a required PR gate;
- PIE/device visual QA still must confirm no trunk/large-rock clipping, no stuck/jitter, no obstacle-avoidance speed burst, and correct facing.

IR-D follow-up requirement:
- authoritative Gather/Store `GridPos` remains the target locus;
- when the corresponding visual has a blocking proxy, Character Presentation must use a truthful interaction radius outside the blocker rather than require the capsule to overlap the exact target centre;
- the Core coordinate must never be moved or replaced to make presentation easier.

## Recently closed product checkpoints

### Observer Resident Detail Data v1 — DONE

Canonical contract: `docs/OBSERVER_RESIDENT_DETAIL_DATA_v1.md`.

Delivered by PR #112:
- **Needs:** direct `FLLCoreResidentObservation::Needs`; five physical needs show numeric satisfaction plus compact visual bar while Core keeps deficit semantics (`0=satisfied`, `1=urgent`).
- **Personality:** all 14 authoritative Core dimensions.
- **Traits:** explicit Core `TraitProfile` with Resilience, Creativity, Discipline, Compassion, Adaptability, Boldness, Perseverance, Resourcefulness.
- **Preferences:** explicit Core `PreferenceProfile` with Socializing, Solitude, Exploration, Crafting, Gathering, Comfort, Novelty, Order.
- **Trait/Preference authority:** deterministic Core read profiles derived from persistent Personality + Genetics; no second mutable Save authority.
- **Persistence:** same WorldSeed + PopulationSeed reproduces the same profiles; snapshot encode/decode reproduces the same values.
- **Behavior integration:** the same Core Traits/Preferences influence ordinary Social/Civilization utility. Civilization disposition effects are bounded to a 0.90–1.10 multiplier; urgent Hunger/Thirst provision gathering bypasses ordinary preference modulation so survival priority is preserved.
- **Bridge:** `FLLCoreTraitPreferenceObservation` via `ULLCoreBridgeSubsystem::GetResidentTraitPreferenceObservation`.
- **Skills:** authoritative civilization Gathering / Crafting / Learning.
- **Relationships:** directional Core dimensions exposed while SocialBond/RomancePotential remain summaries.
- **Family / Knowledge & Gear:** existing direct-Core observation retained.
- **Emotion:** display remains truthful/direct-Core; causal daily-life emotion generation is intentionally a separate milestone.

Validation on final PR head:
- Core Tests #515: **PASS, 52/52**, including `test_traits_preferences`.
- deterministic harness smoke: **PASS**.
- Preflight #627: **PASS**.
- Unreal Linux Compile #137: **PASS**, including UHT/UBT integration of the new USTRUCT/UFUNCTION bridge types.
- diff review: no PlayerController/camera/mobile-input files changed; Level 0/1 and existing camera/touch ownership were not replaced by this milestone.

Ownership closeout:
- `ASSIST_LOCK-UI-OBSERVER-DATA-1` is **RELEASED**.
- normal Observer presentation/layout/styling maintenance returns to Dagyeom.
- Core/Bridge data authority remains owned by Jjun/Core.

### Civilization resource/storage spatial authority — DONE
- PR #111 merged as `dda35bac7cef4a88433f4f6a362b89c1b3ea2eee`.
- validation: Core Tests #502 PASS, Preflight #617 PASS, Unreal Linux Compile #130 PASS.
- `ResourceNode` and `StorageSite` own authoritative `GridPos`.
- generated ResourceNodes preserve exact `NaturalResourcePatch.pos`.
- Gather/Store directives resolve actual Core-owned targets instead of guessed scenery.

### Observer Camera Control v1 — DONE / DEVICE FEEL QA REMAINS
- PR #109 merged as `47f75d2cb7fd93b26b8b2082bf9f30cfafe67bed`.
- PC: wheel zoom, right-drag orbit, middle-drag pan, left-click selection.
- Android: short tap selection, one-finger orbit, pinch zoom, two-finger pan.
- `ASSIST_LOCK-UI-CAMERA-1` is **RELEASED**.

### Observer mobile UI polish — DONE / DEVICE QA REMAINS
- PR #110 merged as `e6443d693076791e3981047d827ff5a64e4858f2`.
- Level2 -> Level1 Back, empty state, resident-strip de-emphasis, safe-area/cutout handling, minimum touch targets, overflow and selection/focus feedback are integrated.

### Early Survival Provisioning — DONE
- PR #106 merged as `4b8c938629cca18fafad00de0abcfb8cf41b36c9`.
- urgent Hunger/Thirst can promote only a real matching ResourceNode Gather when needed; resources are never synthesized.

### Context Action Contract v1 — DONE
- PR #103 merged as `cba58803c67f971eb1273aaf4e35f5c22b980f01`.
- `FLLCoreActionDirective` exposes authoritative civilization presentation context for `Gather / Store / Experiment / Craft`.
- Character Presentation consumer work remains Dagyeom-owned through IR-D.

### Character facing / backwards-walk IR-B — DONE
- PR #102 merged as `d3c87996499b353c742ce97fd90978633b9b7ca0`.
- Quaternius visual forward correction is integrated and verified.

### Hardcoding Cleanup B — DONE
- PR #99 merged as `1ca6db8f276cf01211a8ce8c023d2e8d8107d6cd`.
- Needs / UtilityAI tuning flows from config into immutable Core rules.

### World Visual / production map integration — DONE
- World Visual delivery merged through #90/#91/#92/#96.
- production/default map: `/Game/Maps/LifeLensWorld`.

## Current active / next work

### Jjun lane

Status: `WORLD OBSTACLE COLLISION V1 ACTIVE / PR #114`

Current task:
- finish #114 final-head validation and visual collision contract closeout;
- release `ASSIST_LOCK-CHARACTER-OBSTACLE-1` only after merge.

Next canonical priority after #114:
- **Social Communication & Localization** contract support where Core/Bridge changes are required.
- keep Core identifiers language-neutral; Korean is presentation/localization behavior.
- expose only real social action/event/outcome context; Presentation must not invent conversations or relationship changes.

After that, the major Core gap is **Emotion Runtime Integration**: neutral founders are valid, but ordinary life/survival outcomes need causal emotion generation/decay.

### Dagyeom lane

Status: `IR-D CONTEXT MOTION CONSUMER WORK / CHARACTER OBSTACLE ASSIST LOCK ACTIVE ON ONE FILE`

Current facts:
- IR-D provider contracts are merged through #103 and spatial authority #111.
- Character Presentation can consume real Gather/Store target coordinates.
- Dagyeom owns Context Motion Router/action-to-animation presentation.
- while #114 is open, Jjun temporarily owns only `LLResidentCharacter.cpp` through `ASSIST_LOCK-CHARACTER-OBSTACLE-1`; normal Character ownership returns after merge.
- Observer normal presentation maintenance is no longer locked by Jjun after #112.

### Android / device lane

Status: `GATE B PENDING — NO CANONICAL SUCCESSFUL SEED CACHE YET`

The split #108 workflow is ready. Do not describe device-baseline validation as already underway unless an actual workflow run is started and verified.

Canonical execution rule:
1. Run `mode=seed` once when a fresh engine cache is required.
2. Successful `seed-engine` chains automatically into `build-android` and APK production.
3. Only after a genuinely successful Android engine cache exists, use `mode=fast` for normal APK checks.
4. Use `full` only as an evidence-driven fallback.

Do not run repeated expensive seed/full jobs blindly. Capture the exact failing run/job/step/log first if a future run fails.

## Current validation risk

`OPEN PIE/APK VISUAL + DEVICE QA RISK — NOT A CORE AUTHORITY BLOCKER`

Core/Preflight/Linux compile gates prove source integration, not final device presentation. The next real PIE/APK pass should confirm:
- `/Game/Maps/LifeLensWorld` boots correctly.
- four founders and generated-world presentation remain readable.
- residents do not pass through tree trunks or meaningful rocks.
- obstacle avoidance does not cause visible jitter, permanent stuck states, or speed bursts.
- explicit pebbles/shrubs/grass remain traversable.
- Android framing/safe area and Observer gestures feel correct.
- Level 2 Needs/Personality/Traits/Skills/Preferences/Relationships/Family/Knowledge detail is readable.
- Gather approaches the actual Core resource target using a valid interaction radius outside any blocking visual proxy.
- Store uses the actual storage target once storage exists.
- Sleep and Hygiene fallbacks visibly complete through UE execution/ACK.
- the food/water provisioning deadlock does not reappear.
- corrected character facing remains valid across context actions and appearance variants.

## Tracked implementation gaps — DO NOT DROP

### Emotion runtime integration gap — OPEN

Resident Emotion can remain all `0%` because founders begin neutral and many ordinary life events still do not drive emotion changes.

Required follow-up:
- keep neutral-at-start semantics; do not random-fill emotion just to avoid zeros.
- causally connect meaningful survival/life outcomes: unresolved need pressure, relief, contamination/hazard, successful gathering/crafting/work, repeated failure/frustration, threat/loss, etc.
- reconcile implemented emotion dimensions with the Master Spec.
- add Core regression coverage for daily-life emotion generation/decay and Observer projection.

### Observer resident-detail data fidelity gap — CLOSED BY #112

Resolved:
- Needs direct-Core numeric value/bar.
- all 14 Core personality dimensions.
- explicit 8-axis Core TraitProfile.
- explicit 8-axis Core PreferenceProfile.
- Traits/Preferences participate in ordinary behavior selection and remain stable across Save/Load.
- authoritative civilization skills.
- detailed directional relationship dimensions.
- authoritative Family and direct-Core Knowledge/Gear.

### Explicit Core trait/preference model gap — CLOSED BY #112

Resolved with explicit named Core semantics, deterministic derivation, Bridge projection, Observer display, behavioral integration, and regression coverage.

### Localization + social communication presentation gap — OPEN

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

### Character context-motion gap — OPEN / IR-D

Canonical contract: `docs/CHARACTER_CONTEXT_MOTION_v1.md`.

Required follow-up in Character Presentation:
- Context Motion Router.
- social talking/facing/gaze.
- sitting enter/idle/exit where real sit affordances exist.
- generic interact/pickup/kneeling work mappings.
- truthful fallbacks for Eat/Drink/Sleep/Toilet/Hygiene.
- privacy-first Toilet/outdoor sanitation sequence.
- consume typed `Gather / Store / Experiment / Craft` data from #103.
- consume authoritative Gather/Store spatial targets from #111.
- treat Gather/Store target coordinates as authoritative target loci; where a solid visual/collision proxy blocks the exact centre, stop within an interaction radius outside the blocker and interact with the same Core target rather than shifting the target coordinate.

## Active blockers / locks

- Formal Integration Requests: **IR-D open**; provider side DONE, presentation consumer belongs to Dagyeom.
- Assist locks: **ASSIST_LOCK-CHARACTER-OBSTACLE-1 ACTIVE** for #114.
- #114 final-head Preflight/Unreal compile validation: **pending after final refinements**.
- Android real-device baseline: **pending actual seed/APK execution**.
- PIE/APK visual/input/collision QA remains open but is not a Core authority blocker.

## Long compile rule

When a long UE compile/package is started, record HEAD + Run ID and continue safe independent work. The user reports compile/package completion; do not repeatedly poll a healthy long-running compile. Do not restart a healthy run merely because the large Unreal image pull is slow. A successful compile is integration evidence; visual/input quality still requires PIE/APK validation.
