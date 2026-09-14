# LifeLens Team Board

이 파일은 쭌(sjLim91), 다겸(STILLofficial), 양쪽 AI의 작업 잠금/분배 보드다.

## 최우선 규칙

**기능 작업보다 상태 동기화가 먼저다.** actual main/branch/PR/Actions를 `WORK_STATE.md` → 역할별 READY queue → 이 보드 → `HANDOFF_LOG.md`와 대조하고, 다르면 코드 전에 문서를 갱신한다.

상태: `TODO` / `DOING` / `REVIEW` / `DONE` / `BLOCKED` / `FROZEN`

## Active Work

| 담당 | 브랜치 / PR | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 쭌 AI | `jjun/autonomous-family-progression-v1` | Autonomous Family Progression v1 | `Source/LifeLensCore/**`, Core tests/CMake only | DOING — dating→cohabitation→engagement→marriage→pregnancy→birth runtime wiring |
| 쭌 + 쭌 AI | `task/03-fast-test`, PR #2 | old Android validation | Bridge/build | FROZEN — Run `34739283266` 재실행/수정/병합 금지 |
| 다겸 + 다겸 AI | `dagyeom/observer-ui-v2`, PR #17 | Observer HUD v2 + Core Observer Bridge binding | `Source/LifeLens/UI/**` | REVIEW / RECOVERING — latest main reconcile 필요 |
| 다겸 + 다겸 AI | `dagyeom/ui-foundation-v1`, PR #26 | Android landscape UI foundation | UI foundation | REVIEW |
| 다겸 + 다겸 AI | `dagyeom/character-presentation-v1`, PR #29 | Character Presentation v1 | Character presentation | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/observer-ux-polish-v1`, PR #30 | Observer UX Polish | `Source/LifeLens/UI/**` | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/mobile-touch-v1`, PR #36 | Mobile Touch v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #30 |
| 다겸 + 다겸 AI | `dagyeom/visual-feedback-v1`, PR #38 | Visual Feedback v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #36 |

## Current Jjun lock — Autonomous Family Progression v1

- Branch: `jjun/autonomous-family-progression-v1` (created from latest main after state sync).
- Allowed scope: `Source/LifeLensCore/**` and Core tests/CMake only.
- Explicitly forbidden in this task: `Source/LifeLens/Simulation/**`, `Source/LifeLens/UI/**`, Character presentation, Content, Config, Android workflows, frozen TASK_03.
- Product purpose: wire already-existing Core Romance / Household / Marriage / Pregnancy / Birth APIs into deterministic `Simulation::step()` behavior.
- Contract:
  - no hard-coded founder couples;
  - periodic state-driven candidate evaluation;
  - minimum stage durations prevent dating/cohabitation/engagement/marriage/pregnancy from collapsing into one instant chain;
  - romance is not restricted by sex;
  - biological pregnancy follows current reproductive eligibility model;
  - pregnancy advances with simulation time and due pregnancies create actual child residents;
  - birth updates genetics, genealogy, parent/child links, household membership, LifeHistory and runtime state;
  - same seed/state yields same result.
- Validation required: full Core Release tests, deterministic harness, new end-to-end progression tests, Structural Preflight.

## Latest completed Jjun work

### PR #42 — Production NEW GAME Unreal Runtime Integration

- Status: DONE
- Feature HEAD: `5f61ce04963166af418fb672fb4442a6cf0598e6`
- Merge: `5c6f2c071ca00bcd4b26d6e002bf5c618d02ff1e`
- Validation: Preflight `34799244015` PASS; Unreal Linux Compile `34799244011` PASS including actual UHT/UBT.
- Main now starts formal Core New Game, projects founder Sex/Age/LifeStage/Personality to Unreal, preserves stable WorldSeed+CharacterId GUIDs, and no longer randomizes a second four-person population in `ULLSimulationSubsystem::NewGame()`.
- No Dagyeom UI/Character presentation files changed.

### Earlier integration

- PR #41 Production NEW GAME Core v1 — merge `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68`; Core `34798427843` PASS incl deterministic harness; Preflight `34798427848` PASS.
- PR #37 Observer Runtime Bridge — merge `938d0a2798e600929b4ccc755b48bcd39026ac75`; Unreal UHT/UBT `34796067278` PASS.
- PR #39 authoritative Family Runtime State — merge `612229cc610d2ea6283e080309bdee58ef42d1db`; Core `34796213647` PASS.
- PR #40 Family + World Observer Bridge — merge `9261581df3abd5332d92855628fd7d03203748af`; Preflight `34796892593`, Unreal UHT/UBT `34796892609` PASS.

## Dagyeom API handoff

Former six `BLOCKED-BY-JJUN` items remain RESOLVED / READY FOR BINDING via #37/#39/#40: Relationship 13D + target, Emotion details, SocialIntent+target, Family summary, World aggregates, read-only Blueprint/USTRUCT Bridge.

PR #42 is now merged and additionally exposes founder Sex/Age/LifeStage/Personality through the same Core resident DTO.

If a new concrete API gap is found, add a new Integration Request rather than reopening the old six.

## Shared File Lock

Jjun default ownership: `LifeLens.uproject`, `Source/LifeLens/LifeLens.Build.cs`, `Source/LifeLens/Core/LLTypes.h`, `Source/LifeLens/Simulation/**`, `Source/LifeLensCore/**`, `Config/**`, `.github/workflows/**`, `Tools/validate_bootstrap.py`.

Dagyeom default ownership: `Source/LifeLens/UI/**`, Character appearance/presentation code, `Content/UI/**`, `Content/Characters/**`.

## Integration Requests

Current open requests: **none**.

## Merge / reconciliation queue

1. Jjun `jjun/autonomous-family-progression-v1` — DOING, Core-only.
2. Dagyeom PR #17 — reconcile latest main + bind current Core Bridge + review fixes + verify.
3. Dagyeom PR #26 — reconcile latest main independently.
4. After #17: #29/#30 → #36 → #38.
5. PR #2 remains FROZEN.

## Completion rule

Code existence alone is not completion. Required validation + merge + state synchronization are required. If GitHub and docs disagree, update docs first.
