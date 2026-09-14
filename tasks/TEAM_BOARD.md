# LifeLens Team Board

이 파일은 쭌(sjLim91), 다겸(STILLofficial), 양쪽 AI의 작업 잠금/분배 보드다.

> **LIVE LOCK STATUS (main): `ASSIST_LOCK-29-R1 = RELEASED`. Current active assist locks = 0.**
> PR #63 merged as `c9164ebf3cc70a194d3f8f6e50dcfb3c9df2a986`. Older branch copies that still show ACTIVE are stale and must not be treated as authoritative.

## 최우선 규칙

- 실제 `main` / branch / PR / Actions가 문서보다 우선한다.
- 기능 작업 전에 `WORK_STATE.md` → 역할별 READY queue → 이 보드 → `HANDOFF_LOG.md`를 맞춘다.
- ACTIVE_LOCK 파일은 lock owner 외 수정 금지.
- `NEXT`/`AFTER`/`HIGH PRIORITY`는 작업 허가가 아니다. 실제 착수 가능 상태는 `READY_NOW` 또는 `PARALLEL_SAFE_NOW`다.
- 쭌이 다겸 작업을 도울 때는 `docs/INTEGRATION_SPRINT.md`를 따른다.
- 기본 지원은 REVIEW_ONLY이며 `dagyeom/*` direct push 금지.

## Active Work

| 담당 | 브랜치 / PR | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 다겸 + 다겸 AI | NEW milestone | Character Appearance v1 | Character appearance/presentation + `Content/Characters/**` | READY_NOW |
| 쭌 + 쭌 AI | Integration support | Character Appearance Core/Bridge support if requested | Core/Bridge/SaveLoad | REVIEW_ONLY |
| 다겸 + 쭌 Bridge support as needed | NEW milestone | Character Motion & Context v1 | Character presentation/animation | AFTER APPEARANCE |
| 다겸 + 다겸 AI | PR #30 | Observer UX Polish | `Source/LifeLens/UI/**` | AFTER HUMAN CHARACTER MINIMUM |
| 다겸 + 다겸 AI | PR #36 | Mobile Touch v1 | `Source/LifeLens/UI/**` | AFTER #30 |
| 다겸 + 다겸 AI | PR #38 | Visual Feedback v1 | `Source/LifeLens/UI/**` | AFTER #36 |
| 쭌 + 쭌 AI | PR #2 | old Android validation | Bridge/build | FROZEN |

## Current Assist Locks

**0개. `ASSIST_LOCK-29-R1`은 RELEASED.**

### Completed assist — Character Presentation v1

- Original PR #29: stale source/history only; do not revive as product integration path.
- Integration PR #63: MERGED to main as `c9164ebf3cc70a194d3f8f6e50dcfb3c9df2a986`.
- Final integration cleanup head: `6d257d5f69e63eb721d2cbd36322765e0473fabf`.
- Preflight `34843425475`: PASS.
- Unreal Linux Compile `34843425495`: PASS including actual UE 5.6 UHT/UBT/link.
- Temporary `Source/LifeLens/Characters/**` workflow trigger removed before merge.
- Final product diff: exactly four Character Presentation C++ files.
- `ASSIST_LOCK-29-R1`: DONE / RELEASED.

### Completed assist — UI Foundation

- Integration PR #61 merged to main.
- Unreal Run `34840467864` PASS including UE 5.6 UHT/UBT/link.
- `ASSIST_LOCK-26-R1`: DONE / RELEASED.

### Completed assist — Observer HUD

- PR #17 merged to main.
- Corrected Unreal Run `34833994155` PASS including UHT/UBT/link.
- Observer assist locks released.

## Character Appearance v1 — READY_NOW

Canonical acceptance criteria: `docs/CHARACTER_APPEARANCE_ROADMAP.md`.

Minimum:
- real humanoid skeletal mesh
- skin / face / eyes / hair / default clothing
- shared/common skeleton + modular appearance
- deterministic `AppearanceProfile` from WorldSeed + CharacterId
- NEW GAME residents visually distinct
- Save/Load appearance continuity
- Android LOD/mobile fallback
- asset license/provenance recorded

Authority boundary:
- Core remains simulation authority.
- Character/UI presentation reads authoritative state; no competing action chooser or authoritative cache.
- If Appearance/Genetics/SaveLoad needs a new authoritative field, create an Integration Request below before implementing a substitute in presentation code.

## Character visual direction — canonical

Current Cylinder/Sphere Character Presentation is a development placeholder.
Final direction:
- common humanoid skeleton
- real human body / skin / face / eyes / hair / clothing
- deterministic appearance continuity
- modular parts + LOD + Android fallback
- paid runtime/API dependency 없음
- asset license/provenance 기록

After Appearance, Character Motion & Context v1 minimum adds locomotion, turn, sit/stand/lie, gaze, context hooks, basic IK and transition smoothing driven by Core directives.

## Dagyeom API / design handoff

Current blockers from Jjun: **0**.

Main exposes:
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
- civilization detail stays out of Level 0

## Shared File Ownership

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

## Integration Requests

Current open requests: **none**.

## Merge / reconciliation queue

1. Character Presentation v1 — DONE via PR #63.
2. Character Appearance v1 — READY_NOW.
3. Character Motion & Context v1 minimum.
4. PR #30 Observer UX Polish.
5. PR #36 Mobile Touch.
6. PR #38 Visual Feedback.
7. Core + Observer + Human Character integrated runtime verification.
8. Android smoke APK.
9. Appearance Genetics & Lifecycle.
10. Clothing/Equipment civilization linkage.
11. Resume deeper civilization production chains.

## Completion rule

검증 + merge + 상태 동기화까지 완료되어야 DONE이다.
