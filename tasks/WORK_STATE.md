# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — PR #43 merged as `179e3a65aaa6ff8d2243117c7aebfd760812c73d` after full Core Release tests + deterministic harness + Structural Preflight PASS. PR #42 remains the latest Unreal runtime integration checkpoint with actual UHT/UBT PASS.

## Mandatory sync gate

1. 작업 시작/재개 전에 actual `main` HEAD, 대상 branch HEAD, PR state/head/base, Actions를 확인한다.
2. 이 문서 → 역할별 READY queue → `TEAM_BOARD.md` → `HANDOFF_LOG.md`와 대조한다.
3. 다르면 **코드보다 문서를 먼저 갱신**한다.
4. branch 생성, PR 생성, CI 결과, 실패 원인 확정, merge 등 의미 있는 checkpoint마다 즉시 갱신한다.
5. GitHub 실제 상태가 항상 stale 문서보다 우선한다.

---

## Active / unresolved work

### 1. Full Core Save/Load v1 — Jjun

- Owner: 쭌 + 쭌 AI
- Status: `PLANNED`
- Branch: not created yet; must branch from actual latest `main` when work starts.
- Goal: replace transitional seed+minute replay persistence with a true authoritative Core snapshot that restores the exact evolved world.
- Required snapshot scope includes at minimum:
  - world seed + simulation minute + deterministic RNG continuation state;
  - all Character identity/lifecycle/Needs/Personality/Emotion/Memory/Belief/Genetics/LifeCondition/LifeHistory state;
  - directional RelationshipBook;
  - Genealogy / Romance / Household / Pregnancy / Birth state;
  - runtime decision/cooldown state needed for continuity or an explicitly deterministic reconstruction contract;
  - stable resident identity mapping semantics across save/load.
- Required verification: save an evolved world after social/family progression, load into a fresh simulation, compare authoritative state, then continue both worlds and prove deterministic continuation.
- Unreal SaveGame adapter integration should follow the Core serializer contract rather than serializing a second independent resident truth.
- Exact next action when started: sync actual main/docs → create new Core Save/Load branch → define versioned snapshot DTO/serialization boundary → roundtrip + continuation tests before Unreal adapter changes.

### 2. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Status: `RECOVERING`
- Former six `BLOCKED-BY-JJUN` Observer requirements remain resolved.
- Main now additionally provides:
  - PR #42: founder Sex / AgeYears / LifeStage / 14-axis Personality through Core resident DTO;
  - PR #43: Romance/Household/Marriage/Pregnancy/Birth state is no longer test-only state; it can evolve autonomously in Core and newborns enter the real World resident list.
- Exact next action: reconcile actual latest main, bind Core Observer Bridge, address Dagyeom-owned UI review findings, verify UHT/UBT + PIE, update state before merge.

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

## Latest completed milestone — Autonomous Family Progression v1

### PR #43 `[CORE] Wire autonomous family progression into Simulation`

- Status: `DONE`
- Feature HEAD: `81dbb57d190810cc3318e82ba952064d50fd8fdc`
- Merge SHA: `179e3a65aaa6ff8d2243117c7aebfd760812c73d`
- Changed files: exactly 5 under `Source/LifeLensCore/**` including Core CMake/test; no Unreal/UI/Character/Content files.
- Implemented:
  - gradual state-driven romantic chemistry;
  - deterministic daily mutual dating selection;
  - no forced founder couples;
  - dating→cohabitation→engagement→marriage minimum-duration progression;
  - valid same-sex romance while current biological pregnancy retains gestational/genetic eligibility rules;
  - marriage→periodic deterministic pregnancy attempts;
  - pregnancy advancement and due birth;
  - newborn becomes real Character/World resident with inherited genetics, genealogy, household membership, parent-child relationships, LifeHistory and runtime state;
  - authoritative Simulation-owned BirthBook;
  - spouse links in Genealogy;
  - daily Aging lifecycle update.
- Validation:
  - Core Tests Run `34801572846` PASS: Configure / Build / Test / Deterministic harness smoke.
  - Structural Preflight Run `34801572858` PASS.
  - first run `34801444613` failed only on duplicate helper-name compilation; fixed in final head and superseded by the PASS run.

## Previous completed milestone — Production NEW GAME Unreal Runtime Integration

### PR #42 `[UE] Make production New Game Core-authoritative`

- Status: `DONE`
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
