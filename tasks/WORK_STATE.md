# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — actual `main` HEAD `7f7777cd9d30ca06f486ffe5130d691ff9ff4c58` verified. Core Decision → Unreal Physical Action Bridge v1 is now starting as the active Jjun task; state lock is being established before branch/code work.

## Mandatory sync gate

1. 작업 시작/재개 전에 actual `main` HEAD, 대상 branch HEAD, PR state/head/base, Actions를 확인한다.
2. 이 문서 → 역할별 READY queue → `TEAM_BOARD.md` → `HANDOFF_LOG.md`와 대조한다.
3. 다르면 **코드보다 문서를 먼저 갱신**한다.
4. branch 생성, PR 생성, CI 결과, 실패 원인 확정, merge 등 의미 있는 checkpoint마다 즉시 갱신한다.
5. GitHub 실제 상태가 항상 stale 문서보다 우선한다.

---

## Active / unresolved work

### 1. Core Decision → Unreal Physical Action Bridge v1 — Jjun

- Owner: 쭌 + 쭌 AI
- Status: `DOING / STATE_LOCKED`
- Planned branch: `jjun/core-physical-action-bridge-v1` from the actual latest main after this state update.
- Goal: remove the remaining split-brain between authoritative Core decisions/state and the physical 3D WorldDirector behavior.
- Required direction:
  - Core remains the only source of Needs/social/family/decision truth;
  - Unreal World/AI executes or presents Core-selected actions rather than independently choosing a competing life decision;
  - preserve current stable resident GUID mapping and post-load continuity;
  - do not modify Dagyeom UI or Character appearance/presentation without a new explicit coordination request.
- Initial bounded scope:
  - map Core physical decisions for Eat / Sleep / Toilet / Hygiene / Idle into Unreal execution intent;
  - map current SocialIntent + target into Unreal presentation/execution routing without a competing legacy chooser;
  - reuse existing WorldDirector movement/action plumbing where safe, but legacy `ULLDecisionComponent` must not remain decision authority for covered intents;
  - keep SaveGame v2 restore continuity intact.
- Allowed scope: Jjun-owned `Source/LifeLens/Simulation/**`, `Source/LifeLens/AI/**`, `Source/LifeLens/World/**`, narrowly required Core read DTO/validator/tests.
- Forbidden scope: `Source/LifeLens/UI/**`, Dagyeom Character appearance/presentation, `Content/UI/**`, `Content/Characters/**`, frozen PR #2.
- Required verification: structural contract + actual UE 5.6 UHT/UBT, then targeted runtime/PIE verification where available.
- Exact next action: create `jjun/core-physical-action-bridge-v1` from the latest main after state-doc commits, inspect current Core observation/action DTO and WorldDirector/DecisionComponent execution path, then implement the smallest authority handoff first.

### 2. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Status: `RECOVERING`
- Former six `BLOCKED-BY-JJUN` Observer requirements remain resolved.
- Main now includes #42 Core-authoritative founders, #43 autonomous family progression/newborn population growth, #44 full Core snapshot contract, and #45 actual SaveGame v2 snapshot persistence/restore.
- Exact next action: reconcile actual latest main, bind current Core Observer Bridge, address Dagyeom-owned UI review findings, verify UHT/UBT + PIE.

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

## Latest completed milestone — Unreal SaveGame Adapter v1

### PR #45 `[UE] Persist authoritative Core snapshots through SaveGame v2`

- Status: `DONE`
- Feature HEAD: `547b3f94e0c3df6df15d723e72c8084cd10a7c29`
- Merge SHA: `3c646ba331b8199a295fd6f2e9cac1235d844679`
- Changed files: exactly 10 Jjun-owned Save/Simulation/Core/validator files; UI/Characters/Content untouched.
- Implemented:
  - pure Core persistent binary snapshot codec with magic `LLSNAP01` and explicit binary/Core snapshot versions;
  - canonical CharacterId ordering for runtime-map serialization;
  - full World/RNG/Character/SmartObject/Relationship/Genealogy/Romance/Household/Pregnancy/Birth/runtime/log state encoding;
  - malformed/corrupt/truncated/trailing payload rejection;
  - Core Bridge `CaptureCoreSnapshotBytes` / `RestoreCoreSnapshotBytes`;
  - restore validates against a temporary Core candidate before replacing the live world;
  - `ULLSaveGame` v2 stores `CoreSnapshotBytes` as authoritative simulation truth;
  - v2 Load restores Core directly and rebuilds compatibility projection;
  - v1 legacy save migration retains seed+minute replay only; legacy resident/relationship arrays never become authority.
- Validation:
  - Core Tests `34803226434` PASS: Configure / Build / Test / deterministic harness.
  - Structural Preflight `34803226458` PASS.
  - Unreal Linux Compile `34803226433` PASS including actual UE 5.6 UHT + UBT.
  - binary codec roundtrip/canonical bytes/corrupt payload/10,000-minute continuation tests PASS.

## Previous completed milestone — Full Core Save/Load v1

### PR #44 `[CORE] Add authoritative full-state snapshot save/load v1`

- Status: `DONE`
- Feature HEAD: `64d2fb25f6453112aabaef2d809b0528c9dc4566`
- Merge SHA: `2b3f9882703ed73cb8318ae262f26bebb995c209`
- Core Tests `34802611336` PASS incl Configure / Build / Test / deterministic harness.
- Structural Preflight `34802611299` PASS.
- Rich deep roundtrip and exact 10,000-minute post-load continuation PASS.

## Earlier completed milestones

- PR #43 Autonomous Family Progression v1 — merge `179e3a65aaa6ff8d2243117c7aebfd760812c73d`; Core `34801572846` PASS; Preflight `34801572858` PASS.
- PR #42 Production NEW GAME Unreal Runtime Integration — merge `5c6f2c071ca00bcd4b26d6e002bf5c618d02ff1e`; Preflight `34799244015`; Unreal UHT/UBT `34799244011` PASS.
- PR #41 Production NEW GAME Core v1 — merge `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68`; Core `34798427843` PASS; Preflight `34798427848` PASS.
- Earlier bridge/family milestones: #37 `938d0a27...`, #39 `612229cc...`, #40 `9261581d...`.

---

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/board/queue → update stale docs first → resume only from last verified GitHub checkpoint.
