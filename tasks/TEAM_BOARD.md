# LifeLens Team Board

이 파일은 쭌(sjLim91), 다겸(STILLofficial), 양쪽 AI의 작업 잠금/분배 보드다.

> **LIVE LOCK STATUS (main): active assist locks = 0.**
> 실제 `main` / branch / PR / Actions가 문서보다 우선한다.

## 최우선 규칙

- 기능 작업 전 `WORK_STATE.md` → 역할별 READY queue → 이 보드 → `HANDOFF_LOG.md`를 맞춘다.
- ACTIVE_LOCK 파일은 lock owner 외 수정 금지.
- 실제 즉시 착수 가능 상태는 `READY_NOW` 또는 `PARALLEL_SAFE_NOW`뿐이다.
- `NEXT` / `AFTER` / `HIGH PRIORITY` / `READY_AFTER_*`는 즉시 착수 허가가 아니다.
- 쭌이 다겸 작업을 도울 때는 `docs/INTEGRATION_SPRINT.md`를 따른다.
- 기본 지원은 REVIEW_ONLY이며 `dagyeom/*` direct push 금지.
- 검증 전용 PR은 제품 코드로 병합하지 않는다.

## Active Work

| 담당 | 브랜치 / PR | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 쭌 AI | NEW milestone | Core ↔ World Execution Sync v1 | Core / Bridge / World / SaveLoad | READY_NOW / HIGHEST PRIORITY |
| 다겸 + 다겸 AI | PR #67 / `dagyeom/character-appearance-v1` | Character Appearance v1 | Character appearance + `Content/Characters/**` | ACTIVE / CLOSEOUT |
| 다겸 + 다겸 AI | `dagyeom/character-motion-v1` after #67 | Character Motion Bootstrap | Character locomotion presentation | READY_AFTER_#67 |
| 다겸 + 다겸 AI | `dagyeom/world-visual-environment-v1` after Motion bootstrap | World Visual Environment v1 | `Content/Environment/**`, `Content/Maps/**`, `Content/WorldPresentation/**` | HIGH PRIORITY / READY_AFTER_MOTION_BOOTSTRAP |
| 다겸 + 쭌 Bridge support as needed | after World Visual v1 | Character Motion & Context remainder | Character presentation/animation | AFTER WORLD VISUAL v1 |
| 다겸 + 다겸 AI | PR #30 | Observer UX Polish | `Source/LifeLens/UI/**` | AFTER HUMAN + WORLD VISUAL MINIMUM |
| 다겸 + 다겸 AI | PR #36 | Mobile Touch v1 | `Source/LifeLens/UI/**` | AFTER #30 |
| 다겸 + 다겸 AI | PR #38 | Visual Feedback v1 | `Source/LifeLens/UI/**` | AFTER #36 |
| 쭌 + 쭌 AI | PR #2 | old Android validation | Bridge/build | FROZEN |

## Current Assist Locks

**0개.** Previous appearance/presentation assist locks are released.

## Whole Project Verification v1 — DONE WITH FINDINGS

Canonical report: `docs/WHOLE_PROJECT_VERIFICATION_2026-09-14.md`.

Baseline main: `92bb92f6b6dab77a327158460c765b325a0fc149`.
Verification PR #69: **CLOSED WITHOUT MERGE**.

Automated gates:
- Structural Preflight `34856094580`: PASS.
- Core Tests `34856094696`: PASS, 39/39 + deterministic harness diff clean.
- Unreal Linux Compile `34856094645`: PASS including actual UE 5.6 UHT/UBT/link.

Important: #69 validates current merged main only. It does not include unmerged PR #67 Character Appearance.

### Findings requiring product work

P0:
- Core physical action completion can occur before Unreal actor arrival/use.
- Core and Unreal independently resolve physical affordance/fallback state.
- Core residue grid position and Unreal visible emergency location use different algorithms.
- World resident actors are not dynamically reconciled after births/deaths; spawn layout is effectively four-slot.
- activity anchors are collected only at BeginPlay, so newly built civilization facilities are not automatically usable.
- actual Unreal facilities are not currently reconciled into one Core/World authoritative affordance result.

P1:
- physical position is not restored into Unreal presentation from authoritative saved state.
- legacy `ULLDecisionComponent::ChooseAction()` remains a dormant Blueprint-callable competing action chooser.
- UE compile workflow path filters do not cover all `Source/LifeLens/**` C++ areas such as Character-only changes.
- binary snapshot v1-v3 migration logic lacks an explicit dedicated legacy-fixture test found by this verification.

## Core ↔ World Execution Sync v1 — READY_NOW

Owner: 쭌 / 쭌 AI.

Goal: remove split-brain physical execution before deeper environment feedback/civilization physical expression.

Minimum:
- stable physical action token and phase.
- one resolved affordance/target/tier per action.
- Core completion only after World arrival/use acknowledgement.
- explicit failure/re-resolve path for unavailable/disappeared/path-failed targets.
- canonical Core Grid ↔ Unreal World coordinate transform.
- residue/environment consequence uses exact completed action location.
- actual-world facilities/resources participate through one resolution contract.
- dynamic resident actor reconciliation for population changes.
- scalable spawn/re-entry positioning for population >4.
- dynamic activity-anchor registration/reconciliation.
- physical position continuity through Save/Load.
- regression tests for preferred/emergency timing and population >4.

After this milestone, resume environment exposure → Health / Memory / Avoidance and sanitation/civilization feedback.

## World Affordance / Environment — canonical

Simulation/environment consequence: `docs/WORLD_AFFORDANCE_ENVIRONMENT_v1.md`.
World visual presentation: `docs/WORLD_VISUAL_ENVIRONMENT_v1.md`.

Rules:
- initial world must not silently spawn modern civilization infrastructure.
- fallback concept remains `Preferred → Primitive → Natural → Emergency → Unavailable`.
- Core is simulation/intent authority; physical World reports execution facts.
- Eat/Drink never create resources implicitly.
- environmental consequence belongs to Core authority and Save/Load.
- visual environment must not become a second world authority.

### PR #66 — DONE

- World Affordance Fallback v1 merged as `0ad8d6b4c80c134832fcad9bf9b34dcfabf2b68a`.
- Preflight `34848772905`: PASS.
- UE compile `34848772895`: PASS.

### PR #68 — DONE

- Environmental Residue v1 merged as `92bb92f6b6dab77a327158460c765b325a0fc149`.
- Preflight `34853523382`: PASS.
- Core Tests `34853523542`: PASS, 39/39.
- UE compile `34853523394`: PASS including actual UHT/UBT/link.
- production New Game modern Core SmartObjects removed.
- Core residue state / accumulation / decay / exposure query / snapshot v4 / SaveLoad / Unreal read DTO delivered.

Boundary: Health/Memory/Avoidance, weather/media spread, cleanup and sanitation knowledge progression remain later work.

## Character Appearance v1 — ACTIVE / PR #67 CLOSEOUT

Owner: 다겸 / 다겸 AI.
Canonical acceptance: `docs/CHARACTER_APPEARANCE_ROADMAP.md`.
Canonical asset track: `docs/CHARACTER_ASSET_TRACK.md`.

Current reported state:
- humanoid residents visible.
- #65 deterministic appearance projection consumed.
- hair/skin variation present.
- UAL animation assets imported.
- locomotion not yet wired.

Remaining DONE gates:
- final required CI/UE compile record.
- Save/Load same resident → same appearance verification.
- minimum default clothing; underwear-only does not satisfy current minimum.
- final review + merge + docs sync.

## Character visual direction

After #67:
1. Motion Bootstrap: Idle/Walk/Jog + basic orientation only.
2. World Visual Environment v1.
3. remaining Motion/Context: turn refinement, sit/stand/lie/wake, gaze, IK, interaction transitions.

## World Visual Environment v1 — PROMOTED HIGH PRIORITY

Canonical: `docs/WORLD_VISUAL_ENVIRONMENT_v1.md`.

Dagyeom owns:
- `Content/Environment/**`
- `Content/Maps/**`
- `Content/WorldPresentation/**`

Minimum:
- terrain/ground.
- sky/lighting/atmosphere.
- trees/grass/rocks/natural dressing.
- Observer readability.
- Android-friendly LOD/instancing/material budget.
- no implicit modern infrastructure.
- asset provenance.

## Dagyeom API / design handoff

Current blockers from Jjun for #67: **0**.

Main exposes relevant read contracts including:
- `GetWorldObservation()`
- `GetResidentObservations()`
- `GetResidentObservation(...)`
- `GetFamilyObservation(...)`
- `GetResidentActionDirective(...)`
- `GetRecentCoreEvents()`
- `OnCoreRuntimeStateChanged`
- `GetResidentCivilizationObservation(...)`
- `GetCivilizationWorldObservation(...)`
- `GetEnvironmentObservation(...)`
- `ULLAppearanceProfileLibrary::MakeDeterministicAppearanceProfile(...)`

Rules:
- never hard-code runtime population to 4.
- UI rebuilds from Bridge after load.
- Character/UI presentation does not choose competing actions.
- if authoritative Core/Bridge data is missing, add an Integration Request rather than duplicating state.

## Shared File Ownership

Jjun default:
- `Source/LifeLensCore/**`
- `Source/LifeLens/AI/**`
- `Source/LifeLens/Simulation/**`
- `Source/LifeLens/World/**`
- Save/Load, Bridge, CI/build

Dagyeom default:
- `Source/LifeLens/UI/**`
- Character appearance/presentation
- `Content/UI/**`
- `Content/Characters/**`
- `Content/Environment/**`
- `Content/Maps/**`
- `Content/WorldPresentation/**`

## Integration Requests

Current open blockers from Jjun for Dagyeom work: **0**.

### Dagyeom request — Motion promotion

**ACCEPTED WITH GATE.**
- Motion Bootstrap becomes READY_NOW automatically after #67 is validated, merged and docs are synchronized.
- bootstrap is limited to Idle/Walk/Jog + basic orientation.
- World Visual v1 takes priority after that checkpoint.

### Dagyeom request — clothing / skin variety

**PARTIALLY ACCEPTED / SPLIT.**
- minimum default clothing is already a #67 acceptance requirement.
- additional clothing and skin detail can be follow-up enhancement.
- every extra asset pack needs exact source/version/license verification.

### User priority — World Visual Environment

**PROMOTED.**
- background work is no longer postponed until deep Motion/UX completion.
- status: `HIGH PRIORITY / READY_AFTER_MOTION_BOOTSTRAP`.

## Merge / execution queue

1. Character Presentation v1 — DONE via #63.
2. Appearance projection contract — DONE via #65.
3. World Affordance Fallback — DONE via #66.
4. Environmental Residue — DONE via #68.
5. Whole Project Verification — DONE WITH FINDINGS via closed verify-only #69.
6. **Core ↔ World Execution Sync v1 — READY_NOW.**
7. Character Appearance #67 — ACTIVE CLOSEOUT in parallel.
8. Character Motion Bootstrap — READY_AFTER_#67.
9. **World Visual Environment v1 — HIGH PRIORITY / READY_AFTER_MOTION_BOOTSTRAP.**
10. Character Motion & Context remainder.
11. Environment exposure → Health/Memory/Avoidance + sanitation progression.
12. PR #30 Observer UX Polish.
13. PR #36 Mobile Touch.
14. PR #38 Visual Feedback.
15. integrated runtime verification.
16. Android smoke APK + device profiling.
17. MetaHuman comparison / upgrade decision.
18. genetics/lifecycle visual work and clothing/equipment civilization linkage.

## Completion rule

검증 + merge + 상태 동기화까지 완료되어야 DONE이다.
