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
캐릭터 외형/표현 구현은 **`docs/CHARACTER_APPEARANCE_ROADMAP.md`가 canonical 실행 기준**이다.
쭌이 다겸 작업을 도울 때는 `docs/INTEGRATION_SPRINT.md`를 따른다.

## Current product checkpoints

- #17 Observer HUD v2 — DONE / MERGED `aa194db7c5b500cdf5041fd6d43b25f97b9dd0b6`
- #26 UI Foundation — DONE via Integration PR #61
  - merge `7826aaa917b4877bdd3b5ebbd6d5bfd139309b9b`
  - Preflight `34840301429` PASS
  - verify-only #62 CLOSED / NOT MERGED
  - Unreal Run #21 `34840467864` PASS including UHT/UBT/link
  - `ASSIST_LOCK-26-R1` released

## HIGHEST PRIORITY

### DQ-02 — PR #29 Character Presentation Foundation — NEXT

- Re-fetch exact PR #29 / branch HEAD before work.
- Reconcile onto actual latest main, which now includes #17 + UI Foundation #61.
- Current Cylinder body + Sphere head + flat material are **development placeholders**, not final graphics.
- Preserve PresentationComponent / selection ring / label LOD / life-stage hooks.
- Keep presentation read-only with respect to Core action/identity.
- Goal is a clean foundation that can immediately receive actual human character assets.

### DQ-03 — Character Appearance v1 — HIGH PRIORITY AFTER #29

Detailed acceptance criteria: `docs/CHARACTER_APPEARANCE_ROADMAP.md`

Minimum:
- real humanoid skeletal mesh
- skin / face / eyes / hair / default clothing
- shared skeleton + modular appearance
- deterministic AppearanceProfile from WorldSeed + CharacterId
- NEW GAME residents visually distinct
- Save/Load appearance continuity
- Android LOD/mobile fallback
- asset license/provenance documented

### DQ-04 — Character Motion & Context v1 — AFTER APPEARANCE

Minimum:
- idle / walk / run
- turn-in-place
- sit / stand / lie / wake
- gaze/head tracking
- context interaction hook
- basic IK / transition smoothing
- Core action and visual action remain consistent

### DQ-05 — PR #30 Observer UX Polish

- After Human Character Appearance/Motion minimum checkpoint.
- Preserve Level 0 cleanliness and current Core-backed detail tabs.

### DQ-06 — PR #36 Mobile Touch
- after #30

### DQ-07 — PR #38 Visual Feedback
- after #36

## LATER CHARACTER MILESTONES

- Appearance Genetics & Lifecycle v1
- Clothing/Equipment civilization linkage

## BLOCKED-BY-JJUN

**0개.**

새 API/SaveLoad gap이 실제 발견될 때만 `TEAM_BOARD.md`에 Integration Request를 만든다.

## Integration Sprint rule

Jjun default support = `REVIEW_ONLY`.
실제 Dagyeom-owned 수정은 exact HEAD 확인 → `ASSIST_LOCK` → `integration/dagyeom-<scope>-assist` → 검증/handoff → lock 해제 순서다.
`dagyeom/*` branch에 Jjun AI direct push 금지.

## Canonical order

1. #29 Character Presentation Foundation
2. **Character Appearance v1**
3. **Character Motion & Context v1 minimum**
4. #30 Observer UX Polish
5. #36 Mobile Touch
6. #38 Visual Feedback
7. Core + Observer + Human Character integrated runtime verification
8. Android smoke APK
9. Appearance Genetics & Lifecycle
10. Clothing/Equipment civilization linkage
11. deeper civilization production chains

## Canonical product direction

`Need / Curiosity → Gather → Store → Experiment → Discovery → Personal Knowledge → Craft/Build → Teach/Imitate → Culture → Specialization → Generational Civilization`

Character/UI presentation은 원시 자원·도구부터 이후 기술까지 수용 가능하게 유지한다.
