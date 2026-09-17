# LifeLens Development Milestones

이 문서는 **큰 개발 단위와 integration gate의 canonical roadmap**이다.

현재 실행 상태는 `tasks/WORK_STATE.md`, ownership/locks/IR은 `tasks/TEAM_BOARD.md`, 상세 감사 근거는 `docs/INTEGRATED_AUDIT_2026-09-17.md`를 따른다.

## Delivery rule

> Same purpose + same layer + same validation scope = one milestone-sized PR.

기본 원칙:
- Core / World is the sole simulation authority.
- Presentation consumes read/action contracts and never fabricates outcomes.
- stale branches are not merged wholesale.
- latest `main`에서 실제로 빠진 변화만 재구현한다.
- functional merge evidence는 exact-head CI를 기준으로 한다.

---

## Completed foundations

주요 완료 기반:
- Character / Appearance / World Affordance / Residue / Runtime ACK / Save-Load — DONE.
- World Genesis WG-1/WG-2 / World Generation Milestone A / production map integration — DONE.
- Context Action Contract v1 / spatial authority / Observer camera + resident detail — DONE.
- Lifecycle Core Correctness #115 — DONE.
- Action Completion Unification #116 — DONE.
- Social Communication & Localization #117 — DONE.
- Facilities / Tools / PrimitiveStorage #118 — DONE.
- Tool Effectiveness #119 — DONE.
- DiggingStick / StoneHammer #120 — DONE.
- Fire / Heat / FirePit #121 — DONE.
- Furnace / Copper Smelting #122 — DONE.
- World / Facility / Obstacle Authority Normalization #123 — DONE.
- Legacy Authority Removal v1 #124 — DONE.
- Lifecycle Presentation v1 #125 — DONE.
- Social bubble readability #126 — DONE.
- Integrated audit / canonical roadmap reconciliation #127 — DONE.
- Mac editor `-Wshadow` unblocker #128 — DONE.
- Knowledge Transmission Spatial Authority v1 #130 — DONE.

Current completed functional baseline:
`fb1842ad60c25f0054eb040f46d757340f65991c`

---

## Gate A — Integrated Runtime Checkpoint A — DONE

Acceptance already established:
- NEW GAME boots with four founders, 2 male / 2 female.
- initial world/population generation is deterministic.
- no free starting house/toilet/farm/storage/road/tool/modern infrastructure.
- missing sanitation creates authoritative HumanWaste residue.
- Save/Load restores state without identity reroll.
- continuation remains deterministic.

---

## Correctness / Authority Chain — DONE THROUGH #130

Completed chain:
1. Lifecycle Core Correctness — #115.
2. Action Completion Unification — #116.
3. Social Communication/Localization — #117.
4. Facilities/Tools/Fire/Smelting — #118~#122.
5. World/Facility/Obstacle Authority Normalization — #123.
6. Legacy Authority Removal v1 — #124.
7. Lifecycle Presentation v1 — #125.
8. Knowledge Transmission Spatial Authority v1 — #130.

Result:
- Social/Civilization/Parenting outcomes require completion-aware authority.
- Core owns target/resource/facility/lifecycle/knowledge truth.
- knowledge witness is spatially local rather than globally telepathic.
- direct teaching is a physical `KnowledgeTeaching` ContextAction and applies only after real approach/talking/ACK.
- pending teaching is not replayed after Save/Load.

#130 exact-head validation:
- Preflight #715 PASS.
- Core Tests #664 PASS.
- Unreal Linux Compile #203 PASS.

---

## Milestone 0 — Canonical Integrated Roadmap Reconciliation — DONE

Completed by #127 and refreshed after #130.

Purpose:
- one Jjun + Dagyeom roadmap;
- current ownership / locks / IR separated from live execution state;
- integrated audit retained as historical evidence;
- Android remains a paused gate rather than disappearing from the plan.

---

## Milestone 1 — Dagyeom World Readability Envelope — ACTIVE PR #100

Owner: Dagyeom WorldPresentation.
Branch: `dagyeom/world-visual-readability-envelope`.
Current head: `62b191a6afb2c0c5ae32ddbb1e0ae7876ae00556`.

Purpose:
- improve resident/facility readability without changing Core world/resource authority.

Current evidence:
- Preflight #707 PASS.
- Unreal Linux Compile #200 PASS.
- Mac editor build blocker removed by #128.

Remaining:
- Mac PIE final visual confirmation;
- merge if visually acceptable.

---

## Milestone 2 — Knowledge Transmission Spatial Authority v1 — DONE / PR #130

Owner: Jjun Core/Simulation/World.
Merge: `fb1842ad60c25f0054eb040f46d757340f65991c`.

Delivered:
- witness eligibility limited to real nearby residents;
- no direct hourly remote teaching mutation;
- teacher/learner/technique carried by typed ContextAction;
- teacher physically approaches the learner and talks before ACK;
- World follows the live learner actor while Core performs final spatial revalidation;
- wrong token / far completion rejected;
- deterministic Core tests;
- pending teaching dropped on restore.

This milestone closes the remote-knowledge-telepathy audit defect.

---

## Milestone 3 — Character Context Motion v2

Owner: Dagyeom Character Presentation.

Provider status:
- ContextAction target/ACK contracts are merged.
- #130 adds authoritative `KnowledgeTeaching` target + technique + token data.

Scope:
- Sit / Stand / Lie / Wake only when compatible real facilities exist.
- PickUp / Carry / Use.
- Gather / Cut / Chop / Dig / Strike.
- Craft / Build / Fire / Smelt.
- Parenting care.
- sanitation interaction.
- teaching / social context motion.
- gaze/body orientation from authoritative targets.

Asset policy:
- existing Quaternius CC0 animations first.
- never show sitting/lying on invented furniture.

---

## Milestone 4 — Observer Readability + Real Scrolling

Owner: Dagyeom UI/Observer.

Rules:
- old PR #98 is stale; do not merge wholesale.
- selectively reimplement still-useful concepts on current main.

Scope:
- true scrollable Detail content;
- generated-world readability tuning;
- QA resident framing helpers separated from production camera behavior;
- teaching/lifecycle/history labels consume authoritative provider data.

---

## Milestone 5 — Emotion Runtime Integration v1 — NEXT JJUN PROVIDER WORK

Owner: Jjun Core provider; Dagyeom presentation consumer.

Existing foundation:
- multi-dimensional `EmotionState`;
- emotion event deltas;
- valence/arousal summary;
- decay.

Purpose:
- make emotion causally reflect lived simulation outcomes rather than decorative values.

Scope:
- need pressure -> bounded anxiety/fear;
- successful need resolution -> relief/joy;
- action/craft success -> pride/joy;
- repeated failure -> anger/anxiety;
- environment threat/contamination;
- parenting/family care;
- existing social/romantic events consistency;
- close-person death/loss -> grief scaled by relationship/family;
- civilization discovery/craft accomplishment;
- persistence + deterministic tests;
- bounded utility influence so survival needs remain dominant.

Rules:
- founders remain neutral until events occur.
- UI never invents emotion.

---

## Milestone 6 — Lifecycle Event Presentation v2

Owner: Dagyeom Presentation.

Already done:
- dead residents removed from living physical projection;
- stage growth updates body/capsule scale.

Next:
- birth visibility;
- growth transition visibility;
- death/history visibility;
- deceased inspection from Core DTO/history;
- optional truthful death animation without inventing corpse/grave authority.

---

## Milestone 7 — Civilization Phase 2

Owner: Jjun Core/World provider -> Dagyeom presentation.

Existing stable progression:
- PrimitiveStorage;
- SharpFlake / StoneCuttingTool / DiggingStick / StoneHammer;
- FirePit / Heat / Charcoal;
- Furnace / CopperOre / CopperMetal / CopperSmelting.

Next candidates:
- SleepingPlace;
- Shelter;
- WorkSurface;
- facility effects on sleep/comfort/work efficiency;
- TinOre;
- Bronze / bronze tools.

No free starting infrastructure.

---

## Milestone 8 — Cleanup + Long-run Performance

Owner: Jjun.

Scope:
- remove `SimulationSnapshotCodecLegacy.cpp` after compatibility checks are updated;
- remove stale Preflight legacy markers;
- explicit per-frame simulation catch-up budget;
- residue/HISM refresh profiling;
- population growth CPU/memory stability.

---

## Milestone 9 — Health / Disease / Premature Mortality

Scope candidates:
- pathogens / infection;
- contaminated water/soil;
- sanitation-linked health;
- illness/recovery;
- environment/accident mortality;
- deterministic, testable health effects.

---

## Milestone 10 — Migration / Multiple Settlements / Economy / Society

Long-range scope:
- carrying-capacity pressure;
- exploration;
- household migration;
- multiple settlements;
- jobs/specialization;
- trade/economy;
- institutions/governance/politics;
- multi-generation social continuity.

Depends on trustworthy spatial knowledge and long-run performance.

---

## Gate B — Android Smoke / Real-device Baseline — PAUSED BY USER

Do not run Android builds until explicitly resumed.

When resumed:
- prefer cached/fast paths after verified engine cache;
- avoid blind repeat of long seed/full builds;
- validate APK boot, four founders, Observer controls/data, movement/actions, Save/Load, FPS/memory/thermal baseline.

MetaHuman comparison remains after this gate.

---

## Current parallel dispatch

**Jjun:** Emotion Runtime Integration v1 -> Civilization Phase 2 -> cleanup/performance -> health/society milestones.

**Dagyeom:** #100 closeout -> Character Context Motion v2 -> Observer Readability/Scrolling -> Lifecycle Event Presentation -> presentation support for later Core milestones.

**Android/device:** Gate B PAUSED until explicit user resume.

Cross-lane request checkpoints from `tasks/TEAM_BOARD.md` are mandatory at work start/end, long-build boundaries and before main-changing merges/rebases.
