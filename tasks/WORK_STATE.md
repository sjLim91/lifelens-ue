# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` + **`docs/CIVILIZATION_PROGRESSION_v1.md`** · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 통합 지원 규칙: **`docs/INTEGRATION_SPRINT.md`** · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — actual `main` now includes macOS clang shadow hotfix PR #54 merge `3b649b900c44a4e48bb89171b38f5e685e757b14` plus blocker-release state docs. **Civilization Observer Read DTOs v1** remains open as PR #53 on `jjun/civilization-observer-read-v1`, head `e3a9f6611118866a6c79eb300a7b6ab2d5ffaf31`. Latest Core `34821150702` PASS, Preflight `34821150693` PASS, and Unreal Linux Compile Run #16 `34821150704` PASS including actual UE 5.6 UHT + UBT + final link. PR #53 is not currently mergeable only because main advanced through collaboration docs and the isolated #54 hotfix after the feature branch split; feature validation itself is green.

## Mandatory sync gate

1. 작업 시작/재개 전에 actual `main` HEAD, 대상 branch HEAD, PR state/head/base, Actions를 확인한다.
2. 이 문서 → 역할별 READY queue → `TEAM_BOARD.md` → `HANDOFF_LOG.md`와 대조한다.
3. 다르면 **코드보다 문서를 먼저 갱신**한다.
4. branch 생성, PR 생성, CI 결과, 실패 원인 확정, merge 등 의미 있는 checkpoint마다 즉시 갱신한다.
5. GitHub 실제 상태가 항상 stale 문서보다 우선한다.
6. 쭌이 다겸 소유 작업을 돕는 경우 **`docs/INTEGRATION_SPRINT.md`를 반드시 먼저 적용한다.**

---

## Canonical product direction — Autonomous Civilization

**4명의 초기 인간 → 자연 자원 채집 → 저장/소유 → 실험/실패/발견 → 개인 지식 → 제작/도구 → 목격·모방·교육을 통한 전파 → 전문화/교환 → 세대 누적 → emergent civilization.**

Rules:
- no globally pre-unlocked recipe list;
- no forced era unlock gate;
- technology emerges from needs, resources, experiment, personal knowledge and skill;
- knowledge begins personal, can spread socially, and can be lost;
- current modern-looking bootstrap anchors are development affordances only.

---

## Active / unresolved work

### 1. Civilization Observer Read DTOs v1 — Jjun

- Owner: 쭌 + 쭌 AI
- Status: `READY_TO_MERGE / NEEDS_MAIN_RECONCILE`
- Branch/PR: `jjun/civilization-observer-read-v1`, PR #53
- Latest head: `e3a9f6611118866a6c79eb300a7b6ab2d5ffaf31`
- Validation:
  - latest Core `34821150702` PASS including Build / full tests / deterministic harness;
  - latest Preflight `34821150693` PASS including the compile-unit regression guard;
  - UE Linux Compile Run #16 `34821150704` PASS including actual UE 5.6 UHT / UBT / final link.
- Superseded failure: Run #15 `34819825591` failed at **link** after UHT PASS because `Source/LifeLens/Simulation/LLCoreCompileUnit.cpp` omitted `CivilizationKnowledgeTransmission.cpp`; fixed in the current feature head.
- Current merge blocker: main advanced after the PR split through docs/state commits and isolated PR #54 Core warning hotfix. Current main-side changes do not overlap PR #53 feature files except normal repository history divergence.
- Exact next action: reconcile PR #53 with latest main while preserving the validated feature tree and PR #54 hotfix, confirm the resulting diff has no unintended changes, then merge #53. Do not rerun long UE compile unless the reconcile materially changes Unreal/feature code.
- Goal: expose authoritative civilization state to Observer/Unreal without leaking mutable Core internals or inventing strategy-game authority in UI.
- Implemented v1 scope:
  1. separate Core `ResidentCivilizationObservation` and `CivilizationWorldObservation` read models;
  2. inventory/resource/storage deterministic summaries;
  3. personal technique level/confidence/practice and gathering/crafting/learning skills;
  4. provenance summary as SelfDiscovery / DirectWitness / Teaching with origin/immediate-source/fact/hop/minute;
  5. recent actual discovery summaries while repeated Craft demonstrations are excluded from major discovery list;
  6. separate Unreal `LLCivilizationReadTypes.h` USTRUCT/UENUM layer;
  7. read-only `GetResidentCivilizationObservation` / `GetCivilizationWorldObservation` Bridge getters using stable FGuid identity;
  8. no second authority/cache; post-load reads rebuilt from current Core;
  9. no UI/Character Presentation files changed.

### 2. Post-#53 Integration Sprint — Jjun assists Dagyeom

- Status: `PLANNED / START AFTER #53 MERGE`
- Protocol: `docs/INTEGRATION_SPRINT.md`
- Purpose: close the gap between the fast-moving Core/Bridge main and Dagyeom UI/Presentation PR chain before more large Jjun features are added.
- Default support mode: `REVIEW_ONLY`.
- If the user asks Jjun to modify Dagyeom-owned code: register `ASSIST_LOCK` in TEAM_BOARD, create `integration/dagyeom-<scope>-assist` from the exact target branch HEAD, and **never push directly to `dagyeom/*`**.
- Parent-first order:
  1. PR #17 Observer HUD v2 latest-main reconciliation + current Bridge/civilization binding + review fixes;
  2. PR #26 UI Foundation latest-main reconciliation independently where safe;
  3. #29/#30 after #17;
  4. #36 after #30;
  5. #38 after #36.
- Large new Jjun Core feature slices are paused during this integration checkpoint except blocker/API/CI/runtime integration fixes.

### 3. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Status: `RECOVERING / READY_TO_RESUME_R1`
- Existing legacy Observer blockers remain 0.
- macOS clang `-Wshadow -Werror` Core blocker was resolved by PR #54 merge `3b649b900c44a4e48bb89171b38f5e685e757b14`; PR #17 may resume latest-main reconciliation now.
- Civilization UI binding becomes READY only after PR #53 merges; current PR #17 reconciliation work is not blocked by Jjun.
- Jjun assistance must follow `docs/INTEGRATION_SPRINT.md`; no direct writes to Dagyeom branch.

### 4. Dagyeom stacked UI / presentation chain

- #26 UI foundation — reconcile latest main independently.
- #29 Character Presentation — stacked on #17.
- #30 Observer UX Polish — stacked on #17.
- #36 Mobile Touch — stacked on #30.
- #38 Visual Feedback — stacked on #36.
- Do not mass-rebase/force-update this chain. Parent-first reconciliation only.

### 5. Old Android validation

- `task/03-fast-test`, PR #2: **FROZEN**.
- Run `34739283266`: FAILURE; Cook/Package/APK not reached.
- Do not modify, rerun, revive, or merge.

---

## Latest completed milestones

### PR #54 — macOS clang shadow hotfix
- Merge: `3b649b900c44a4e48bb89171b38f5e685e757b14`.
- Change: behavior-neutral range-for variable rename `pregnancy` → `entry` in `ObserverReadModelV2.h` to satisfy macOS clang `-Wshadow -Werror`.
- Preflight `34822441249` PASS.
- Core `34822441296` PASS including Build / tests / deterministic harness.
- Dagyeom PR #17 R1 blocker released and PR #17 comment posted.

### PR #52 — Civilization Knowledge Transmission v1
- Merge: `b90da9242003fbc0cbc553605b9abc46a17aa044`
- Feature head: `8fe9369682834a3fae44bb6605f0d872bb80b971`
- Discovery/Craft facts spread through witness/imitation/teaching without global tech unlock.
- Snapshot binary v3 persists SocialKnowledge provenance; v1/v2 remain readable.
- PR Core `34814310235` PASS incl 37/37 + deterministic harness; Preflight `34814310291` PASS.

### PR #51 — Autonomous Civilization Action Loop v1
- Merge: `55d5211160c8edad32b01177e2b9326a9faa2b78`
- Gather / Store / Experiment / Craft autonomous; Core `34811869666` PASS; Preflight `34811869612` PASS.

### PR #50 / #49 / #48 / #47 / #46
- #50 Civilization Runtime/Persistence merge `c31c422c305a3a79a9553d37ac86247aa31d1853`.
- #49 Civilization Foundation merge `36bd1ac81192bc689c1e811553f068f255642508`.
- #48 World Affordance merge `a90ff6a5d870858a9e555ddf1e6e526bb7aa34e1`; UE Run #14 PASS.
- #47 Witness/Rumor merge `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c`.
- #46 Physical Action Bridge merge `753df19657ea634ea2fa7c2ac935f6273ce14c10`; UE Run #10 PASS.

## Next sequencing

1. Reconcile latest main into PR #53 safely and merge the already-green Civilization Observer Read DTOs v1.
2. Enter **Integration Sprint** under `docs/INTEGRATION_SPRINT.md`; pause new large Jjun Core slices.
3. Dagyeom resumes PR #17 R1 from latest main; Jjun support is REVIEW_ONLY by default or explicit ASSIST_LOCK + `integration/*-assist` for code edits.
4. Reconcile PR #26 independently where file locks do not overlap; then parent-first #29/#30 → #36 → #38.
5. Verify one integrated runtime slice with Core civilization + Observer UI + Character Presentation.
6. Android smoke APK after the integrated slice is observable.
7. Resume deeper civilization production chains only after the integration checkpoint.

---

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/board/queue → update stale docs first → resume only from last verified GitHub checkpoint.