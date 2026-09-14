# LifeLens Team Board

이 파일은 쭌(sjLim91), 다겸(STILLofficial), 양쪽 AI의 작업 잠금/분배 보드다.

## 최우선 규칙

**기능 작업보다 상태 동기화가 먼저다.** actual main/branch/PR/Actions를 `WORK_STATE.md` → 역할별 READY queue → 이 보드 → `HANDOFF_LOG.md`와 대조하고, 다르면 코드 전에 문서를 갱신한다.

제품 방향 기준은 `docs/LIFELENS_SPEC_v1.1.md`와 **`docs/CIVILIZATION_PROGRESSION_v1.md`**를 함께 따른다.

상태: `TODO` / `DOING` / `REVIEW` / `DONE` / `BLOCKED` / `FROZEN`

## Active Work

| 담당 | 브랜치 / PR | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 쭌 AI | `jjun/civilization-observer-read-v1`, PR #53 | Civilization Observer Read DTOs v1 | `Source/LifeLensCore/**`, `Source/LifeLens/Simulation/**`, validator/preflight | REVIEW / CI_RUNNING |
| 쭌 + 쭌 AI | `task/03-fast-test`, PR #2 | old Android validation | Bridge/build | FROZEN — Run `34739283266` 재실행/수정/병합 금지 |
| 다겸 + 다겸 AI | `dagyeom/observer-ui-v2`, PR #17 | Observer HUD v2 + Core Observer Bridge binding | `Source/LifeLens/UI/**` | REVIEW / RECOVERING — latest main reconcile 필요 |
| 다겸 + 다겸 AI | `dagyeom/ui-foundation-v1`, PR #26 | Android landscape UI foundation | UI foundation | REVIEW |
| 다겸 + 다겸 AI | `dagyeom/character-presentation-v1`, PR #29 | Character Presentation v1 | Character presentation | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/observer-ux-polish-v1`, PR #30 | Observer UX Polish | `Source/LifeLens/UI/**` | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/mobile-touch-v1`, PR #36 | Mobile Touch v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #30 |
| 다겸 + 다겸 AI | `dagyeom/visual-feedback-v1`, PR #38 | Visual Feedback v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #36 |

## Canonical direction — autonomous civilization progression

`Need / Curiosity → Observe → Gather → Carry/Store → Experiment → Discovery → Personal Knowledge → Craft/Build → Teach/Imitate → Shared Culture → Specialization/Exchange → Generational Civilization`

Rules:
- recipe/tech는 전역 unlock되지 않는다.
- 기술은 개인 지식에서 시작하고 목격·모방·설명·교육을 통해 퍼진다.
- 시대 레이블은 강제 gate가 아니라 실제 문명 상태의 관찰 요약이다.
- 지식은 죽음/단절로 소실될 수 있다.
- Memory / Belief / Witness-Rumor / Family / Generation을 재사용한다.
- Observer HUD는 계속 단순하게 유지한다.

## Current Jjun lock — Civilization Observer Read DTOs v1

- Branch/PR: `jjun/civilization-observer-read-v1`, PR #53.
- Head: `c7753047a28ea3b1dae75c4e6abae7e1f289f962`.
- Final push Core `34819660707`: PASS 38/38 + deterministic harness.
- PR Core `34819825563`: in progress.
- PR Preflight `34819825627`: in progress.
- Unreal Linux Compile Run #15 `34819825591`: in progress; actual UE 5.6 UHT/UBT required.
- Published read-only contracts:
  - resident inventory stacks/total carrying;
  - resident technique level/confidence/practice + gathering/crafting/learning skills;
  - SelfDiscovery / DirectWitness / Teaching provenance;
  - world ResourceNode / StorageSite summaries;
  - knowledge/discovery aggregates and recent actual discoveries;
  - stable resident FGuid projection through Bridge.
- New Unreal getters: `GetResidentCivilizationObservation(...)`, `GetCivilizationWorldObservation(...)`.
- Existing main HUD DTO is intentionally unchanged; detailed civilization state stays opt-in.
- No Jjun edits to `Source/LifeLens/UI/**`, Character appearance/presentation, `Content/UI/**`, or `Content/Characters/**`.
- Intermediate Core Run `34819279308` failure: test fixture used future event timestamps; Snapshot validator correctly rejected it. Fixed in final head.

## Latest completed Jjun work

### PR #52 — Civilization Knowledge Transmission v1
- DONE / MERGED `b90da9242003fbc0cbc553605b9abc46a17aa044`
- Feature head `8fe9369682834a3fae44bb6605f0d872bb80b971`.
- Discovery/Craft facts spread through deterministic witness/imitation/teaching without global unlock.
- `SocialKnowledgeBook` provenance is authoritative Simulation state and persists in snapshot binary v3; v1/v2 remain readable.
- PR Core `34814310235` PASS incl 37/37 + deterministic harness; Preflight `34814310291` PASS.

### PR #51 — Autonomous Civilization Action Loop v1
- DONE / MERGED `55d5211160c8edad32b01177e2b9326a9faa2b78`
- Gather / Store / Experiment / Craft autonomous; Core `34811869666` PASS; Preflight `34811869612` PASS.

### PR #50 / #49 / #48 / #47
- #50 Civilization Runtime/Persistence merge `c31c422c305a3a79a9553d37ac86247aa31d1853`.
- #49 Civilization Foundation merge `36bd1ac81192bc689c1e811553f068f255642508`.
- #48 World Affordance merge `a90ff6a5d870858a9e555ddf1e6e526bb7aa34e1`; UE Run #14 PASS.
- #47 Witness/Rumor merge `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c`.

## Dagyeom API / design handoff

Former Observer blockers remain RESOLVED / READY FOR BINDING.

Civilization status relevant to Dagyeom:
- Core owns Inventory / personal Knowledge / skills / ResourceNode / StorageSite / autonomous decisions / transmission provenance.
- PR #53 publishes the real civilization Bridge read contract but **do not bind it until #53 is merged and green**.
- Existing PR #17 reconciliation can proceed independently.
- Do not invent placeholders or duplicate authority in UI.
- Character Presentation should remain generic for primitive resources/tools through later technology.
- main HUD remains observer-first, not a strategy resource dashboard.

## Shared File Lock

Jjun default ownership: `LifeLens.uproject`, `Source/LifeLens/LifeLens.Build.cs`, `Source/LifeLens/Core/LLTypes.h`, `Source/LifeLens/Simulation/**`, `Source/LifeLensCore/**`, `Config/**`, `.github/workflows/**`, `Tools/validate_bootstrap.py`, and Jjun-owned AI/World runtime integration.

Dagyeom default ownership: `Source/LifeLens/UI/**`, Character appearance/presentation code, `Content/UI/**`, `Content/Characters/**`.

## Integration Requests

Current open requests: **none**.

## Merge / reconciliation queue

1. Finish PR #53 Civilization Observer Read DTOs v1.
2. Dagyeom PR #17 — latest-main reconcile + current Bridge binding + review fixes + verify.
3. Dagyeom PR #26 — reconcile latest main independently.
4. After #17: #29/#30 → #36 → #38.
5. PR #2 remains FROZEN.

## Completion rule

Code existence alone is not completion. Required validation + merge + state synchronization are required. If GitHub and docs disagree, update docs first.