# LifeLens Team Board

이 파일은 쭌(sjLim91), 다겸(STILLofficial), 양쪽 AI의 작업 잠금/분배 보드다.

## 최우선 규칙

**기능 작업보다 상태 동기화가 먼저다.** actual main/branch/PR/Actions를 `WORK_STATE.md` → 역할별 READY queue → 이 보드 → `HANDOFF_LOG.md`와 대조하고, 다르면 코드 전에 문서를 갱신한다.

제품 방향 기준은 `docs/LIFELENS_SPEC_v1.1.md`와 **`docs/CIVILIZATION_PROGRESSION_v1.md`**를 함께 따른다.

상태: `TODO` / `DOING` / `REVIEW` / `DONE` / `BLOCKED` / `FROZEN`

## Active Work

| 담당 | 브랜치 / PR | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 쭌 AI | `jjun/civilization-runtime-state-v1` (creation next) | Civilization Runtime State + Persistence v1 | `Source/LifeLensCore/**` | DOING / STATE_LOCKED |
| 쭌 + 쭌 AI | `task/03-fast-test`, PR #2 | old Android validation | Bridge/build | FROZEN — Run `34739283266` 재실행/수정/병합 금지 |
| 다겸 + 다겸 AI | `dagyeom/observer-ui-v2`, PR #17 | Observer HUD v2 + Core Observer Bridge binding | `Source/LifeLens/UI/**` | REVIEW / RECOVERING — latest main reconcile 필요 |
| 다겸 + 다겸 AI | `dagyeom/ui-foundation-v1`, PR #26 | Android landscape UI foundation | UI foundation | REVIEW |
| 다겸 + 다겸 AI | `dagyeom/character-presentation-v1`, PR #29 | Character Presentation v1 | Character presentation | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/observer-ux-polish-v1`, PR #30 | Observer UX Polish | `Source/LifeLens/UI/**` | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/mobile-touch-v1`, PR #36 | Mobile Touch v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #30 |
| 다겸 + 다겸 AI | `dagyeom/visual-feedback-v1`, PR #38 | Visual Feedback v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #36 |

## Canonical direction — autonomous civilization progression

LifeLens의 최종 방향은 현대 가구를 자율적으로 사용하는 NPC 시뮬레이터에 한정되지 않는다.

`Need / Curiosity → Observe → Gather → Carry/Store → Experiment → Discovery → Personal Knowledge → Craft/Build → Teach/Imitate → Shared Culture → Specialization/Exchange → Generational Civilization`

Rules:
- recipe/tech는 시작부터 전역 unlock되지 않는다.
- 기술은 개인 지식에서 시작하고 목격·모방·설명·교육을 통해 퍼진다.
- 석기/청동기/철기 같은 시대는 강제 gate가 아니라 실제 재료/기술 상태를 요약하는 관찰 레이블이다.
- 지식은 죽음/단절로 소실될 수 있다.
- Memory / Belief / Witness-Rumor / Family / Generation을 지식 전파와 세대 누적에 재사용한다.
- Observer HUD는 계속 단순하게 유지한다.

## Current Jjun lock — Civilization Runtime State + Persistence v1

- Planned branch: `jjun/civilization-runtime-state-v1` from actual latest main.
- Goal: PR #49 civilization domain을 실제 Character/World 권위 상태에 붙이고 Save/Load까지 보존.
- Required:
  - Character별 personal inventory/knowledge/skills;
  - World resource nodes/shared storage;
  - minimal natural-resource NEW GAME seed;
  - snapshot capture/restore + binary codec persistence;
  - migration/version strategy;
  - deep roundtrip + deterministic continuation tests.
- Out of scope: autonomous gather/craft AI, Unreal resource visuals, agriculture/metallurgy/economy.

## Latest completed Jjun work

### PR #49 — Civilization Foundation v1
- DONE / MERGED `36bd1ac81192bc689c1e811553f068f255642508`
- Feature head `ac8e0868f819a49d8b2c0aec947978258da56028`
- Pure Core resource/material + inventory/storage + personal knowledge + deterministic experiment/discovery + craft reproduction.
- Core Tests `34809360826` PASS incl Configure/Build/Test/Deterministic harness.
- Preflight `34809360739` PASS.

### PR #48 — World Affordance Execution v1
- DONE / MERGED `a90ff6a5d870858a9e555ddf1e6e526bb7aa34e1`
- Feature head `505e356d277cb0896109f6d4cdd75bcf06cdab54`
- Reservation / occupancy / use-transform / multi-capability infrastructure.
- Preflight `34808292399` PASS.
- Unreal Run #14 `34808292682` PASS incl actual UE 5.6 UHT + UBT.
- Treat as generic affordance infrastructure for resource nodes, fires, work surfaces, storage, crafting stations, tools, machines and furniture.

### PR #47 — Witness / Rumor / Social Knowledge v1
- DONE / MERGED `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c`
- Core `34806559374` PASS; Preflight `34806559379` PASS.

### PR #46 — Core Decision → Unreal Physical Action Bridge v1
- DONE / MERGED `753df19657ea634ea2fa7c2ac935f6273ce14c10`
- Core `34805882778` PASS; Preflight `34805882776` PASS; Unreal Run #10 `34805882789` PASS.

## Dagyeom API / design handoff

Former Observer read blockers remain RESOLVED / READY FOR BINDING.

New direction for Dagyeom:
- do not assume a permanent modern-household setting;
- Character Presentation should remain generic for primitive tools, gathered resources, crafted objects and later tech;
- main HUD stays observer-first, not a strategy-tech dashboard;
- future resident detail may show Inventory / Known Techniques / Skill / current experiment;
- future world detail may show discoveries / shortages / cultural knowledge differences;
- do not invent civilization fields before Jjun read APIs exist.

## Shared File Lock

Jjun default ownership: `LifeLens.uproject`, `Source/LifeLens/LifeLens.Build.cs`, `Source/LifeLens/Core/LLTypes.h`, `Source/LifeLens/Simulation/**`, `Source/LifeLensCore/**`, `Config/**`, `.github/workflows/**`, `Tools/validate_bootstrap.py`, and Jjun-owned AI/World runtime integration.

Dagyeom default ownership: `Source/LifeLens/UI/**`, Character appearance/presentation code, `Content/UI/**`, `Content/Characters/**`.

## Integration Requests

Current open requests: **none**.

## Merge / reconciliation queue

1. Civilization Runtime State + Persistence v1.
2. Dagyeom PR #17 — latest-main reconcile + Bridge binding + review fixes + verify.
3. Dagyeom PR #26 — reconcile latest main independently.
4. After #17: #29/#30 → #36 → #38.
5. PR #2 remains FROZEN.

## Completion rule

Code existence alone is not completion. Required validation + merge + state synchronization are required. If GitHub and docs disagree, update docs first.
