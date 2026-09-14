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

Current product checkpoint known to this queue: PR #40 merge `9261581df3abd5332d92855628fd7d03203748af` — latest Core Observer Bridge + family/world observer reads are on `main` and actual Unreal 5.6 Linux UHT/UBT passed.

---

## RECONCILE FIRST — highest Dagyeom priority

### DQ-R1 Observer HUD v2 latest-main reconciliation

- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Verified HEAD: `dc3351ea9025ee33ea70f8ce1026100070c20092`
- Current fact: PR is OPEN and currently not mergeable against latest main.
- Reason: PR branch predates many Jjun Core/Bridge merges and shared-doc updates.
- Do first:
  1. fetch latest main;
  2. reconcile branch with current main without overwriting current shared-state docs;
  3. confirm UI-owned code conflicts separately from shared-doc conflicts;
  4. only then continue UI feature edits.

### DQ-R2 Bind current Core Observer Bridge into Observer HUD

Former Jjun blockers are now **READY**, not BLOCKED.

Available read surfaces on main:
- `ULLCoreBridgeSubsystem::GetWorldObservation()`
- `ULLCoreBridgeSubsystem::GetResidentObservations()`
- `ULLCoreBridgeSubsystem::GetResidentObservation(...)`
- `ULLCoreBridgeSubsystem::GetFamilyObservation(...)`
- `ULLCoreBridgeSubsystem::GetRecentCoreEvents()`
- `OnCoreRuntimeStateChanged`

Available data includes:
- Relationship 13 dimensions + target + derived scores
- Emotion 11 dimensions + valence/arousal/intensity
- Physical/Social activity + SocialIntent + target
- Family Partner/Parents/Children/Siblings
- romance/marriage/cohabitation/pregnancy state
- population/life-stage/household/couple/pregnancy/major-LifeHistory aggregate data

Binding rules:
- UI reads only; do not modify Core/Simulation state.
- Do not invent placeholder values for absent data.
- Names are display data, never identity keys.
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
- PR #26 OPEN, verified HEAD `70dfa5ebeabf24b661c9f9fd0bc63e3ad01ac180`
- Core API dependency: none.
- Current action: reconcile with latest main; preserve newest shared-state docs; address known display-metrics link concern if still applicable; verify.

### DQ-02 Character Presentation v1 — PR #29

- Branch: `dagyeom/character-presentation-v1`
- PR #29 OPEN, verified HEAD `a4d47b9f69c9270665a8e2613b7863c40bf0f83e`
- Dependency: stacked on PR #17.
- Current action: wait for/reconcile after #17 latest-main resolution, then retarget and verify.
- Jjun decision request still open: camera distance/FOV visual size and whether legacy DebugBody should be removed from file rather than runtime-hidden.

### DQ-03 Observer UX Polish v1 — PR #30

- Branch: `dagyeom/observer-ux-polish-v1`
- PR #30 OPEN, verified HEAD `c420457c3ddbc73ac2ddbfe692dd20ce63edf45e`
- Dependency: stacked on PR #17.
- Current action: reconcile after #17, retarget, verify.

### DQ-04 Mobile Touch v1 — PR #36

- Branch: `dagyeom/mobile-touch-v1`
- PR #36 OPEN, verified HEAD `15eec5b9216d6a30655f59450e410f0f80bb0343`
- Dependency: stacked on PR #30.
- Current action: reconcile after parent chain, verify safe-area/touch behavior and CI.

### DQ-05 Visual Feedback v1 — PR #38

- Branch: `dagyeom/visual-feedback-v1`
- PR #38 OPEN, verified HEAD `ffbc32c0cc9465a46feb1491f0bb7d5e0d1cd57a`
- Dependency: stacked on PR #36.
- Current action: verify panel fade / selection flash / underline / strip highlight after parent chain reconciliation.

---

## BLOCKED-BY-JJUN

**현재 기존 Observer read 관련 BLOCKED-BY-JJUN 항목은 0개다.**

2026-09-14에 아래 6개는 PR #37/#39/#40으로 해제되었다:
1. Relationship 13D + target resident ID/name
2. Emotion detailed axes + summaries
3. SocialIntent + target
4. Family summary + marriage/cohabitation/pregnancy
5. World family/lifecycle aggregates
6. Blueprint/USTRUCT read-only Core Bridge

A new blocker is valid only if Dagyeom identifies a concrete missing data/API after using latest `main`. New blocker must be recorded in `TEAM_BOARD.md` with requested field, usage, and related PR.

---

## Merge dependency order

Recommended current order:

1. PR #17 latest-main reconciliation + new Core Bridge binding + UI review fixes + verification
2. PR #26 latest-main reconciliation/verification — independent enough to proceed separately
3. PR #29 / #30 reconcile after #17
4. PR #36 after #30
5. PR #38 after #36

Do not collapse the whole chain into one giant PR merely to make merging easier.

---

## Current Dagyeom state summary

- Dagyeom is **not waiting for Jjun API implementation anymore** for the previous Observer items.
- The immediate Dagyeom task is **reconciliation + binding + verification**, not Core modification.
- Jjun should not edit Dagyeom UI/Character presentation branches unless a new explicit coordination request is created.
- Every checkpoint must synchronize `WORK_STATE.md` / this queue / `TEAM_BOARD.md` and append a HANDOFF when meaningful work completes, fails, or changes dependency state.
