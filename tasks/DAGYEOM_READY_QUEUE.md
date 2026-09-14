# Dagyeom READY NOW Queue

이 문서는 다겸(STILLofficial) / 다겸 측 AI가 **Core/API 대기 때문에 유휴 상태가 되지 않도록** 유지하는 실행 큐다.

## 최우선 규칙 — 상태 동기화부터

기능 수정 전에 반드시 실제 GitHub 상태를 확인한다.

1. latest `main` HEAD
2. 현재 Dagyeom branch HEAD / PR state / base / mergeability / CI
3. `tasks/WORK_STATE.md`
4. 이 READY queue
5. `tasks/TEAM_BOARD.md`
6. `tasks/HANDOFF_LOG.md`

문서가 GitHub와 다르면 **코드 수정 전에 문서부터 최신화**한다.

제품 방향 기준은 `docs/LIFELENS_SPEC_v1.1.md`와 **`docs/CIVILIZATION_PROGRESSION_v1.md`**를 함께 읽는다.

Current product checkpoints:
- #42 Production New Game Runtime — merged / UE compile PASS.
- #43 Autonomous Family Progression — merged / Core+Preflight PASS.
- #44 Full Core Save/Load — merged / deterministic continuation PASS.
- #45 SaveGame v2 — merged / Core+Preflight+UE compile PASS.
- #46 Core-authoritative action directive bridge — merged / UE Run #10 PASS.
- #47 Witness/Rumor/Social Knowledge — merged / future discovery knowledge-transmission substrate.
- #48 World Affordance Execution v1 — merge `a90ff6a5d870858a9e555ddf1e6e526bb7aa34e1`; Run #14 `34808292682` actual UE 5.6 UHT+UBT PASS.
- #49 Civilization Foundation v1 — merge `36bd1ac81192bc689c1e811553f068f255642508`; Core `34809360826` full suite + deterministic harness PASS; Preflight `34809360739` PASS.

---

## NEW CANONICAL PRODUCT DIRECTION — read before UI/Presentation work

LifeLens is **not** only a modern-household autonomous life simulator.

Long-term flow:

`Need / Curiosity → Observe → Gather → Carry/Store → Experiment → Discovery → Personal Knowledge → Craft/Build → Teach/Imitate → Shared Culture → Specialization/Exchange → Generational Civilization`

Dagyeom implications:
- Do not permanently theme the game around a finished modern house/appliances.
- Character Presentation should support primitive resources/tools, crafted objects, work surfaces and later technology.
- Stone shard, branch, fire, basket, storage pile, bronze tool and modern device are all future held/used-object categories.
- Technology is not globally auto-unlocked; knowledge can differ per resident.
- Main Observer HUD stays clean; do not turn Level 0 into a strategy resource/tech dashboard.
- Future Resident Detail may show Inventory / Known Techniques / Skill / Current Experiment after real read APIs exist.
- Future World/Civilization Detail may show major discoveries / shortages / cultural knowledge differences.
- Major discoveries can become observer notifications/cinematic events.
- Current Eat/Drink/Sleep/Toilet/Hygiene anchors are development affordances, not proof that canonical NEW GAME starts with modern appliances.

PR #49 domain types are now on main, but **civilization state is not UI-ready yet**. Jjun's current next slice is authoritative runtime/persistence integration. Do not invent UI fields before Bridge DTOs are published.

---

## RECONCILE FIRST — highest Dagyeom priority

### DQ-R1 Observer HUD v2 latest-main reconciliation

- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Re-fetch branch head before work.
- Main has advanced substantially; reconcile from actual current main and preserve newest shared docs.

### DQ-R2 Bind current Core Observer Bridge into Observer HUD

Former Jjun blockers are **READY**, not BLOCKED.

Available read surfaces:
- `ULLCoreBridgeSubsystem::GetWorldObservation()`
- `ULLCoreBridgeSubsystem::GetResidentObservations()`
- `ULLCoreBridgeSubsystem::GetResidentObservation(...)`
- `ULLCoreBridgeSubsystem::GetFamilyObservation(...)`
- `ULLCoreBridgeSubsystem::GetResidentActionDirective(...)`
- `ULLCoreBridgeSubsystem::GetRecentCoreEvents()`
- `OnCoreRuntimeStateChanged`

Existing readable data includes Relationship 13D, Emotion 11D, Physical/Social activity, family state, lifecycle/world aggregates, founder identity/personality and typed current action directives.

Important runtime facts:
- family progression can grow population; never hard-code 4 residents.
- SaveGame v2 restores Core directly; rebuild UI from Bridge after load.
- Core owns life decisions; presentation must not choose competing actions.
- Witness/rumor and civilization fields must not be invented before read APIs exist.

### DQ-R3 Resolve PR #17 Dagyeom-owned review findings

Known examples:
- hide LEVEL 0 selection hint when inspector is open;
- constrain/wrap/truncate inspector text on narrow canvases.

Then verify UHT/UBT + PIE.

---

## READY NOW — existing Dagyeom work

### DQ-01 UI Foundation / Android landscape — PR #26
- reconcile latest main; verify display metrics/safe layout.

### DQ-02 Character Presentation v1 — PR #29
- reconcile after #17.
- consume intent for visuals only; do not become simulation authority.
- keep held-object/equipment hooks data-driven rather than modern-appliance-specific.

### DQ-03 Observer UX Polish v1 — PR #30
- reconcile after #17, retarget, verify.

### DQ-04 Mobile Touch v1 — PR #36
- reconcile after parent chain; verify safe-area/touch behavior.

### DQ-05 Visual Feedback v1 — PR #38
- verify after parent chain reconciliation.

---

## BLOCKED-BY-JJUN

**현재 기존 Observer read 관련 BLOCKED-BY-JJUN 항목은 0개다.**

Future Inventory/Knowledge/Discovery UI is not a blocker for current Dagyeom work. It becomes READY only when Jjun publishes actual read DTOs.

---

## Merge dependency order

1. PR #17 latest-main reconciliation + current Bridge binding + review fixes + verification
2. PR #26 latest-main reconciliation/verification — may proceed independently
3. PR #29 / #30 after #17
4. PR #36 after #30
5. PR #38 after #36

---

## Current Dagyeom state summary

- Dagyeom is not waiting for Jjun APIs for current Observer tasks.
- Civilization direction is canonical and should influence generic presentation design now.
- Civilization data itself is not yet a current UI dependency.
- Main UI remains human/world first; deeper civilization information comes later through real APIs.
- Jjun does not edit Dagyeom UI/Character branches without explicit coordination.
