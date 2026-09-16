# LifeLens Canonical Work State

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
> Long-term order: `docs/DEVELOPMENT_MILESTONES.md`.
> Durable design decisions: `docs/DECISION_LOG.md`.

Last reconciled: 2026-09-16 KST after PR #111 merge.

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

### Android pipeline timeout/split recovery — DONE / SEED VALIDATION PENDING
- PR #107 increased the old monolithic seed/full timeout budget and merged as `e58374a4c3a427ae47c2c507ae7c23b282d0078e`.
- PR #108 then split the workflow into two jobs and merged as `0d3da3d7f69c2e3c2ad54b2b4f43d19822ee8760`.
- `seed-engine` builds Linux editor/cook tools and writes the encrypted host cache.
- `build-android` is a separate job; a `seed` dispatch chains into it automatically after the seed job.
- `fast` reuses the encrypted Android engine cache and must fail clearly on cache miss rather than silently falling back to a full engine build.
- the historical seed Run #3 failure belongs to the pre-split workflow and is no longer the canonical current pipeline state.
- **No seed has yet been dispatched against the split #108 workflow.** The next Android pipeline validation should run `mode=seed` once on latest `main`; after a successful seed/cache creation, normal APK validation should use `fast`.

### Observer Camera Control v1 — DONE / DEVICE FEEL QA REMAINS
- original PR #105 was superseded and closed.
- refreshed PR #109 merged as `47f75d2cb7fd93b26b8b2082bf9f30cfafe67bed`.
- PC: wheel zoom, right-drag orbit, middle-drag pan, left-click selection preserved.
- Android: short tap selection on release, one-finger orbit, pinch zoom, two-finger pan.
- distance/elevation clamps and smoothing are integrated.
- `ASSIST_LOCK-UI-CAMERA-1` is **RELEASED**; there are currently no assist locks.
- compile integration is complete; final gesture feel still belongs to real device QA.

### Observer mobile UI polish — DONE / DEVICE QA REMAINS
- refreshed PR #110 merged as `e6443d693076791e3981047d827ff5a64e4858f2`.
- Level2 -> Level1 Back flow, zero-resident empty state, resident-strip de-emphasis, safe-area/cutout handling, minimum touch targets, overflow indicators and short selection/focus feedback are integrated.
- UI and camera refreshed branches do not overlap.

### Civilization resource/storage spatial authority — DONE
- PR #111 merged as `dda35bac7cef4a88433f4f6a362b89c1b3ea2eee`.
- validation: Core Tests #502 PASS, Preflight #617 PASS, Unreal Linux Compile #130 PASS.
- Core test suite at #111: **51/51 PASS**, including `test_civilization_spatial_targets`; deterministic harness smoke also PASS.
- `ResourceNode` and `StorageSite` now own authoritative `GridPos`.
- production-generated ResourceNodes preserve the exact `NaturalResourcePatch.pos` from world generation.
- compatibility/test resources have deterministic fixed positions.
- civilization snapshot extension v2 persists resource/storage positions and retains v1 read compatibility.
- generated natural resource positions remain recoverable from immutable natural patches for legacy v1 data.
- `ULLCoreBridgeSubsystem::GetResidentActionDirective` resolves ordinary Gather to the actual resource-node position and Store to the actual storage-site position when presenting civilization work.
- Character Presentation must consume those Core-owned targets; it must not guess a nearby tree, rock, container, or scenery proxy.
- the old tracked "Civilization resource/storage spatialization gap" is **RESOLVED** and must not be reopened without new contradictory runtime evidence.

## Current active product work

### Jjun lane

Status: `ANDROID SPLIT PIPELINE SEED VALIDATION PENDING / NEXT CORE SLICE AVAILABLE`

Current facts:
- no open Jjun feature PR from #109/#110/#111 work remains.
- no assist lock is active.
- #111 spatial provider work is complete.
- the next Android pipeline validation is one `mode=seed` dispatch on latest main. A successful seed should automatically continue into `build-android`; subsequent APK builds should use `fast`.
- do not start `full` unless the split-cache path itself proves unusable and evidence requires a fallback.
- IR-D Character Presentation consumption remains Dagyeom-owned; Jjun changes are only needed if a real Core/Bridge contract deficiency appears.

### Dagyeom lane

Status: `IR-B DONE / IR-D CONTEXT MOTION READY / UI-CAMERA LOCK RELEASED`

Current facts:
- IR-B is resolved and merged through #102.
- IR-D provider contract is merged through #103 and spatial target follow-up #111.
- Character Presentation can now consume real Gather/Store target coordinates instead of scenery guesses.
- Dagyeom owns the Context Motion Router and action-to-animation presentation.
- Observer Camera and mobile UI refreshes are merged; remaining device/visual checks are QA, not ownership blockers.
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
- Gather residents approach the actual resource target supplied by Core rather than an arbitrary scenery object.
- Store presentation uses the actual storage target once storage exists.
- Sleep fallback visibly and causally relieves Energy through the UE execution/ACK path.
- outdoor Hygiene fallback visibly and causally relieves Hygiene through the UE execution/ACK path.
- the old food/water provisioning deadlock does not reappear in runtime observation.

## Tracked implementation gaps — DO NOT DROP

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
- consume merged typed `Gather / Store / Experiment / Craft` data from #103.
- consume merged authoritative Gather/Store spatial targets from #111.

## Active blockers / locks

- Formal Integration Requests: **IR-D open**, provider fully DONE / presentation consumption READY.
- Assist locks: **None**.
- Open Jjun code PRs from the camera/UI/spatial sequence: **None**.
- Core/Preflight/Unreal compile blocker for #111: **None**.
- Android pipeline: **not blocked by the old pre-split Run #3 failure**; the split #108 workflow simply still needs its first real `seed` validation.
- Device visual/input QA remains open but is not a code/compile blocker.

## Android validation rule

For the current split workflow:
1. Run `mode=seed` once on latest `main` when a fresh engine cache is required.
2. A successful `seed-engine` automatically allows the chained `build-android` job to produce the APK.
3. After a valid Android engine cache exists, use `mode=fast` for normal APK checks.
4. Use `full` only as an evidence-driven fallback, not as the default.

Do not run repeated expensive seed/full jobs blindly. Capture the exact failing job/step/log first if a future run fails.

## Long compile rule

When a long UE compile/package is started, record HEAD + Run ID and continue safe independent work. Do not continuously poll. A successful compile is evidence for integration correctness, while visual/input quality still requires appropriate PIE/APK validation.
