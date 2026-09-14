# LifeLens Canonical Work State

> 실제 GitHub 상태가 항상 최우선 진실이다.
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` + `docs/CIVILIZATION_PROGRESSION_v1.md`
> World affordance / 환경 consequence 기준: `docs/WORLD_AFFORDANCE_ENVIRONMENT_v1.md`
> World visual presentation 기준: `docs/WORLD_VISUAL_ENVIRONMENT_v1.md`
> 캐릭터 외형/표현 기준: `docs/CHARACTER_APPEARANCE_ROADMAP.md`
> 협업 기준: `docs/STATE_MANAGEMENT.md` + `docs/INTEGRATION_SPRINT.md`
> 전체 검증 기준점: `docs/WHOLE_PROJECT_VERIFICATION_2026-09-14.md`

Last reconciled: 2026-09-14 KST

## Mandatory sync gate

1. 작업 시작 전 actual `main`, target branch/PR, Actions 확인.
2. `WORK_STATE.md` → 역할별 READY queue → `TEAM_BOARD.md` → `HANDOFF_LOG.md` 대조.
3. stale 문서가 있으면 코드보다 먼저 갱신.
4. ACTIVE_LOCK 파일은 lock owner 외 수정 금지.
5. `NEXT`/`AFTER`/`HIGH PRIORITY`는 작업 허가가 아니다. 실제 착수 가능 상태는 `READY_NOW` 또는 `PARALLEL_SAFE_NOW`다.
6. 검증 전용 PR은 제품 PR로 병합하지 않는다.

## Current dispatch

### Whole Project Verification v1 — DONE WITH FINDINGS

Baseline main: `92bb92f6b6dab77a327158460c765b325a0fc149`.
Verification PR #69: CLOSED WITHOUT MERGE.

Automated gates:
- Structural Preflight `34856094580`: PASS.
- Core Tests `34856094696`: PASS, 39/39 + deterministic harness diff clean.
- Unreal Linux Compile `34856094645`: PASS including actual UE 5.6 UHT/UBT/link.

The automated baseline is healthy, but integration review found runtime synchronization gaps that must be fixed before deeper physical/environment feedback work. Canonical report: `docs/WHOLE_PROJECT_VERIFICATION_2026-09-14.md`.

### Environmental Residue v1 — DONE

Owner: 쭌 / 쭌 AI.

- PR #68 `[CORE/WORLD] Add authoritative Environmental Residue v1` merged to main.
- Main merge: `92bb92f6b6dab77a327158460c765b325a0fc149`.
- Preflight `34853523382`: PASS.
- Core Tests `34853523542`: PASS, 39/39.
- Unreal Linux Compile `34853523394`: PASS including actual UE 5.6 UHT/UBT/link.

Delivered:
- production NEW GAME modern SmartObject bootstrap = 0.
- Core-owned environmental residue state.
- outdoor sanitation residue creation, accumulation, deterministic decay and exposure query.
- deterministic resident outdoor sanitation site in Core v1.
- snapshot binary format v4 environmental extension with legacy decoder support.
- Save/Load continuity and deterministic continuation.
- Unreal read-only environment observation DTO/API.
- emergency Eat/Drink requires and consumes actual resident provisions.

Boundary:
- Health / Memory / Avoidance / Knowledge / Civilization discovery feedback remains follow-up.
- weather/water/soil spread, cleanup/burial and sanitation progression remain follow-up.

### Core ↔ World Execution Sync v1 — READY_NOW / HIGHEST JJUN PRIORITY

Owner: 쭌 / 쭌 AI.

Reason: Whole Project Verification found compile-safe but runtime-critical Core/World split-brain risks.

Minimum acceptance:
- stable physical action token/phase.
- one resolved affordance tier/target per physical action.
- Core must not apply physical completion before World arrival/use acknowledgement.
- failure/target disappearance/path failure returns a clean failure/re-resolve result.
- one canonical Core Grid ↔ Unreal World coordinate mapping.
- environmental residue uses the exact completed physical-action position.
- actual-world facilities/affordances and Core availability resolve through one contract, not independent decisions.
- dynamic resident actor reconciliation for births/deaths/load; runtime population is never hard-coded to four.
- scalable deterministic/non-overlapping spawn/re-entry placement.
- dynamic activity-anchor registration/unregistration or reconciliation.
- physical position continuity through Save/Load.
- tests covering preferred vs emergency execution timing and population >4.

After this milestone:
- Environment Exposure → Health/Memory/Avoidance.
- sanitation problem recognition / primitive latrine / facility progression.

### World Affordance Fallback v1 — DONE

Owner: 쭌 / 쭌 AI.

- PR #66 merged as `0ad8d6b4c80c134832fcad9bf9b34dcfabf2b68a`.
- Preflight `34848772905`: PASS.
- Unreal Linux Compile `34848772895`: PASS.
- actual-world tier order: `Preferred → Primitive → Natural → Emergency → Unavailable`.
- no implicit modern facility spawn.

### Character Presentation v1 — DONE

- Integration PR #63 merged as `c9164ebf3cc70a194d3f8f6e50dcfb3c9df2a986`.
- Preflight `34843425475`: PASS.
- Unreal Linux Compile `34843425495`: PASS.
- original PR #29 is stale source/history only.
- `ASSIST_LOCK-29-R1`: RELEASED.

### Character Appearance support contract — DONE

- PR #65 merged as `86663cb10f245bf185984a3622e4a86596e14923`.
- Preflight `34845789763`: PASS.
- Unreal Linux Compile `34845789757`: PASS.
- provides vendor-neutral deterministic `FLLAppearanceProfile` projection from stable resident identity.
- no second appearance SaveGame authority/cache.

### Character Appearance v1 — ACTIVE / PR #67 CLOSEOUT

Owner: 다겸 / 다겸 AI.
Current PR: #67 `dagyeom/character-appearance-v1`.

Reported implemented checkpoint:
- Quaternius humanoid presentation imported.
- deterministic #65 appearance projection used.
- four humanoid residents visible in PIE.
- selection ring / labels preserved.
- UAL animation assets imported.

Remaining DONE gates from current acceptance criteria:
- required final CI/UE compile record for the final PR head.
- Save/Load appearance continuity explicitly verified.
- minimum default clothing present; underwear-only residents do not satisfy the existing minimum.
- final review/merge + live-doc sync.

Whole Project Verification #69 does NOT include #67 because #67 is still unmerged.

### Character Motion Bootstrap — READY_AFTER_#67

Owner: 다겸 / 다겸 AI.

After #67 merge + docs sync:
- Core-directive-driven Idle / Walk / Jog or Run.
- basic orientation smoothing.
- remove Idle-sliding presentation.
- no Character-side competing action authority.

Full sit/lie/gaze/IK remains behind World Visual v1.

### World Visual Environment v1 — HIGH PRIORITY / READY_AFTER_MOTION_BOOTSTRAP

Owner: 다겸 / 다겸 AI.
Canonical: `docs/WORLD_VISUAL_ENVIRONMENT_v1.md`.

Dagyeom presentation ownership:
- `Content/Environment/**`
- `Content/Maps/**`
- `Content/WorldPresentation/**`

Minimum:
- terrain / ground baseline.
- sky / lighting / atmosphere.
- trees / grass / rocks / natural dressing.
- Observer readability.
- Android-friendly LOD / instancing / material budget.
- no automatic modern buildings/facilities.
- asset provenance recorded.

World Visual is presentation-only until an element is explicitly linked to authoritative Core/World state.

### Character Motion & Context remainder — AFTER WORLD VISUAL v1

- turn-in-place refinement.
- sit / stand / lie / wake transitions.
- gaze/head tracking.
- context interaction hooks.
- basic IK / transition smoothing.

## Verification follow-up infrastructure

### Unreal compile trigger coverage — NEEDS FIX

Current `.github/workflows/unreal-linux-compile.yml` does not automatically trigger for all Unreal C++ areas, including Character-only changes. Prefer a C++ source trigger covering `Source/LifeLens/**` while leaving pure Content/docs changes out of the expensive compile.

### Legacy snapshot migration fixtures — NEEDS TEST

Codec supports binary v1–v4, but the verification did not find a dedicated registered test that decodes representative v1/v2/v3 byte fixtures. Add explicit migration fixtures/builders.

### Dormant legacy Unreal decision chooser — REVIEW/REMOVE

`ULLDecisionComponent::ChooseAction()` is not currently called by WorldDirector and its tick is disabled, but it remains a Blueprint-callable legacy competing action chooser. Remove/deprecate/restrict it so production gameplay cannot accidentally regain a second action authority.

## World / civilization causal rule

`Need → Intent → current-world affordance → movement/arrival → action result → environment consequence → experience/memory/health input → problem recognition → experiment/discovery → better affordance → culture/civilization`

Rules:
- Core remains simulation/intent authority.
- physical World reports real execution facts and available affordances; it does not invent a second simulation outcome.
- missing infrastructure remains visible; do not hide it with implicit spawns.
- Emergency fallback is generally lower quality and may have costs/consequences.
- environment consequence belongs to Core authority and Save/Load.
- technology/progress emerges from need, observation, knowledge and resources.

## Canonical execution order

1. Character Presentation v1 — DONE via #63.
2. Appearance projection contract — DONE via #65.
3. World Affordance Fallback v1 — DONE via #66.
4. Environmental Residue v1 — DONE via #68.
5. Whole Project Verification v1 — DONE WITH FINDINGS via closed verify-only #69.
6. **Core ↔ World Execution Sync v1 — READY_NOW / Jjun highest priority.**
7. Character Appearance v1 — ACTIVE / #67 closeout. (Dagyeom parallel lane)
8. Character Motion Bootstrap — READY_AFTER_#67.
9. **World Visual Environment v1 — HIGH PRIORITY / READY_AFTER_MOTION_BOOTSTRAP.**
10. Character Motion & Context remainder.
11. Environment Exposure → Health/Memory/Avoidance + sanitation progression.
12. PR #30 Observer UX Polish.
13. PR #36 Mobile Touch.
14. PR #38 Visual Feedback.
15. integrated Core + Observer + Human Character + World Visual runtime verification.
16. Android smoke APK + device profiling.
17. MetaHuman comparison / upgrade decision.
18. Appearance Genetics & Lifecycle.
19. Clothing/Equipment civilization linkage.
20. deeper civilization production chains.

## Frozen legacy

- PR #2 / `task/03-fast-test`: FROZEN.
- old Run `34739283266` failed before Cook/Package/APK.
- do not revive the old multi-hour path.

## Recovery rule

If interrupted: fetch actual GitHub state → reconcile this file + role queue + TEAM_BOARD + HANDOFF_LOG → inspect ACTIVE_LOCK → choose only READY_NOW/PARALLEL_SAFE_NOW → resume from last verified checkpoint.
