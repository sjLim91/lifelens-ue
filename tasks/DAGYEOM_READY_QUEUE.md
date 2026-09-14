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

## Dispatch rule — 가장 먼저 확인

- `TEAM_BOARD.md`의 `Current Assist Locks`에 ACTIVE lock이 있으면 해당 파일은 **절대 수정하지 않는다**.
- `NEXT`, `AFTER`, `HIGH PRIORITY`는 바로 실행 가능하다는 뜻이 아니다.
- 실제 즉시 착수 가능한 것은 이 문서의 `READY_NOW` 또는 `PARALLEL_SAFE_NOW`뿐이다.
- CI 대기 중이라고 canonical 순서를 임의로 한 단계 넘기지 않는다.
- 잠긴 작업과 겹치지 않는 준비 작업만 병렬 진행한다.

## Current product checkpoints

- #17 Observer HUD v2 — DONE / MERGED `aa194db7c5b500cdf5041fd6d43b25f97b9dd0b6`
- #26 UI Foundation — DONE via Integration PR #61
  - merge `7826aaa917b4877bdd3b5ebbd6d5bfd139309b9b`
  - Preflight `34840301429` PASS
  - verify-only #62 CLOSED / NOT MERGED
  - Unreal Run #21 `34840467864` PASS including UHT/UBT/link
  - `ASSIST_LOCK-26-R1` released
- #29 Character Presentation Foundation — ACTIVE via Integration PR #63
  - `ASSIST_LOCK-29-R1` ACTIVE
  - Preflight `34843425475` PASS
  - Unreal Run `34843425495` last known IN_PROGRESS; actual status must be re-fetched

## ACTIVE LOCK — DO NOT TOUCH

### ASSIST_LOCK-29-R1

Until `WORK_STATE.md` and `TEAM_BOARD.md` both say RELEASED/DONE, 다겸/다겸 AI는 아래 파일을 수정하지 않는다.

- `Source/LifeLens/Characters/LLResidentCharacter.h`
- `Source/LifeLens/Characters/LLResidentCharacter.cpp`
- `Source/LifeLens/Characters/LLResidentPresentationComponent.h`
- `Source/LifeLens/Characters/LLResidentPresentationComponent.cpp`

Reason: current-main reconcile/UE compile validation is already active in PR #63. 같은 파일을 병렬 수정하면 source PR #29 / assist PR #63 / 다음 Appearance 작업의 기준점이 갈라진다.

## PARALLEL_SAFE_NOW

### Character Appearance v1 — PREP_ONLY

#29가 CI/merge 대기 중인 동안 다겸 측은 아래를 진행할 수 있다.

- humanoid skeletal mesh / skin / face / eyes / hair / default clothing 후보 조사 및 선택 기준 정리
- 무료 사용 가능 asset license/provenance 정리
- 신규 `Content/Characters/**` 자산 준비
- deterministic `AppearanceProfile` 요구 필드/변형 축 설계 초안
- Android LOD/mobile fallback 기준 정리
- Save/Load에 새 authoritative data가 필요하면 `TEAM_BOARD.md`에 Integration Request 작성

아직 하면 안 되는 것:
- 위 locked C++ 4개 파일 수정
- PresentationComponent 대체 구현
- #29 source branch 대량 rebase/force-push
- #30/#36/#38을 main 통합 순서보다 앞당기기

## READY_NOW after lock release

### Character Appearance v1 — CODE INTEGRATION

`ASSIST_LOCK-29-R1`이 RELEASED되고 PR #63이 main에 병합된 뒤에만 코드 통합 단계로 승격한다.

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

### Character Motion & Context v1 — AFTER APPEARANCE

Minimum:
- idle / walk / run
- turn-in-place
- sit / stand / lie / wake
- gaze/head tracking
- context interaction hook
- basic IK / transition smoothing
- Core action and visual action remain consistent

### PR #30 Observer UX Polish — AFTER HUMAN CHARACTER MINIMUM

- Preserve Level 0 cleanliness and current Core-backed detail tabs.

### PR #36 Mobile Touch
- after #30

### PR #38 Visual Feedback
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

1. #29 Character Presentation Foundation — ACTIVE / locked
2. **Character Appearance v1** — PREP_ONLY may run in parallel; code after #29 lock release
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
