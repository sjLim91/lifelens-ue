# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` + **`docs/CIVILIZATION_PROGRESSION_v1.md`** · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — actual `main` includes PR #51 merge `55d5211160c8edad32b01177e2b9326a9faa2b78`. Autonomous Civilization Action Loop v1 is MERGED and validated. Next Jjun slice is state-locked as **Civilization Knowledge Transmission v1** on branch `jjun/civilization-knowledge-transmission-v1`.

## Mandatory sync gate

1. 작업 시작/재개 전에 actual `main` HEAD, 대상 branch HEAD, PR state/head/base, Actions를 확인한다.
2. 이 문서 → 역할별 READY queue → `TEAM_BOARD.md` → `HANDOFF_LOG.md`와 대조한다.
3. 다르면 **코드보다 문서를 먼저 갱신**한다.
4. branch 생성, PR 생성, CI 결과, 실패 원인 확정, merge 등 의미 있는 checkpoint마다 즉시 갱신한다.
5. GitHub 실제 상태가 항상 stale 문서보다 우선한다.

---

## Canonical product direction — Autonomous Civilization

LifeLens long-term flow:

**4명의 초기 인간 → 자연 자원 채집 → 저장/소유 → 실험/실패/발견 → 개인 지식 → 제작/도구 → 목격·모방·교육을 통한 전파 → 전문화/교환 → 세대 누적 → emergent civilization.**

Rules:
- no globally pre-unlocked recipe list;
- no forced era unlock gate;
- technology emerges from needs, resources, experiment, personal knowledge and skill;
- knowledge begins personal, can spread socially, and can be lost;
- current modern-looking bootstrap anchors are development affordances only.

---

## Active / unresolved work

### 1. Civilization Knowledge Transmission v1 — Jjun

- Owner: 쭌 + 쭌 AI
- Status: `DOING / STATE_LOCKED`
- Branch: `jjun/civilization-knowledge-transmission-v1`
- Goal: stop discoveries from remaining isolated forever by connecting personal civilization knowledge to witness / imitation / direct teaching while preserving provenance and imperfect transmission.
- Bounded v1 scope:
  1. convert successful Discovery/Craft events into transmissible technique facts;
  2. direct witnesses can gain Observed/Hypothesized knowledge without becoming instantly Reproducible;
  3. imitation can advance knowledge only when observer has prerequisite knowledge/material context;
  4. deliberate teaching uses relationship trust, teacher mastery, learner curiosity/learning skill and deterministic roll;
  5. source/provenance must distinguish self-discovery, witness/imitation and taught/heard knowledge;
  6. no global shared-tech unlock;
  7. death/non-transmission can still remove unique knowledge from the living population;
  8. deterministic tests for spread, failed teaching, loops/duplicates and personal divergence;
  9. reuse #47 Witness/Rumor/Social Knowledge where appropriate instead of creating a second rumor system.
- Explicitly out of scope: Observer/Unreal read DTOs, UI, schools/professions, economy, writing/books, agriculture/metallurgy chains.
- Required gates: Core full suite + deterministic harness + Preflight.

### 2. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Status: `RECOVERING`
- Existing Observer blockers remain 0.
- Civilization fields are still not UI-ready until read DTO/Bridge exposure is published.

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

### PR #51 — Autonomous Civilization Action Loop v1
- Merge: `55d5211160c8edad32b01177e2b9326a9faa2b78`
- Feature head: `729536ac3f8f2222e68c0d0d1a1726c98ffe19fa`
- Pure Core 6-file scope.
- Adds `CivilizationIntent` Gather / Store / Experiment / Craft as a third utility axis while preserving Physical/Social authority.
- Urgent survival (`max Need >= 0.74`) blocks civilization override.
- Civilization competes on bounded 15-minute slots; other 5-minute windows preserve original Physical/Social competition.
- Gather consumes finite resources; Store transfers surplus; Experiment can fail/consume and personally discover; Craft requires personal Reproducible knowledge.
- Renewable resources regenerate daily; repeated NEW GAME resets resource/storage state.
- Full integration test runs 20,000 simulation minutes and checks autonomous discovery/resource depletion, same-seed canonical bytes, Save/Load continuation, and reset behavior.
- Core Tests `34811869666` PASS: Configure / Build / Test / deterministic harness.
- Preflight `34811869612` PASS.

### PR #50 — Civilization Runtime State + Persistence v1
- Merge: `c31c422c305a3a79a9553d37ac86247aa31d1853`
- Core Tests `34810329867` PASS; Preflight `34810329862` PASS.
- Character owns civilization state; World owns resources/storage; binary snapshot v2 persists it with legacy-v1 migration.

### PR #49 — Civilization Foundation v1
- Merge: `36bd1ac81192bc689c1e811553f068f255642508`
- Core `34809360826` PASS; Preflight `34809360739` PASS.

### PR #48 — World Affordance Execution v1
- Merge: `a90ff6a5d870858a9e555ddf1e6e526bb7aa34e1`
- Unreal Run #14 `34808292682` PASS incl actual UE 5.6 UHT+UBT; Preflight PASS.

### PR #47 — Witness / Rumor / Social Knowledge v1
- Merge: `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c`
- Core/Preflight PASS.

### PR #46 — Core-authoritative physical action bridge
- Merge: `753df19657ea634ea2fa7c2ac935f6273ce14c10`
- Core/Preflight/UE actual compile PASS.

## Next sequencing

1. Civilization Knowledge Transmission v1.
2. Civilization Observer read DTOs + discovery/history summaries.
3. Unreal natural-resource affordances + held tool/item presentation.
4. Deeper production chains: stable fire, improved stone tools, containers, construction, agriculture, metallurgy.
5. Economy/specialization after production and knowledge flow are real.
6. Android smoke APK once the new survival/civilization runtime is observable.

---

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/board/queue → update stale docs first → resume only from last verified GitHub checkpoint.