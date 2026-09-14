# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` + **`docs/CIVILIZATION_PROGRESSION_v1.md`** · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — actual `main` HEAD `ffbe136d237d15449ed20588ad641c4d119fce77` verified after adding canonical Civilization Progression v1. PR #48 Physical Interaction / Smart Object Execution v1 remains OPEN on head `505e356d277cb0896109f6d4cdd75bcf06cdab54`; latest Structural Preflight PASS and Unreal Linux Compile Run #14 is externally in progress. In parallel, the next Jjun Core slice is now state-locked as **Civilization Foundation v1**.

## Mandatory sync gate

1. 작업 시작/재개 전에 actual `main` HEAD, 대상 branch HEAD, PR state/head/base, Actions를 확인한다.
2. 이 문서 → 역할별 READY queue → `TEAM_BOARD.md` → `HANDOFF_LOG.md`와 대조한다.
3. 다르면 **코드보다 문서를 먼저 갱신**한다.
4. branch 생성, PR 생성, CI 결과, 실패 원인 확정, merge 등 의미 있는 checkpoint마다 즉시 갱신한다.
5. GitHub 실제 상태가 항상 stale 문서보다 우선한다.

---

## Product-direction expansion — Autonomous Civilization

`docs/CIVILIZATION_PROGRESSION_v1.md` is now a canonical product-direction extension.

LifeLens is not limited to autonomous modern household living. The long-term target is:

**4명의 초기 인간 → 자연 자원 채집 → 저장/소유 → 실험/실패/발견 → 개인 지식 → 제작/도구 → 목격·모방·교육을 통한 지식 전파 → 전문화/교환 → 세대 누적 → emergent civilization.**

Key rules:
- no global pre-unlocked recipe list;
- no forced `Stone Age → Bronze Age` button-style tech tree;
- technology emerges from needs, environment, resources, material properties, experiment, knowledge and skill;
- knowledge is personal first and may spread through witness/imitation/teaching/rumor;
- progress can be lost when knowledge holders die without transmission;
- current modern-looking bootstrap ActivityAnchors are **development affordances only**, not the canonical starting civilization.

---

## Active / unresolved work

### 1. Physical Interaction / Smart Object Execution v1 — Jjun

- Owner: 쭌 + 쭌 AI
- Branch: `jjun/physical-smart-object-v1`
- PR: #48 `[UE] Add reservable physical smart-object execution v1`
- Head: `505e356d277cb0896109f6d4cdd75bcf06cdab54`
- Status: `REVIEW / EXTERNAL_UNREAL_COMPILE`
- Implemented:
  - `ALLActivityAnchor` lifecycle `Available → Reserved → InUse`;
  - stable ResidentId exclusive claim/release;
  - use transform separated from actor origin;
  - multi-capability anchors;
  - deterministic nearest usable selection;
  - reservation held across approach/use and released on state invalidation;
  - covered intents: Eat / Drink / Sleep / Toilet / Hygiene;
  - generic coordinate fallback removed;
  - bootstrap capacity mirrors current Core test world;
  - Unreal compile workflow now includes `Source/LifeLens/World/**`.
- Scope: 4 Jjun-owned World files + `.github/workflows/unreal-linux-compile.yml`; no Dagyeom UI/Character/Content edits.
- Validation:
  - Structural Preflight `34808292399` PASS;
  - Unreal Linux Compile `34808292682` / Run #14 currently external/in progress;
  - Run #11–#13 are superseded.
- Reframing: this is **World Affordance execution infrastructure**, not a commitment to a modern-household-only game. The same reservation/use layer will later serve resource nodes, fires, work surfaces, storage, crafting stations, tools, machines and furniture.

### 2. Civilization Foundation v1 — Jjun Core

- Owner: 쭌 + 쭌 AI
- Status: `DOING / STATE_LOCKED`
- Planned branch: `jjun/civilization-foundation-v1` from latest main after this checkpoint.
- Goal: establish the minimum pure-Core material economy and individual-knowledge substrate required for autonomous civilization progression.
- Bounded v1 scope:
  1. Resource/Material definitions with useful physical-property metadata;
  2. finite ResourceNode quantities;
  3. personal Inventory stacks;
  4. Gather / Carry / Store primitives;
  5. Tool/Item representation;
  6. per-character Knowledge state separate from world truth;
  7. deterministic Experiment/Discovery primitive;
  8. simple Crafting gated by resources + known technique;
  9. hooks compatible with existing Memory/Belief/Witness systems;
  10. deterministic tests and a snapshot/save-load integration plan.
- Explicitly out of v1: agriculture, metallurgy chain, economy, professions, buildings, full teaching system, Unreal visual resource nodes.
- Authority rule: recipes/technology are not globally auto-unlocked; discovery starts personal.
- Scope: `Source/LifeLensCore/**` only unless a narrow validator/CMake registration is required. No #48 World files, no Dagyeom UI/Character files.

### 3. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Status: `RECOVERING`
- Existing `BLOCKED-BY-JJUN` Observer requirements remain 0.
- New product-direction note: do not redesign main HUD into a tech/resource dashboard. Civilization data belongs in selected-resident/world-detail layers; main observer view remains clean.
- Exact next action: reconcile latest main, bind current Core Observer Bridge, address Dagyeom-owned UI review findings, verify UHT/UBT + PIE.

### 4. Dagyeom stacked UI / presentation chain

- #26 `dagyeom/ui-foundation-v1` — reconcile latest main independently.
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

## Latest completed milestones

### PR #46 Core-authoritative physical action bridge
- Merge: `753df19657ea634ea2fa7c2ac935f6273ce14c10`
- Core `34805882778` PASS; Preflight `34805882776` PASS; Unreal Run #10 `34805882789` PASS incl actual UE 5.6 UHT+UBT.

### PR #47 Witness / Rumor / Social Knowledge v1
- Merge: `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c`
- Core `34806559374` PASS incl deterministic harness; Preflight `34806559379` PASS.
- This layer is now also a future knowledge-transmission substrate for discoveries/techniques.

### Previous
- PR #45 Unreal SaveGame Adapter v1 — merge `3c646ba331b8199a295fd6f2e9cac1235d844679`.
- PR #44 Full Core Save/Load v1 — merge `2b3f9882703ed73cb8318ae262f26bebb995c209`.
- PR #43 Autonomous Family Progression v1 — merge `179e3a65aaa6ff8d2243117c7aebfd760812c73d`.
- PR #42 Production NEW GAME Unreal Runtime Integration — merge `5c6f2c071ca00bcd4b26d6e002bf5c618d02ff1e`.
- PR #41 Production NEW GAME Core v1 — merge `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68`.

## Next sequencing

1. Finish/merge #48 only if latest actual UE 5.6 UHT+UBT passes.
2. In parallel implement Civilization Foundation v1 pure Core.
3. Then wire Gather/Inventory/Craft/Discovery into autonomous Simulation decisions.
4. Then expose Resource/Knowledge read DTOs and Unreal world resource affordances.
5. Animation/IK remains important presentation work, but no longer outranks the civilization substrate.
6. Android smoke APK after the new runtime slice is sufficiently observable.

---

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/board/queue → update stale docs first → resume only from last verified GitHub checkpoint.
