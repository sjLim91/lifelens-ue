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

Current product checkpoints known to this queue:
- PR #42 merge `5c6f2c071ca00bcd4b26d6e002bf5c618d02ff1e` — production New Game is Core-authoritative; actual Unreal 5.6 Linux UHT/UBT PASS.
- PR #43 merge `179e3a65aaa6ff8d2243117c7aebfd760812c73d` — autonomous family progression is on main; Core full tests + deterministic harness + Preflight PASS.
- PR #44 merge `2b3f9882703ed73cb8318ae262f26bebb995c209` — full authoritative Core snapshot capture/restore + deterministic continuation PASS.
- PR #45 merge `3c646ba331b8199a295fd6f2e9cac1235d844679` — Unreal SaveGame v2 now persists/restores the authoritative Core snapshot; Core/Preflight/actual UE 5.6 UHT+UBT all PASS.
- PR #47 merge `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c` — pure-Core Witness/Rumor/Social Knowledge v1 is available; no UI binding required yet because live Simulation/read DTO wiring is deferred.
- PR #46 merge `753df19657ea634ea2fa7c2ac935f6273ce14c10` — Core-authoritative physical/social action directive bridge is on main; Run #10 (`34805882789`) actual UE 5.6 UHT+UBT PASS.

---

## RECONCILE FIRST — highest Dagyeom priority

### DQ-R1 Observer HUD v2 latest-main reconciliation

- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Last verified historical HEAD: `dc3351ea9025ee33ea70f8ce1026100070c20092` — **re-fetch before work**.
- Current product main has advanced through #42/#43/#44/#45/#47/#46 since the old branch snapshot.
- Do first:
  1. fetch latest main;
  2. reconcile branch with current main without overwriting current shared-state docs;
  3. confirm UI-owned code conflicts separately from shared-doc conflicts;
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
- typed current action directive from #46: physical Eat/Drink/Sleep/Toilet/Hygiene or social Approach/Avoid/Repair/Comfort + stable target ResidentId
- Family Partner/Parents/Children/Siblings
- romance/marriage/cohabitation/pregnancy state
- population/life-stage/household/couple/pregnancy/major-LifeHistory aggregate data
- founder Sex / AgeYears / LifeStage / 14-axis Personality from PR #42

Important runtime facts:
- PR #43: family progression is live; residents can date/cohabit/engage/marry, pregnancies can progress, and births add real World residents. Never hard-code population to 4.
- PR #45: SaveGame v2 restores the authoritative Core state directly. After load, UI should refresh from Bridge/read DTOs and must not treat cached UI-side resident arrays as simulation truth.
- PR #46: WorldDirector no longer chooses covered life actions independently; Core is the authority and Unreal mirrors typed action directives. UI/presentation must not reintroduce a competing action chooser.
- PR #47: witness/rumor provenance exists in pure Core, but runtime/Observer exposure is not wired yet; do not invent rumor UI fields until an actual read API exists.

Binding rules:
- UI reads only; do not modify Core/Simulation state.
- Do not invent placeholder values for absent data.
- Names are display data, never identity keys.
- Never hard-code the resident list to 4 entries.
- On load/state-change events, rebuild visible UI state from current Bridge data.
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
- PR #26 OPEN, historical HEAD `70dfa5ebeabf24b661c9f9fd0bc63e3ad01ac180`; re-fetch before work.
- Core API dependency: none.
- Current action: reconcile with latest main; preserve newest shared-state docs; verify display metrics/safe layout.

### DQ-02 Character Presentation v1 — PR #29

- Branch: `dagyeom/character-presentation-v1`
- Historical HEAD `a4d47b9f69c9270665a8e2613b7863c40bf0f83e`; stacked on PR #17.
- Current action: reconcile after #17 latest-main resolution, then retarget and verify.
- Important #46 constraint: Character Presentation may consume current intent/action for visuals, but must not become a second simulation decision authority.
- Jjun decision request remains: camera distance/FOV visual size and whether legacy DebugBody should be removed from file rather than runtime-hidden.

### DQ-03 Observer UX Polish v1 — PR #30

- Branch: `dagyeom/observer-ux-polish-v1`
- Historical HEAD `c420457c3ddbc73ac2ddbfe692dd20ce63edf45e`; stacked on PR #17.
- Current action: reconcile after #17, retarget, verify.

### DQ-04 Mobile Touch v1 — PR #36

- Branch: `dagyeom/mobile-touch-v1`
- Historical HEAD `15eec5b9216d6a30655f59450e410f0f80bb0343`; stacked on #30.
- Current action: reconcile after parent chain; verify safe-area/touch behavior and CI.

### DQ-05 Visual Feedback v1 — PR #38

- Branch: `dagyeom/visual-feedback-v1`
- Historical HEAD `ffbc32c0cc9465a46feb1491f0bb7d5e0d1cd57a`; stacked on #36.
- Current action: verify panel fade / selection flash / underline / strip highlight after parent chain reconciliation.

---

## BLOCKED-BY-JJUN

**현재 기존 Observer read 관련 BLOCKED-BY-JJUN 항목은 0개다.**

Resolved surfaces now include:
1. Relationship 13D + target resident ID/name
2. Emotion detailed axes + summaries
3. SocialIntent + target
4. Family summary + marriage/cohabitation/pregnancy
5. World family/lifecycle aggregates
6. Blueprint/USTRUCT read-only Core Bridge
7. Founder Sex/Age/LifeStage/Personality identity fields
8. Post-load authoritative Core restoration through SaveGame v2
9. Typed Core current action directive for physical/social presentation

A new blocker is valid only if Dagyeom identifies a concrete missing data/API after using latest `main`. Record requested field, usage and related PR in `TEAM_BOARD.md`.

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

- Dagyeom is **not waiting for Jjun API implementation** for the previous Observer items.
- Immediate task is **reconciliation + binding + verification**, not Core modification.
- Main now starts at four founders but may gain descendants; UI must remain population-dynamic.
- Save/load no longer reconstructs the world from UI compatibility data; after load, read current Bridge state.
- Current physical/social action presentation can read #46 typed directives; do not make presentation code choose competing simulation actions.
- Witness/rumor UI is not READY yet because #47 is domain-only until runtime/read-model wiring exists.
- Jjun should not edit Dagyeom UI/Character presentation branches without a new explicit coordination request.
- Every checkpoint synchronizes `WORK_STATE.md` / this queue / `TEAM_BOARD.md`; meaningful completion/failure/dependency changes also require handoff recording.
