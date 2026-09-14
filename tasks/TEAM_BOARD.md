# LifeLens Team Board

이 파일은 쭌(sjLim91), 다겸(STILLofficial), 양쪽 AI의 작업 잠금/분배 보드다.

## 최우선 규칙

**기능 작업보다 상태 동기화가 먼저다.** actual main/branch/PR/Actions를 `WORK_STATE.md` → 역할별 READY queue → 이 보드 → `HANDOFF_LOG.md`와 대조하고, 다르면 코드 전에 문서를 갱신한다.

제품 방향 기준은 `docs/LIFELENS_SPEC_v1.1.md`와 **`docs/CIVILIZATION_PROGRESSION_v1.md`**를 함께 따른다.

상태: `TODO` / `DOING` / `REVIEW` / `DONE` / `BLOCKED` / `FROZEN`

## Active Work

| 담당 | 브랜치 / PR | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 쭌 AI | `jjun/physical-smart-object-v1`, PR #48 | Physical Interaction / World Affordance Execution v1 | Jjun-owned World + narrow Unreal compile workflow trigger | REVIEW / EXTERNAL_UNREAL_COMPILE — head `505e356d277cb0896109f6d4cdd75bcf06cdab54`; Preflight PASS; Run #14 in progress |
| 쭌 + 쭌 AI | `jjun/civilization-foundation-v1` (creation next) | Civilization Foundation v1 | `Source/LifeLensCore/**` | DOING / STATE_LOCKED — resources, inventory, knowledge, experiment, crafting substrate |
| 쭌 + 쭌 AI | `task/03-fast-test`, PR #2 | old Android validation | Bridge/build | FROZEN — Run `34739283266` 재실행/수정/병합 금지 |
| 다겸 + 다겸 AI | `dagyeom/observer-ui-v2`, PR #17 | Observer HUD v2 + Core Observer Bridge binding | `Source/LifeLens/UI/**` | REVIEW / RECOVERING — latest main reconcile 필요 |
| 다겸 + 다겸 AI | `dagyeom/ui-foundation-v1`, PR #26 | Android landscape UI foundation | UI foundation | REVIEW |
| 다겸 + 다겸 AI | `dagyeom/character-presentation-v1`, PR #29 | Character Presentation v1 | Character presentation | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/observer-ux-polish-v1`, PR #30 | Observer UX Polish | `Source/LifeLens/UI/**` | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/mobile-touch-v1`, PR #36 | Mobile Touch v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #30 |
| 다겸 + 다겸 AI | `dagyeom/visual-feedback-v1`, PR #38 | Visual Feedback v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #36 |

## Canonical direction — autonomous civilization progression

LifeLens의 최종 방향은 현대 가구를 자율적으로 사용하는 NPC 시뮬레이터에 한정되지 않는다.

핵심 흐름:

`Need / Curiosity → Observe → Gather → Carry/Store → Experiment → Discovery → Personal Knowledge → Craft/Build → Teach/Imitate → Shared Culture → Specialization/Exchange → Generational Civilization`

Rules:
- recipe/tech는 시작부터 전역 unlock되지 않는다.
- 기술은 개인 지식에서 시작하며 다른 캐릭터는 목격·모방·설명·교육을 통해 배운다.
- 석기/청동기/철기 같은 시대는 고정 unlock gate가 아니라 실제 재료/기술 상태를 요약하는 관찰 레이블이다.
- 지식은 죽음/단절로 소실될 수도 있다.
- 기존 Memory / Belief / Witness-Rumor / Family / Generation 시스템을 지식 전파와 세대 누적에 재사용한다.
- Observer HUD는 계속 단순하게 유지한다. 자원/기술 상세는 resident/world detail 계층으로 간다.

## Current Jjun lock A — PR #48 World Affordance Execution v1

- Branch/head: `jjun/physical-smart-object-v1` / `505e356d277cb0896109f6d4cdd75bcf06cdab54`.
- Core remains WHAT authority; Unreal chooses WHERE/HOW.
- Eat / Drink / Sleep / Toilet / Hygiene를 현재 검증 slice로 사용한다.
- Reservation / occupancy / use-transform / multi-capability 구조는 향후 resource node, harvest point, fire, work surface, storage, crafting station, tool, machine, furniture에 공통 사용한다.
- 현재 modern-looking bootstrap anchors are development-only affordances, **not canonical starting-world content**.
- Structural Preflight `34808292399` PASS.
- Unreal Linux Compile `34808292682` / Run #14 external/in progress.
- Merge only after actual UE 5.6 UHT+UBT PASS.

## Current Jjun lock B — Civilization Foundation v1

- Planned branch: `jjun/civilization-foundation-v1` from actual latest main.
- Pure-Core first to avoid overlap with #48.
- v1 bounded scope:
  - Material/Resource metadata;
  - finite ResourceNode;
  - Inventory/Item/Tool primitives;
  - Gather/Carry/Store operations;
  - personal Knowledge state;
  - deterministic Experiment/Discovery;
  - simple Crafting gated by known technique + resources;
  - Memory/Belief/Witness-compatible event hooks;
  - deterministic Core tests.
- Out of scope: full agriculture/metallurgy/economy/professions/buildings/UI/Unreal resource visuals.

## Latest completed Jjun work

### PR #46 — Core Decision → Unreal Physical Action Bridge v1
- DONE / MERGED `753df19657ea634ea2fa7c2ac935f6273ce14c10`
- Core `34805882778` PASS; Preflight `34805882776` PASS; Unreal Run #10 `34805882789` PASS.

### PR #47 — Witness / Rumor / Social Knowledge v1
- DONE / MERGED `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c`
- Core `34806559374` PASS; Preflight `34806559379` PASS.
- This becomes a future discovery/knowledge-transmission substrate.

## Dagyeom API / design handoff

Former Observer read blockers remain RESOLVED / READY FOR BINDING.

New direction for Dagyeom:
- do not assume a permanent modern-household setting;
- Character Presentation must remain generic enough for primitive tools, gathered resources, crafted objects and later technology;
- main HUD stays observer-first, not a strategy-tech dashboard;
- future resident detail may show Inventory / Known Techniques / Skill / current experiment;
- future world detail may show major discoveries / shortages / cultural knowledge differences;
- do not invent these fields before Jjun read APIs exist.

## Shared File Lock

Jjun default ownership: `LifeLens.uproject`, `Source/LifeLens/LifeLens.Build.cs`, `Source/LifeLens/Core/LLTypes.h`, `Source/LifeLens/Simulation/**`, `Source/LifeLensCore/**`, `Config/**`, `.github/workflows/**`, `Tools/validate_bootstrap.py`, and Jjun-owned AI/World runtime integration.

Dagyeom default ownership: `Source/LifeLens/UI/**`, Character appearance/presentation code, `Content/UI/**`, `Content/Characters/**`.

## Integration Requests

Current open requests: **none**.

## Merge / reconciliation queue

1. PR #48 — wait for Run #14 actual UHT/UBT result; merge only on PASS.
2. Civilization Foundation v1 — pure-Core parallel slice; no #48 overlap.
3. Dagyeom PR #17 — latest-main reconcile + Bridge binding + review fixes + verify.
4. Dagyeom PR #26 — reconcile latest main independently.
5. After #17: #29/#30 → #36 → #38.
6. PR #2 remains FROZEN.

## Completion rule

Code existence alone is not completion. Required validation + merge + state synchronization are required. If GitHub and docs disagree, update docs first.
