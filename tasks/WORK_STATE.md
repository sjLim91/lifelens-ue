# LifeLens Canonical Work State

> 이 파일은 **현재 진행 상태의 단일 기준판(canonical live state)**이다. 쭌/다겸/양쪽 AI는 작업 시작 전에 반드시 읽고 실제 GitHub 상태와 대조한다.
>
> 제품 요구사항은 `docs/LIFELENS_SPEC_v1.1.md`, 상태관리 규칙은 `docs/STATE_MANAGEMENT.md`, 역할/잠금은 `tasks/TEAM_BOARD.md`, 변경 이력은 `tasks/HANDOFF_LOG.md`를 따른다.

Last reconciled: 2026-09-14 KST — actual `main` `9261581df3abd5332d92855628fd7d03203748af`; PR #37/#39/#40 merged; Unreal 5.6 Linux UHT/UBT PASS; Dagyeom Core/API blockers released.

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

### 1. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch: `dagyeom/observer-ui-v2`
- PR: #17 `[UI] Observer HUD v2: LEVEL 0 overview, LEVEL 1 quick inspector, LEVEL 2 detail tabs`
- Last verified HEAD: `dc3351ea9025ee33ea70f8ce1026100070c20092`
- Base shown by PR: old `main` snapshot `d1324c766749847c9fc85ea69af60c23c293da31`
- Status: `RECOVERING`
- Current GitHub fact: PR is OPEN, non-draft, currently `mergeable=false`; reviewer is `sjLim91`.
- Important change since PR body was written: all six former `BLOCKED-BY-JJUN` observer read requirements are now available on `main` via PR #37/#39/#40.
- Existing UI review findings remain Dagyeom-owned: selection hint visibility and narrow-panel text overflow among others.
- Exact next action: Dagyeom side first reconciles PR #17 with latest `main` `9261581d...`, then binds the newly available Core Observer Bridge APIs, addresses its UI review items, runs local/CI UHT/UBT + PIE verification, and updates state before merge.
- Handoff safety: `CONDITIONAL` — current branch is older than latest main and must be reconciled before feature edits/merge.

### 2. Dagyeom stacked UI / presentation PR chain

These are active and must not be flattened or modified by Jjun without coordination.

- PR #26 `dagyeom/ui-foundation-v1` — OPEN, mergeable; HEAD `70dfa5ebeabf24b661c9f9fd0bc63e3ad01ac180`; base `main` but behind current main.
- PR #29 `dagyeom/character-presentation-v1` — OPEN, mergeable; HEAD `a4d47b9f69c9270665a8e2613b7863c40bf0f83e`; stacked on PR #17 branch.
- PR #30 `dagyeom/observer-ux-polish-v1` — OPEN, mergeable; HEAD `c420457c3ddbc73ac2ddbfe692dd20ce63edf45e`; stacked on PR #17 branch.
- PR #36 `dagyeom/mobile-touch-v1` — OPEN, mergeable; HEAD `15eec5b9216d6a30655f59450e410f0f80bb0343`; stacked on PR #30.
- PR #38 `dagyeom/visual-feedback-v1` — OPEN, mergeable; HEAD `ffbc32c0cc9465a46feb1491f0bb7d5e0d1cd57a`; stacked on PR #36.
- Status: `IN_PROGRESS / CONDITIONAL`.
- Exact next action: resolve #17 against latest main first; then retarget/reconcile stacked PRs in dependency order and verify each before merge. #26 may be reconciled independently because it targets main.

### 3. TASK_03 old Android validation

- Owner: 쭌 + 쭌 AI
- Branch: `task/03-fast-test`
- PR: #2 `[UE] Bridge LifeLensCore into Unreal runtime`
- Last known HEAD: `f8f461a8ec669ba65ad6dd669e6bac45f186d230`
- Status: `FROZEN`
- Android Run `34739283266`: FAILURE; Cook/Package/APK not reached.
- Rule: **do not modify, rerun, revive, or merge this old branch.**
- Exact next action: none. Future Android work uses a new latest-main task/branch.

### 4. Production NEW GAME / 4-person runtime integration

- Owner: 쭌 + 쭌 AI
- Status: `PLANNED`
- Goal: latest authoritative Core on `main` → production New Game with WorldSeed → exactly 2 male + 2 female adults generated once → stable identity → four-person social simulation.
- Required follow-ons: full Core Save/Load, runtime bridge integration, Android smoke APK after Linux gates remain clean.
- Exact next action: start only after this state reconciliation is complete and Dagyeom unblock handoff is published.

---

## Jjun observer/runtime integration — completed

### Observer Runtime Bridge v1 — PR #37

- Branch: `jjun/observer-runtime-bridge-v1`
- Status: `DONE`
- Merge SHA: `938d0a2798e600929b4ccc755b48bcd39026ac75`
- Final head before merge: `d9be0be6d58bdbd4c9f394cd64c5498c2143d42a`
- Validation: LifeLens Preflight PASS; Unreal 5.6 Linux Compile Run `34796067278` PASS including image verification + actual UHT/UBT compile.
- Provides: stable `(WorldSeed, Core CharacterId) -> FGuid`; Needs; 11-axis Emotion + summaries; Physical/Social activity; SocialIntent target; 13D Relationship + derived scores; Memory/Belief counts; basic World observation.

### Authoritative Family Runtime State — PR #39

- Branch: `jjun/family-runtime-state-v1`
- Status: `DONE`
- Merge SHA: `612229cc610d2ea6283e080309bdee58ef42d1db`
- Validation: Core Test Run `34796213647` PASS — Configure / Build / Test / deterministic harness.
- Core `Simulation` now owns authoritative `GenealogyBook`, `RomanceBook`, `HouseholdBook`, `PregnancyBook` and exposes family/world observations.
- Note: autonomous romance→cohabitation→marriage→pregnancy progression inside `Simulation::step()` remains future product work; ownership/read path is complete.

### Family + World Observer Bridge — PR #40

- Branch: `jjun/family-observer-bridge-v1`
- Status: `DONE`
- Merge SHA / current main checkpoint: `9261581df3abd5332d92855628fd7d03203748af`
- Final head: `816bb0021b45390e96c4497611970484c9c96879`
- Validation:
  - Structural Preflight Run `34796892593` PASS.
  - Unreal 5.6 Linux Compile Run `34796892609` PASS including actual UHT/UBT.
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

No Dagyeom work should continue to describe these six as blocked by Jjun unless a new concrete API gap is discovered.

---

## Recently completed Core milestones

- Relationship Core — DONE, PR #4.
- Emotion Core — DONE, PR #6.
- Memory Core — DONE, PR #7.
- Belief Core — DONE, PR #8.
- Social Cognition — DONE, PR #9.
- Social Utility — DONE, PR #10.
- Social Execution — DONE, PR #11.
- Simulation Social Loop — DONE, PR #12.
- Observer Read Model v1 — DONE, PR #13.
- Romance — DONE, PR #14.
- Household/Cohabitation — DONE, PR #15.
- Marriage — DONE, PR #16.
- Pregnancy — DONE, PR #18.
- Birth/Genetics — DONE, PR #19.
- Growth — DONE, PR #20.
- Parenting — DONE, PR #21.
- Genealogy — DONE, merge `d1324c766749847c9fc85ea69af60c23c293da31`.
- Aging — DONE, PR #23 merge `82b4001058332656b08f13a47728079172b10962`.
- Death — DONE, PR #25 merge `07509a73f235928c16bf9b26f8b13c689b6a030b`.
- LifeHistory — DONE, PR #27 merge `b46869acc62830c432bb5b3693f98535c37f2e0a`.
- LifeHistory Wiring — DONE, PR #32 merge `8454c0c91e1770e2e503d52d5489446beafe6535`.
- Generation Continuity — DONE, PR #33 merge `37bc2e7af7924fdf8a262d086426e15f2c3e2db3`.
- Core validation recovery — DONE, PR #35 merge `31b2263de3f4a9c80650d1139213d0c38acc8058`.
- Observer Read Model v2 — DONE, PR #34 merge `b4403faf138c153bcd83be266cde5026ea53b831`.
- Observer Runtime Bridge / family authoritative state / family-world bridge — DONE, PR #37/#39/#40.

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
