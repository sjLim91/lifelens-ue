# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — PR #42 merged as `5c6f2c071ca00bcd4b26d6e002bf5c618d02ff1e` after Structural Preflight + actual Unreal 5.6 Linux UHT/UBT PASS. Next Jjun task is the Core-only autonomous family/lifecycle progression loop.

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
- Status: `PLANNED → IN_PROGRESS` after branch creation.
- Branch: `jjun/autonomous-family-progression-v1` (to be created from latest main after this state update).
- Scope: **`Source/LifeLensCore/**` + Core tests/CMake only.** No Unreal Simulation/UI/Characters/Content files.
- Goal: existing Romance / Household / Marriage / Pregnancy / Birth systems must stop being isolated APIs and become an autonomous deterministic part of `Simulation::step()`.
- Required lifecycle path:
  - relationship quality can naturally trigger dating; no forced initial couple;
  - dating can lead to cohabitation;
  - sufficiently mature/stable dating can lead to engagement;
  - engagement can lead to marriage;
  - eligible committed couples may attempt pregnancy;
  - pregnancy advances over simulation time;
  - due pregnancy produces a real child with genetics + genealogy + household membership + LifeHistory;
  - child becomes a normal `World::characters` resident and receives runtime state.
- Determinism: same WorldSeed + same starting state must produce the same progression and child identity/name/genetics.
- Safety rules:
  - no instant chain in one minute; explicit minimum relationship/stage durations and periodic checks;
  - same-sex romance remains valid; current biological pregnancy only when one partner can gestate and the other can contribute genetics under existing reproductive model;
  - founders still start as strangers; progression must arise from state, not hard-coded pairing.
- Validation required: existing full Core Release test suite + deterministic harness + new end-to-end progression tests + Structural Preflight. Unreal UHT/UBT is not required unless Unreal-side files are touched (not planned).
- Exact next action: create branch → inspect existing stage APIs and current `Simulation::step()` → implement deterministic periodic progression coordinator + tests.

### 2. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Status: `RECOVERING`
- Former six `BLOCKED-BY-JJUN` observer requirements are available on main via #37/#39/#40.
- PR #42 now also exposes founder Sex / AgeYears / LifeStage / 14-axis Personality through the Core resident DTO.
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
- Validation:
  - Structural Preflight Run `34799244015` — PASS.
  - Unreal Linux Compile Run `34799244011` — PASS including actual UHT/UBT.
- Main now:
  - starts production Core via `StartCoreNewGame` / `Simulation::setupNewGame()`;
  - uses Core founders as the single New Game population source;
  - exposes Sex/Age/LifeStage/Personality identity through Unreal DTOs;
  - maps Core CharacterId + WorldSeed to stable FGuid;
  - projects Core residents/relationships into legacy compatibility data rather than randomizing another four residents;
  - advances Core from the Unreal simulation path;
  - reconstructs current transitional save state from seed + minute replay rather than trusting legacy resident arrays as authority.
- Explicit remaining limitation: WorldDirector physical/action presentation is still compatibility-layer driven; full Core-action→3D presentation and full Core snapshot Save/Load remain later bounded tasks.

### PR #41 Production NEW GAME Core v1

- Status: `DONE`
- Merge `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68`
- Core Tests `34798427843` PASS incl deterministic harness; Preflight `34798427848` PASS.
- Core owns deterministic exact 2M+2F founders, identity, personality/genetics/needs/lifecycle birth time and neutral initial relationship state.

---

## Earlier completed integration relevant to Dagyeom

- #37 Observer Runtime Bridge — merge `938d0a2798e600929b4ccc755b48bcd39026ac75`; Unreal UHT/UBT `34796067278` PASS.
- #39 Authoritative Family Runtime State — merge `612229cc610d2ea6283e080309bdee58ef42d1db`; Core `34796213647` PASS.
- #40 Family + World Observer Bridge — merge `9261581df3abd5332d92855628fd7d03203748af`; Preflight `34796892593` + Unreal UHT/UBT `34796892609` PASS.

---

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/board/queue → update stale docs first → resume only from last verified GitHub checkpoint.
