# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` + **`docs/CIVILIZATION_PROGRESSION_v1.md`** · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — actual `main` includes PR #52 merge `b90da9242003fbc0cbc553605b9abc46a17aa044` plus current state reconciliation commits. **Civilization Observer Read DTOs v1** is open as PR #53 on `jjun/civilization-observer-read-v1`, head `c7753047a28ea3b1dae75c4e6abae7e1f289f962`. Final push Core Run `34819660707` PASS (38/38 + deterministic harness). PR Core `34819825563` PASS and Preflight `34819825627` PASS. Unreal Linux Compile Run #15 `34819825591` FAILED at final link after UHT succeeded: `LLCoreCompileUnit.cpp` omitted `CivilizationKnowledgeTransmission.cpp`, leaving `processCivilizationKnowledgeEvent(...)` and `advanceCivilizationKnowledgeTeaching()` undefined. Fix/revalidation is the only active Jjun task before merge.

## Mandatory sync gate

1. 작업 시작/재개 전에 actual `main` HEAD, 대상 branch HEAD, PR state/head/base, Actions를 확인한다.
2. 이 문서 → 역할별 READY queue → `TEAM_BOARD.md` → `HANDOFF_LOG.md`와 대조한다.
3. 다르면 **코드보다 문서를 먼저 갱신**한다.
4. branch 생성, PR 생성, CI 결과, 실패 원인 확정, merge 등 의미 있는 checkpoint마다 즉시 갱신한다.
5. GitHub 실제 상태가 항상 stale 문서보다 우선한다.

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
- Status: `REVIEW / CI_FIXING`
- Branch/PR: `jjun/civilization-observer-read-v1`, PR #53
- Head before fix: `c7753047a28ea3b1dae75c4e6abae7e1f289f962`
- Validation:
  - final push Core `34819660707` PASS 38/38 + deterministic harness;
  - PR Core `34819825563` PASS;
  - Preflight `34819825627` PASS;
  - UE Linux Compile Run #15 `34819825591` FAILED at **link** after UHT passed.
- Run #15 root cause:
  - `Source/LifeLens/Simulation/LLCoreCompileUnit.cpp` includes Core implementation `.cpp` files manually for Unreal;
  - it did not include `Source/LifeLensCore/src/CivilizationKnowledgeTransmission.cpp`;
  - therefore linker could not resolve `Simulation::processCivilizationKnowledgeEvent(...)` and `Simulation::advanceCivilizationKnowledgeTeaching()`;
  - this is not a `dev-slim-5.6.0`, Android SDK, UHT, or DTO design failure.
- Exact next action: add the missing compile-unit include, add structural validator coverage so it cannot regress, then run one latest-head UE 5.6 UHT/UBT validation.
- Goal: expose authoritative civilization state to Observer/Unreal without leaking mutable Core internals or inventing strategy-game authority in UI.
- Implemented v1 scope:
  1. separate Core `ResidentCivilizationObservation` and `CivilizationWorldObservation` read models;
  2. inventory/resource/storage deterministic summaries;
  3. personal technique level/confidence/practice and gathering/crafting/learning skills;
  4. provenance summary as SelfDiscovery / DirectWitness / Teaching with origin/immediate-source/fact/hop/minute;
  5. recent actual discovery summaries while repeated craft demonstrations are excluded from major discovery list;
  6. separate Unreal `LLCivilizationReadTypes.h` USTRUCT/UENUM layer;
  7. read-only `GetResidentCivilizationObservation` / `GetCivilizationWorldObservation` Bridge getters using stable FGuid identity;
  8. no second authority/cache; post-load reads rebuilt from current Core;
  9. no UI/Character Presentation files changed.
- Intermediate Core Run `34819279308` failure was test-only: fixture recorded knowledge events in future minutes; Snapshot validator correctly rejected it. Fixed fixture; final push run passes.
- Required merge gates: PR Core full suite + deterministic harness + Structural Preflight + actual UE 5.6 UHT/UBT.

### 2. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Status: `RECOVERING`
- Existing legacy Observer blockers remain 0.
- Civilization UI binding becomes READY only after PR #53 merges; current PR #17 reconciliation work can continue independently.

### 3. Dagyeom stacked UI / presentation chain

- #26 UI foundation — reconcile latest main independently.
- #29 Character Presentation — stacked on #17.
- #30 Observer UX Polish — stacked on #17.
- #36 Mobile Touch — stacked on #30.
- #38 Visual Feedback — stacked on #36.
- Jjun does not modify/flatten these branches.

### 4. Old Android validation

- `task/03-fast-test`, PR #2: **FROZEN**.
- Run `34739283266`: FAILURE; Cook/Package/APK not reached.
- Do not modify, rerun, revive, or merge.

---

## Latest completed milestones

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

1. Fix/revalidate/merge Civilization Observer Read DTOs v1 PR #53.
2. **Integration Sprint:** pause new large Jjun Core slices while Dagyeom PR #17 catches up to latest main and binds the published Observer/Bridge APIs.
3. Reconcile Dagyeom PR #26 independently, then stacked #29/#30 → #36 → #38.
4. Verify one integrated runtime slice with Core civilization + Observer UI + character presentation.
5. Android smoke APK after the integrated slice is observable.
6. Resume deeper civilization production chains (fire, improved stone tools, containers, construction, agriculture, metallurgy) after the integration checkpoint.

---

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/board/queue → update stale docs first → resume only from last verified GitHub checkpoint.