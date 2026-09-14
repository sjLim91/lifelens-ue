# Dagyeom READY NOW Queue

다겸(STILLofficial) / 다겸 측 AI가 바로 실행할 수 있는 작업 큐다.

## 시작 전 필수

1. actual latest `main`
2. target Dagyeom branch/PR HEAD + CI
3. `tasks/WORK_STATE.md`
4. 이 READY queue
5. `tasks/TEAM_BOARD.md`
6. `tasks/HANDOFF_LOG.md`

문서가 GitHub와 다르면 코드보다 문서를 먼저 고친다.

제품 방향은 `docs/LIFELENS_SPEC_v1.1.md` + `docs/CIVILIZATION_PROGRESSION_v1.md`.
쭌이 다겸 작업을 도울 때는 `docs/INTEGRATION_SPRINT.md`를 따른다.

---

## Current product checkpoints

- #48 World Affordance — merged / UE Run #14 PASS
- #49 Civilization Foundation — merged
- #50 Civilization Runtime/Persistence — merged
- #51 Autonomous Civilization Action Loop — merged
- #52 Civilization Knowledge Transmission — merged
- #53 Civilization Observer Read DTOs — **merged `ec30d80b2986247f0f16572efb2c082a933d796d`**
  - Core `34821150702` PASS
  - Preflight `34821150693` PASS
  - Unreal Run #16 `34821150704` PASS including UHT/UBT/link
- #54 macOS clang shadow hotfix — **merged `3b649b900c44a4e48bb89171b38f5e685e757b14`**
  - R1 mac compile blocker resolved

---

## RECONCILE FIRST — highest priority

### DQ-R1 — PR #17 Observer HUD v2 latest-main reconciliation

- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Status: **READY NOW**
- Re-fetch exact PR/branch HEAD before work.
- Reconcile against actual latest main.
- macOS clang blocker is resolved by #54.
- Preserve latest collaboration docs and Jjun Core/Bridge authority.

### DQ-R2 — Bind current Observer Bridge

Existing getters:
- `GetWorldObservation()`
- `GetResidentObservations()`
- `GetResidentObservation(...)`
- `GetFamilyObservation(...)`
- `GetResidentActionDirective(...)`
- `GetRecentCoreEvents()`
- `OnCoreRuntimeStateChanged`

New civilization getters now **READY FOR BINDING**:
- `GetResidentCivilizationObservation(...)`
- `GetCivilizationWorldObservation(...)`

Civilization detail available:
- inventory / carried units
- technique level / confidence / practice
- gathering / crafting / learning skills
- SelfDiscovery / DirectWitness / Teaching provenance
- world ResourceNode / StorageSite summaries
- recent discoveries / civilization aggregates

UI rules:
- Level 0 stays clean
- civilization data belongs in selected-resident/detail views and major discovery feedback
- do not create a competing simulation authority
- do not hard-code resident count to 4 after runtime begins
- after Load, rebuild from Bridge

### DQ-R3 — Resolve PR #17 review findings

Known examples:
- hide Level 0 selection hint when inspector is open
- constrain/wrap/truncate inspector on narrow canvases

Then verify compile/runtime as available.

---

## READY NOW — other Dagyeom work

### DQ-01 — PR #26 UI Foundation
- reconcile latest main
- verify Android landscape/safe layout
- may proceed independently only where files do not overlap an active ASSIST_LOCK

### DQ-02 — PR #29 Character Presentation
- after #17
- consume Core intent for visuals only
- data-driven primitive→modern tool/equipment hooks

### DQ-03 — PR #30 Observer UX Polish
- after #17

### DQ-04 — PR #36 Mobile Touch
- after #30

### DQ-05 — PR #38 Visual Feedback
- after #36

---

## BLOCKED-BY-JJUN

**0개.**

- old Observer read blockers: resolved
- macOS shadow blocker: resolved by #54
- civilization-detail read contract: resolved by #53

새 API gap이 실제로 발견될 때만 `TEAM_BOARD.md`에 Integration Request를 만든다.

---

## Integration Sprint rule

Jjun default support = `REVIEW_ONLY`.

Jjun이 다겸-owned 파일을 실제 수정해야 하면:
1. exact target HEAD 확인
2. TEAM_BOARD에 `ASSIST_LOCK`
3. `integration/dagyeom-<scope>-assist` 생성
4. locked paths만 수정
5. handoff/merge 후 lock 해제

`dagyeom/*` branch에 Jjun AI direct push 금지.

Parent-first order:
1. #17
2. #26 where independent
3. #29/#30
4. #36
5. #38

---

## Canonical product direction

LifeLens는 현대 가정 시뮬레이터에 고정되지 않는다.

`Need / Curiosity → Gather → Store → Experiment → Discovery → Personal Knowledge → Craft/Build → Teach/Imitate → Culture → Specialization → Generational Civilization`

Character/UI presentation은 원시 자원·도구부터 이후 기술까지 수용 가능하게 유지한다.

## Current summary

- 다겸은 이제 쭌 API를 기다릴 필요가 없다.
- PR #17 R1이 최우선.
- #53 civilization APIs are live on main.
- Integration Sprint 동안 Jjun은 큰 새 Core 기능보다 합류 지원을 우선한다.
