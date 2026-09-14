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

## Current product checkpoints

- #53 Civilization Observer Read DTOs — merged `ec30d80b2986247f0f16572efb2c082a933d796d`, Unreal Run #16 PASS
- #54 macOS clang shadow hotfix — merged `3b649b900c44a4e48bb89171b38f5e685e757b14`
- PR #17 R1 latest-main reconciliation — **DONE**
  - current branch HEAD `0716a95eb4b6000c7098f8c0bb812bd622afa6a3`
  - helper PR #56 merged into Dagyeom branch
  - verify-only PR #57 closed without merge
  - Preflight `34824371968` PASS
  - Unreal Run #17 `34824371965` PASS including UHT/UBT/link
  - former Codex review threads resolved

## HIGHEST PRIORITY

### DQ-R2 — Bind current Observer Bridge — DOING

- Target: PR #17 `dagyeom/observer-ui-v2`
- Assist helper: `integration/dagyeom-observer-r2-assist`
- Active lock: `ASSIST_LOCK-17-R2`
- Locked files:
  - `Source/LifeLens/UI/LLObserverHUD.cpp`
  - `Source/LifeLens/UI/LLObserverHUD.h`
  - `Source/LifeLens/UI/LLObserverLabels.h`

Existing getters:
- `GetWorldObservation()`
- `GetResidentObservations()`
- `GetResidentObservation(...)`
- `GetFamilyObservation(...)`
- `GetResidentActionDirective(...)`
- `GetRecentCoreEvents()`
- `OnCoreRuntimeStateChanged`

Civilization getters:
- `GetResidentCivilizationObservation(...)`
- `GetCivilizationWorldObservation(...)`

Binding rules:
- Level 0 stays clean
- selected resident/detail may show authoritative emotion/relationship/family/civilization data
- current action presentation reads Core directive/read model; UI never chooses actions
- civilization detail belongs in detail views and major-discovery feedback
- no competing UI simulation/cache
- no hard-coded population 4 after runtime begins
- after Load, rebuild from Bridge

### DQ-R3 — after R2

- PR #17 description/state docs reconcile
- review state already functionally fixed/resolved
- final Preflight + actual UE UHT/UBT validation
- then #17 merge readiness

## READY NOW — other Dagyeom work

### DQ-01 — PR #26 UI Foundation
- reconcile latest main where it does not overlap active ASSIST_LOCK

### DQ-02 — PR #29 Character Presentation
- after #17
- visual-only consumer of Core intent

### DQ-03 — PR #30 Observer UX Polish
- after #17

### DQ-04 — PR #36 Mobile Touch
- after #30

### DQ-05 — PR #38 Visual Feedback
- after #36

## BLOCKED-BY-JJUN

**0개.**

새 API gap이 실제 발견될 때만 `TEAM_BOARD.md`에 Integration Request를 만든다.

## Integration Sprint rule

Jjun default support = `REVIEW_ONLY`.
실제 Dagyeom-owned 수정은 exact HEAD 확인 → `ASSIST_LOCK` → `integration/dagyeom-<scope>-assist` → 검증/handoff → lock 해제 순서다.
`dagyeom/*` direct push 금지.

Parent-first order:
1. #17
2. #26 where independent
3. #29/#30
4. #36
5. #38

## Canonical product direction

`Need / Curiosity → Gather → Store → Experiment → Discovery → Personal Knowledge → Craft/Build → Teach/Imitate → Culture → Specialization → Generational Civilization`

Character/UI presentation은 원시 자원·도구부터 이후 기술까지 수용 가능하게 유지한다.
