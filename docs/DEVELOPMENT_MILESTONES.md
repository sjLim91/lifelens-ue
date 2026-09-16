# LifeLens Development Milestones

이 문서는 **큰 개발 단위와 integration gate의 canonical roadmap**이다. 작은 TODO 리스트가 아니다.

## Delivery rule

> Same purpose + same layer + same validation scope = one milestone-sized PR.

각 milestone은 내부적으로 여러 작은 commit을 가질 수 있지만, 무거운 UE 검증과 canonical doc closeout은 의미 있는 끝점에서 한 번 수행한다.

## Completed foundations

- Character Presentation #63 — DONE.
- Appearance projection #65/#67 — DONE.
- World Affordance Fallback #66 — DONE.
- Environmental Residue #68 — DONE.
- Runtime resident / physical ACK / restore #70~#74 — DONE.
- Exposure / sanitation causal chain #75~#80 — DONE.
- HumanWaste Environmental Visual Feedback #82 — DONE.
- World Genesis WG-1 #83 / WG-2 #85 — DONE.
- Character Motion Bootstrap #84 — DONE.
- World Generation Milestone A #87 — DONE.
- Gate A — Integrated Runtime Checkpoint A — DONE.
- World Visual / production map integration #90/#91/#92/#96 — DONE.
- Hardcoding Cleanup B #99 — DONE.
- Character facing IR-B #102 — DONE.
- Context Action Contract v1 #103 — DONE.
- Early Survival Provisioning #106 — DONE.
- Android pipeline timeout/split recovery #107/#108 — DONE; real seed/device validation remains a separate runtime gate.
- Observer Camera Control v1 #109 — DONE; device feel QA remains.
- Observer Mobile UI Polish #110 — DONE; device QA remains.
- Civilization Resource/Storage Spatial Authority #111 — DONE.
- Observer Resident Detail Data v1 + authoritative Traits/Preferences #112 — DONE.
- World Obstacle Collision v1 #114 — DONE as immediate visual-clipping mitigation; authoritative obstacle ownership remains future work.

## Gate A — Integrated Runtime Checkpoint A — DONE

Runtime acceptance established the first coherent Core/World vertical slice:
- NEW GAME starts successfully.
- four founders are generated as 2 male / 2 female.
- generated start region and natural state are authoritative and deterministic.
- no starting house/toilet/farm/storage/road/tool/modern infrastructure is fabricated.
- missing-facility sanitation produces authoritative HumanWaste residue.
- Save/Load restores resident/world state without reroll.
- deterministic continuation remains stable after restore.

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

Animation remains presentation of Core/World action authority. Character Presentation must consume supplied real target coordinates rather than guess scenery.

## Jjun Milestone 1 — Lifecycle Core Correctness v1 — ACTIVE

Owner: Jjun Core/Simulation.
Canonical contract: `docs/LIFECYCLE_CORE_CORRECTNESS_v1.md`.
Branch: `jjun/lifecycle-core-correctness-v1`.

This milestone was promoted ahead of localization/emotion after the 2026-09-16 full-source audit found correctness gaps that would corrupt long-run generational simulation.

Scope:
- dead residents become runtime-inactive and release transient activity/reservations;
- Baby/Toddler stop running the adult autonomous loop;
- parent care becomes authoritative Core behavior and cannot synthesize food/water;
- close genetic kin are excluded from autonomous romance progression;
- newborn runtime position follows the gestational parent rather than absolute `{0,0}`;
- newborn base metabolism/sleep values avoid multiplier compounding;
- deterministic natural age mortality is connected to production runtime;
- gestational-parent death closes pregnancy;
- death updates household/romance/life-history state without deleting genealogy;
- Core regression covers the production wiring, not only isolated helper functions.

Validation gate:
- all Core tests PASS, including `test_lifecycle_runtime_correctness`;
- deterministic harness PASS;
- Preflight PASS;
- Unreal Linux UHT/UBT PASS;
- no Android build solely for this Core milestone.

## Jjun Milestone 2 — Action Completion Unification — NEXT

Purpose: remove the remaining `decision == outcome` shortcuts.

Current physical Needs actions already follow the correct model:

`Core intent -> UE movement/interaction -> arrival/completion -> Core ACK -> authoritative outcome`

Social, Civilization and the newly wired Parenting behavior must converge on the same model.

Required scope:
- Social Approach/Repair/Comfort does not mutate relationships/emotion before physical interaction completes.
- Civilization Gather/Store/Experiment/Craft does not alter inventory/resource/knowledge before the resident reaches/uses the authoritative target/context.
- Parenting care gets an authoritative completion path rather than remaining instant Core presentation-blind work.
- interruption/death/target invalidation cancels pending work without committing an outcome.
- Save/Load must not replay stale pending presentation work as a completed event.

## Jjun Milestone 3 — World / Facility / Obstacle Authority Normalization

Purpose: restore the authority rule all the way through physical traversal and facility use.

Audit findings to close:
- #114 collision proxies currently mirror `WorldPresentation` dressing; final blocking topology must derive from Core/World-owned physical facts, not presentation decoration.
- `LLActivityAnchor` currently lacks a stable Core facility identity; a UE-authored bed/toilet/sink must not create facility-level simulation truth by existing visually.
- Gather/Store interaction loci remain Core-owned; presentation collision may change approach path/radius only, never the target coordinate.
- storage construction/creation needs a real production progression path before Store is naturally reachable.

## Jjun Milestone 4 — Legacy Authority Removal

Remove compatibility surfaces once preceding migrations make them unnecessary:
- `ULLDecisionComponent` second callable AI surface;
- `ULLSimulationSubsystem::ApplyActionOutcome`;
- `ULLSimulationSubsystem::ApplySocialInteraction`;
- stale projection-only contracts and Preflight assertions that pin them;
- dead `SimulationSnapshotCodecLegacy.cpp` under the current-only pre-release save policy.

## Jjun Milestone 5 — Lifecycle Presentation Integration

After Core lifecycle truth is stable:
- dead resident actor/presentation state and Observer population handling;
- Baby/Toddler/Child/Teen/Adult mesh scale updates across LifeStage changes;
- Character capsule scale/interaction footprint follows lifecycle safely;
- appearance updates without rerolling identity/genetics;
- birth/death/growth presentation remains a consumer of Core state.

## Gate B — Android Smoke / Real-device Baseline — PENDING ACTUAL VERIFIED RUN

The split #108 Android pipeline remains canonical.

Execution:
- use `mode=seed` when a fresh engine cache is required;
- successful seed chains into Android build/APK;
- use `mode=fast` only after a verified valid Android cache exists;
- `full` is evidence-driven fallback only.

Acceptance on a real Android device:
- APK installs and boots into `/Game/Maps/LifeLensWorld`;
- New Game exposes four founders/generated world;
- Observer selection/detail/camera gestures operate correctly;
- authoritative data is readable;
- movement, resource approach, fallbacks and Save/Load can be observed;
- establish FPS/memory/thermal/rendering baseline.

MetaHuman comparison happens only after this gate.

## Social Communication & Localization — AFTER CORRECTNESS CHAIN

Canonical contract: `docs/SOCIAL_COMMUNICATION_LOCALIZATION_v1.md`.

- normal-user UI defaults to Korean while Core identifiers stay language-neutral;
- real social actions become visible through approach/facing/gaze/context presentation and lightweight bubbles/icons/Event Feed/history;
- deterministic/data-driven dialogue baseline without paid LLM/API;
- Presentation never invents relationship events.

## Emotion Runtime Integration — AFTER CORRECTNESS CHAIN

Keep founders neutral at New Game, then causally connect real survival/life outcomes to emotion:
- need pressure/relief;
- environmental hazard/contamination;
- successful/failed work;
- frustration/threat/loss;
- social/family/lifecycle events.

UI must never fabricate non-zero emotion merely for visual interest.

## Later simulation milestones

- health/pathogen + water/soil contamination and premature illness/accident mortality;
- generic facilities/resources beyond sanitation;
- open-ended material/component/connection artifact runtime;
- carrying-capacity pressure / exploration / migration;
- multiple settlements / regional society / economy / politics;
- multi-generation civilization and long-run stability.

## Current parallel dispatch

**Jjun:** Lifecycle Core Correctness v1 -> Action Completion Unification -> World/Facility/Obstacle Authority -> Legacy Authority Removal -> Lifecycle Presentation Integration.

**Dagyeom:** Character Context Motion / IR-D consumer work; normal UI/Character/WorldPresentation ownership remains in that lane outside explicit assist locks.

**Android/device:** Gate B remains pending an actual verified APK/device run. Do not duplicate expensive seed/full runs blindly.

`tasks/WORK_STATE.md` contains live execution state. `tasks/TEAM_BOARD.md` contains ownership/locks/Integration Requests.
