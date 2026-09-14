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
- `NEXT`/`AFTER`/`READY_AFTER_*`는 바로 실행 가능하다는 뜻이 아니다.
- current-main 기준으로 새 작업을 시작하고 stale stacked branch를 제품 기준으로 사용하지 않는다.

## Current product checkpoints

- Observer HUD v2 PR #17 — DONE / MERGED.
- UI Foundation — DONE via PR #61; actual UE 5.6 compile PASS.
- Character Presentation v1 — DONE via integration PR #63.
  - main merge `c9164ebf3cc70a194d3f8f6e50dcfb3c9df2a986`
  - Preflight `34843425475` PASS
  - Unreal Linux Compile `34843425495` PASS
  - `ASSIST_LOCK-29-R1` RELEASED
- Character Appearance projection support — DONE via PR #65.
  - main merge `86663cb10f245bf185984a3622e4a86596e14923`
  - Preflight `34845789763` PASS
  - Unreal Linux Compile `34845789757` PASS including actual UE 5.6 UHT/UBT/link
  - use `FLLAppearanceProfile` / `ULLAppearanceProfileLibrary::MakeDeterministicAppearanceProfile(...)`
- Character Appearance v1 — ACTIVE / PR #67 CLOSEOUT.
  - branch `dagyeom/character-appearance-v1`
  - reported PIE: four humanoid residents visible; deterministic appearance mapping active
  - UAL animations imported; locomotion not wired yet
  - remaining gates: required CI/compile record, Save/Load appearance continuity verification, minimum default clothing, merge + live-doc sync

## ACTIVE LOCK

**없음. `ASSIST_LOCK-29-R1`은 RELEASED.**

## ACTIVE / CLOSEOUT

### Character Appearance v1 — PR #67

Detailed acceptance criteria: `docs/CHARACTER_APPEARANCE_ROADMAP.md`.
Canonical asset decision: `docs/CHARACTER_ASSET_TRACK.md`.
Appearance projection contract: `docs/CHARACTER_APPEARANCE_DATA_CONTRACT.md`.

**Default asset track: Track B — Quaternius CC0.**

Already established:
- real humanoid skeletal mesh path
- shared/common skeleton
- deterministic #65 AppearanceProfile projection
- resident visual differentiation
- selection ring / label presentation preserved
- asset provenance recorded for imported baseline packs

Must be closed before DONE:
- required GitHub/UE validation recorded
- Save/Load produces the same appearance for the same resident/world
- minimum default clothing set present; underwear-only presentation does not satisfy the current Appearance v1 minimum
- final PR review/merge
- live docs updated with merge SHA

Additional skin-tone/detail variety beyond the minimum distinctness bar may be a follow-up enhancement. Additional Quaternius packs must have their exact pack/version/license verified independently.

## READY_AFTER_#67

### Character Motion Bootstrap — first slice of Motion & Context v1

Dagyeom's Motion promotion request is accepted in principle. After PR #67 is validated, merged, and live docs are synchronized, start a **small locomotion slice first** instead of the full Motion milestone.

Expected branch: `dagyeom/character-motion-v1` from latest `main`.

Minimum bootstrap:
- Core-directive-driven Idle / Walk / Jog or Run transition
- basic orientation smoothing
- remove the current "Idle while sliding" presentation
- no competing Character-side action chooser or simulation authority

The bootstrap should stay intentionally small so visual environment work can begin early.

## READY_AFTER_MOTION_BOOTSTRAP — HIGH PRIORITY

### World Visual Environment v1

Canonical: `docs/WORLD_VISUAL_ENVIRONMENT_v1.md`.

This milestone is intentionally promoted ahead of the remaining deep Motion/Context work.

Expected branch: `dagyeom/world-visual-environment-v1` from the latest merged `main` after the locomotion bootstrap checkpoint.

Dagyeom-owned presentation paths:
- `Content/Environment/**`
- `Content/Maps/**`
- `Content/WorldPresentation/**`

Minimum:
- non-placeholder natural terrain / ground
- sky / lighting / atmosphere baseline
- trees / grass / rocks / natural dressing
- observer readability for resident labels and selection rings
- Android-friendly LOD / instancing / material budget
- no automatic modern infrastructure
- visual-only decor must not become a second world authority
- external asset source/version/license provenance recorded

Do not edit `Source/LifeLensCore/**` or authoritative `Source/LifeLens/World/**` for visual convenience. If the environment presentation needs a new Core/World read API, add an Integration Request to `TEAM_BOARD.md`.

## AFTER WORLD VISUAL ENVIRONMENT v1

### Character Motion & Context v1 — remaining scope

Continue the rest of the Motion milestone after the visual environment baseline:
- turn-in-place refinement
- sit / stand / lie / wake
- gaze/head tracking
- context interaction hook
- basic IK / transition smoothing
- visual action stays consistent with Core action directive

Imported UAL animations may be reused if their exact provenance/license remains recorded.

## AFTER HUMAN CHARACTER + WORLD VISUAL MINIMUM

### PR #30 Observer UX Polish

Proceed after the Human Character minimum and World Visual Environment v1 baseline.

### PR #36 Mobile Touch

Proceed after #30.

### PR #38 Visual Feedback

Proceed after #36.

## BLOCKED-BY-JJUN

**0개.**

PR #65 support is already merged. 새 API/SaveLoad gap이 실제 발견될 때만 `TEAM_BOARD.md`에 Integration Request를 만든다.

## Integration Sprint rule

Jjun default support = `REVIEW_ONLY`.
실제 Dagyeom-owned 수정 지원은 exact HEAD 확인 → ASSIST_LOCK → `integration/dagyeom-<scope>-assist` → 검증/handoff → lock 해제 순서다.
`dagyeom/*` branch에 Jjun AI direct push 금지.

## Canonical order

1. Character Presentation v1 — DONE.
2. Appearance data/projection support — DONE via PR #65.
3. Character Appearance v1 — ACTIVE / PR #67 CLOSEOUT.
4. Character Motion Bootstrap — READY_AFTER_#67.
5. **World Visual Environment v1 — HIGH PRIORITY / READY_AFTER_MOTION_BOOTSTRAP.**
6. Character Motion & Context v1 remaining scope.
7. PR #30 Observer UX Polish.
8. PR #36 Mobile Touch.
9. PR #38 Visual Feedback.
10. Core + Observer + Human Character + World Visual integrated runtime verification.
11. Android smoke APK + profiling.
12. MetaHuman comparison / upgrade decision.
13. Appearance Genetics & Lifecycle.
14. Clothing/Equipment civilization linkage.
15. deeper civilization production chains.

## Canonical product direction

`Need / Curiosity → Gather → Store → Experiment → Discovery → Personal Knowledge → Craft/Build → Teach/Imitate → Culture → Specialization → Generational Civilization`

Character/UI/environment presentation은 원시 자원·도구부터 이후 기술까지 수용 가능하게 유지하며, 실제 세계 상태의 authority는 Core/World에 둔다.
