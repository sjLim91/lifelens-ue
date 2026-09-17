# LifeLens Development Milestones

이 문서는 **큰 개발 단위와 integration gate의 canonical roadmap**이다. 작은 TODO 목록이 아니다.

상세 통합 감사 근거는 `docs/INTEGRATED_AUDIT_2026-09-17.md`를 따른다.
현재 실행 상태는 `tasks/WORK_STATE.md`, ownership/locks/IR은 `tasks/TEAM_BOARD.md`를 따른다.

## Delivery rule

> Same purpose + same layer + same validation scope = one milestone-sized PR.

각 milestone은 내부적으로 여러 작은 commit을 가질 수 있지만, 무거운 UE 검증과 canonical doc closeout은 의미 있는 끝점에서 한 번 수행한다.

기본 원칙:
- Core / World is the sole simulation authority.
- Presentation consumes read contracts and never fabricates outcomes.
- old branches are not merged wholesale merely because they contain once-useful code.
- latest `main`에서 실제로 빠진 변화만 재구현한다.
- exact-head CI만 merge evidence로 인정한다.

---

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
- Android pipeline timeout/split recovery #107/#108 — DONE; device gate remains separate.
- Observer Camera Control v1 #109 — DONE.
- Observer Mobile UI Polish #110 — DONE.
- Civilization Resource/Storage Spatial Authority #111 — DONE.
- Observer Resident Detail Data v1 + authoritative Traits/Preferences #112 — DONE.
- World Obstacle Collision v1 #114 — DONE as immediate clipping mitigation.
- Lifecycle Core Correctness v1 #115 — DONE.
- Action Completion Unification v1 #116 — DONE.
- Social Communication & Localization v1 #117 — DONE.
- Facilities / Tools / PrimitiveStorage v1 #118 — DONE.
- Tool Effectiveness + Held Tool #119 — DONE.
- DiggingStick / StoneHammer #120 — DONE.
- Fire / Heat / FirePit #121 — DONE.
- Furnace / Copper Smelting #122 — DONE.
- World / Facility / Obstacle Authority Normalization #123 — DONE.
- Legacy Authority Removal v1 #124 — DONE.
- Lifecycle Presentation v1 #125 — DONE.

Current completed baseline after #125:
`e6e005ba1180a4152476ab9b7a19ad9953c07287`

---

## Gate A — Integrated Runtime Checkpoint A — DONE

Runtime acceptance established the first coherent Core/World vertical slice:
- NEW GAME starts successfully.
- four founders are generated as 2 male / 2 female.
- generated start region and natural state are authoritative and deterministic.
- no starting house/toilet/farm/storage/road/tool/modern infrastructure is fabricated.
- missing-facility sanitation produces authoritative HumanWaste residue.
- Save/Load restores resident/world state without reroll.
- deterministic continuation remains stable after restore.

---

## Correctness Chain — DONE THROUGH #125

The 2026-09-16~17 correctness chain is now closed:

1. Lifecycle Core Correctness — #115.
2. Action Completion Unification — #116.
3. Social Communication/Localization — #117.
4. Facilities/Tools/Fire/Smelting — #118~#122.
5. World/Facility/Obstacle Authority Normalization — #123.
6. Legacy Authority Removal v1 — #124.
7. Lifecycle Presentation v1 — #125.

Result:
- Social/Civilization/Parenting outcomes are completion-aware.
- Core owns action/target/resource/facility/lifecycle truth.
- WorldPresentation no longer defines obstacle authority.
- obsolete callable legacy AI/projection mutation surfaces were removed.
- deceased physical actors are cleaned up from living projection.
- LifeStage growth updates body/capsule presentation without rerolling identity.

Known v1 visual limitation:
- younger stages still use uniform scale of an adult Quaternius skeleton.

---

## Milestone 0 — Canonical Integrated Roadmap Reconciliation — CURRENT ADMINISTRATIVE CLOSEOUT

Purpose:
- reconcile docs through #125;
- record Jjun + Dagyeom integrated audit;
- release stale Lifecycle Presentation assist lock;
- establish one roadmap with separate ownership lanes;
- prevent reimplementation from stale branches.

Artifacts:
- `docs/INTEGRATED_AUDIT_2026-09-17.md`
- `tasks/WORK_STATE.md`
- `docs/DEVELOPMENT_MILESTONES.md`
- `tasks/TEAM_BOARD.md`

No functional runtime change belongs in this milestone.

---

## Milestone 1 — Dagyeom World Readability Envelope — ACTIVE PR #100

Owner: Dagyeom WorldPresentation.
Branch: `dagyeom/world-visual-readability-envelope`.

Purpose:
- improve resident/facility readability without changing Core world/resource authority.

Scope:
- 0~1200 UU living core: aggressive ambient vegetation thinning.
- 1200~3000 UU activity zone: gradual density restoration.
- outside activity zone: natural density unchanged.
- reference center follows Core `InitialCenterGrid`.
- ResourcePatch positions remain authoritative; presentation may scale/thin visuals only.

Merge rule:
- latest exact-head required CI must pass.
- PIE/device visual quality can remain follow-up QA if structural/runtime authority is clean.

---

## Milestone 2 — Knowledge Transmission Spatial Authority v1

Owner: Jjun Core/Simulation.

Problem:
- civilization witness/teaching currently does not require a real physical encounter.

Purpose:
- make knowledge propagation a spatial/social event rather than remote global probability.

Required scope:
- discovery/craft witness eligibility requires authoritative proximity/encounter.
- teaching requires real teacher/learner meeting/context completion.
- no cross-settlement remote knowledge transfer.
- deterministic outcomes remain seed-stable.
- Save/Load does not duplicate pending teaching outcomes.

Validation:
- Core regression tests.
- deterministic harness.
- Preflight.
- Unreal compile only if bridge/read contract changes require it.

---

## Milestone 3 — Character Context Motion v2

Owner: Dagyeom Character Presentation.

Provider status:
- Context Action contract, spatial targets, completion ACK, social facing/talking, tool presentation are already merged.

Current presentation limitation:
- many concrete actions still collapse into broad `Interact / Gather / Build` fallback states.

Scope:
- Sit / Stand / Lie / Wake.
- PickUp / Carry / Use.
- Gather / Cut / Chop / Dig / Strike.
- Craft / Build / Fire / Smelt.
- Parenting care.
- sanitation interaction.
- locomotion ↔ context transition cleanup.
- gaze/body orientation where the authoritative target exists.

Asset policy:
- use existing Quaternius animations first (`Sitting_*`, `PickUp_Table`, `Fixing_Kneeling`, `Interact`, `Death01`, etc.).
- add new assets only when existing CC0/free assets cannot truthfully represent the action.

Authority rule:
- animation consumes Core/World directive and target.
- Presentation cannot commit simulation outcomes or bypass ACK.

---

## Milestone 4 — Observer Readability + Real Scrolling

Owner: Dagyeom UI/Observer.

Source note:
- old PR #98 is stale and must not be merged wholesale.
- only still-useful ideas are selectively reimplemented on current main.

Scope:
- true scrollable Detail content.
- generated-world panel opacity/readability tuning.
- retain clear Quick -> Detail observation hierarchy.
- debug resident framing helpers may exist only as QA-only tooling.
- deceased/history view must read Core observer/history DTOs rather than living-only physical projection.

---

## Milestone 5 — Emotion Runtime Integration v1

Owner: Jjun Core provider; Dagyeom presentation consumer.

Existing Core foundation:
- multi-dimensional `EmotionState`.
- event deltas.
- valence/arousal summary.
- decay.

Purpose:
- connect emotion to actual lived outcomes rather than decorative UI values.

Scope:
- need pressure and relief.
- work/craft success and failure.
- repeated frustration.
- threat/contamination/environmental exposure.
- parenting/family care.
- social/romantic events.
- lifecycle loss/grief.
- civilization discovery/crafting accomplishment.

Rules:
- New Game founders remain neutral until events occur.
- emotion changes only from authoritative Core events/state.
- UI never fabricates emotion for visual interest.

---

## Milestone 6 — Lifecycle Event Presentation v2

Owner: Dagyeom Presentation.

Already done in #125:
- deceased resident removal from living physical projection.
- growth body/capsule scaling.

Next scope:
- birth event visibility.
- growth transition visibility.
- death event/history visibility.
- deceased family/history inspection from Core DTOs.
- optional truthful death animation without turning corpse/grave into invented world truth.

Corpse/grave systems, if added later, require explicit Core/world contracts first.

---

## Milestone 7 — Civilization Phase 2

Owner: Jjun Core/World provider -> Dagyeom Presentation.

Existing stable progression:
- no free starting infrastructure.
- PrimitiveStorage.
- SharpFlake / StoneCuttingTool / DiggingStick / StoneHammer.
- FirePit / Heat / Charcoal.
- Furnace / CopperOre / CopperMetal / CopperSmelting.

Next progression candidates:
- SleepingPlace.
- Shelter.
- WorkSurface / primitive workbench precursor.
- facility effects on sleep/comfort/work efficiency.
- TinOre.
- Bronze.
- bronze tool progression.

Rule:
- every facility/resource/tool/technology must be discovered/constructed/produced through Core progression.
- Presentation never creates free infrastructure.

---

## Milestone 8 — Cleanup + Long-run Performance

Owner: Jjun.

Scope:
- remove `SimulationSnapshotCodecLegacy.cpp` when current-only compatibility checks no longer require it.
- delete stale Preflight marker/path assertions that pin removed legacy surfaces.
- explicit per-frame simulation catch-up budget.
- residue/HISM refresh profiling.
- population growth and long-run CPU/memory checks.

---

## Milestone 9 — Health / Disease / Premature Mortality

Owner: Jjun provider -> Dagyeom presentation.

Scope candidates:
- pathogens and infection exposure.
- contaminated water/soil.
- sanitation-linked health risk.
- illness/recovery.
- environment/accident mortality.
- health effect on pregnancy/parenting/work where appropriate.

Health rules must remain deterministic/testable and must not be inferred by UI.

---

## Milestone 10 — Migration / Multiple Settlements / Economy / Society

Long-range simulation scope:
- carrying-capacity pressure.
- exploration.
- household split/migration.
- multiple settlements.
- jobs/specialization.
- economy/trade.
- institutions/governance/politics.
- multi-generation social continuity.

This milestone depends on spatial knowledge transmission and long-run performance being trustworthy first.

---

## Gate B — Android Smoke / Real-device Baseline — PAUSED BY USER

Status changed on **2026-09-17**: user requested Android/APK work be set aside while the rest of the project proceeds.

The gate remains canonical but is not currently executable work.

When explicitly resumed:
- use `mode=seed` only when a fresh engine cache is genuinely required;
- use `mode=fast` only after verified valid cache exists;
- use `full` only as evidence-driven fallback;
- never repeat long seed/full runs blindly.

Real-device acceptance remains:
- APK installs/boots into `/Game/Maps/LifeLensWorld`;
- four founders/generated world visible;
- Observer selection/detail/camera gestures work;
- authoritative data readable;
- movement/fallback/resource actions observable;
- Save/Load observable;
- FPS/memory/thermal/rendering baseline established.

MetaHuman comparison remains after this gate.

---

## Current parallel dispatch

**Jjun:** docs reconciliation -> Knowledge Transmission Spatial Authority -> Emotion Runtime -> Civilization Phase 2 -> cleanup/performance -> health/society milestones.

**Dagyeom:** PR #100 closeout -> Character Context Motion v2 -> Observer Readability/Scrolling -> Lifecycle Event Presentation -> presentation support for later Core milestones.

**Android/device:** Gate B is PAUSED until explicit user resume.

This is one integrated schedule. Ownership lanes are parallel execution lanes, not separate roadmaps.
