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
| 다겸 + 다겸 AI | `dagyeom/observer-ui-v2`, PR #17 | Observer HUD v2 + latest main reconcile + Bridge binding | `Source/LifeLens/UI/**` | READY / R1 RESUME |
| 다겸 + 다겸 AI | `dagyeom/ui-foundation-v1`, PR #26 | Android landscape UI foundation | UI foundation | REVIEW |
| 다겸 + 다겸 AI | `dagyeom/character-presentation-v1`, PR #29 | Character Presentation v1 | Character presentation | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/observer-ux-polish-v1`, PR #30 | Observer UX Polish | `Source/LifeLens/UI/**` | REVIEW — stacked on #17 |
| 다겸 + 다겸 AI | `dagyeom/mobile-touch-v1`, PR #36 | Mobile Touch v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #30 |
| 다겸 + 다겸 AI | `dagyeom/visual-feedback-v1`, PR #38 | Visual Feedback v1 | `Source/LifeLens/UI/**` | REVIEW — stacked on #36 |
| 쭌 + 쭌 AI | `task/03-fast-test`, PR #2 | old Android validation | Bridge/build | FROZEN |

## Current Assist Locks

### ASSIST_LOCK-17-R1 — Observer HUD v2 reconciliation

- Mode: active integration assist
- Target owner: 다겸 / STILLofficial
- Target PR: #17 `dagyeom/observer-ui-v2`
- Base HEAD: `71b900e1a67dc8e5e9643b3cf5d404470a0791dc`
- Helper branch: `integration/dagyeom-observer-r1-assist`
- Locked paths:
  - `Source/LifeLens/UI/LLObservationSubsystem.cpp`
  - `Source/LifeLens/UI/LLObservationSubsystem.h`
  - `Source/LifeLens/UI/LLObserverHUD.cpp`
  - `Source/LifeLens/UI/LLObserverHUD.h`
  - `Source/LifeLens/UI/LLObserverLabels.h`
  - `Source/LifeLens/UI/LLObserverPlayerController.cpp`
  - `Source/LifeLens/UI/LLObserverPlayerController.h`
- Reason: latest-main reconcile + current Core/Bridge binding + close stale review findings without concurrent edits.
- Status: `LOCKED / JJUN ASSIST STARTING`
- Unlock condition: helper branch handoff is validated and integrated into PR #17, or user explicitly cancels the assist.
- While locked, Dagyeom side should not edit the same seven UI files.

## Canonical direction — autonomous civilization

`Need / Curiosity → Observe → Gather → Store → Experiment → Discovery → Personal Knowledge → Craft/Build → Teach/Imitate → Shared Culture → Specialization/Exchange → Generational Civilization`

- 전역 tech unlock 금지
- 시대 gate 강제 금지
- 개인 지식/출처/전파를 Core가 권위 상태로 소유
- Observer HUD Level 0은 계속 단순하게 유지
- 원시 자원/도구부터 현대 기술까지 수용 가능한 presentation 유지

## Latest completed Jjun work

### PR #54 — macOS clang shadow hotfix
- DONE / MERGED `3b649b900c44a4e48bb89171b38f5e685e757b14`
- `ObserverReadModelV2.h` loop variable rename only
- Preflight + Core full tests + deterministic harness PASS
- IR-MAC-SHADOW-01 resolved

### PR #53 — Civilization Observer Read DTOs v1
- DONE / MERGED `ec30d80b2986247f0f16572efb2c082a933d796d`
- Feature head `e3a9f6611118866a6c79eb300a7b6ab2d5ffaf31`
- Core `34821150702` PASS
- Preflight `34821150693` PASS
- Unreal Linux Compile Run #16 `34821150704` PASS including UHT + UBT + final link
- Published getters:
  - `GetResidentCivilizationObservation(...)`
  - `GetCivilizationWorldObservation(...)`
- Published data:
  - inventory/carrying
  - technique level/confidence/practice
  - gathering/crafting/learning skill
  - SelfDiscovery / DirectWitness / Teaching provenance
  - ResourceNode / StorageSite summaries
  - civilization aggregates / recent discoveries
- UI/Character files untouched
- Core remains sole authority

### PR #52 → #46
- #52 Knowledge Transmission: `b90da9242003fbc0cbc553605b9abc46a17aa044`
- #51 Autonomous Civilization Loop: `55d5211160c8edad32b01177e2b9326a9faa2b78`
- #50 Runtime/Persistence: `c31c422c305a3a79a9553d37ac86247aa31d1853`
- #49 Civilization Foundation: `36bd1ac81192bc689c1e811553f068f255642508`
- #48 World Affordance: `a90ff6a5d870858a9e555ddf1e6e526bb7aa34e1`
- #47 Witness/Rumor: `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c`
- #46 Physical Action Bridge: `753df19657ea634ea2fa7c2ac935f6273ce14c10`

## Dagyeom API / design handoff

Current Observer blockers from Jjun: **0**.

PR #17 may now bind:
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
- civilization detail belongs in selected resident/detail/major-discovery layers, not a Level 0 strategy dashboard

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
- Requester: Dagyeom/Claude
- Target owner: Jjun
- File: `ObserverReadModelV2.h`
- Status: **DONE / RESOLVED**
- Resolution: PR #54 merge `3b649b900c44a4e48bb89171b38f5e685e757b14`
- Dagyeom R1 may resume

Current open requests: **none**.

## Merge / reconciliation queue

1. PR #17 latest-main reconciliation + current Bridge/civilization binding + review fixes.
2. PR #26 where independent.
3. #29/#30 after #17.
4. #36 after #30.
5. #38 after #36.
6. Integrated runtime verification.
7. Android smoke APK.
8. Then resume deeper civilization production chains.

## Completion rule

검증 + merge + 상태 동기화까지 완료되어야 DONE이다.
