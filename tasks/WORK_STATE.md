# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — PR #44 Full Core Save/Load v1 merged as `2b3f9882703ed73cb8318ae262f26bebb995c209` after Core Release suite + deterministic harness + Structural Preflight PASS. Next bounded task is Unreal SaveGame adapter v1.

## Mandatory sync gate

1. 작업 시작/재개 전에 actual `main` HEAD, 대상 branch HEAD, PR state/head/base, Actions를 확인한다.
2. 이 문서 → 역할별 READY queue → `TEAM_BOARD.md` → `HANDOFF_LOG.md`와 대조한다.
3. 다르면 **코드보다 문서를 먼저 갱신**한다.
4. branch 생성, PR 생성, CI 결과, 실패 원인 확정, merge 등 의미 있는 checkpoint마다 즉시 갱신한다.
5. GitHub 실제 상태가 항상 stale 문서보다 우선한다.

---

## Active / unresolved work

### 1. Unreal SaveGame Adapter v1 — Jjun

- Owner: 쭌 + 쭌 AI
- Planned branch: `jjun/unreal-savegame-adapter-v1`
- Status: `STARTING`
- Base requirement: use PR #44 `SimulationStateSnapshot` as the single Core persistence contract.
- Goal:
  - replace seed+minute replay load in `ULLSimulationSubsystem` with actual Core snapshot restore;
  - persist the Core snapshot through Unreal SaveGame/file storage without reintroducing a second resident truth;
  - preserve stable WorldSeed/CharacterId→FGuid mapping semantics;
  - after load, rebuild compatibility projection from restored Core state only.
- Scope lock: `Source/LifeLens/Simulation/**` plus narrowly required Core snapshot serialization helpers if needed; no `Source/LifeLens/UI/**` or Dagyeom Character presentation edits.
- Required validation: Structural Preflight + actual UE 5.6 Linux UHT/UBT; add save/load contract checks before merge.

### 2. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Status: `RECOVERING`
- Former six `BLOCKED-BY-JJUN` Observer requirements remain resolved.
- Main has #42 founder runtime integration, #43 autonomous family progression/newborn growth, and #44 full authoritative Core snapshot restore contract.
- Exact next action: reconcile latest main, bind current Core Observer Bridge, address Dagyeom-owned UI review findings, verify UHT/UBT + PIE.

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

## Latest completed milestone — Full Core Save/Load v1

### PR #44 `[CORE] Add authoritative full-state snapshot save/load v1`

- Status: `DONE`
- Feature HEAD: `64d2fb25f6453112aabaef2d809b0528c9dc4566`
- Merge SHA: `2b3f9882703ed73cb8318ae262f26bebb995c209`
- Changed files: exactly 5, all under `Source/LifeLensCore/**`.
- Snapshot v1 captures World/RNG, complete Character state, Relationship/Genealogy/Romance/Household/Pregnancy/Birth books, Simulation runtime plan/cooldown/position/social state, and Core logs.
- Restore validates version and cross-reference integrity before committing state; callbacks remain external runtime attachments.
- Validation:
  - Core Tests `34802611336` PASS including Configure / Build / Test / Deterministic harness smoke.
  - Structural Preflight `34802611299` PASS.
  - Deep rich-state roundtrip equality PASS.
  - Unsupported snapshot version rejection without destination mutation PASS.
  - Original/restored worlds remain deeply identical after an additional 10,000 simulated minutes.

## Earlier completed milestones

- PR #43 Autonomous Family Progression v1 — merge `179e3a65aaa6ff8d2243117c7aebfd760812c73d`; Core `34801572846` PASS; Preflight `34801572858` PASS.
- PR #42 Production NEW GAME Unreal Runtime Integration — merge `5c6f2c071ca00bcd4b26d6e002bf5c618d02ff1e`; Preflight `34799244015`; Unreal UHT/UBT `34799244011` PASS.
- PR #41 Production NEW GAME Core v1 — merge `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68`; Core `34798427843` PASS; Preflight `34798427848` PASS.
- Earlier bridge/family milestones: #37 `938d0a27...`, #39 `612229cc...`, #40 `9261581d...`.

---

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/board/queue → update stale docs first → resume only from last verified GitHub checkpoint.
