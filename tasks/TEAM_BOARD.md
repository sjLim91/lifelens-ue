# LifeLens Team Board

이 파일은 쭌(sjLim91)과 다겸(STILLofficial), 그리고 각자의 AI 에이전트가 동시에 작업할 때 사용하는 작업 잠금/분배 보드다.

## 최우선 동기화 규칙

**모든 기능 작업보다 상태 동기화가 먼저다.** 작업 시작/재개 시 실제 GitHub `main`/branch/PR/Actions 상태를 확인하고 `WORK_STATE.md` → 역할별 READY 큐 → 이 보드 → `HANDOFF_LOG.md`와 대조한다. 문서가 실제 상태와 다르면 코드 수정 전에 문서부터 갱신한다.

**필수 동기화:** 작업 시작 전에 최신 `tasks/WORK_STATE.md`, 역할별 READY 큐, 이 파일, `tasks/HANDOFF_LOG.md`를 읽고, 의미 있는 코드/설정/워크플로우/API 변경이 끝나면 상태판과 HANDOFF를 갱신한다. 상대 AI가 모르는 변경을 남기지 않는다.

상태: `TODO` / `DOING` / `REVIEW` / `DONE` / `BLOCKED` / `FROZEN`

Product integration checkpoint: `9261581df3abd5332d92855628fd7d03203748af` (PR #40 merge). Later main commits may be state/collaboration-only; always fetch current main before work.

## Active Work

| 담당 | 브랜치 / PR | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 쭌 AI | `jjun/production-new-game-core-v1` | Production NEW GAME Core v1: WorldSeed → exact 2M+2F authoritative founders | `Source/LifeLensCore/**` only for this first PR | DOING — Core identity/founder generation + deterministic tests |
| 쭌 + 쭌 AI | `task/03-fast-test`, PR #2 | old Android validation | Bridge/build | FROZEN — Run `34739283266` 실패 상태 보존, 수정/재실행/병합 금지 |
| 다겸 + 다겸 AI | `dagyeom/observer-ui-v2`, PR #17 | Observer HUD v2 + Core Observer Bridge binding | `Source/LifeLens/UI/**` | REVIEW / RECOVERING — latest main과 reconcile 필요 |
| 다겸 + 다겸 AI | `dagyeom/ui-foundation-v1`, PR #26 | Android landscape UI foundation | UI foundation | REVIEW — latest main reconcile 필요 |
| 다겸 + 다겸 AI | `dagyeom/character-presentation-v1`, PR #29 | Character Presentation v1 | Character presentation | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/observer-ux-polish-v1`, PR #30 | Observer UX Polish | `Source/LifeLens/UI/**` | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/mobile-touch-v1`, PR #36 | Mobile Touch v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #30 |
| 다겸 + 다겸 AI | `dagyeom/visual-feedback-v1`, PR #38 | Visual Feedback v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #36 |

## Current Jjun lock — Production NEW GAME Core v1

- Branch: `jjun/production-new-game-core-v1`
- Purpose: authoritative Core founder generation for SPEC 9~18 / 78.
- Allowed files for this first slice: `Source/LifeLensCore/**` and its tests/CMake.
- Explicitly not touching in this slice: `Source/LifeLens/UI/**`, Dagyeom Character presentation, Unreal legacy `ULLSimulationSubsystem` generation, Save/Load, Android workflows.
- Required behavior:
  - same WorldSeed → same four founder identities/traits;
  - different seed → meaningfully different founder output;
  - exactly 4 adults, exactly 2 male + 2 female;
  - stable unique Core IDs and unique display names;
  - generated personality/genetics/needs and birth time compatible with lifecycle aging;
  - initial relationships remain stranger/low familiarity, with no forced couple/family/pregnancy.
- After this PR: separate integration work will map Core founders into Unreal runtime/Save data rather than maintaining two competing generators.

## Latest verified Dagyeom PR chain

| PR | Branch | Verified HEAD | Base/dependency | Current note |
|---|---|---|---|---|
| #17 | `dagyeom/observer-ui-v2` | `dc3351ea9025ee33ea70f8ce1026100070c20092` | old `main` snapshot | OPEN, currently not mergeable against latest main; reconcile first |
| #26 | `dagyeom/ui-foundation-v1` | `70dfa5ebeabf24b661c9f9fd0bc63e3ad01ac180` | `main` | OPEN, mergeable but behind latest main |
| #29 | `dagyeom/character-presentation-v1` | `a4d47b9f69c9270665a8e2613b7863c40bf0f83e` | #17 branch | OPEN, mergeable stacked PR |
| #30 | `dagyeom/observer-ux-polish-v1` | `c420457c3ddbc73ac2ddbfe692dd20ce63edf45e` | #17 branch | OPEN, mergeable stacked PR |
| #36 | `dagyeom/mobile-touch-v1` | `15eec5b9216d6a30655f59450e410f0f80bb0343` | #30 branch | OPEN, mergeable stacked PR |
| #38 | `dagyeom/visual-feedback-v1` | `ffbc32c0cc9465a46feb1491f0bb7d5e0d1cd57a` | #36 branch | OPEN, mergeable stacked PR |

## Recently completed Jjun integration

| 담당 | 브랜치/PR | 작업 | 결과 |
|---|---|---|---|
| 쭌 + 쭌 AI | `jjun/observer-runtime-bridge-v1`, PR #37 | Core Observer Runtime Bridge | `main` merge `938d0a2798e600929b4ccc755b48bcd39026ac75`; Unreal 5.6 Linux UHT/UBT Run `34796067278` PASS |
| 쭌 + 쭌 AI | `jjun/family-runtime-state-v1`, PR #39 | authoritative Family/Romance/Household/Pregnancy ownership in Core Simulation | `main` merge `612229cc610d2ea6283e080309bdee58ef42d1db`; Core Run `34796213647` PASS |
| 쭌 + 쭌 AI | `jjun/family-observer-bridge-v1`, PR #40 | Family + World aggregate Observer read API | `main` merge `9261581df3abd5332d92855628fd7d03203748af`; Preflight `34796892593` PASS; Unreal UHT/UBT `34796892609` PASS |

## Jjun → Dagyeom API handoff — all previous blockers released

The previous six `BLOCKED-BY-JJUN` integration requests are **RESOLVED / READY FOR BINDING**:

1. Relationship 13D + target resident ID/name — available.
2. Emotion 11 axes + valence/arousal/intensity — available.
3. SocialIntent (Approach/Repair/Comfort/Avoid) + target — available.
4. Family summary: Partner/Parents/Children/Siblings + romance/marriage/cohabitation/pregnancy — available.
5. World overview: population/life stages/households/couples/stages/pregnancies/major LifeHistory records — available.
6. Unreal read-only Blueprint/USTRUCT Bridge — available.

Primary consumer surface:
- `ULLCoreBridgeSubsystem::GetWorldObservation()`
- `ULLCoreBridgeSubsystem::GetResidentObservations()`
- `ULLCoreBridgeSubsystem::GetResidentObservation(...)`
- `ULLCoreBridgeSubsystem::GetFamilyObservation(...)`
- `ULLCoreBridgeSubsystem::GetRecentCoreEvents()`
- `OnCoreRuntimeStateChanged`

If Dagyeom discovers a **new concrete missing field/API**, add a new Integration Request below. Do not keep the old six marked blocked.

## 쭌 측 next actions

1. Complete `jjun/production-new-game-core-v1` with Core tests/CI only.
2. Keep Dagyeom UI/Character presentation untouched while their branches reconcile.
3. After founder generation merges, start a separate latest-main runtime/Save integration branch.
4. Full Core Save/Load follows authoritative runtime integration.
5. Android smoke APK only after current Linux gates remain clean; old TASK_03 stays frozen.

## 다겸 측 next actions

1. **Before feature edits, fetch latest main and reconcile #17 first.**
2. Consume the now-available Core Observer Bridge instead of old placeholder/legacy read assumptions.
3. Address PR #17 UI review findings owned by Dagyeom.
4. Verify #17 with local/CI UHT/UBT + PIE; update docs before merge.
5. Then retarget/reconcile stacked PRs in dependency order: #29/#30 → #36 → #38. #26 may be reconciled independently.
6. Preserve Observer-first hierarchy; do not modify Core/Simulation internals to make UI data.
7. Record any new API gap as a new Integration Request.

## Shared File Lock

Default Jjun ownership — Dagyeom should not modify without an Integration Request/coordination:
- `LifeLens.uproject`
- `Source/LifeLens/LifeLens.Build.cs`
- `Source/LifeLens/Core/LLTypes.h`
- `Source/LifeLens/Simulation/**`
- `Source/LifeLensCore/**`
- `Config/**`
- `.github/workflows/**`
- `Tools/validate_bootstrap.py`

Default Dagyeom ownership — Jjun should avoid modifying while Dagyeom PRs are active:
- `Source/LifeLens/UI/**`
- Character appearance/presentation code
- `Content/UI/**`
- `Content/Characters/**`

State-only shared docs may be updated directly on `main` under `docs/STATE_MANAGEMENT.md`, but must reflect actual GitHub facts and must not overwrite newer counterpart state.

## Integration Requests

Current open requests: **none**.

Resolved 2026-09-14: the six Observer read requests formerly marked `BLOCKED-BY-JJUN` were completed by PR #37/#39/#40 and are now READY FOR DAGYEOM BINDING.

## Merge / reconciliation queue

1. Jjun Production NEW GAME Core v1 — new branch active; PR to be opened after first verified checkpoint.
2. Dagyeom PR #17 — reconcile latest main + bind current Observer Bridge + UI review fixes + verify.
3. Dagyeom PR #26 — reconcile latest main and verify independently.
4. After #17: PR #29 and #30 retarget/reconcile; then #36; then #38.
5. PR #2 remains FROZEN and is not part of this queue.

## 완료/인수인계 규칙

- Work is not considered done merely because code exists.
- Merge completion requires actual validation defined for that work plus state synchronization.
- Meaningful checkpoint/merge/failure must update `WORK_STATE.md` and append `HANDOFF_LOG.md`.
- **If GitHub and docs disagree, update docs first, then continue work.**
- A commit/PR without current state/handoff information is not considered collaboration-complete.
