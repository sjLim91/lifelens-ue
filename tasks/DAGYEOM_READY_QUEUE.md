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

## Dispatch rule

- ACTIVE_LOCK 파일은 절대 수정하지 않는다.
- 실제 즉시 착수 가능한 것은 `READY_NOW` 또는 `PARALLEL_SAFE_NOW`뿐이다.
- `NEXT`/`AFTER`는 바로 실행 가능하다는 뜻이 아니다.
- current-main 기준으로 새 작업을 시작하고 stale stacked branch를 제품 기준으로 사용하지 않는다.

## Current product checkpoints

- Observer HUD v2 PR #17 — DONE / MERGED.
- UI Foundation — DONE via PR #61; actual UE 5.6 compile PASS.
- Character Presentation v1 — DONE via integration PR #63.
  - main merge `c9164ebf3cc70a194d3f8f6e50dcfb3c9df2a986`
  - Preflight `34843425475` PASS
  - Unreal Linux Compile `34843425495` PASS including actual UE 5.6 UHT/UBT/link
  - `ASSIST_LOCK-29-R1` RELEASED

## ACTIVE LOCK

**없음. `ASSIST_LOCK-29-R1`은 RELEASED.**

## READY_NOW

### Character Appearance v1

Start from current latest `main` at or after `c9164ebf3cc70a194d3f8f6e50dcfb3c9df2a986`.

Detailed acceptance criteria: `docs/CHARACTER_APPEARANCE_ROADMAP.md`.
Canonical asset decision: `docs/CHARACTER_ASSET_TRACK.md`.

**Default asset track: Track B — Quaternius CC0.**

- baseline body: Quaternius Universal Base Characters pack version whose pack page explicitly states CC0
- baseline animation source: Quaternius Universal Animation Library version whose pack page explicitly states CC0
- record exact source/version/license provenance at import time
- do not assume every Quaternius pack is CC0; verify any additional pack separately
- MetaHuman is an upgrade/comparison path only after the Android smoke/performance gate is green

Minimum:
- real humanoid skeletal mesh
- skin / face / eyes / hair / default clothing
- shared/common skeleton + modular appearance
- deterministic `AppearanceProfile` from stable resident identity
- NEW GAME residents visually distinct
- Save/Load appearance continuity
- Android LOD/mobile fallback
- free-use asset license/provenance documented

Implementation rules:
- Core action/simulation authority remains in Core/Bridge.
- Character presentation may read authoritative data but must not add a competing action chooser or simulation authority.
- do not revive/rebase original stale PR #29 as the product path; Character Presentation is already in current main.
- if AppearanceProfile/Genetics/SaveLoad requires new authoritative data, add an Integration Request to `tasks/TEAM_BOARD.md` for Jjun support.
- keep Level 0 observer UX uncluttered.

## AFTER APPEARANCE

### Character Motion & Context v1 minimum

- idle / walk / run
- turn-in-place
- sit / stand / lie / wake
- gaze/head tracking
- context interaction hook
- basic IK / transition smoothing
- visual action stays consistent with Core action directive

### PR #30 Observer UX Polish

Proceed only after Human Character Appearance/Motion minimum checkpoint.

### PR #36 Mobile Touch

Proceed after #30.

### PR #38 Visual Feedback

Proceed after #36.

## BLOCKED-BY-JJUN

**0개.**

새 API/SaveLoad gap이 실제 발견될 때만 `TEAM_BOARD.md`에 Integration Request를 만든다.

## Integration Sprint rule

Jjun default support = REVIEW_ONLY.
실제 Dagyeom-owned 수정 지원은 exact HEAD 확인 → ASSIST_LOCK → `integration/dagyeom-<scope>-assist` → 검증/handoff → lock 해제 순서다.
`dagyeom/*` branch에 Jjun AI direct push 금지.

## Canonical order

1. Character Presentation v1 — DONE.
2. Character Appearance v1 — READY_NOW / Track B Quaternius CC0.
3. Character Motion & Context v1 minimum.
4. PR #30 Observer UX Polish.
5. PR #36 Mobile Touch.
6. PR #38 Visual Feedback.
7. Core + Observer + Human Character integrated runtime verification.
8. Android smoke APK + profiling.
9. MetaHuman comparison / upgrade decision.
10. Appearance Genetics & Lifecycle.
11. Clothing/Equipment civilization linkage.
12. deeper civilization production chains.

## Canonical product direction

`Need / Curiosity → Gather → Store → Experiment → Discovery → Personal Knowledge → Craft/Build → Teach/Imitate → Culture → Specialization → Generational Civilization`

Character/UI presentation은 원시 자원·도구부터 이후 기술까지 수용 가능하게 유지한다.
