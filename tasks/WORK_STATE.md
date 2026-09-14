# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — PR #46 Core Decision → Unreal Physical Action Bridge v1 remains at head `31c00cb0d751bb33825df31c7e758e60ff54402c` with Core + Preflight PASS and external Unreal Run #10 still the remaining gate. In parallel, pure-Core Witness / Rumor / Social Knowledge v1 is now open as PR #47 at head `8c03b349d8489a847b4e4fb7e22dc974e91be8cf`; its Core/Preflight checks are running and it does not overlap PR #46 files.

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
- Branch: `jjun/core-physical-action-bridge-v1`
- PR: #46 `[UE] Drive physical world actions from authoritative Core decisions`
- Head: `31c00cb0d751bb33825df31c7e758e60ff54402c`
- Status: `WAITING_UNREAL_COMPILE`
- Implemented:
  - pure Core `ResidentObservation` exposes typed `physicalGoal` / `socialIntent` rather than requiring label parsing;
  - Core observer test locks typed action authority;
  - Unreal `FLLCoreActionDirective` carries physical/social typed intent + stable target ResidentId;
  - `ULLCoreBridgeSubsystem::GetResidentActionDirective()` maps authoritative Core runtime state into Unreal;
  - `ELLActionIntent` persisted/Blueprint ordinals preserved: Idle=0, Eat=1, Sleep=2, Socialize=3, Hygiene=4, Toilet=5, HaveFun=6; appended Drink resolves to 7;
  - `ALLWorldDirector` no longer calls legacy `DecisionComponent->ChooseAction()` for life actions;
  - WorldDirector no longer calls `ApplyActionOutcome()` / `ApplySocialInteraction()` as a second state authority;
  - physical Eat/Drink/Sleep/Toilet/Hygiene and social Approach/Avoid/Repair/Comfort are presented from Core directives;
  - social Avoid moves away; other social intents approach/face the Core-selected target;
  - Structural Preflight prevents reintroduction of competing WorldDirector decision authority.
- Scope: Jjun-owned Core/Simulation/World/validator only; no Dagyeom UI/Character presentation/Content edits.
- Latest-head validation:
  - Core Tests `34805882778` PASS including Configure / Build / Test / deterministic harness.
  - Structural Preflight `34805882776` PASS.
  - Unreal Linux Compile `34805882789` IN PROGRESS; actual UHT/UBT pending.
- Superseded validation history:
  - Preflight `34805764910` failed only because its validator expected literal `Drink,` while the compatibility-safe enum used explicit assignment; corrected in head `31c00cb0...` without changing ordinal semantics.
  - Superseded Unreal Runs `34805435883` and `34805764898` were automatically cancelled by workflow concurrency before UHT/UBT after the PR head moved.
- Required validation: actual UE 5.6 Linux UHT/UBT, then targeted runtime/PIE where available.
- Exact next action: when user reports Run #10 complete, verify actual result; on PASS, verify PR scope/head/mergeability and merge.

### 2. Witness / Rumor / Social Knowledge v1 — Jjun (parallel Core-only)

- Owner: 쭌 + 쭌 AI
- Branch: `jjun/witness-rumor-core-v1`
- PR: #47 `[CORE] Add witness rumor social knowledge v1`
- Head: `8c03b349d8489a847b4e4fb7e22dc974e91be8cf`
- Status: `WAITING_CORE_CI`
- Goal: establish the first non-omniscient social-information pipeline required by spec sections 29–30 and 51–52: event witness → personal memory/evidence → statement/rumor transfer → receiver belief with confidence/source provenance.
- Implemented in the bounded pure-Core slice:
  - `SocialFact`, `KnowledgeReceipt`, `SocialStatement`, `SocialKnowledgeBook`;
  - direct-witness provenance into existing `MemoryState` / `BeliefState`;
  - heard-statement provenance with immediate speaker + original witness + transmission path;
  - deterministic retelling confidence attenuation/distortion without consuming simulation RNG;
  - trust-sensitive receiver acceptance;
  - duplicate receipt suppression and provenance-path loop suppression;
  - positive/negative evidence stance preservation;
  - tests for direct witness, one-hop/multi-hop rumor, confidence loss, duplicate amplification prevention, loop suppression, low-trust rejection, negative belief stance and deterministic replay.
- Changed scope: exactly 3 Core files at PR creation (`WitnessRumor.h`, `test_witness_rumor.cpp`, Core `CMakeLists.txt`); no Unreal-side files.
- CI at last check:
  - Core Tests Run `34806559374` IN PROGRESS.
  - Structural Preflight Run `34806559379` IN PROGRESS.
  - Unreal UHT/UBT intentionally not triggered/required for this pure-Core slice.
- Integration into live `Simulation.cpp` is intentionally deferred until PR #46 is merged/reconciled.
- Exact next action: inspect PR #47 Core/Preflight results; fix only the first root cause if needed; after PASS, verify exact changed-file scope and merge independently of #46.

### 3. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Status: `RECOVERING`
- Former six `BLOCKED-BY-JJUN` Observer requirements remain resolved.
- Main now includes #42 Core-authoritative founders, #43 autonomous family progression/newborn population growth, #44 full Core snapshot contract, and #45 actual SaveGame v2 snapshot persistence/restore.
- Exact next action: reconcile actual latest main, bind current Core Observer Bridge, address Dagyeom-owned UI review findings, verify UHT/UBT + PIE.

### 4. Dagyeom stacked UI / presentation chain

- #26 `dagyeom/ui-foundation-v1` — independent main-target PR; reconcile latest main.
- #29 `dagyeom/character-presentation-v1` — stacked on #17.
- #30 `dagyeom/observer-ux-polish-v1` — stacked on #17.
- #36 `dagyeom/mobile-touch-v1` — stacked on #30.
- #38 `dagyeom/visual-feedback-v1` — stacked on #36.
- Jjun does not modify/flatten these branches.

### 5. Old Android validation

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
- Validation: Core `34803226434` PASS; Preflight `34803226458` PASS; Unreal `34803226433` PASS including UE 5.6 UHT/UBT.

## Previous completed milestone — Full Core Save/Load v1

- PR #44 merge `2b3f9882703ed73cb8318ae262f26bebb995c209`; Core `34802611336` PASS incl deterministic harness; Preflight `34802611299` PASS.

## Earlier completed milestones

- PR #43 Autonomous Family Progression v1 — merge `179e3a65aaa6ff8d2243117c7aebfd760812c73d`; Core `34801572846` PASS; Preflight `34801572858` PASS.
- PR #42 Production NEW GAME Unreal Runtime Integration — merge `5c6f2c071ca00bcd4b26d6e002bf5c618d02ff1e`; Preflight `34799244015`; Unreal UHT/UBT `34799244011` PASS.
- PR #41 Production NEW GAME Core v1 — merge `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68`; Core `34798427843` PASS; Preflight `34798427848` PASS.
- Earlier bridge/family milestones: #37 `938d0a27...`, #39 `612229cc...`, #40 `9261581d...`.

---

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/board/queue → update stale docs first → resume only from last verified GitHub checkpoint.
