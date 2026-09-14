# LifeLens Team Board

이 파일은 쭌(sjLim91)과 다겸(STILLofficial), 그리고 각자의 AI 에이전트가 동시에 작업할 때 사용하는 작업 잠금/분배 보드다.

## 최우선 동기화 규칙

**모든 기능 작업보다 상태 동기화가 먼저다.** 작업 시작/재개 시 실제 GitHub `main`/branch/PR/Actions 상태를 확인하고 `WORK_STATE.md` → 역할별 READY 큐 → 이 보드 → `HANDOFF_LOG.md`와 대조한다. 문서가 실제 상태와 다르면 코드 수정 전에 문서부터 갱신한다.

상태: `TODO` / `DOING` / `REVIEW` / `DONE` / `BLOCKED` / `FROZEN`

Product integration checkpoint: `9261581df3abd5332d92855628fd7d03203748af` (PR #40 merge). Later main commits may be state/collaboration-only; always fetch current main before work.

## Active Work

| 담당 | 브랜치 / PR | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 쭌 AI | `jjun/production-new-game-core-v1`, PR #41 | Production NEW GAME Core v1: WorldSeed → exact 2M+2F authoritative founders | `Source/LifeLensCore/**` only for this first PR | REVIEW / WAITING_CI — head `fd94b6b9...`; Core Tests + Preflight running |
| 쭌 + 쭌 AI | `task/03-fast-test`, PR #2 | old Android validation | Bridge/build | FROZEN — Run `34739283266` 실패 상태 보존, 수정/재실행/병합 금지 |
| 다겸 + 다겸 AI | `dagyeom/observer-ui-v2`, PR #17 | Observer HUD v2 + Core Observer Bridge binding | `Source/LifeLens/UI/**` | REVIEW / RECOVERING — latest main과 reconcile 필요 |
| 다겸 + 다겸 AI | `dagyeom/ui-foundation-v1`, PR #26 | Android landscape UI foundation | UI foundation | REVIEW — latest main reconcile 필요 |
| 다겸 + 다겸 AI | `dagyeom/character-presentation-v1`, PR #29 | Character Presentation v1 | Character presentation | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/observer-ux-polish-v1`, PR #30 | Observer UX Polish | `Source/LifeLens/UI/**` | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/mobile-touch-v1`, PR #36 | Mobile Touch v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #30 |
| 다겸 + 다겸 AI | `dagyeom/visual-feedback-v1`, PR #38 | Visual Feedback v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #36 |

## Current Jjun lock — Production NEW GAME Core v1

- Branch/PR: `jjun/production-new-game-core-v1`, PR #41
- Current head: `fd94b6b9c2c9d4ae23a6d65abf59ff4686218788`
- Purpose: authoritative Core founder generation for SPEC 9~18 / 78.
- Allowed files for this first slice: `Source/LifeLensCore/**` and its tests/CMake.
- Explicitly not touching: `Source/LifeLens/UI/**`, Dagyeom Character presentation, legacy Unreal `ULLSimulationSubsystem` generation, Save/Load, Android workflows.
- Implemented contract:
  - same WorldSeed → same four founder identities/traits;
  - different seed → different founder output;
  - exactly 4 adults, exactly 2 male + 2 female;
  - stable unique Core IDs and unique display names;
  - generated personality/genetics/needs and birth time compatible with lifecycle aging;
  - initial relationships are stranger/low familiarity only, with no forced couple/family/pregnancy;
  - `Simulation::setupNewGame()` resets authoritative social/family/runtime state and reseeds RNG from WorldSeed.
- Validation in progress:
  - Core Tests Run `34798427843` / job `103835989354`.
  - Structural Preflight Run `34798427848` / job `103835989348`.
  - Additional push Core Run `34798413152` / job `103835947136`.
- After this PR: separate integration work will map Core founders into Unreal runtime/Save data rather than maintaining two competing generators.

## Latest verified Dagyeom PR chain

| PR | Branch | Verified HEAD | Base/dependency | Current note |
|---|---|---|---|---|
| #17 | `dagyeom/observer-ui-v2` | `dc3351ea9025ee33ea70f8ce1026100070c20092` | old `main` snapshot | OPEN; reconcile latest main first |
| #26 | `dagyeom/ui-foundation-v1` | `70dfa5ebeabf24b661c9f9fd0bc63e3ad01ac180` | `main` | OPEN, behind latest main |
| #29 | `dagyeom/character-presentation-v1` | `a4d47b9f69c9270665a8e2613b7863c40bf0f83e` | #17 branch | OPEN stacked PR |
| #30 | `dagyeom/observer-ux-polish-v1` | `c420457c3ddbc73ac2ddbfe692dd20ce63edf45e` | #17 branch | OPEN stacked PR |
| #36 | `dagyeom/mobile-touch-v1` | `15eec5b9216d6a30655f59450e410f0f80bb0343` | #30 branch | OPEN stacked PR |
| #38 | `dagyeom/visual-feedback-v1` | `ffbc32c0cc9465a46feb1491f0bb7d5e0d1cd57a` | #36 branch | OPEN stacked PR |

## Recently completed Jjun integration

- PR #37 Core Observer Runtime Bridge — merge `938d0a2798e600929b4ccc755b48bcd39026ac75`; Unreal 5.6 Linux UHT/UBT `34796067278` PASS.
- PR #39 authoritative Family/Romance/Household/Pregnancy state — merge `612229cc610d2ea6283e080309bdee58ef42d1db`; Core `34796213647` PASS.
- PR #40 Family + World aggregate Observer read API — merge `9261581df3abd5332d92855628fd7d03203748af`; Preflight `34796892593` PASS; Unreal UHT/UBT `34796892609` PASS.

## Jjun → Dagyeom API handoff — all previous blockers released

The previous six `BLOCKED-BY-JJUN` integration requests are **RESOLVED / READY FOR BINDING**: Relationship 13D + target, Emotion detail, SocialIntent + target, Family summary, World family/lifecycle aggregates, and the read-only Blueprint/USTRUCT Core Bridge.

Primary consumer surface:
- `ULLCoreBridgeSubsystem::GetWorldObservation()`
- `ULLCoreBridgeSubsystem::GetResidentObservations()`
- `ULLCoreBridgeSubsystem::GetResidentObservation(...)`
- `ULLCoreBridgeSubsystem::GetFamilyObservation(...)`
- `ULLCoreBridgeSubsystem::GetRecentCoreEvents()`
- `OnCoreRuntimeStateChanged`

If Dagyeom discovers a **new concrete missing field/API**, add a new Integration Request. Do not keep the old six marked blocked.

## Next actions

### 쭌
1. Finish PR #41 CI. Fix only verified root causes if needed.
2. If PASS, merge #41 and sync state docs.
3. Start a separate latest-main Unreal runtime/Save integration branch to replace the legacy duplicate New Game generator.
4. Keep Dagyeom UI/Character presentation untouched.
5. Android smoke APK only after latest-main integration gates are clean; old TASK_03 stays frozen.

### 다겸
1. Reconcile PR #17 with latest main before feature edits.
2. Bind the now-available Core Observer Bridge.
3. Address Dagyeom-owned UI review findings and verify UHT/UBT + PIE.
4. Then reconcile stacked PRs #29/#30 → #36 → #38. #26 may proceed independently.

## Shared File Lock

Default Jjun ownership: `LifeLens.uproject`, `Source/LifeLens/LifeLens.Build.cs`, `Source/LifeLens/Core/LLTypes.h`, `Source/LifeLens/Simulation/**`, `Source/LifeLensCore/**`, `Config/**`, `.github/workflows/**`, `Tools/validate_bootstrap.py`.

Default Dagyeom ownership: `Source/LifeLens/UI/**`, Character appearance/presentation code, `Content/UI/**`, `Content/Characters/**`.

State-only shared docs may be updated directly on `main` under `docs/STATE_MANAGEMENT.md`, but must reflect actual GitHub facts and must not overwrite newer counterpart state.

## Integration Requests

Current open requests: **none**.

## Merge / reconciliation queue

1. PR #41 Production NEW GAME Core v1 — WAITING_CI.
2. Dagyeom PR #17 — reconcile latest main + bind current Observer Bridge + UI review fixes + verify.
3. Dagyeom PR #26 — reconcile latest main and verify independently.
4. After #17: PR #29/#30, then #36, then #38.
5. PR #2 remains FROZEN and is not part of this queue.

## 완료/인수인계 규칙

Work is not done merely because code exists. Required validation + merge + state synchronization are all part of completion. If GitHub and docs disagree, update docs first, then continue work.
