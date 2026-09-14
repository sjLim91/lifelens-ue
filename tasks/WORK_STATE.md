# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — PR #41 merged as `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68`; new runtime integration branch `jjun/production-new-game-runtime-v1` created from current main `3f1848873e38325b6aa240f061d1035a5f9c4665`.

## Mandatory sync gate

1. 작업 시작/재개 전에 actual `main` HEAD, 대상 branch HEAD, PR state/head/base, Actions를 확인한다.
2. 이 문서 → 역할별 READY queue → `TEAM_BOARD.md` → `HANDOFF_LOG.md`와 대조한다.
3. 다르면 **코드보다 문서를 먼저 갱신**한다.
4. branch 생성, PR 생성, CI 결과, 실패 원인 확정, merge 등 의미 있는 checkpoint마다 즉시 갱신한다.
5. GitHub 실제 상태가 항상 stale 문서보다 우선한다.

---

## Active / unresolved work

### 1. Production NEW GAME Unreal Runtime Integration — Jjun

- Owner: 쭌 + 쭌 AI
- Branch: `jjun/production-new-game-runtime-v1`
- Base at creation: `3f1848873e38325b6aa240f061d1035a5f9c4665`
- PR: not opened yet.
- Status: `IN_PROGRESS`
- Goal: PR #41의 authoritative Core `Simulation::setupNewGame()`을 Unreal runtime의 실제 NEW GAME 시작 경로에 연결하고, legacy `ULLSimulationSubsystem`의 별도 2M+2F 생성기가 두 번째 진실이 되지 않도록 정리한다.
- Initial verified gap:
  - `ULLCoreBridgeSubsystem` currently exposes only `StartCoreObserverDemo()` and calls `setupSocialDemo()` / `setupDemo()`.
  - `FLLCoreResidentObservation` currently lacks founder Sex / AgeYears / LifeStage identity fields.
  - legacy `ULLSimulationSubsystem::NewGame()` still owns a separate four-resident generator and legacy SaveGame arrays.
- Required contract:
  - WorldSeed 하나가 Core Simulation의 founder 생성 원천.
  - Core CharacterId + WorldSeed → 기존 stable Unreal `FGuid` 규칙 유지.
  - production bridge path calls `Simulation::setupNewGame()`.
  - Unreal read DTO exposes enough founder identity (Sex/Age/LifeStage) for runtime/presentation consumption without leaking Core types.
  - no independent second founder randomization in the production path.
  - Dagyeom `Source/LifeLens/UI/**` / Character presentation files are untouched.
- Validation: structural preflight + actual UE 5.6 Linux UHT/UBT required for any Unreal-side changes. Core tests required if Core files change.
- Exact next action: inspect all legacy `ULLSimulationSubsystem` callers/usages → choose bounded adapter strategy → implement production start/read identity path → validate before any Save/Load follow-on.
- Handoff safety: `SAFE` while confined to Jjun-owned Simulation/Core adapter files.

### 2. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Last verified HEAD: `dc3351ea9025ee33ea70f8ce1026100070c20092`
- Status: `RECOVERING`
- Fact: former six `BLOCKED-BY-JJUN` Observer read requirements are available on main via #37/#39/#40.
- Exact next action: reconcile actual latest main, bind current Core Observer Bridge, address Dagyeom-owned UI review findings, verify UHT/UBT + PIE, update state before merge.

### 3. Dagyeom stacked UI / presentation chain

- #26 `dagyeom/ui-foundation-v1` — independent main-target PR; reconcile latest main.
- #29 `dagyeom/character-presentation-v1` — stacked on #17.
- #30 `dagyeom/observer-ux-polish-v1` — stacked on #17.
- #36 `dagyeom/mobile-touch-v1` — stacked on #30.
- #38 `dagyeom/visual-feedback-v1` — stacked on #36.
- Jjun does not modify/flatten these branches.

### 4. Old Android validation

- Branch/PR: `task/03-fast-test`, PR #2
- Status: `FROZEN`
- Run `34739283266`: FAILURE; Cook/Package/APK not reached.
- Rule: **do not modify, rerun, revive, or merge.**

---

## Latest completed milestone — Production NEW GAME Core v1

- PR #41 — DONE
- Feature HEAD `fd94b6b9c2c9d4ae23a6d65abf59ff4686218788`
- Merge `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68`
- Core Tests `34798427843` PASS including deterministic harness.
- Structural Preflight `34798427848` PASS.
- Main now has Core Sex identity, deterministic exact 2M+2F founder generation, lifecycle birth times, neutral initial relationships, and `Simulation::setupNewGame()`.

---

## Completed integration chain relevant to Dagyeom

- #37 Observer Runtime Bridge — DONE, merge `938d0a2798e600929b4ccc755b48bcd39026ac75`; Unreal UHT/UBT `34796067278` PASS.
- #39 Authoritative Family Runtime State — DONE, merge `612229cc610d2ea6283e080309bdee58ef42d1db`; Core `34796213647` PASS.
- #40 Family + World Observer Bridge — DONE, merge `9261581df3abd5332d92855628fd7d03203748af`; Preflight `34796892593` + Unreal UHT/UBT `34796892609` PASS.

Former Dagyeom blockers now READY: Relationship 13D, Emotion detail, SocialIntent+target, Family summary, World family/lifecycle aggregates, Blueprint/USTRUCT read-only Bridge.

---

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/board/queue → update stale docs first → resume only from last verified GitHub checkpoint.
