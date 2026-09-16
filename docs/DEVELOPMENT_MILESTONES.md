# LifeLens Development Milestones

이 문서는 **큰 개발 단위와 integration gate의 canonical roadmap**이다. 작은 TODO 리스트가 아니다.

## Delivery rule

> Same purpose + same layer + same validation scope = one milestone-sized PR.

각 milestone은 내부적으로 여러 작은 commit을 가질 수 있지만, 무거운 UE 검증과 canonical doc closeout은 의미 있는 끝점에서 한 번 수행한다.

## Completed foundations

- Character Presentation #63 — DONE.
- Appearance projection #65 — DONE.
- World Affordance Fallback #66 — DONE.
- Environmental Residue #68 — DONE.
- Runtime resident / physical ACK / restore #70~#74 — DONE.
- Exposure / sanitation causal chain #75~#80 — DONE.
- HumanWaste Environmental Visual Feedback #82 — DONE.
- World Genesis WG-1 #83 — DONE.
- World Genesis WG-2 #85 — DONE.
- Character Appearance #67 — DONE.
- Character Motion Bootstrap #84 — DONE.
- World Generation Milestone A #87 — DONE.
- Gate A — Integrated Runtime Checkpoint A — DONE.
- World Visual / production map integration #90/#91/#92/#96 — DONE.
- Hardcoding Cleanup B #99 — DONE.
- Character facing IR-B #102 — DONE.
- Context Action Contract v1 #103 — DONE.
- Early Survival Provisioning #106 — DONE.
- Android pipeline timeout/split recovery #107/#108 — DONE; real seed/device validation is a separate runtime gate.
- Observer Camera Control v1 #109 — DONE; device feel QA remains.
- Observer Mobile UI Polish #110 — DONE; device QA remains.
- Civilization Resource/Storage Spatial Authority #111 — DONE.

## Gate A — Integrated Runtime Checkpoint A — DONE

Runtime acceptance established the first coherent Core/World vertical slice:
- NEW GAME starts successfully.
- four founders are generated as 2 male / 2 female.
- generated start region and natural state are authoritative and deterministic.
- no starting house/toilet/farm/storage/road/tool/modern infrastructure is fabricated.
- missing-facility sanitation produces authoritative HumanWaste residue.
- Save/Load restores resident/world state without reroll.
- deterministic continuation remains stable after restore.

Manual visual items waived at this gate were carried into later visual/device validation rather than counted as visual PASS.

## World Visual Milestone A — DONE

Owner: Dagyeom visual/content lane with Jjun integration where required.

Delivered through #90/#91/#92/#96:
- production-oriented generated-world presentation consuming Core/World facts.
- production map `/Game/Maps/LifeLensWorld`.
- default/startup map integration.
- generated natural presentation without a parallel simulation authority.

Remaining appearance/framing/performance checks are device QA, not an unfinished World Visual implementation milestone.

## Character Context Motion Milestone — ACTIVE IN DAGYEOM LANE

Owner: Dagyeom Character presentation.
Canonical contract: `docs/CHARACTER_CONTEXT_MOTION_v1.md`.
Integration request: TEAM_BOARD IR-D.

Provider state:
- Context Action Contract #103 merged.
- authoritative Gather/Store spatial targets #111 merged.

Consumer scope:
- Sit / Stand / Lie / Wake.
- PickUp / Use / Work.
- sanitation / digging / crafting context animation hooks.
- locomotion ↔ context transition.
- basic gaze/body orientation layering.
- truthful fallbacks where a dedicated animation/facility is absent.

Animation remains presentation of Core/World action authority. Character Presentation must consume the supplied real target coordinates rather than guess scenery.

## Observer Resident Detail Data v1 — ACTIVE IN JJUN LANE

Owner during milestone: Jjun under explicit UI assist lock `ASSIST_LOCK-UI-OBSERVER-DATA-1`.
Branch: `jjun/observer-data-completeness-v1`.
Canonical contract: `docs/OBSERVER_RESIDENT_DETAIL_DATA_v1.md`.

Purpose: complete Level 2 resident-detail fidelity end-to-end while keeping Core/World as the only resident-state authority.

Scope:
- Needs from direct Core physical need data with numeric satisfaction and compact bar.
- all 14 authoritative Core personality dimensions.
- explicit Core `TraitProfile` with Resilience / Creativity / Discipline / Compassion / Adaptability / Boldness / Perseverance / Resourcefulness.
- explicit Core `PreferenceProfile` with Socializing / Solitude / Exploration / Crafting / Gathering / Comfort / Novelty / Order.
- Traits/Preferences are deterministic Core read models derived from persistent Personality/Genetics, projected through `FLLCoreTraitPreferenceObservation`; Observer does not invent or own them.
- civilization Gathering/Crafting/Learning as real resident skills.
- detailed directional relationship dimensions rather than only aggregate bond/romance summaries.
- authoritative Family data.
- preserve direct-Core Knowledge & Gear.
- keep Overview concise.

Persistence/determinism acceptance for Traits/Preferences:
- same WorldSeed + PopulationSeed produces the same profiles.
- a different PopulationSeed may produce different founder profiles.
- values stay normalized to `[0,1]`.
- Core snapshot encode/decode reproduces the same profiles because their authoritative Personality/Genetics inputs are persisted; no duplicate mutable profile state is introduced.

Explicit non-scope:
- Emotion causal generation/decay remains the later Emotion Runtime Integration milestone.
- full Korean localization/social presentation remains a later milestone.

Integration gate:
- Core Tests PASS, including `test_traits_preferences`.
- Preflight PASS.
- Unreal Linux Compile PASS.
- no regression to Level 0/1, tap routing, camera behavior, mobile safe area/touch targets, selection feedback.
- Android seed/full is not duplicated for this PR; device validation uses the separately produced APK baseline.

After merge, the assist lock is released and normal Observer layout/styling/mobile-maintenance ownership returns to Dagyeom while preserving the data-authority contract.

## Gate B — Android Smoke / Real-device Baseline — UNDERWAY SEPARATELY

The split #108 Android pipeline is the canonical path:
- first valid engine-cache creation uses `mode=seed`.
- a successful seed chains into the Android build job and APK artifact.
- later normal APK checks use `mode=fast`.
- `full` is evidence-driven fallback only.

Acceptance on a real Android device:
- APK installs and boots into `/Game/Maps/LifeLensWorld`.
- New Game exposes four founders and generated world presentation.
- Observer selection/detail and camera gestures operate correctly.
- autonomous movement/actions, resource approach, survival fallbacks, Save/Load can be observed.
- establish initial FPS / memory / thermal / rendering-budget baseline.

MetaHuman comparison happens only after this gate.

## Next priority — Social Communication & Localization

Canonical contract: `docs/SOCIAL_COMMUNICATION_LOCALIZATION_v1.md`.

Purpose:
- normal-user UI defaults to Korean without changing language-neutral Core identifiers.
- resident-to-resident social behavior becomes visibly understandable through approach/facing/gaze/context animation plus lightweight bubbles/icons/Event Feed/history.
- deterministic/data-driven dialogue baseline without paid LLM/API dependency.

Ownership after the current Observer assist lock closes:
- Jjun/Core: authoritative social action/event/outcome/read context where required.
- Dagyeom presentation: Korean UI rendering and social presentation.

Acceptance:
- raw enum/action identifiers do not leak into normal user UI.
- actual Core social events are visibly distinguishable.
- Presentation never fabricates social events or relationship changes.
- Android presentation remains observation-first rather than filling the screen with permanent chat text.

## Next priority — Emotion Runtime Integration

Keep founder emotion neutral at New Game, then causally connect ordinary survival/life outcomes to emotion.

Acceptance direction:
- hunger/thirst/fatigue/bladder/hygiene pressure and relief can affect emotion where appropriate.
- environmental hazard/contamination, success, repeated failure/frustration, threat/loss and meaningful social/life events drive emotion through Core causality.
- UI never fabricates non-zero values merely for presentation.
- implemented emotion dimensions are reconciled with the Master Spec.
- authoritative emotion survives save/restore and is exposed through Observer read models.

## Later simulation milestones

These remain product direction after the current playable/device/readability gates:
- generic facility/resource authority beyond sanitation.
- health/pathogen + water/soil contamination.
- birth physical position/lifecycle presentation refinements.
- open-ended material/component/connection artifact runtime.
- carrying-capacity pressure / exploration / migration.
- multiple settlements / regional society / economy / politics / generational civilization.

## Current parallel dispatch

**Android/device lane:** Gate B seed/APK/device baseline is being validated separately. Do not duplicate expensive seed/full runs from feature PRs.

**Dagyeom lane:** Character Context Motion milestone / IR-D consumer work.

**Jjun lane:** Observer Resident Detail Data v1 on `jjun/observer-data-completeness-v1`, including explicit Core Traits/Preferences, UI under the recorded assist lock, and mandatory canonical doc updates.

When Observer data work merges, release its assist lock in TEAM_BOARD before moving the UI lane back to normal maintenance ownership.

`tasks/WORK_STATE.md` contains live execution state. `tasks/TEAM_BOARD.md` contains ownership/locks/Integration Requests.
