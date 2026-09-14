# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — PR #42 merged as `5c6f2c071ca00bcd4b26d6e002bf5c618d02ff1e`. PR #43 current head `81dbb57d190810cc3318e82ba952064d50fd8fdc` passed full Core Release CI + deterministic harness + Structural Preflight and is READY_TO_MERGE.

## Mandatory sync gate

1. 작업 시작/재개 전에 actual `main` HEAD, 대상 branch HEAD, PR state/head/base, Actions를 확인한다.
2. 이 문서 → 역할별 READY queue → `TEAM_BOARD.md` → `HANDOFF_LOG.md`와 대조한다.
3. 다르면 **코드보다 문서를 먼저 갱신**한다.
4. branch 생성, PR 생성, CI 결과, 실패 원인 확정, merge 등 의미 있는 checkpoint마다 즉시 갱신한다.
5. GitHub 실제 상태가 항상 stale 문서보다 우선한다.

---

## Active / unresolved work

### 1. Autonomous Family Progression v1 — Jjun

- Owner: 쭌 + 쭌 AI
- Branch: `jjun/autonomous-family-progression-v1`
- PR: #43 `[CORE] Wire autonomous family progression into Simulation`
- Current head: `81dbb57d190810cc3318e82ba952064d50fd8fdc`
- Status: `READY_TO_MERGE`
- Scope: Core only — `FamilyProgression.h`, `Simulation.h/.cpp`, Core CMake, `test_autonomous_family_progression.cpp`.
- Implemented:
  - state-driven romantic chemistry from familiarity/social bond/personality compatibility;
  - daily deterministic family-decision cadence;
  - mutual-readiness dating candidate ranking without hard-coded founder couples;
  - dating→cohabitation minimum 30 days;
  - dating→engagement minimum 90 days plus mature cohabitation;
  - engagement→marriage minimum 60 days;
  - marriage→pregnancy minimum 30 days with weekly deterministic attempts;
  - same-sex romance remains valid; current biological pregnancy follows existing gestational/genetic eligibility;
  - active pregnancy advances with simulation time;
  - due pregnancy creates real child Character with genetics, genealogy, household membership, parent/child links, LifeHistory and runtime state;
  - Simulation now owns authoritative BirthBook;
  - successful marriage links spouses in Genealogy;
  - daily Aging is wired into lifecycle progression.
- Validation on head `81dbb57d...`:
  - LifeLens Core Tests Run `34801572846` — PASS: Configure / Build / Test / Deterministic harness smoke.
  - LifeLens Preflight Run `34801572858` — PASS.
  - first failed run `34801444613` was a bounded helper-name collision and is superseded by the successful head.
- Exact next action: verify changed-file scope remains Core-only, then merge #43 with expected head SHA and immediately sync docs/HANDOFF.
- Handoff safety: `SAFE` — no Dagyeom or Unreal runtime files changed.

### 2. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Status: `RECOVERING`
- Former six `BLOCKED-BY-JJUN` observer requirements are available on main via #37/#39/#40.
- PR #42 is merged and additionally exposes founder Sex / AgeYears / LifeStage / 14-axis Personality through the Core resident DTO.
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

## Latest completed milestone — Production NEW GAME Unreal Runtime Integration

### PR #42 `[UE] Make production New Game Core-authoritative`

- Status: `DONE`
- Feature HEAD: `5f61ce04963166af418fb672fb4442a6cf0598e6`
- Merge SHA: `5c6f2c071ca00bcd4b26d6e002bf5c618d02ff1e`
- Validation: Structural Preflight `34799244015` PASS; Unreal Linux Compile `34799244011` PASS including actual UHT/UBT.
- Main uses Core founders as the single production New Game population source and projects Core identity/state into Unreal compatibility data.
- Remaining explicit runtime debt: Core-action→3D presentation and full Core snapshot Save/Load.

### PR #41 Production NEW GAME Core v1

- Status: `DONE`
- Merge `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68`
- Core Tests `34798427843` PASS incl deterministic harness; Preflight `34798427848` PASS.

---

## Earlier completed integration relevant to Dagyeom

- #37 Observer Runtime Bridge — merge `938d0a2798e600929b4ccc755b48bcd39026ac75`; Unreal UHT/UBT `34796067278` PASS.
- #39 Authoritative Family Runtime State — merge `612229cc610d2ea6283e080309bdee58ef42d1db`; Core `34796213647` PASS.
- #40 Family + World Observer Bridge — merge `9261581df3abd5332d92855628fd7d03203748af`; Preflight `34796892593` + Unreal UHT/UBT `34796892609` PASS.

---

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/board/queue → update stale docs first → resume only from last verified GitHub checkpoint.
