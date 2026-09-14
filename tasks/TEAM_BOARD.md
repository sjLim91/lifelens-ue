# LifeLens Team Board

이 파일은 쭌(sjLim91), 다겸(STILLofficial), 양쪽 AI의 작업 잠금/분배 보드다.

## 최우선 규칙

**기능 작업보다 상태 동기화가 먼저다.** actual main/branch/PR/Actions를 `WORK_STATE.md` → 역할별 READY queue → 이 보드 → `HANDOFF_LOG.md`와 대조하고, 다르면 코드 전에 문서를 갱신한다.

상태: `TODO` / `DOING` / `REVIEW` / `DONE` / `BLOCKED` / `FROZEN`

## Active Work

| 담당 | 브랜치 / PR | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 쭌 AI | `jjun/unreal-savegame-adapter-v1`, PR #45 | Unreal SaveGame Adapter v1 | `Source/LifeLens/Simulation/**`, `Source/LifeLens/Save/**`, narrow Core codec/tests, validator | REVIEW / WAITING_UNREAL_COMPILE — Core + Preflight PASS; UE Run `34803226433` running |
| 쭌 + 쭌 AI | `task/03-fast-test`, PR #2 | old Android validation | Bridge/build | FROZEN — Run `34739283266` 재실행/수정/병합 금지 |
| 다겸 + 다겸 AI | `dagyeom/observer-ui-v2`, PR #17 | Observer HUD v2 + Core Observer Bridge binding | `Source/LifeLens/UI/**` | REVIEW / RECOVERING — latest main reconcile 필요 |
| 다겸 + 다겸 AI | `dagyeom/ui-foundation-v1`, PR #26 | Android landscape UI foundation | UI foundation | REVIEW |
| 다겸 + 다겸 AI | `dagyeom/character-presentation-v1`, PR #29 | Character Presentation v1 | Character presentation | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/observer-ux-polish-v1`, PR #30 | Observer UX Polish | `Source/LifeLens/UI/**` | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/mobile-touch-v1`, PR #36 | Mobile Touch v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #30 |
| 다겸 + 다겸 AI | `dagyeom/visual-feedback-v1`, PR #38 | Visual Feedback v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #36 |

## Current Jjun lock — PR #45 Unreal SaveGame Adapter v1

- Branch/head: `jjun/unreal-savegame-adapter-v1` / `547b3f94e0c3df6df15d723e72c8084cd10a7c29`.
- Changed scope verified: 10 Jjun-owned Save/Simulation/Core/validator files; no UI/Characters/Content.
- SaveGame v2 stores canonical Core snapshot bytes; v2 Load restores Core directly and rebuilds compatibility projection.
- Restore is transactional via a temporary Core candidate; corrupt/incompatible saves do not replace the live world.
- v1 replay remains migration-only and legacy Residents/Relationships never become authority.
- Validation:
  - Core Tests `34803226434` PASS incl deterministic harness.
  - Structural Preflight `34803226458` PASS.
  - Unreal Linux Compile `34803226433` IN PROGRESS; actual UHT/UBT is the only remaining gate.

## Latest completed Jjun work

- PR #44 Full Core Save/Load v1 — merge `2b3f9882703ed73cb8318ae262f26bebb995c209`; Core `34802611336` PASS incl deterministic harness; Preflight `34802611299` PASS.
- PR #43 Autonomous Family Progression v1 — merge `179e3a65aaa6ff8d2243117c7aebfd760812c73d`; Core `34801572846` PASS; Preflight `34801572858` PASS.
- PR #42 Production NEW GAME Unreal Runtime Integration — merge `5c6f2c071ca00bcd4b26d6e002bf5c618d02ff1e`; Preflight `34799244015`; Unreal UHT/UBT `34799244011` PASS.
- PR #41 Production NEW GAME Core v1 — merge `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68`; Core `34798427843` PASS; Preflight `34798427848` PASS.
- #37/#39/#40 Observer/family runtime bridge chain remains merged and validated.

## Dagyeom API handoff

Former six `BLOCKED-BY-JJUN` items remain RESOLVED / READY FOR BINDING. Main supports autonomous population growth and full Core snapshot restoration; UI remains read-only over Bridge state.

## Shared File Lock

Jjun default ownership: `LifeLens.uproject`, `Source/LifeLens/LifeLens.Build.cs`, `Source/LifeLens/Core/LLTypes.h`, `Source/LifeLens/Simulation/**`, `Source/LifeLensCore/**`, `Config/**`, `.github/workflows/**`, `Tools/validate_bootstrap.py`.

Dagyeom default ownership: `Source/LifeLens/UI/**`, Character appearance/presentation code, `Content/UI/**`, `Content/Characters/**`.

## Integration Requests

Current open requests: **none**.

## Merge / reconciliation queue

1. PR #45 — WAITING_UNREAL_COMPILE; merge only after Run `34803226433` UHT/UBT PASS.
2. Dagyeom PR #17 — reconcile latest main + bind current Core Bridge + review fixes + verify.
3. Dagyeom PR #26 — reconcile latest main independently.
4. After #17: #29/#30 → #36 → #38.
5. PR #2 remains FROZEN.

## Completion rule

Code existence alone is not completion. Required validation + merge + state synchronization are required. If GitHub and docs disagree, update docs first.
