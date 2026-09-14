# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — PR #42 opened at head `5f61ce04963166af418fb672fb4442a6cf0598e6`; Structural Preflight and Unreal 5.6 Linux Compile are running.

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
- PR: #42 `[UE] Make production New Game Core-authoritative`
- Head: `5f61ce04963166af418fb672fb4442a6cf0598e6`
- Status: `WAITING_CI`
- Implemented scope:
  - `ULLCoreBridgeSubsystem::StartCoreNewGame(Seed)` → Core `setupNewGame()`.
  - Core resident read DTO now exposes Sex, AgeYears, 8-stage LifeStage, 14-axis Personality.
  - `ULLSimulationSubsystem::NewGame()` no longer generates a second population; it starts Core and projects the Core founders into compatibility `FLLResidentData`.
  - `AdvanceSimulationMinutes()` advances Core then refreshes the compatibility projection.
  - compatibility relationship rows are projected from Core directional Relationship data.
  - Save writes seed/minute + projection for compatibility; Load ignores legacy resident arrays as authority and reconstructs Core deterministically from WorldSeed + SimulationMinute replay.
  - old `GenerateInitialPopulation`, `GenerateAdult`, `MakeDeterministicGuid` production path removed.
  - Dagyeom UI/Character presentation files untouched.
- Transitional limitation (explicit): current `WorldDirector` physical movement/action layer still uses compatibility projection; `ApplyActionOutcome` / `ApplySocialInteraction` are presentation compatibility only and Core next tick overwrites them. Full Core-action→world-presentation integration is a later bounded task.
- Validation currently running:
  - LifeLens Preflight Run `34799244015` — in progress.
  - LifeLens Unreal Linux Compile Run `34799244011` — in progress; actual UHT/UBT required before merge.
- Exact next action: inspect both CI results. If failure, fetch exact job logs and fix first root cause only. If both PASS, reconcile latest main/state docs, verify changed-file scope, then merge #42.
- Handoff safety: `CONDITIONAL` until actual UHT/UBT PASS.

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
- Main has Core Sex identity, deterministic exact 2M+2F founders, lifecycle birth times, neutral initial relationships, and `Simulation::setupNewGame()`.

---

## Completed integration chain relevant to Dagyeom

- #37 Observer Runtime Bridge — DONE, merge `938d0a2798e600929b4ccc755b48bcd39026ac75`; Unreal UHT/UBT `34796067278` PASS.
- #39 Authoritative Family Runtime State — DONE, merge `612229cc610d2ea6283e080309bdee58ef42d1db`; Core `34796213647` PASS.
- #40 Family + World Observer Bridge — DONE, merge `9261581df3abd5332d92855628fd7d03203748af`; Preflight `34796892593` + Unreal UHT/UBT `34796892609` PASS.

Former Dagyeom blockers now READY: Relationship 13D, Emotion detail, SocialIntent+target, Family summary, World family/lifecycle aggregates, Blueprint/USTRUCT read-only Bridge.

---

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/board/queue → update stale docs first → resume only from last verified GitHub checkpoint.
