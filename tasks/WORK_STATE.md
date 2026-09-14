# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` + **`docs/CIVILIZATION_PROGRESSION_v1.md`** · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — actual `main` is `b90da9242003fbc0cbc553605b9abc46a17aa044`, PR #52 Civilization Knowledge Transmission v1 is MERGED. PR Core `34814310235` PASS, Preflight `34814310291` PASS, push Core `34814219163` PASS. Next Jjun slice is **Civilization Observer Read DTOs v1**, planned from latest main; branch not yet created at this checkpoint.

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
- Status: `PLANNED / STATE_LOCKED`
- Branch: not created yet at this checkpoint.
- Goal: expose authoritative civilization state to Observer/Unreal without leaking mutable Core internals or inventing strategy-game authority in UI.
- Bounded v1 scope:
  1. Core read DTO for per-resident Inventory summary, personal Knowledge/skill summary and civilization activity summary;
  2. Core world read DTO for ResourceNode/Storage summary and civilization aggregate counts;
  3. discovery/knowledge provenance summary from authoritative `SocialKnowledgeBook` without exposing mutable book internals;
  4. Unreal `USTRUCT` projection and read-only `ULLCoreBridgeSubsystem` getters using stable resident FGuid mapping;
  5. post-load reads rebuilt from restored Core state, never legacy arrays;
  6. Observer-first contract: summary APIs only; Level 0 must not become a resource/tech strategy dashboard;
  7. no UI/Character Presentation file changes in Jjun branch.
- Required gates: Core full suite + deterministic harness + Structural Preflight + actual UE 5.6 UHT/UBT because Unreal Bridge types change.

### 2. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Status: `RECOVERING`
- Existing legacy Observer blockers remain 0.
- Civilization UI binding remains blocked only until the new civilization read DTO/Bridge API is published; this does not block current PR #17 reconciliation work.

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
- Pure Core scope.
- Discovery/Craft creates transmissible technique facts; direct witness/imitation/teaching can spread individual knowledge without global tech unlock.
- Transmission depends on prerequisite/material context, teacher mastery, relationship trust/respect/familiarity and learner traits.
- `SocialKnowledgeBook` is authoritative Simulation state; provenance/transmission paths are persisted in binary snapshot v3; v1/v2 remain readable.
- duplicate/loop amplification blocked; repeated NEW GAME clears previous culture history.
- PR Core `34814310235` PASS incl Configure/Build/37 tests/deterministic harness; Preflight `34814310291` PASS.

### PR #51 — Autonomous Civilization Action Loop v1
- Merge: `55d5211160c8edad32b01177e2b9326a9faa2b78`
- Gather / Store / Experiment / Craft are autonomous Core decisions while urgent survival remains dominant.
- Core Tests `34811869666` PASS; Preflight `34811869612` PASS.

### PR #50 — Civilization Runtime State + Persistence v1
- Merge: `c31c422c305a3a79a9553d37ac86247aa31d1853`
- Character owns civilization state; World owns resources/storage; binary snapshot v2 persisted it before v3 provenance extension.
- Core Tests `34810329867` PASS; Preflight `34810329862` PASS.

### PR #49 — Civilization Foundation v1
- Merge: `36bd1ac81192bc689c1e811553f068f255642508`
- Core `34809360826` PASS; Preflight `34809360739` PASS.

### PR #48 — World Affordance Execution v1
- Merge: `a90ff6a5d870858a9e555ddf1e6e526bb7aa34e1`
- Unreal Run #14 `34808292682` PASS incl actual UE 5.6 UHT+UBT.

### PR #47 / #46
- Witness/Rumor merge `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c`.
- Core-authoritative Physical Action Bridge merge `753df19657ea634ea2fa7c2ac935f6273ce14c10`; UE Run #10 PASS.

## Next sequencing

1. Civilization Observer Read DTOs v1.
2. Unreal natural-resource affordances + held tool/item presentation.
3. Deeper production chains: stable fire, improved stone tools, containers, construction, agriculture, metallurgy.
4. Economy/specialization after production and knowledge flow are real.
5. Android smoke APK once the new survival/civilization runtime is observable.

---

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/board/queue → update stale docs first → resume only from last verified GitHub checkpoint.