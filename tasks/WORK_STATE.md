# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — actual main `0d82aefd84e0dda79ceb568345968cbb23c809ed`. PR #42/#43 are merged and validated. Full Core Save/Load v1 starts now on `jjun/core-save-load-v1`.

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
- Branch: `jjun/core-save-load-v1`
- Status: `DOING`
- Scope lock: `Source/LifeLensCore/**` first. Unreal SaveGame adapter is a later bounded integration after the Core snapshot contract passes.
- Goal: replace transitional seed+minute replay persistence with a versioned authoritative Core snapshot that restores the exact evolved world.
- Required snapshot scope:
  - world seed + simulation minute + deterministic RNG continuation state;
  - all Character identity/lifecycle/Needs/Personality/Emotion/Memory/Belief/Genetics/LifeCondition/LifeHistory state;
  - directional RelationshipBook;
  - Genealogy / Romance / Household / Pregnancy / Birth state;
  - runtime decision/cooldown/position state required for deterministic continuation;
  - stable CharacterId semantics across save/load.
- Required verification:
  1. evolve a world through social/family state;
  2. snapshot it;
  3. restore into a fresh Simulation;
  4. compare authoritative state deeply;
  5. continue original/restored worlds and prove deterministic continuation.
- No Dagyeom UI/Character presentation changes in this slice.

### 2. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Status: `RECOVERING`
- Former six `BLOCKED-BY-JJUN` Observer requirements remain resolved.
- Main additionally has #42 founder identity runtime integration and #43 autonomous family progression/newborn population growth.
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

## Latest completed milestones

### PR #43 Autonomous Family Progression v1
- DONE — merge `179e3a65aaa6ff8d2243117c7aebfd760812c73d`.
- Core Tests `34801572846` PASS including deterministic harness; Preflight `34801572858` PASS.
- Romance→cohabitation→engagement→marriage→pregnancy→birth can evolve in `Simulation::step()`, and newborns become real World residents.

### PR #42 Production NEW GAME Unreal Runtime Integration
- DONE — merge `5c6f2c071ca00bcd4b26d6e002bf5c618d02ff1e`.
- Preflight `34799244015` PASS; Unreal Linux Compile `34799244011` PASS including actual UHT/UBT.
- Core founders are the production New Game source of truth.

### PR #41 Production NEW GAME Core v1
- DONE — merge `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68`.
- Core Tests `34798427843` PASS; Preflight `34798427848` PASS.

Earlier completed bridge/family milestones: #37 `938d0a27...`, #39 `612229cc...`, #40 `9261581d...`.

---

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/board/queue → update stale docs first → resume only from last verified GitHub checkpoint.
