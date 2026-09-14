# LifeLens Team Board

이 파일은 쭌(sjLim91), 다겸(STILLofficial), 양쪽 AI의 작업 잠금/분배 보드다.

## 최우선 규칙

**기능 작업보다 상태 동기화가 먼저다.** actual main/branch/PR/Actions를 `WORK_STATE.md` → 역할별 READY queue → 이 보드 → `HANDOFF_LOG.md`와 대조하고, 다르면 코드 전에 문서를 갱신한다.

상태: `TODO` / `DOING` / `REVIEW` / `DONE` / `BLOCKED` / `FROZEN`

## Active Work

| 담당 | 브랜치 / PR | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 쭌 AI | `jjun/physical-smart-object-v1` (creation next) | Physical Interaction / Smart Object Execution v1 | Jjun-owned World/Simulation + narrow validator | DOING / STATE_LOCKED — Eat/Drink/Sleep/Toilet/Hygiene reservation + interaction target |
| 쭌 + 쭌 AI | `task/03-fast-test`, PR #2 | old Android validation | Bridge/build | FROZEN — Run `34739283266` 재실행/수정/병합 금지 |
| 다겸 + 다겸 AI | `dagyeom/observer-ui-v2`, PR #17 | Observer HUD v2 + Core Observer Bridge binding | `Source/LifeLens/UI/**` | REVIEW / RECOVERING — latest main reconcile 필요 |
| 다겸 + 다겸 AI | `dagyeom/ui-foundation-v1`, PR #26 | Android landscape UI foundation | UI foundation | REVIEW |
| 다겸 + 다겸 AI | `dagyeom/character-presentation-v1`, PR #29 | Character Presentation v1 | Character presentation | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/observer-ux-polish-v1`, PR #30 | Observer UX Polish | `Source/LifeLens/UI/**` | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/mobile-touch-v1`, PR #36 | Mobile Touch v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #30 |
| 다겸 + 다겸 AI | `dagyeom/visual-feedback-v1`, PR #38 | Visual Feedback v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #36 |

## Current Jjun lock — Physical Interaction / Smart Object Execution v1

- Planned branch: `jjun/physical-smart-object-v1` from latest main after state checkpoint.
- Core remains the sole decision authority. Unreal may select **where/how** to execute a Core-selected physical intent but must not decide **what** intent to perform.
- v1 physical intents: Eat / Drink / Sleep / Toilet / Hygiene only.
- Required execution contract:
  - capability-matched usable anchor/object;
  - deterministic nearest usable selection;
  - one-resident exclusive reservation/occupancy;
  - explicit interaction/approach transform, not raw actor origin;
  - release on directive change/death/despawn/invalid target/completion;
  - resident switching target only when prior reservation is released or becomes unusable.
- Preserve existing `ELLActionIntent` ordinals and stable ResidentId/SaveGame v2 identity behavior.
- Allowed scope: Jjun World/Simulation plus narrow validator/read metadata only where necessary.
- Forbidden: Dagyeom UI/Character presentation/Content and frozen PR #2.
- Required gates before merge: Structural Preflight + actual UE 5.6 UHT/UBT; Core suite if any pure-Core file is touched.

## Latest completed Jjun work

### PR #46 — Core Decision → Unreal Physical Action Bridge v1

- Status: DONE / MERGED
- Feature HEAD: `31c00cb0d751bb33825df31c7e758e60ff54402c`
- Merge: `753df19657ea634ea2fa7c2ac935f6273ce14c10`
- Core remains decision authority; Unreal WorldDirector consumes typed Core action directives.
- Covered physical intents: Eat / Drink / Sleep / Toilet / Hygiene; covered social intents: Approach / Avoid / Repair / Comfort.
- Legacy WorldDirector `ChooseAction()` authority and projection-only outcome mutation were removed for covered actions.
- Existing `ELLActionIntent` ordinal values remain 0–6; appended `Drink` resolves to 7.
- Validation:
  - Core Tests `34805882778` PASS incl deterministic harness.
  - Structural Preflight `34805882776` PASS.
  - Unreal Linux Compile `34805882789` / Run #10 PASS incl actual UE 5.6 UHT + UBT.
- Scope: exactly 9 Jjun-owned Core/Simulation/World/validator files; no Dagyeom UI/Character presentation/Content edits.
- Remaining QA: targeted PIE/runtime interaction verification when runnable scene/build verification is available.

### PR #47 — Witness / Rumor / Social Knowledge v1

- Status: DONE / MERGED
- Feature HEAD: `8c03b349d8489a847b4e4fb7e22dc974e91be8cf`
- Merge: `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c`
- Scope: exactly 3 pure-Core files; no Unreal overlap.
- Adds provenance-aware witness/rumor knowledge, deterministic retelling attenuation/distortion, trust-sensitive reception, duplicate/loop suppression, and Memory/Belief integration.
- Core `34806559374` PASS incl deterministic harness; Preflight `34806559379` PASS.
- Live Simulation wiring is a follow-up, not part of #47.

### Previous completed work

- PR #45 Unreal SaveGame Adapter v1 — merge `3c646ba331b8199a295fd6f2e9cac1235d844679`.
- PR #44 Full Core Save/Load v1 — merge `2b3f9882703ed73cb8318ae262f26bebb995c209`.
- PR #43 Autonomous Family Progression v1 — merge `179e3a65aaa6ff8d2243117c7aebfd760812c73d`.
- PR #42 Production NEW GAME Unreal Runtime Integration — merge `5c6f2c071ca00bcd4b26d6e002bf5c618d02ff1e`.
- PR #41 Production NEW GAME Core v1 — merge `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68`.

## Dagyeom API handoff

Former Observer read blockers remain RESOLVED / READY FOR BINDING. Main now also provides:
- typed `FLLCoreActionDirective` for physical/social current action + stable target resident;
- Core-authoritative WorldDirector behavior for covered actions;
- SaveGame v2 authoritative Core restore;
- dynamic population from family progression.

PR #47 witness/rumor is Core-domain-only for now; no new UI binding is required until runtime/observer read surfaces are wired.

## Shared File Lock

Jjun default ownership: `LifeLens.uproject`, `Source/LifeLens/LifeLens.Build.cs`, `Source/LifeLens/Core/LLTypes.h`, `Source/LifeLens/Simulation/**`, `Source/LifeLensCore/**`, `Config/**`, `.github/workflows/**`, `Tools/validate_bootstrap.py`, and Jjun-owned AI/World runtime integration.

Dagyeom default ownership: `Source/LifeLens/UI/**`, Character appearance/presentation code, `Content/UI/**`, `Content/Characters/**`.

## Integration Requests

Current open requests: **none**.

## Merge / reconciliation queue

1. Physical Interaction / Smart Object Execution v1 — current Jjun active slice.
2. Dagyeom PR #17 — reconcile latest main + bind current Core Bridge + review fixes + verify.
3. Dagyeom PR #26 — reconcile latest main independently.
4. After #17: #29/#30 → #36 → #38.
5. PR #2 remains FROZEN.

## Completion rule

Code existence alone is not completion. Required validation + merge + state synchronization are required. If GitHub and docs disagree, update docs first.
