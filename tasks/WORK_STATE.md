# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` + **`docs/CIVILIZATION_PROGRESSION_v1.md`** · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — actual `main` HEAD `c31c422c305a3a79a9553d37ac86247aa31d1853` verified. PR #50 Civilization Runtime State + Persistence v1 is MERGED and validated. Next Jjun slice is state-locked as **Autonomous Civilization Action Loop v1**.

## Mandatory sync gate

1. 작업 시작/재개 전에 actual `main` HEAD, 대상 branch HEAD, PR state/head/base, Actions를 확인한다.
2. 이 문서 → 역할별 READY queue → `TEAM_BOARD.md` → `HANDOFF_LOG.md`와 대조한다.
3. 다르면 **코드보다 문서를 먼저 갱신**한다.
4. branch 생성, PR 생성, CI 결과, 실패 원인 확정, merge 등 의미 있는 checkpoint마다 즉시 갱신한다.
5. GitHub 실제 상태가 항상 stale 문서보다 우선한다.

---

## Canonical product direction — Autonomous Civilization

`docs/CIVILIZATION_PROGRESSION_v1.md` is canonical alongside the original master spec.

LifeLens long-term flow:

**4명의 초기 인간 → 자연 자원 채집 → 저장/소유 → 실험/실패/발견 → 개인 지식 → 제작/도구 → 목격·모방·교육을 통한 전파 → 전문화/교환 → 세대 누적 → emergent civilization.**

Rules:
- no globally pre-unlocked recipe list;
- no forced `Stone Age → Bronze Age → Iron Age` button tech tree;
- technology emerges from needs, materials, environment, experiment, knowledge and skill;
- knowledge begins personal and may be lost if not transmitted;
- existing Memory/Belief/Witness/Family/Generation systems become part of knowledge transmission;
- modern-looking runtime bootstrap anchors are development affordances only.

---

## Active / unresolved work

### 1. Autonomous Civilization Action Loop v1 — Jjun

- Owner: 쭌 + 쭌 AI
- Status: `DOING / STATE_LOCKED`
- Branch: create from actual latest main after this state checkpoint.
- Goal: make residents autonomously choose and execute the first civilization loop using the authoritative state merged in #50.
- Bounded v1 scope:
  1. CivilizationIntent / decision result separate from physical/social intent;
  2. utility based on Needs, curiosity, inventory, known techniques, available resources and storage pressure;
  3. autonomous Gather from finite ResourceNode;
  4. autonomous Store into shared StorageSite;
  5. autonomous Experiment when a resident has usable material and lacks a reproducible technique;
  6. autonomous Craft when a known reproducible technique and required inputs exist;
  7. failures consume resources where the domain contract says they should;
  8. successful discovery remains personal, never global unlock;
  9. deterministic event/log output suitable for later Observer read DTOs;
  10. deterministic Core tests proving multi-resident divergence and repeated progression.
- Authority rule: Core decides WHAT. This slice is pure Core decision/execution first; Unreal resource navigation/presentation comes later.
- Explicitly out of scope: teaching/imitation, agriculture/metallurgy chains, economy/professions, Unreal resource actors, UI fields.
- Required gates: Core full suite + deterministic harness + Preflight.

### 2. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Status: `RECOVERING`
- Existing Observer blockers remain 0.
- Civilization fields are not UI-ready until Jjun publishes read DTOs.
- Keep Level 0 clean; Inventory/Knowledge/Discovery belongs in deeper resident/world views.

### 3. Dagyeom stacked UI / presentation chain

- #26 `dagyeom/ui-foundation-v1` — reconcile latest main independently.
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

## Latest completed milestones

### PR #50 — Civilization Runtime State + Persistence v1
- Merge: `c31c422c305a3a79a9553d37ac86247aa31d1853`
- Feature head: `543ee006a48d8a97d7cf37de65f03c74c1b60e3e`
- Pure Core 11-file scope; no Dagyeom UI/Character/Unreal World edits.
- Character now owns authoritative `IndividualCivilizationState`; World owns authoritative ResourceNode/StorageSite state.
- Natural-resource starting world includes Stone / Flint / Wood / Fiber / Clay / Water / PlantFood.
- Binary snapshot format v2 persists resource depletion, inventory, personal knowledge/confidence/practice and storage.
- Legacy binary v1 decode remains supported with deterministic civilization migration.
- Core Tests `34810329867` PASS: Configure / Build / Test / deterministic harness.
- Structural Preflight `34810329862` PASS.

### PR #49 — Civilization Foundation v1
- Merge: `36bd1ac81192bc689c1e811553f068f255642508`
- Feature head: `ac8e0868f819a49d8b2c0aec947978258da56028`
- Material/resource + inventory/storage + personal knowledge + deterministic experiment/discovery + craft domain.
- Core Tests `34809360826` PASS incl deterministic harness; Preflight `34809360739` PASS.

### PR #48 — World Affordance / reservable physical execution v1
- Merge: `a90ff6a5d870858a9e555ddf1e6e526bb7aa34e1`
- Preflight `34808292399` PASS; Unreal Linux Compile Run #14 `34808292682` PASS incl actual UE 5.6 UHT + UBT.
- Generic reservation/use infrastructure for future resources, storage, crafting stations, tools, machines and furniture.

### PR #47 — Witness / Rumor / Social Knowledge v1
- Merge: `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c`
- Core `34806559374` PASS; Preflight `34806559379` PASS.

### PR #46 — Core-authoritative physical action bridge
- Merge: `753df19657ea634ea2fa7c2ac935f6273ce14c10`
- Core `34805882778` PASS; Preflight `34805882776` PASS; Unreal Run #10 `34805882789` PASS.

### Previous
- #45 SaveGame v2 `3c646ba331b8199a295fd6f2e9cac1235d844679`
- #44 Full Core Save/Load `2b3f9882703ed73cb8318ae262f26bebb995c209`
- #43 Autonomous Family Progression `179e3a65aaa6ff8d2243117c7aebfd760812c73d`
- #42 Production New Game Runtime `5c6f2c071ca00bcd4b26d6e002bf5c618d02ff1e`
- #41 Production New Game Core `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68`

## Next sequencing

1. Autonomous Civilization Action Loop v1.
2. Knowledge transmission: witness / imitation / teaching + observer events.
3. Civilization Observer read DTOs.
4. Unreal natural-resource affordances + held tool/item presentation.
5. Deeper production chains: fire, improved stone tools, containers, construction, agriculture, metallurgy.
6. Android smoke APK once the new survival/civilization runtime slice is observable.

---

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/board/queue → update stale docs first → resume only from last verified GitHub checkpoint.
