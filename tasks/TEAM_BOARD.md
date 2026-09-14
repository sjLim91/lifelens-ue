# LifeLens Team Board

이 파일은 쭌(sjLim91), 다겸(STILLofficial), 양쪽 AI의 작업 잠금/분배 보드다.

## 최우선 규칙

**기능 작업보다 상태 동기화가 먼저다.** actual main/branch/PR/Actions를 `WORK_STATE.md` → 역할별 READY queue → 이 보드 → `HANDOFF_LOG.md`와 대조하고, 다르면 코드 전에 문서를 갱신한다.

상태: `TODO` / `DOING` / `REVIEW` / `DONE` / `BLOCKED` / `FROZEN`

## Active Work

| 담당 | 브랜치 / PR | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 쭌 AI | `jjun/production-new-game-runtime-v1` | Production NEW GAME Unreal Runtime Integration | `Source/LifeLens/Simulation/**`, Core↔Unreal read adapter, 필요 시 쭌 소유 Core read model | DOING — Core founder source를 Unreal production start path에 연결 |
| 쭌 + 쭌 AI | `task/03-fast-test`, PR #2 | old Android validation | Bridge/build | FROZEN — Run `34739283266` 재실행/수정/병합 금지 |
| 다겸 + 다겸 AI | `dagyeom/observer-ui-v2`, PR #17 | Observer HUD v2 + Core Observer Bridge binding | `Source/LifeLens/UI/**` | REVIEW / RECOVERING — latest main reconcile 필요 |
| 다겸 + 다겸 AI | `dagyeom/ui-foundation-v1`, PR #26 | Android landscape UI foundation | UI foundation | REVIEW |
| 다겸 + 다겸 AI | `dagyeom/character-presentation-v1`, PR #29 | Character Presentation v1 | Character presentation | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/observer-ux-polish-v1`, PR #30 | Observer UX Polish | `Source/LifeLens/UI/**` | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/mobile-touch-v1`, PR #36 | Mobile Touch v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #30 |
| 다겸 + 다겸 AI | `dagyeom/visual-feedback-v1`, PR #38 | Visual Feedback v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #36 |

## Current Jjun lock — Production NEW GAME Unreal Runtime Integration

- Branch: `jjun/production-new-game-runtime-v1`
- Base at branch creation: `3f1848873e38325b6aa240f061d1035a5f9c4665`
- Allowed scope: Jjun-owned `Source/LifeLens/Simulation/**`, Core↔Unreal bridge/read DTO, narrowly required Core read fields/tests.
- Forbidden scope in this task: `Source/LifeLens/UI/**`, Dagyeom Character presentation, Content UI/Characters, old TASK_03.
- Verified starting gap:
  - Core Bridge only starts demo/social demo today.
  - Production Core `setupNewGame()` from PR #41 is not yet exposed to Unreal.
  - Core resident Unreal DTO lacks Sex/AgeYears/LifeStage founder identity.
  - legacy `ULLSimulationSubsystem::NewGame()` independently randomizes another 2M+2F population.
- Target:
  - production bridge start calls Core `setupNewGame()`;
  - stable WorldSeed + CharacterId GUID mapping preserved;
  - founder identity projected read-only to Unreal;
  - no new second randomization in the production path;
  - validate Structural Preflight + actual UE 5.6 Linux UHT/UBT before merge.
- Save/Load is not silently claimed solved here; persistent Core-state save can be a following bounded task.

## Latest completed Jjun work

- PR #41 Production NEW GAME Core v1 — DONE, merge `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68`; Core `34798427843` PASS incl deterministic harness; Preflight `34798427848` PASS.
- PR #37 Observer Runtime Bridge — merge `938d0a2798e600929b4ccc755b48bcd39026ac75`; Unreal UHT/UBT `34796067278` PASS.
- PR #39 authoritative Family Runtime State — merge `612229cc610d2ea6283e080309bdee58ef42d1db`; Core `34796213647` PASS.
- PR #40 Family + World Observer Bridge — merge `9261581df3abd5332d92855628fd7d03203748af`; Preflight `34796892593`, Unreal UHT/UBT `34796892609` PASS.

## Dagyeom API handoff

Former six `BLOCKED-BY-JJUN` items remain RESOLVED / READY FOR BINDING via #37/#39/#40: Relationship 13D + target, Emotion details, SocialIntent+target, Family summary, World aggregates, read-only Blueprint/USTRUCT Bridge.

If a new concrete API gap is found, add a new Integration Request rather than reopening the old six.

## Shared File Lock

Jjun default ownership: `LifeLens.uproject`, `Source/LifeLens/LifeLens.Build.cs`, `Source/LifeLens/Core/LLTypes.h`, `Source/LifeLens/Simulation/**`, `Source/LifeLensCore/**`, `Config/**`, `.github/workflows/**`, `Tools/validate_bootstrap.py`.

Dagyeom default ownership: `Source/LifeLens/UI/**`, Character appearance/presentation code, `Content/UI/**`, `Content/Characters/**`.

## Integration Requests

Current open requests: **none**.

## Merge / reconciliation queue

1. Jjun `jjun/production-new-game-runtime-v1` — DOING.
2. Dagyeom PR #17 — reconcile latest main + bind current Core Bridge + review fixes + verify.
3. Dagyeom PR #26 — reconcile latest main independently.
4. After #17: #29/#30 → #36 → #38.
5. PR #2 remains FROZEN.

## Completion rule

Code existence alone is not completion. Required validation + merge + state synchronization are required. If GitHub and docs disagree, update docs first.
