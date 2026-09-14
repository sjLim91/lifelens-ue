# LifeLens Canonical Work State

> 이 파일은 **현재 진행 상태의 단일 기준판(canonical live state)**이다. 쭌/다겸/양쪽 AI는 작업 시작 전에 반드시 읽고 실제 GitHub 상태와 대조한다.
>
> 제품 요구사항은 `docs/LIFELENS_SPEC_v1.1.md`, 상태관리 규칙은 `docs/STATE_MANAGEMENT.md`, 역할/잠금은 `tasks/TEAM_BOARD.md`, 변경 이력은 `tasks/HANDOFF_LOG.md`를 따른다.

Last product-state reconciled: 2026-09-14 KST — product integration checkpoint `9261581df3abd5332d92855628fd7d03203748af` (PR #40 merge), followed by collaboration/state-document sync commits. PR #37/#39/#40 are merged; Unreal 5.6 Linux UHT/UBT PASS; Dagyeom Core/API blockers released. **Always fetch actual current `main` before work.**

## Status legend
`PLANNED` / `IN_PROGRESS` / `WAITING_CI` / `READY_TO_MERGE` / `BLOCKED` / `INTERRUPTED` / `RECOVERING` / `FROZEN` / `DONE`

---

## Mandatory sync gate — highest priority

**모든 기능/코드/빌드 작업보다 상태 동기화가 먼저다.**

작업 시작/재개 시 반드시 실제 `main`/branch/PR/Actions를 이 파일, 역할별 READY 큐, `TEAM_BOARD.md`, `HANDOFF_LOG.md`와 대조하고, 불일치하면 코드 수정 전에 문서를 먼저 갱신한다. 의미 있는 checkpoint/실패/병합 시에도 즉시 상태를 다시 맞춘다.

---

## Active / unresolved work

### 1. Production NEW GAME Core v1 — Jjun

- Owner: 쭌 + 쭌 AI
- Branch: `jjun/production-new-game-core-v1`
- Branch start base: `cd5f2af04189fb80a76c4a20c016c681725110b2` (latest main at branch creation; later main changes are state-doc only).
- PR: #41 `[CORE] Add authoritative production New Game founders`
- Current head: `fd94b6b9c2c9d4ae23a6d65abf59ff4686218788`
- Status: `WAITING_CI`
- Changed scope: exactly 6 Core files; no Unreal UI/Character presentation files.
- Implemented:
  - authoritative Core `Sex` identity;
  - `InitialPopulation.h` founder generator;
  - WorldSeed deterministic exact 2 male + 2 female founders;
  - unique Core IDs + unique names;
  - generated Personality / Genetics / Needs / metabolism / sleep tendency;
  - 25~34-year birthMinute + existing Aging/Growth compatibility;
  - `Simulation::setupNewGame()` full state reset + RNG reseed + four-person minimum Needs objects;
  - all directed founder relationships exist but familiarity is only 0~0.04, with no forced romance/commitment/family/household/pregnancy;
  - `test_production_new_game` covers population composition, identity uniqueness, lifecycle consistency, neutral social/family start, same-seed determinism, different-seed variation, and repeated setup determinism.
- CI currently running on head `fd94b6b9...`:
  - Core Tests Run `34798427843`, job `103835989354` — in progress.
  - Structural Preflight Run `34798427848`, job `103835989348` — in progress.
  - Additional push-triggered Core Tests Run `34798413152`, job `103835947136` — in progress/redundant validation.
- Exact next action: wait for actual CI result; if failure, inspect first failing step/log and fix only root cause. If Core + Preflight PASS, mark READY_TO_MERGE and merge #41, then start a separate latest-main Unreal runtime/Save integration PR.
- Handoff safety: `SAFE` — Dagyeom files untouched.

### 2. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch: `dagyeom/observer-ui-v2`
- PR: #17
- Last verified HEAD: `dc3351ea9025ee33ea70f8ce1026100070c20092`
- Status: `RECOVERING`
- Current known GitHub fact: PR is OPEN; its branch predates latest main. All six former `BLOCKED-BY-JJUN` read requirements are available via #37/#39/#40.
- Exact next action: reconcile actual latest main, bind current Core Observer Bridge APIs, address Dagyeom-owned UI review items, verify UHT/UBT + PIE, update state before merge.
- Handoff safety: `CONDITIONAL`.

### 3. Dagyeom stacked UI / presentation chain

- #26 `dagyeom/ui-foundation-v1` — OPEN, independent main-target PR, reconcile latest main.
- #29 `dagyeom/character-presentation-v1` — stacked on #17.
- #30 `dagyeom/observer-ux-polish-v1` — stacked on #17.
- #36 `dagyeom/mobile-touch-v1` — stacked on #30.
- #38 `dagyeom/visual-feedback-v1` — stacked on #36.
- Rule: Jjun does not modify/flatten these branches; Dagyeom reconciles in dependency order after #17.

### 4. TASK_03 old Android validation

- Owner: 쭌 + 쭌 AI
- Branch: `task/03-fast-test`, PR #2
- Status: `FROZEN`
- Android Run `34739283266`: FAILURE; Cook/Package/APK not reached.
- Rule: **do not modify, rerun, revive, or merge.** Future Android work uses a new latest-main branch.

---

## Completed Jjun observer/runtime integration

- PR #37 Observer Runtime Bridge — DONE, merge `938d0a2798e600929b4ccc755b48bcd39026ac75`; Unreal 5.6 Linux actual UHT/UBT Run `34796067278` PASS.
- PR #39 Authoritative Family Runtime State — DONE, merge `612229cc610d2ea6283e080309bdee58ef42d1db`; Core Run `34796213647` PASS.
- PR #40 Family + World Observer Bridge — DONE, merge `9261581df3abd5332d92855628fd7d03203748af`; Preflight `34796892593` PASS; Unreal UHT/UBT `34796892609` PASS.

The former six Dagyeom `BLOCKED-BY-JJUN` observer requirements are all **READY FOR DAGYEOM BINDING**: Relationship 13D, Emotion detail, SocialIntent+target, Family summary, World family/lifecycle aggregates, and read-only Blueprint/USTRUCT Core Bridge.

---

## Recently completed Core milestones

Relationship #4, Emotion #6, Memory #7, Belief #8, Social Cognition #9, Social Utility #10, Social Execution #11, Simulation Social Loop #12, Observer Read Model #13, Romance #14, Household #15, Marriage #16, Pregnancy #18, Birth/Genetics #19, Growth #20, Parenting #21, Genealogy, Aging #23, Death #25, LifeHistory #27, LifeHistory Wiring #32, Generation Continuity #33, Observer Read Model v2 #34, Core validation recovery #35, Observer Runtime Bridge #37, Family Runtime State #39, Family/World Bridge #40 are main-integrated.

---

## Mandatory recovery rule

If a session/tool/chat is interrupted: fetch actual main/branch HEAD, PR state/head/base, and related Actions; compare with this file + board/queue; update stale docs first; resume only from the last verified GitHub checkpoint.
