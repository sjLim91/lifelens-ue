# LifeLens Team Board

이 파일은 쭌(sjLim91), 다겸(STILLofficial), 양쪽 AI의 작업 잠금/분배 보드다.

## 최우선 규칙

- 실제 `main` / branch / PR / Actions가 문서보다 우선한다.
- 기능 작업 전에 `WORK_STATE.md` → 역할별 READY queue → 이 보드 → `HANDOFF_LOG.md`를 맞춘다.
- 제품 방향은 `docs/LIFELENS_SPEC_v1.1.md` + `docs/CIVILIZATION_PROGRESSION_v1.md`.
- 캐릭터 외형/표현 실행 순서는 `docs/CHARACTER_APPEARANCE_ROADMAP.md`를 따른다.
- 쭌이 다겸 작업을 도울 때는 `docs/INTEGRATION_SPRINT.md` 필수.
- 기본 지원은 `REVIEW_ONLY`.
- 실제 다겸 소유 코드 수정은 `ASSIST_LOCK` + `integration/dagyeom-<scope>-assist`; `dagyeom/*` direct push 금지.
- **ACTIVE_LOCK이 있는 파일은 lock owner 외 수정 금지.** 다른 작업자는 `PARALLEL_SAFE_NOW`로 명시된 범위만 진행한다.
- `NEXT`, `AFTER`, `HIGH PRIORITY`는 작업 허가가 아니다. 선행 checkpoint가 DONE이거나 명시적으로 `PARALLEL_SAFE_NOW`여야 착수 가능하다.

상태: `TODO` / `DOING` / `REVIEW` / `DONE` / `BLOCKED` / `FROZEN` / `WAITING_CI` / `PREP_ONLY`

## Active Work

| 담당 | 브랜치 / PR | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 다겸 | Integration Sprint | Core/Bridge ↔ UI/Presentation 합류 | REVIEW/ASSIST only | DOING |
| 다겸 + 다겸 AI | PR #17 | Observer HUD v2 + Core/civilization binding | `Source/LifeLens/UI/**` | DONE / MERGED `aa194db7...` |
| 다겸 + 쭌 assist | PR #61 | UI Foundation current-main reconcile | `LLObserverUIFoundation.*` | DONE / MERGED `7826aaa...` |
| 다겸 + 쭌 assist | PR #63 (source PR #29) | Character Presentation Foundation current-main reconcile | locked Character presentation files | WAITING_CI |
| 다겸 + 다겸 AI | NEW milestone | Character Appearance v1 preparation | `Content/Characters/**`, asset/provenance prep only | PREP_ONLY / PARALLEL_SAFE_NOW |
| 다겸 + 쭌 Bridge support as needed | NEW milestone | Character Appearance v1 code integration | Appearance + Character presentation | AFTER #29 LOCK RELEASE |
| 다겸 + 쭌 Bridge support as needed | NEW milestone | Character Motion & Context v1 | Character presentation/animation | AFTER APPEARANCE |
| 다겸 + 다겸 AI | `dagyeom/observer-ux-polish-v1`, PR #30 | Observer UX Polish | `Source/LifeLens/UI/**` | AFTER HUMAN CHARACTER MINIMUM |
| 다겸 + 다겸 AI | `dagyeom/mobile-touch-v1`, PR #36 | Mobile Touch v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #30 |
| 다겸 + 다겸 AI | `dagyeom/visual-feedback-v1`, PR #38 | Visual Feedback v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #36 |
| 쭌 + 쭌 AI | `task/03-fast-test`, PR #2 | old Android validation | Bridge/build | FROZEN |

## Current Assist Locks

### ASSIST_LOCK-29-R1 — ACTIVE

- Owner while active: 쭌 측 integration assist
- Original owner: 다겸
- Integration branch: `integration/dagyeom-character-presentation-r1-assist`
- Integration PR: #63
- Current integration HEAD: `b9d22d47745227f6cc5d36316a9a7e6da706fd64`
- Source/history: `dagyeom/character-presentation-v1`, PR #29
- Locked files:
  - `Source/LifeLens/Characters/LLResidentCharacter.h`
  - `Source/LifeLens/Characters/LLResidentCharacter.cpp`
  - `Source/LifeLens/Characters/LLResidentPresentationComponent.h`
  - `Source/LifeLens/Characters/LLResidentPresentationComponent.cpp`
- Temporary validation-only change on assist branch:
  - `.github/workflows/unreal-linux-compile.yml` includes `Source/LifeLens/Characters/**` trigger so actual UE compile runs.
  - This workflow change is **not product scope** and must be removed before merge.
- Validation:
  - Preflight `34843425475` PASS.
  - Unreal Linux Compile `34843425495` was IN_PROGRESS at last reconciliation; re-fetch actual status before changing code or merge state.
- Release condition:
  1. actual UE 5.6 UHT/UBT/link result reconciled;
  2. temporary workflow trigger removed;
  3. final PR #63 product diff = the four Character files only;
  4. PR #63 merged;
  5. live docs synchronized to DONE/UNLOCKED.

**While ACTIVE:** 다겸/다겸 AI/다른 AI는 위 4개 locked C++ 파일을 수정하지 않는다. 동일 기능의 별도 helper/rebase/cherry-pick도 만들지 않는다.

## Parallel-safe lane

### Character Appearance v1 — PREP_ONLY / PARALLEL_SAFE_NOW

다겸 측은 #29 CI/merge 대기 중 아래만 병렬 진행 가능하다.

- 실제 humanoid character 후보/구조 조사 및 선택 기준 정리.
- 무료 사용 가능 asset license/provenance 정리.
- 신규 `Content/Characters/**` 자산 준비.
- AppearanceProfile 요구 필드 초안/설계 정리.
- Core/SaveLoad authoritative data가 새로 필요하면 코드로 임의 생성하지 말고 Integration Request 등록.

금지:
- ASSIST_LOCK-29-R1 locked C++ 4개 파일 수정.
- #29 대체 구현 생성.
- #30/#36/#38을 canonical 순서보다 먼저 main 통합.
- old stacked branches mass force-rebase.

### Completed assist — PR #26 UI Foundation

- Original PR #26: CLOSED / NOT MERGED / stale-base superseded.
- Helper PR #61: MERGED to main as `7826aaa917b4877bdd3b5ebbd6d5bfd139309b9b`.
- Helper Preflight `34840301429` PASS.
- Verify-only PR #62: CLOSED / NOT MERGED.
- Verify Preflight `34840467738` PASS.
- Unreal Run #21 `34840467864` PASS including UE 5.6 UHT/UBT/link.
- `ASSIST_LOCK-26-R1`: DONE / UNLOCKED.

### Completed assist — PR #17 R1/R2

- R1 helper PR #56 merged into `dagyeom/observer-ui-v2`.
- R1 verify-only PR #57 closed without merge.
- R1 Unreal Run #17 `34824371965` PASS including UHT + UBT + link.
- R2 helper PR #58 merged into `dagyeom/observer-ui-v2`.
- R2 verify-only PR #59 closed without merge.
- Corrected R2 Unreal Run #20 `34833994155` PASS including UHT + UBT + final link.
- PR #17 merged to main as `aa194db7c5b500cdf5041fd6d43b25f97b9dd0b6`.
- Former review threads resolved; unresolved = 0.

## Character visual direction — canonical

현재 PR #29의 Cylinder body + Sphere head + 단색 material은 **development placeholder**다.

최종 방향은 `docs/CHARACTER_APPEARANCE_ROADMAP.md` 기준:
- common humanoid skeleton
- real human body / skin / face / eyes / hair / clothing
- deterministic `AppearanceProfile` (`WorldSeed + CharacterId`)
- Save/Load appearance continuity
- modular appearance + LOD + Android fallback
- Core action을 presentation이 표현만 함
- paid runtime/API dependency 없음
- asset license/provenance 기록

Character Appearance v1은 #29 직후 code integration으로 승격하며 #30/#36/#38 UI polish보다 우선한다. 단, #29 lock 동안 asset/provenance 준비는 `PARALLEL_SAFE_NOW`로 허용한다.

그 다음 Character Motion & Context v1 최소 세트에서 locomotion, turn, sit/stand/lie, gaze, context hook, basic IK/transition을 Core directive에 맞춰 표현한다.

후속:
- Appearance Genetics & Lifecycle
- civilization clothing/equipment linkage

## Canonical direction — autonomous civilization

`Need / Curiosity → Observe → Gather → Store → Experiment → Discovery → Personal Knowledge → Craft/Build → Teach/Imitate → Shared Culture → Specialization/Exchange → Generational Civilization`

- 전역 tech unlock 금지
- 시대 gate 강제 금지
- 개인 지식/출처/전파를 Core가 권위 상태로 소유
- Observer HUD Level 0은 계속 단순하게 유지
- 원시 자원/도구부터 현대 기술까지 수용 가능한 presentation 유지

## Dagyeom API / design handoff

Current blockers from Jjun: **0**.

Main now exposes and Observer HUD can consume:
- `GetWorldObservation()`
- `GetResidentObservations()`
- `GetResidentObservation(...)`
- `GetFamilyObservation(...)`
- `GetResidentActionDirective(...)`
- `GetRecentCoreEvents()`
- `OnCoreRuntimeStateChanged`
- `GetResidentCivilizationObservation(...)`
- `GetCivilizationWorldObservation(...)`

Rules:
- never hard-code population to 4 after runtime starts
- UI rebuilds from Bridge after load
- presentation does not choose competing actions
- civilization detail belongs in selected resident/detail/major-discovery layers, not Level 0
- appearance/genetics에 새 authoritative data가 필요하면 TEAM_BOARD Integration Request → Jjun Core/Bridge/SaveLoad flow로 추가

## Shared File Lock

Jjun default:
- `Source/LifeLensCore/**`
- `Source/LifeLens/AI/**`
- `Source/LifeLens/Simulation/**`
- `Source/LifeLens/World/**`
- Save/Load, Bridge, CI/build

Dagyeom default:
- `Source/LifeLens/UI/**`
- Character appearance/presentation
- `Content/UI/**`
- `Content/Characters/**`

ASSIST_LOCK은 임시이며 해제 후 원래 소유권으로 복귀한다. ACTIVE인 동안에는 기본 소유권보다 lock이 우선한다.

## Integration Requests

Current open requests: **none**.

## Merge / reconciliation queue

1. PR #29 Character Presentation Foundation — **ACTIVE via PR #63 / ASSIST_LOCK-29-R1**.
2. Character Appearance v1 — **PREP_ONLY now; code after #29 lock release**.
3. Character Motion & Context v1 minimum.
4. PR #30 Observer UX Polish.
5. #36 after #30.
6. #38 after #36.
7. Core + Observer + Human Character integrated runtime verification.
8. Android smoke APK.
9. Appearance Genetics & Lifecycle.
10. Clothing/Equipment civilization linkage.
11. Resume deeper civilization production chains.

## Completion rule

검증 + merge + 상태 동기화까지 완료되어야 DONE이다.
