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
- #17 Observer HUD v2 — **DONE / MERGED `aa194db7c5b500cdf5041fd6d43b25f97b9dd0b6`**
  - R1 helper #56 merged, verify-only #57 closed
  - R1 Preflight `34824371968` PASS
  - R1 Unreal Run #17 `34824371965` PASS including UHT/UBT/link
  - R2 helper #58 merged, verify-only #59 closed
  - corrected R2 Preflight `34833994138` PASS
  - corrected R2 Unreal Run #20 `34833994155` PASS including UHT/UBT/link
  - unresolved review threads: 0
  - all Observer assist locks released

## HIGHEST PRIORITY

### DQ-01 — PR #26 UI Foundation — NEXT

- Re-fetch exact PR #26 / branch HEAD before work.
- Reconcile against actual latest main, which now includes PR #17.
- Preserve PR #17 Observer hierarchy and Core authority.
- Validate Android landscape/safe-area/typography behavior.
- If Jjun assistance edits Dagyeom-owned files: create a new ASSIST_LOCK and a fresh `integration/dagyeom-...-assist` branch first.

### DQ-02 — PR #29 Character Presentation — READY AFTER #17

- PR #17 dependency is now cleared.
- Reconcile parent/base before code changes.
- Consume Core action intent for visuals only.
- Keep appearance/equipment hooks data-driven from primitive resources/tools through later technology.

### DQ-03 — PR #30 Observer UX Polish — READY AFTER #17

- Reconcile to merged #17 baseline.
- Preserve Level 0 cleanliness and current Core-backed detail tabs.

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
`dagyeom/*` branch에 Jjun AI direct push 금지.

Parent-first order:
1. #26 next
2. #29/#30 from merged #17 baseline
3. #36 after #30
4. #38 after #36
5. integrated runtime verification
6. Android smoke APK

## Canonical product direction

`Need / Curiosity → Gather → Store → Experiment → Discovery → Personal Knowledge → Craft/Build → Teach/Imitate → Culture → Specialization → Generational Civilization`

Character/UI presentation은 원시 자원·도구부터 이후 기술까지 수용 가능하게 유지한다.
