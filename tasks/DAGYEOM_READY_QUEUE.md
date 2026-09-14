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

**쭌이 다겸 작업을 도울 때는 `docs/INTEGRATION_SPRINT.md`를 반드시 읽는다.** 기본 지원은 REVIEW_ONLY다. 실제 다겸 소유 코드 수정이 필요하면 TEAM_BOARD에 `ASSIST_LOCK`을 먼저 만들고, 쭌은 `integration/dagyeom-<scope>-assist` branch를 사용한다. `dagyeom/*` branch에 쭌 AI가 직접 push하지 않는다.

Current product checkpoints:
- #42 Production New Game Runtime — merged / UE compile PASS.
- #43 Autonomous Family Progression — merged / Core+Preflight PASS.
- #44 Full Core Save/Load — merged / deterministic continuation PASS.
- #45 SaveGame v2 — merged / Core+Preflight+UE compile PASS.
- #46 Core-authoritative action directive bridge — merged / UE Run #10 PASS.
- #47 Witness/Rumor/Social Knowledge — merged.
- #48 World Affordance Execution v1 — merge `a90ff6a5d870858a9e555ddf1e6e526bb7aa34e1`; UE Run #14 PASS.
- #49 Civilization Foundation v1 — merge `36bd1ac81192bc689c1e811553f068f255642508`.
- #50 Civilization Runtime State + Persistence v1 — merge `c31c422c305a3a79a9553d37ac86247aa31d1853`.
- #51 Autonomous Civilization Action Loop v1 — merge `55d5211160c8edad32b01177e2b9326a9faa2b78`.
- #52 Civilization Knowledge Transmission v1 — merge `b90da9242003fbc0cbc553605b9abc46a17aa044`; PR Core `34814310235` PASS 37/37 + deterministic harness; Preflight `34814310291` PASS.
- #53 Civilization Observer Read DTOs v1 — **OPEN / latest head `e3a9f661...`**; Core `34821150702` PASS, Preflight `34821150693` PASS, Unreal Linux Compile Run #16 `34821150704` PASS including UHT/UBT/link. Needs only latest-main reconciliation/merge.
- #54 macOS clang shadow hotfix — merge `3b649b900c44a4e48bb89171b38f5e685e757b14`; Preflight `34822441249` PASS, Core `34822441296` PASS. Dagyeom R1 macOS `-Wshadow -Werror` blocker released.

---

## CANONICAL PRODUCT DIRECTION — read before UI/Presentation work

LifeLens is **not** only a modern-household autonomous life simulator.

`Need / Curiosity → Observe → Gather → Carry/Store → Experiment → Discovery → Personal Knowledge → Craft/Build → Teach/Imitate → Shared Culture → Specialization/Exchange → Generational Civilization`

Dagyeom implications:
- Do not permanently theme the game around a finished modern house/appliances.
- Character Presentation should support primitive resources/tools, crafted objects, work surfaces and later technology.
- Technology is not globally auto-unlocked; knowledge can differ per resident.
- Main Observer HUD stays clean; do not turn Level 0 into a strategy resource/tech dashboard.
- Major discoveries can become observer notifications/cinematic events.
- Current Eat/Drink/Sleep/Toilet/Hygiene anchors are development affordances, not canonical starting-world content.

After #52 authoritative Core now has:
- per-resident Inventory / personal Knowledge / skills;
- World ResourceNode / shared StorageSite;
- autonomous Gather / Store / Experiment / Craft;
- witness / imitation / deliberate teaching of techniques;
- authoritative fact/receipt/transmissionPath provenance;
- Save/Load deterministic continuation including provenance through snapshot binary v3.

#53 publishes the read-only resident/world civilization DTO contract. **Do not bind the civilization-specific DTO until #53 is actually merged**, even though its Core/Preflight/UHT/UBT gates are now green. Existing Observer reconciliation can continue independently.

---

## INTEGRATION SPRINT RULE — Jjun may help, but without branch collision

After #53 merges, Jjun enters Integration Support instead of immediately starting another large Core feature.

### Default

`REVIEW_ONLY`

Jjun may inspect this queue, Dagyeom PRs, diffs, merge conflicts and CI and may leave exact fixes, but does not write Dagyeom-owned files.

### If user asks Jjun to patch Dagyeom code

Before any edit:
1. Re-fetch the exact target Dagyeom PR/branch HEAD and CI.
2. Register `ASSIST_LOCK` in `tasks/TEAM_BOARD.md` with exact paths.
3. Create `integration/dagyeom-<scope>-assist` from that exact Dagyeom branch HEAD.
4. Jjun edits only the locked paths on the assist branch.
5. Dagyeom side does not edit the same locked paths until handoff/merge.
6. After integration, remove the lock and return ownership to Dagyeom.

Do **not** direct-push from Jjun into `dagyeom/*` branches.

### Parent-first protection

- First priority: PR #17.
- #26 can be worked independently only where file locks do not overlap #17.
- Do not mass-rebase #29/#30/#36/#38 while #17 is unresolved.
- After #17, reconcile #29/#30; then #36; then #38.

---

## RECONCILE FIRST — highest Dagyeom priority

### DQ-R1 Observer HUD v2 latest-main reconciliation

- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Re-fetch branch head before work.
- Reconcile from actual current main and preserve newest shared docs.
- **macOS clang blocker is resolved on main by PR #54 merge `3b649b900c44a4e48bb89171b38f5e685e757b14`; R1 may resume now.**
- If Jjun is asked to actively help, use the Integration Sprint ASSIST_LOCK protocol above; do not both edit the same HUD files simultaneously.

### DQ-R2 Bind current Core Observer Bridge into Observer HUD

Existing READY read surfaces remain:
- `ULLCoreBridgeSubsystem::GetWorldObservation()`
- `ULLCoreBridgeSubsystem::GetResidentObservations()`
- `ULLCoreBridgeSubsystem::GetResidentObservation(...)`
- `ULLCoreBridgeSubsystem::GetFamilyObservation(...)`
- `ULLCoreBridgeSubsystem::GetResidentActionDirective(...)`
- `ULLCoreBridgeSubsystem::GetRecentCoreEvents()`
- `OnCoreRuntimeStateChanged`

Existing readable data includes Relationship 13D, Emotion 11D, Physical/Social activity, family state, lifecycle/world aggregates, founder identity/personality and typed current action directives.

After #53 merges, new civilization getters become bindable:
- `ULLCoreBridgeSubsystem::GetResidentCivilizationObservation(...)`
- `ULLCoreBridgeSubsystem::GetCivilizationWorldObservation(...)`

Important runtime facts:
- family progression can grow population; never hard-code 4 residents;
- SaveGame restores authoritative Core snapshot; rebuild UI from Bridge after load;
- Core owns life decisions; presentation must not choose competing actions;
- civilization detail belongs in selected-resident/detail/major-discovery layers, not Level 0 resource-dashboard clutter.

### DQ-R3 Resolve PR #17 Dagyeom-owned review findings

Known examples:
- hide LEVEL 0 selection hint when inspector is open;
- constrain/wrap/truncate inspector text on narrow canvases.

Then verify UHT/UBT + PIE when available.

---

## READY NOW — existing Dagyeom work

### DQ-01 UI Foundation / Android landscape — PR #26
- reconcile latest main; verify display metrics/safe layout.
- May proceed independently from #17 only if its files are not under an active ASSIST_LOCK for #17.

### DQ-02 Character Presentation v1 — PR #29
- reconcile after #17;
- consume intent for visuals only; do not become simulation authority;
- keep held-object/equipment hooks data-driven rather than modern-appliance-specific.

### DQ-03 Observer UX Polish v1 — PR #30
- reconcile after #17, retarget, verify.

### DQ-04 Mobile Touch v1 — PR #36
- reconcile after parent chain; verify safe-area/touch behavior.

### DQ-05 Visual Feedback v1 — PR #38
- verify after parent chain reconciliation.

---

## BLOCKED-BY-JJUN

**현재 PR #17 R1을 막는 BLOCKED-BY-JJUN 항목은 0개다.** macOS clang shadow blocker는 #54로 해제됐다.

Civilization-detail binding is conditional only on PR #53 being merged. This does not block current PR #17/#26 reconciliation.

---

## Current Jjun direction visible to Dagyeom

Current Jjun task: reconcile/merge PR #53. Core/Preflight/UE Run #16 are all green.

After #53: **Jjun pauses new large Core slices and helps close the integration gap under `docs/INTEGRATION_SPRINT.md`.**

Observer-first rule remains: Level 0 clean; detailed civilization information belongs in selected-resident/detail views and major-discovery notifications.

---

## Merge dependency order

1. PR #53 latest-main reconcile + merge
2. PR #17 latest-main reconciliation + current Bridge/civilization binding + review fixes + verification
3. PR #26 latest-main reconciliation/verification — may proceed independently where locks do not overlap
4. PR #29 / #30 after #17
5. PR #36 after #30
6. PR #38 after #36
7. Integrated runtime verification, then Android smoke APK

---

## Current Dagyeom state summary

- PR #17 R1 macOS Core compile blocker is resolved on main.
- Dagyeom is not waiting for Jjun APIs for current Observer reconciliation tasks.
- Civilization direction is canonical and should influence generic presentation design now.
- Civilization detail UI waits only for #53 merge rather than placeholders.
- Main UI remains human/world first.
- Jjun may actively help after #53, but only through REVIEW_ONLY or explicit ASSIST_LOCK + integration assist branch so the two workstreams do not collide.