# LifeLens Team Board

이 파일은 쭌(sjLim91), 다겸(STILLofficial), 양쪽 AI의 작업 잠금/분배 보드다.

## 최우선 규칙

**기능 작업보다 상태 동기화가 먼저다.** actual main/branch/PR/Actions를 `WORK_STATE.md` → 역할별 READY queue → 이 보드 → `HANDOFF_LOG.md`와 대조하고, 다르면 코드 전에 문서를 갱신한다.

상태: `TODO` / `DOING` / `REVIEW` / `DONE` / `BLOCKED` / `FROZEN`

## Active Work

| 담당 | 브랜치 / PR | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 쭌 AI | `jjun/physical-smart-object-v1`, PR #48 | Physical Interaction / Smart Object Execution v1 | Jjun-owned World + narrow Unreal compile workflow trigger | REVIEW / WAITING_UNREAL_COMPILE — head `cda509956feddacf9c159e998b79d631713a8e45`; Preflight PASS; Unreal Run #11 in progress |
| 쭌 + 쭌 AI | `task/03-fast-test`, PR #2 | old Android validation | Bridge/build | FROZEN — Run `34739283266` 재실행/수정/병합 금지 |
| 다겸 + 다겸 AI | `dagyeom/observer-ui-v2`, PR #17 | Observer HUD v2 + Core Observer Bridge binding | `Source/LifeLens/UI/**` | REVIEW / RECOVERING — latest main reconcile 필요 |
| 다겸 + 다겸 AI | `dagyeom/ui-foundation-v1`, PR #26 | Android landscape UI foundation | UI foundation | REVIEW |
| 다겸 + 다겸 AI | `dagyeom/character-presentation-v1`, PR #29 | Character Presentation v1 | Character presentation | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/observer-ux-polish-v1`, PR #30 | Observer UX Polish | `Source/LifeLens/UI/**` | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/mobile-touch-v1`, PR #36 | Mobile Touch v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #30 |
| 다겸 + 다겸 AI | `dagyeom/visual-feedback-v1`, PR #38 | Visual Feedback v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #36 |

## Current Jjun lock — PR #48 Physical Interaction / Smart Object Execution v1

- Branch/head: `jjun/physical-smart-object-v1` / `cda509956feddacf9c159e998b79d631713a8e45`.
- Core remains the sole decision authority; Unreal chooses only where/how to execute the Core-selected physical intent.
- Implemented physical intents: Eat / Drink / Sleep / Toilet / Hygiene.
- `ALLActivityAnchor` now has `Available → Reserved → InUse`, stable ResidentId ownership, use transform offset/rotation, exclusive reservation and release.
- WorldDirector selects the nearest usable capability-matched anchor with a stable path tie-break, holds the reservation across movement/use, and releases it on directive changes, idle/death, bridge loss, invalid target, despawn, or action transition.
- Generic coordinate fallback for covered physical intents is removed.
- Current Entry runtime has no placed anchors, so missing intent types get one bootstrap reservable anchor. Placed anchors remain preferred when present.
- CI discovery: Unreal compile workflow previously ignored `Source/LifeLens/World/**`; PR #48 adds that path so World changes now receive actual UHT/UBT validation.
- Scope at latest checkpoint: 4 World files + `.github/workflows/unreal-linux-compile.yml`; no UI, Character presentation, Content, pure Core, or frozen PR #2 edits.
- Validation:
  - Structural Preflight `34808013903` PASS on latest head.
  - Unreal Linux Compile `34808013906` / Run #11 IN PROGRESS.
- Merge gate: actual UE 5.6 image verification + UHT + UBT must PASS before merge.

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

### PR #47 — Witness / Rumor / Social Knowledge v1

- Status: DONE / MERGED
- Feature HEAD: `8c03b349d8489a847b4e4fb7e22dc974e91be8cf`
- Merge: `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c`
- Scope: exactly 3 pure-Core files; no Unreal overlap.
- Core `34806559374` PASS incl deterministic harness; Preflight `34806559379` PASS.

## Dagyeom API handoff

Former Observer read blockers remain RESOLVED / READY FOR BINDING. Main already provides typed action directives, Core-authoritative SaveGame v2 restore and dynamic population. PR #48 does not require a new Dagyeom API contract.

## Shared File Lock

Jjun default ownership: `LifeLens.uproject`, `Source/LifeLens/LifeLens.Build.cs`, `Source/LifeLens/Core/LLTypes.h`, `Source/LifeLens/Simulation/**`, `Source/LifeLensCore/**`, `Config/**`, `.github/workflows/**`, `Tools/validate_bootstrap.py`, and Jjun-owned AI/World runtime integration.

Dagyeom default ownership: `Source/LifeLens/UI/**`, Character appearance/presentation code, `Content/UI/**`, `Content/Characters/**`.

## Integration Requests

Current open requests: **none**.

## Merge / reconciliation queue

1. PR #48 — wait for Run #11 actual UHT/UBT result; merge only on PASS.
2. Dagyeom PR #17 — reconcile latest main + bind current Core Bridge + review fixes + verify.
3. Dagyeom PR #26 — reconcile latest main independently.
4. After #17: #29/#30 → #36 → #38.
5. PR #2 remains FROZEN.

## Completion rule

Code existence alone is not completion. Required validation + merge + state synchronization are required. If GitHub and docs disagree, update docs first.
