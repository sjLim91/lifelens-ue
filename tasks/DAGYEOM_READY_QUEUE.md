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

- #53 Civilization Observer Read DTOs — merged `ec30d80b2986247f0f16572efb2c082a933d796d`, Unreal Run #16 PASS
- #54 macOS clang shadow hotfix — merged `3b649b900c44a4e48bb89171b38f5e685e757b14`
- #17 Observer HUD v2 — **DONE / MERGED `aa194db7c5b500cdf5041fd6d43b25f97b9dd0b6`**
  - corrected R2 Unreal Run #20 `34833994155` PASS including UHT/UBT/link
  - unresolved review threads: 0
  - Observer assist locks released

## HIGHEST PRIORITY

### DQ-01 — PR #26 UI Foundation — DOING

- Original branch/head: `dagyeom/ui-foundation-v1` / `70dfa5ebeabf24b661c9f9fd0bc63e3ad01ac180`
- Active lock: `ASSIST_LOCK-26-R1`
- Helper: `integration/dagyeom-ui-foundation-r1-assist`
- Locked files:
  - `Source/LifeLens/UI/LLObserverUIFoundation.cpp`
  - `Source/LifeLens/UI/LLObserverUIFoundation.h`
- Reconcile strategy: latest main + the two Foundation files only; stale old shared-state docs are not replayed.
- Helper PR #61 Preflight `34840301429` PASS.
- Verify-only PR #62 Preflight `34840467738` PASS.
- Unreal Run #21 `34840467864` 진행 중. 마지막 확인은 UE 5.6 image pull 단계.
- While lock is active, Dagyeom side should not edit the two Foundation files.

### DQ-02 — PR #29 Character Presentation Foundation — NEXT AFTER #26

현재 PR #29의 Cylinder 몸통 + Sphere 머리 + 단색 material은 **개발용 placeholder**다. 최종 사람 스킨이 아니다.

#29에서는 먼저 다음 기반만 current main에 안정화한다:
- PresentationComponent
- selection ring
- label LOD
- life-stage scale hook
- real human mesh로 교체 가능한 구조
- Core action/identity를 표현만 하는 presentation contract

### DQ-03 — Character Appearance v1 — NEW HIGH PRIORITY

**#29 직후 시작하며 #30/#36/#38 UI polish보다 우선한다.**

상세 기준: `docs/CHARACTER_APPEARANCE_ROADMAP.md`

최소 목표:
- real humanoid skeletal mesh
- skin / face / eyes / hair / default clothing
- shared skeleton + modular appearance
- deterministic `AppearanceProfile` from WorldSeed + CharacterId
- NEW GAME마다 서로 다른 초기 4명 외형
- Save/Load 후 동일 외형 유지
- Android LOD/mobile fallback
- asset license/provenance 기록

### DQ-04 — Character Motion & Context v1 — AFTER APPEARANCE

최소 세트:
- idle / walk / run
- turn-in-place
- sit / stand / lie / wake
- gaze/head tracking
- context interaction hook
- basic IK / transition smoothing
- Core action과 화면 행동이 모순되지 않게 유지

### DQ-05 — PR #30 Observer UX Polish

- Human Character Appearance/Motion 최소 checkpoint 이후 진행.
- merged #17의 Level 0 cleanliness와 Core-backed detail tabs 유지.

### DQ-06 — PR #36 Mobile Touch
- after #30

### DQ-07 — PR #38 Visual Feedback
- after #36

## LATER CHARACTER MILESTONES

- Appearance Genetics & Lifecycle v1
  - 부모 외형 parameter 조합
  - child → teen → adult → elder 성장 표현
  - family resemblance / aging
- Clothing/Equipment civilization linkage
  - 개인 소유/제작/지식 상태를 기반으로 의상/도구 표현
  - 전역 시대 unlock 금지

## BLOCKED-BY-JJUN

**0개.**

새 API/SaveLoad gap이 실제 발견될 때만 `TEAM_BOARD.md`에 Integration Request를 만든다.

## Integration Sprint rule

Jjun default support = `REVIEW_ONLY`.
실제 Dagyeom-owned 수정은 exact HEAD 확인 → `ASSIST_LOCK` → `integration/dagyeom-<scope>-assist` → 검증/handoff → lock 해제 순서다.
`dagyeom/*` branch에 Jjun AI direct push 금지.

## Canonical order

1. #26 UI Foundation
2. #29 Character Presentation Foundation
3. **Character Appearance v1**
4. **Character Motion & Context v1 minimum**
5. #30 Observer UX Polish
6. #36 Mobile Touch
7. #38 Visual Feedback
8. Core + Observer + Human Character integrated runtime verification
9. Android smoke APK
10. Appearance Genetics & Lifecycle
11. Clothing/Equipment civilization linkage
12. deeper civilization production chains

## Canonical product direction

`Need / Curiosity → Gather → Store → Experiment → Discovery → Personal Knowledge → Craft/Build → Teach/Imitate → Culture → Specialization → Generational Civilization`

Character/UI presentation은 원시 자원·도구부터 이후 기술까지 수용 가능하게 유지한다.
