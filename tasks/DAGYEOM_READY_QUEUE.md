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

Current product checkpoints known to this queue:
- PR #42 merge `5c6f2c071ca00bcd4b26d6e002bf5c618d02ff1e` — production New Game is Core-authoritative; actual Unreal 5.6 Linux UHT/UBT PASS.
- PR #43 merge `179e3a65aaa6ff8d2243117c7aebfd760812c73d` — autonomous family progression on main; Core full tests + deterministic harness + Preflight PASS.
- PR #44 merge `2b3f9882703ed73cb8318ae262f26bebb995c209` — full authoritative Core snapshot capture/restore + deterministic continuation PASS.
- PR #45 merge `3c646ba331b8199a295fd6f2e9cac1235d844679` — Unreal SaveGame v2 persists/restores authoritative Core snapshot; Core/Preflight/UE compile PASS.
- PR #47 merge `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c` — pure-Core Witness/Rumor/Social Knowledge v1; future discovery/knowledge transmission substrate.
- PR #46 merge `753df19657ea634ea2fa7c2ac935f6273ce14c10` — Core-authoritative physical/social action directive bridge; Unreal Run #10 PASS.
- PR #48 is Jjun-side World Affordance/physical reservation execution work, currently external UE compile validation.
- `docs/CIVILIZATION_PROGRESSION_v1.md` adds autonomous resource/discovery/crafting/knowledge/civilization progression as a top-level product direction.

---

## NEW CANONICAL PRODUCT DIRECTION — read before UI/Presentation work

LifeLens is **not** only a modern-household autonomous life simulator.

Long-term product flow:

`Need / Curiosity → Observe → Gather → Carry/Store → Experiment → Discovery → Personal Knowledge → Craft/Build → Teach/Imitate → Shared Culture → Specialization/Exchange → Generational Civilization`

Important implications for Dagyeom-owned presentation/UI:

- Do not permanently theme the game around a finished modern house or modern appliances.
- Character Presentation should remain generic enough for primitive resources/tools, crafted objects, work surfaces and later technologies.
- A stone shard, branch, fire, basket, storage pile, bronze tool and modern device are all possible future held/used objects.
- Do not assume technology is globally unlocked. Knowledge can differ per resident.
- Main Observer HUD stays clean; do **not** turn Level 0 into a strategy-game resource/tech dashboard.
- Future Resident Detail may expose Inventory / Known Techniques / Skill / Current Experiment when real read APIs exist.
- Future World/Civilization Detail may expose major discoveries, shortages, shared/cultural knowledge and historical technology milestones.
- Major discoveries should be suitable for observer notifications/cinematic follow, e.g. first stable fire, first cutting tool, first successful metallurgy.
- Current Eat/Drink/Sleep/Toilet/Hygiene runtime anchors are development affordances, not proof that final NEW GAME starts with refrigerators or a complete modern home.

Do not invent placeholder civilization fields in UI before a real Bridge/read DTO exists.

---

## RECONCILE FIRST — highest Dagyeom priority

### DQ-R1 Observer HUD v2 latest-main reconciliation

- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Last verified historical HEAD: `dc3351ea9025ee33ea70f8ce1026100070c20092` — **re-fetch before work**.
- Main has advanced substantially; always reconcile from actual current main.
- Do first:
  1. fetch latest main;
  2. reconcile branch without overwriting current shared-state docs;
  3. confirm UI-owned conflicts separately from shared-doc conflicts;
  4. only then continue UI feature edits.

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

Available data includes:
- Relationship 13 dimensions + target + derived scores
- Emotion 11 dimensions + valence/arousal/intensity
- Physical/Social activity + SocialIntent + target
- typed current action directive from #46
- Family Partner/Parents/Children/Siblings
- romance/marriage/cohabitation/pregnancy state
- population/life-stage/household/couple/pregnancy/major-LifeHistory aggregate data
- founder Sex / AgeYears / LifeStage / 14-axis Personality

Important runtime facts:
- family progression is live and population can grow; never hard-code 4 residents.
- SaveGame v2 restores authoritative Core state directly; rebuild UI from Bridge/read DTOs after load.
- Core owns covered life decisions; presentation must not reintroduce a competing chooser.
- Witness/rumor provenance exists in Core but runtime/Observer exposure is not wired yet.
- Civilization Foundation fields are **not UI-ready yet**. Wait for actual read APIs.

Binding rules:
- UI reads only; do not modify Core/Simulation state.
- Do not invent placeholder values for absent data.
- Names are display data, never identity keys.
- Never hard-code the resident list to 4 entries.
- On load/state-change events, rebuild visible UI from current Bridge data.
- If a genuinely missing API/field is found, add a **new** Integration Request to `TEAM_BOARD.md`.

### DQ-R3 Resolve PR #17 Dagyeom-owned review findings

Known review examples:
- hide the LEVEL 0 selection hint once inspector is open;
- constrain/wrap/truncate inspector text on narrow canvases.

After latest-main reconciliation + Bridge binding, run local/CI UHT/UBT and PIE interaction verification before merge decision.

---

## READY NOW — existing Dagyeom work

### DQ-01 UI Foundation / Android landscape — PR #26

- Branch: `dagyeom/ui-foundation-v1`
- PR #26 OPEN; re-fetch current head before work.
- Core API dependency: none.
- Current action: reconcile with latest main; preserve newest shared-state docs; verify display metrics/safe layout.

### DQ-02 Character Presentation v1 — PR #29

- Branch: `dagyeom/character-presentation-v1`
- Stacked on PR #17; re-fetch current head.
- Current action: reconcile after #17 latest-main resolution, then retarget and verify.
- Presentation may consume current intent/action but must not become a second simulation decision authority.
- New direction: keep presentation equipment/held-object hooks data-driven; avoid hard-wiring a modern-appliance-only visual model.

### DQ-03 Observer UX Polish v1 — PR #30

- Branch: `dagyeom/observer-ux-polish-v1`
- Stacked on PR #17.
- Current action: reconcile after #17, retarget, verify.

### DQ-04 Mobile Touch v1 — PR #36

- Branch: `dagyeom/mobile-touch-v1`
- Stacked on #30.
- Current action: reconcile after parent chain; verify safe-area/touch behavior and CI.

### DQ-05 Visual Feedback v1 — PR #38

- Branch: `dagyeom/visual-feedback-v1`
- Stacked on #36.
- Current action: verify panel fade / selection flash / underline / strip highlight after parent chain reconciliation.

---

## BLOCKED-BY-JJUN

**현재 기존 Observer read 관련 BLOCKED-BY-JJUN 항목은 0개다.**

Resolved surfaces include:
1. Relationship 13D + target
2. Emotion detailed axes + summaries
3. SocialIntent + target
4. Family summary + marriage/cohabitation/pregnancy
5. World family/lifecycle aggregates
6. Blueprint/USTRUCT read-only Core Bridge
7. Founder Sex/Age/LifeStage/Personality
8. Post-load authoritative Core restoration through SaveGame v2
9. Typed Core current action directive

Future civilization Inventory/Knowledge/Discovery UI is not a blocker for current Dagyeom work. It becomes READY only when Jjun publishes read DTOs.

---

## Merge dependency order

1. PR #17 latest-main reconciliation + current Core Bridge binding + UI review fixes + verification
2. PR #26 latest-main reconciliation/verification — may proceed independently
3. PR #29 / #30 reconcile after #17
4. PR #36 after #30
5. PR #38 after #36

Do not collapse the whole chain into one giant PR merely to make merging easier.

---

## Current Dagyeom state summary

- Dagyeom is **not waiting for Jjun API implementation** for current Observer items.
- Immediate task remains reconciliation + binding + verification.
- New civilization direction is canonical now, but no new Dagyeom blocker is introduced.
- Main UI remains human/world first; civilization complexity goes into deeper layers when APIs arrive.
- Jjun should not edit Dagyeom UI/Character presentation branches without explicit coordination.
- Every checkpoint synchronizes `WORK_STATE.md` / this queue / `TEAM_BOARD.md`; meaningful completion/failure/dependency changes require handoff recording.
