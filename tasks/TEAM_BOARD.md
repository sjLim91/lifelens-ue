# LifeLens Team Board

이 파일은 쭌(sjLim91), 다겸(STILLofficial), 양쪽 AI의 작업 잠금/분배 보드다.

## 최우선 규칙

- 실제 `main` / branch / PR / Actions가 문서보다 우선한다.
- 기능 작업 전에 `WORK_STATE.md` → 역할별 READY queue → 이 보드 → `HANDOFF_LOG.md`를 맞춘다.
- 제품 방향은 `docs/LIFELENS_SPEC_v1.1.md` + `docs/CIVILIZATION_PROGRESSION_v1.md`.
- 쭌이 다겸 작업을 도울 때는 `docs/INTEGRATION_SPRINT.md` 필수.
- 기본 지원은 `REVIEW_ONLY`.
- 실제 다겸 소유 코드 수정은 `ASSIST_LOCK` + `integration/dagyeom-<scope>-assist`; `dagyeom/*` direct push 금지.

상태: `TODO` / `DOING` / `REVIEW` / `DONE` / `BLOCKED` / `FROZEN`

## Active Work

| 담당 | 브랜치 / PR | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 다겸 | Integration Sprint | Core/Bridge ↔ UI/Presentation 합류 | REVIEW/ASSIST only | DOING |
| 다겸 + 다겸 AI | PR #17 | Observer HUD v2 + Core/civilization binding | `Source/LifeLens/UI/**` | DONE / MERGED `aa194db7...` |
| 다겸 + 쭌 assist | `dagyeom/ui-foundation-v1`, PR #26 | Android landscape UI foundation | `LLObserverUIFoundation.*` | DOING / ASSIST_LOCK-26-R1 |
| 다겸 + 다겸 AI | `dagyeom/character-presentation-v1`, PR #29 | Character Presentation v1 | Character presentation | READY — #17 dependency cleared |
| 다겸 + 다겸 AI | `dagyeom/observer-ux-polish-v1`, PR #30 | Observer UX Polish | `Source/LifeLens/UI/**` | READY — #17 dependency cleared |
| 다겸 + 다겸 AI | `dagyeom/mobile-touch-v1`, PR #36 | Mobile Touch v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #30 |
| 다겸 + 다겸 AI | `dagyeom/visual-feedback-v1`, PR #38 | Visual Feedback v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #36 |
| 쭌 + 쭌 AI | `task/03-fast-test`, PR #2 | old Android validation | Bridge/build | FROZEN |

## Current Assist Locks

### ASSIST_LOCK-26-R1 — UI Foundation latest-main reconcile

- Mode: active integration assist
- Target owner: 다겸 / STILLofficial
- Target PR: #26 `dagyeom/ui-foundation-v1`
- Base HEAD: `70dfa5ebeabf24b661c9f9fd0bc63e3ad01ac180`
- Planned helper branch: `integration/dagyeom-ui-foundation-r1-assist`
- Locked paths:
  - `Source/LifeLens/UI/LLObserverUIFoundation.cpp`
  - `Source/LifeLens/UI/LLObserverUIFoundation.h`
- Reason: preserve the two UI Foundation files while reconciling PR #26 onto current main, discarding stale shared-state docs from the old branch, then validate UE 5.6 UHT/UBT/link before handoff.
- Status: `LOCKED / JJUN ASSIST R1`
- While locked, Dagyeom side should not edit these two files.
- Unlock condition: validated helper is integrated into PR #26 or user explicitly cancels the assist.

### Completed assist — PR #17 R1/R2

- R1 helper PR #56 merged into `dagyeom/observer-ui-v2`.
- R1 verify-only PR #57 closed without merge.
- R1 Preflight `34824371968` PASS.
- R1 Unreal Run #17 `34824371965` PASS including UHT + UBT + link.
- R2 helper PR #58 merged into `dagyeom/observer-ui-v2` as `5eaff7dd6d579606331919b89b0060a776a7ad80`.
- R2 verify-only PR #59 closed without merge.
- R2 first verify Run #18 failed because verify tree omitted four R1 UI files; helper/product branch was not at fault.
- Corrected R2 Preflight `34833994138` PASS.
- Corrected R2 Unreal Run #20 `34833994155` PASS including UHT + UBT + final link.
- PR #17 merged to main as `aa194db7c5b500cdf5041fd6d43b25f97b9dd0b6`.
- Former Codex review threads resolved; unresolved = 0.
- `ASSIST_LOCK-17-R2`: DONE / UNLOCKED.

## Canonical direction — autonomous civilization

`Need / Curiosity → Observe → Gather → Store → Experiment → Discovery → Personal Knowledge → Craft/Build → Teach/Imitate → Shared Culture → Specialization/Exchange → Generational Civilization`

- 전역 tech unlock 금지
- 시대 gate 강제 금지
- 개인 지식/출처/전파를 Core가 권위 상태로 소유
- Observer HUD Level 0은 계속 단순하게 유지
- 원시 자원/도구부터 현대 기술까지 수용 가능한 presentation 유지

## Latest completed integration

### PR #17 — Observer HUD v2
- DONE / MERGED `aa194db7c5b500cdf5041fd6d43b25f97b9dd0b6`
- authoritative current activity, emotion, relationship, family and civilization read data bound into presentation
- LEVEL 0 remains thin; detail tabs consume read-only Bridge data
- no second simulation authority/cache in UI

### PR #54 — macOS clang shadow hotfix
- DONE / MERGED `3b649b900c44a4e48bb89171b38f5e685e757b14`
- IR-MAC-SHADOW-01 resolved

### PR #53 — Civilization Observer Read DTOs v1
- DONE / MERGED `ec30d80b2986247f0f16572efb2c082a933d796d`
- Core `34821150702` PASS
- Preflight `34821150693` PASS
- Unreal Run #16 `34821150704` PASS

## Dagyeom API / design handoff

Current blockers from Jjun: **0**.

Main now exposes and Observer HUD can consume:
- `GetWorldObservation()`
- `GetResidentObservations()`
- `GetResidentObservation(...)`
- `GetFamilyObservation(...)`
- `GetResidentActionDirective(...)`
- `GetRecentCoreEvents()`
- `OnCoreRuntimeStateChanged`
- `GetResidentCivilizationObservation(...)`
- `GetCivilizationWorldObservation(...)`

Rules:
- never hard-code population to 4 after runtime starts
- UI rebuilds from Bridge after load
- presentation does not choose competing actions
- civilization detail belongs in selected resident/detail/major-discovery layers, not Level 0

## Shared File Lock

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

ASSIST_LOCK은 임시이며 해제 후 원래 소유권으로 복귀한다.

## Integration Requests

### IR-MAC-SHADOW-01
- Status: **DONE / RESOLVED** by PR #54.

Current open requests: **none**.

## Merge / reconciliation queue

1. PR #26 UI Foundation reconcile — **DOING / ASSIST_LOCK-26-R1**.
2. #29 Character Presentation and #30 Observer UX Polish parent-first from the now-merged #17 baseline.
3. #36 after #30.
4. #38 after #36.
5. Integrated runtime verification.
6. Android smoke APK.
7. Resume deeper civilization production chains.

## Completion rule

검증 + merge + 상태 동기화까지 완료되어야 DONE이다.
