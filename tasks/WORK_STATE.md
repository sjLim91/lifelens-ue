# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — actual `main` HEAD `8ca126b8aaad8867d174e7ab801f59adebb371a0` verified. PR #46 and #47 are merged. Jjun next work is now state-locked as **Physical Interaction / Smart Object Execution v1**; branch creation follows this checkpoint.

## Mandatory sync gate

1. 작업 시작/재개 전에 actual `main` HEAD, 대상 branch HEAD, PR state/head/base, Actions를 확인한다.
2. 이 문서 → 역할별 READY queue → `TEAM_BOARD.md` → `HANDOFF_LOG.md`와 대조한다.
3. 다르면 **코드보다 문서를 먼저 갱신**한다.
4. branch 생성, PR 생성, CI 결과, 실패 원인 확정, merge 등 의미 있는 checkpoint마다 즉시 갱신한다.
5. GitHub 실제 상태가 항상 stale 문서보다 우선한다.

---

## Active / unresolved work

### 1. Physical Interaction / Smart Object Execution v1 — Jjun

- Owner: 쭌 + 쭌 AI
- Status: `DOING / STATE_LOCKED`
- Planned branch: `jjun/physical-smart-object-v1` from latest main after this checkpoint.
- Goal: make authoritative Core physical decisions use actual reservable Unreal world objects instead of only moving toward generic action anchors.
- Bounded v1 scope:
  - Eat / Drink / Sleep / Toilet / Hygiene only;
  - typed activity-anchor capability matching;
  - deterministic nearest usable-object selection;
  - exclusive reservation/occupancy per resident;
  - approach/interaction transform separated from object actor origin;
  - reservation release when Core directive changes, resident dies/disappears, target becomes invalid, or interaction ends;
  - no second decision authority in Unreal;
  - preserve `ELLActionIntent` ordinals and SaveGame v2 identity semantics;
  - structural/preflight guards and actual UE 5.6 UHT/UBT verification before merge.
- Allowed scope: Jjun-owned `Source/LifeLens/World/**`, `Source/LifeLens/Simulation/**` only if bridge contract needs a narrow change, `Source/LifeLens/Core/LLTypes.h` only if ordinal-safe metadata is required, and `Tools/validate_bootstrap.py`.
- Forbidden scope: `Source/LifeLens/UI/**`, Dagyeom Character appearance/presentation, `Content/UI/**`, `Content/Characters/**`, frozen PR #2.
- Exact next action: create branch, inspect `LLActivityAnchor` + current #46 `LLWorldDirector` path, implement the smallest reservation/interaction-target layer without reintroducing legacy `ChooseAction()` authority.

### 2. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Status: `RECOVERING`
- Former six `BLOCKED-BY-JJUN` Observer requirements remain resolved.
- Main now additionally includes:
  - #46 Core-authoritative typed physical/social action directives and WorldDirector authority handoff;
  - #47 pure-Core witness/rumor/social-knowledge domain layer.
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

## Latest completed milestone — Core-authoritative physical action bridge

### PR #46 `[UE] Drive physical world actions from authoritative Core decisions`

- Status: `DONE / MERGED`
- Feature HEAD: `31c00cb0d751bb33825df31c7e758e60ff54402c`
- Merge SHA: `753df19657ea634ea2fa7c2ac935f6273ce14c10`
- Implemented:
  - `ResidentObservation` exposes typed `physicalGoal` / `socialIntent`;
  - Unreal `FLLCoreActionDirective` maps Core physical/social intent + stable target ResidentId;
  - `ELLActionIntent` persisted/Blueprint ordinals preserved; `Drink` appended as ordinal 7;
  - `ALLWorldDirector` no longer calls legacy `DecisionComponent->ChooseAction()` for covered life actions;
  - WorldDirector no longer applies projection-only action/social outcomes as a second simulation authority;
  - Eat / Drink / Sleep / Toilet / Hygiene and Approach / Avoid / Repair / Comfort now mirror authoritative Core directives.
- Validation:
  - Core Tests `34805882778` PASS including Configure / Build / Test / deterministic harness.
  - Structural Preflight `34805882776` PASS.
  - Unreal Linux Compile Run `34805882789` / **Run #10** PASS including actual UE 5.6 image verification + UHT + UBT.
  - Changed files: exactly 9 Jjun-owned Core/Simulation/World/validator files; Dagyeom UI/Character presentation/Content untouched.
- Follow-up caveat: targeted PIE/real runtime behavior verification is still needed when a runnable scene/build gate is available; compile success is not a substitute for visual interaction QA.

## Parallel milestone completed — Witness / Rumor / Social Knowledge v1

### PR #47 `[CORE] Add witness rumor social knowledge v1`

- Status: `DONE / MERGED`
- Feature HEAD: `8c03b349d8489a847b4e4fb7e22dc974e91be8cf`
- Merge SHA: `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c`
- Scope: exactly 3 pure-Core files (`CMakeLists.txt`, `WitnessRumor.h`, `test_witness_rumor.cpp`); no Unreal-side files.
- Implemented:
  - `SocialFact`, `KnowledgeReceipt`, `SocialStatement`, `SocialKnowledgeBook`;
  - direct witness vs heard-statement provenance;
  - deterministic retelling attenuation/distortion without consuming simulation RNG;
  - trust-sensitive acceptance;
  - duplicate amplification prevention and transmission-loop suppression;
  - existing `MemoryState` / `BeliefState` integration with positive/negative evidence preservation.
- Validation:
  - Core Tests `34806559374` PASS including Configure / Build / Test / deterministic harness.
  - Structural Preflight `34806559379` PASS.
  - Unreal UHT/UBT intentionally not required because the PR is pure Core-only.
- Follow-up: live `Simulation` event/witness wiring remains separate work; #47 establishes the tested domain layer only.

## Previous completed milestones

- PR #45 Unreal SaveGame Adapter v1 — merge `3c646ba331b8199a295fd6f2e9cac1235d844679`; Core `34803226434`, Preflight `34803226458`, Unreal `34803226433` PASS.
- PR #44 Full Core Save/Load v1 — merge `2b3f9882703ed73cb8318ae262f26bebb995c209`; Core `34802611336` PASS incl deterministic harness; Preflight `34802611299` PASS.
- PR #43 Autonomous Family Progression v1 — merge `179e3a65aaa6ff8d2243117c7aebfd760812c73d`; Core `34801572846`; Preflight `34801572858` PASS.
- PR #42 Production NEW GAME Unreal Runtime Integration — merge `5c6f2c071ca00bcd4b26d6e002bf5c618d02ff1e`; Preflight `34799244015`; Unreal UHT/UBT `34799244011` PASS.
- PR #41 Production NEW GAME Core v1 — merge `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68`; Core `34798427843`; Preflight `34798427848` PASS.
- Earlier bridge/family milestones: #37 `938d0a27...`, #39 `612229cc...`, #40 `9261581d...`.

## Next Jjun candidates after current task

1. **Witness / Rumor Runtime Wiring v1** — connect #47 domain types to live social/lifecycle events so only actual witnesses learn directly and rumors propagate through social interactions.
2. Android FAST smoke APK / real-device verification after Physical Interaction v1 is merged and observable.

---

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/board/queue → update stale docs first → resume only from last verified GitHub checkpoint.
