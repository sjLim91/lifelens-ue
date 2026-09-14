# LifeLens Canonical Work State

> 이 파일은 **현재 진행 상태의 단일 기준판(canonical live state)**이다. 쭌/다겸/양쪽 AI는 작업 시작 전에 반드시 읽고 실제 GitHub 상태와 대조한다.
>
> 제품 요구사항은 `docs/LIFELENS_SPEC_v1.1.md`, 상태관리 규칙은 `docs/STATE_MANAGEMENT.md`, 역할/잠금은 `tasks/TEAM_BOARD.md`, 변경 이력은 `tasks/HANDOFF_LOG.md`를 따른다.

Last product-state reconciled: 2026-09-14 KST — product integration checkpoint `9261581df3abd5332d92855628fd7d03203748af` (PR #40 merge), followed by collaboration/state-document sync commits. PR #37/#39/#40 are merged; Unreal 5.6 Linux UHT/UBT PASS; Dagyeom Core/API blockers released. **Always fetch the actual current `main` HEAD before work.**

## Status legend

`PLANNED` / `IN_PROGRESS` / `WAITING_CI` / `READY_TO_MERGE` / `BLOCKED` / `INTERRUPTED` / `RECOVERING` / `FROZEN` / `DONE`

---

## Mandatory sync gate — highest priority

**모든 기능/코드/빌드 작업보다 상태 동기화가 먼저다.**

작업 시작/재개 시 반드시:
1. 실제 `main` HEAD / 대상 branch HEAD / PR state+head / Actions 상태를 확인한다.
2. 이 파일, `TEAM_BOARD.md`, 역할별 READY 큐, `HANDOFF_LOG.md`와 비교한다.
3. 불일치하면 **코드 수정 전에 문서를 실제 GitHub 상태로 먼저 갱신**한다.
4. 작업 중 의미 있는 checkpoint마다 상태를 갱신한다.
5. 작업 종료/병합/실패 시 문서를 다시 동기화하고 HANDOFF를 남긴다.

GitHub 실제 상태가 항상 문서보다 우선하며, 문서가 stale인 상태에서 새 기능 작업을 시작하지 않는다.

---

## Active / unresolved work

### 1. Production NEW GAME Core v1 — Jjun

- Owner: 쭌 + 쭌 AI
- Branch: `jjun/production-new-game-core-v1`
- Base at branch creation: latest `main` after collaboration sync (`cd5f2af04189fb80a76c4a20c016c681725110b2`).
- PR: not opened yet.
- Status: `IN_PROGRESS`
- Goal: move the formal NEW GAME founder population rule into authoritative `LifeLensCore`: one WorldSeed deterministically creates exactly 4 adult founders, exactly 2 male + 2 female, unique stable Core IDs, unique display names, generated personality/genetics/needs/life condition, and birth times compatible with Aging/Growth.
- Initial relationship rule: founders begin as strangers / low familiarity; no forced couple, marriage, household, pregnancy, or family links.
- Scope of this first PR: **Core generation + Simulation production setup + deterministic tests only.** Do not yet replace Unreal Save/Load or Dagyeom UI code in the same PR.
- Last verified fact: current Core `Character` has no sex field; `Simulation::setupSocialDemo()` creates only fixed `SocialA/SocialB`; legacy Unreal `ULLSimulationSubsystem::NewGame()` already has a separate 2M+2F generator but it is not authoritative Core state.
- Validation required: C++17 Core Release tests with assertions, deterministic same-seed/different-seed checks, existing Core regression suite, structural preflight. Unreal UHT/UBT only if this PR changes Unreal-side files (not planned for v1).
- Exact next action: add Core identity/sex + founder generator, expose `Simulation::setupNewGame()`, add tests/CMake registration, run CI, then open PR and update this state.
- Handoff safety: `SAFE` — no Dagyeom UI/Character presentation files in scope.

### 2. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch: `dagyeom/observer-ui-v2`
- PR: #17 `[UI] Observer HUD v2: LEVEL 0 overview, LEVEL 1 quick inspector, LEVEL 2 detail tabs`
- Last verified HEAD: `dc3351ea9025ee33ea70f8ce1026100070c20092`
- Base shown by PR: old `main` snapshot `d1324c766749847c9fc85ea69af60c23c293da31`
- Status: `RECOVERING`
- Current GitHub fact: PR is OPEN, non-draft, currently `mergeable=false`; reviewer is `sjLim91`.
- Important change since PR body was written: all six former `BLOCKED-BY-JJUN` observer read requirements are now available on `main` via PR #37/#39/#40.
- Existing UI review findings remain Dagyeom-owned: selection hint visibility and narrow-panel text overflow among others.
- Exact next action: Dagyeom side first reconciles PR #17 with actual latest `main`, then binds the newly available Core Observer Bridge APIs, addresses UI review items, runs local/CI UHT/UBT + PIE verification, and updates state before merge.
- Handoff safety: `CONDITIONAL` — current branch is older than latest main and must be reconciled before feature edits/merge.

### 3. Dagyeom stacked UI / presentation PR chain

These are active and must not be flattened or modified by Jjun without coordination.

- PR #26 `dagyeom/ui-foundation-v1` — OPEN, mergeable; HEAD `70dfa5ebeabf24b661c9f9fd0bc63e3ad01ac180`; base `main` but behind current main.
- PR #29 `dagyeom/character-presentation-v1` — OPEN, mergeable; HEAD `a4d47b9f69c9270665a8e2613b7863c40bf0f83e`; stacked on PR #17 branch.
- PR #30 `dagyeom/observer-ux-polish-v1` — OPEN, mergeable; HEAD `c420457c3ddbc73ac2ddbfe692dd20ce63edf45e`; stacked on PR #17 branch.
- PR #36 `dagyeom/mobile-touch-v1` — OPEN, mergeable; HEAD `15eec5b9216d6a30655f59450e410f0f80bb0343`; stacked on PR #30.
- PR #38 `dagyeom/visual-feedback-v1` — OPEN, mergeable; HEAD `ffbc32c0cc9465a46feb1491f0bb7d5e0d1cd57a`; stacked on PR #36.
- Status: `IN_PROGRESS / CONDITIONAL`.
- Exact next action: resolve #17 against latest main first; then retarget/reconcile stacked PRs in dependency order and verify each before merge. #26 may be reconciled independently because it targets main.

### 4. TASK_03 old Android validation

- Owner: 쭌 + 쭌 AI
- Branch: `task/03-fast-test`
- PR: #2 `[UE] Bridge LifeLensCore into Unreal runtime`
- Last known HEAD: `f8f461a8ec669ba65ad6dd669e6bac45f186d230`
- Status: `FROZEN`
- Android Run `34739283266`: FAILURE; Cook/Package/APK not reached.
- Rule: **do not modify, rerun, revive, or merge this old branch.**
- Exact next action: none. Future Android work uses a new latest-main task/branch.

---

## Jjun observer/runtime integration — completed

### Observer Runtime Bridge v1 — PR #37

- Status: `DONE`
- Merge SHA: `938d0a2798e600929b4ccc755b48bcd39026ac75`
- Validation: LifeLens Preflight PASS; Unreal 5.6 Linux Compile Run `34796067278` PASS including actual UHT/UBT.
- Provides: stable `(WorldSeed, Core CharacterId) -> FGuid`; Needs; 11-axis Emotion + summaries; Physical/Social activity; SocialIntent target; 13D Relationship + derived scores; Memory/Belief counts; basic World observation.

### Authoritative Family Runtime State — PR #39

- Status: `DONE`
- Merge SHA: `612229cc610d2ea6283e080309bdee58ef42d1db`
- Validation: Core Test Run `34796213647` PASS.
- Core `Simulation` owns authoritative `GenealogyBook`, `RomanceBook`, `HouseholdBook`, `PregnancyBook` and exposes family/world observations.
- Autonomous romance→cohabitation→marriage→pregnancy progression inside `Simulation::step()` remains future product work.

### Family + World Observer Bridge — PR #40

- Status: `DONE`
- Merge SHA / product checkpoint: `9261581df3abd5332d92855628fd7d03203748af`
- Validation: Structural Preflight `34796892593` PASS; Unreal 5.6 Linux Compile `34796892609` PASS including actual UHT/UBT.
- Provides: Partner / Parents / Children / Siblings, romance/marriage/cohabitation/pregnancy state; households/couples/romance-stage/pregnancies/life-stage/major-life-history aggregates.
- No `Source/LifeLens/UI/**` or Dagyeom Character presentation files were modified.

### Dagyeom integration blockers — RELEASED

The former six `BLOCKED-BY-JJUN` requirements are now **READY FOR DAGYEOM BINDING**:
1. Relationship 13D + target ID/name — READY.
2. Emotion detailed axes + valence/arousal/intensity — READY.
3. SocialIntent + target — READY.
4. Family summary + marriage/cohabitation/pregnancy — READY.
5. World family/lifecycle aggregates — READY.
6. Blueprint/USTRUCT read-only Core Bridge — READY.

---

## Recently completed Core milestones

Relationship #4, Emotion #6, Memory #7, Belief #8, Social Cognition #9, Social Utility #10, Social Execution #11, Simulation Social Loop #12, Observer Read Model #13, Romance #14, Household #15, Marriage #16, Pregnancy #18, Birth/Genetics #19, Growth #20, Parenting #21, Genealogy, Aging #23, Death #25, LifeHistory #27, LifeHistory Wiring #32, Generation Continuity #33, Observer Read Model v2 #34, Core validation recovery #35, Observer Runtime Bridge #37, Family Runtime State #39, Family/World Bridge #40 are `main` integrated.

---

## Shared-state system

- `docs/STATE_MANAGEMENT.md` — state synchronization protocol.
- `tasks/WORK_STATE.md` — this canonical current-state file.
- `tasks/DAGYEOM_READY_QUEUE.md` — Dagyeom executable queue and dependencies.
- `tasks/TEAM_BOARD.md` — ownership / locks / integration requests / merge order.
- `tasks/HANDOFF_LOG.md` — append-only history.

### Mandatory recovery rule

If a session/tool/chat is interrupted:
1. Do not assume the previous action succeeded or failed.
2. Fetch actual main/branch HEAD.
3. Fetch PR state/head/base.
4. Fetch related CI/Actions state.
5. Compare with this file + Team Board + READY queue.
6. **Update stale state documents first.**
7. Resume code only from the last verified GitHub checkpoint.
