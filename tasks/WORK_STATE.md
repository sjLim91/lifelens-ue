# LifeLens Canonical Work State

> 실제 GitHub `main` / target branch / PR / Actions가 항상 최우선 진실이다.
>
> 현재 전체 진행 요약: `docs/PROJECT_PROGRESS_2026-09-15.md`

Canonical product references:
- Master: `docs/LIFELENS_SPEC_v1.1.md`
- Civilization: `docs/CIVILIZATION_PROGRESSION_v1.md`
- Open-ended invention: `docs/OPEN_ENDED_INVENTION_v1.md`
- World affordance/environment: `docs/WORLD_AFFORDANCE_ENVIRONMENT_v1.md`
- World visual environment: `docs/WORLD_VISUAL_ENVIRONMENT_v1.md`
- Environmental visual feedback: `docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md`
- Character appearance: `docs/CHARACTER_APPEARANCE_ROADMAP.md`
- State/collaboration: `docs/STATE_MANAGEMENT.md`, `docs/INTEGRATION_SPRINT.md`
- Whole-project verification baseline: `docs/WHOLE_PROJECT_VERIFICATION_2026-09-14.md`

Last reconciled: 2026-09-15 KST

## Mandatory sync gate

1. 작업 시작 전 actual `main`, target branch/PR, Actions 확인.
2. `WORK_STATE.md` → `TEAM_BOARD.md` → `HANDOFF_LOG.md` 대조.
3. stale 문서가 있으면 코드 작업보다 먼저 갱신.
4. ACTIVE_LOCK 파일은 lock owner 외 수정 금지.
5. 실제 즉시 착수 가능 상태는 `READY_NOW` 또는 `PARALLEL_SAFE_NOW`뿐이다.
6. 검증 전용 PR은 제품 PR로 병합하지 않는다.
7. Compile PASS와 product DONE은 동일하지 않다. merge + state sync까지 완료되어야 DONE이다.

## Current product-code baseline

Latest product merge:
- PR #75 `[CORE] Add environmental exposure perception and avoidance v1`
- merge SHA: `157ce9937e53d5868d7e558b4149a4fa56c4c454`

Validated PR #75 head `c36a31d0f0e1caa069f5389735b3828289d252e9`:
- Structural Preflight: PASS
- Core Tests: PASS, 41/41
- Unreal Linux Compile: PASS
- UE 5.6 UHT / UBT / link: PASS

Documentation-only commits may advance `main` beyond the product-code baseline above.

## Current dispatch

### Jjun lane — World sanitation recommendation integration — WAITING_CI / PR #76

Owner: 쭌 / 쭌 AI
Branch: `jjun/world-sanitation-recommendation-v1`
PR: #76 `[WORLD] Consume Core sanitation recommendation for emergency toilet movement`
Last known HEAD: `e089857710f6ef5430f57940167362ff39d08d64`
Handoff safety: CONDITIONAL — CI completion and merge checkpoint required.

Implemented on PR #76:
- WorldDirector emergency Toilet consumes Core `GetRecommendedOutdoorReliefGridPosition(...)`.
- Core Grid → Unreal target uses the existing `CoreGridCellSizeUU` contract (100 uu/tile default).
- the old independent deterministic 650uu sanitation target is removed.
- emergency Toilet fails closed if Core cannot provide a recommendation instead of inventing a second authority.
- emergency Toilet arrival radius is capped at 0.45 cell / 45uu so the actual World completion point rounds back to the same Core cell.
- existing arrival/use duration → completion ACK flow remains authoritative.
- Structural Preflight guards the recommendation wiring and forbids the old 650uu target.

Validation at this checkpoint:
- Structural Preflight run `34918024951`: PASS.
- Unreal Linux Compile run `34918024929`: IN_PROGRESS; latest checked step is prebuilt UE 5.6 Linux image pull before UHT/UBT/link.
- PR #76 is mergeable at the latest check.

Exact next action:
- finish run `34918024929`.
- if PASS: final diff check → merge #76 → sync state docs.
- if FAIL: record first failing step/root cause and fix before retry.

Acceptance:
- no independent Unreal random sanitation target when Core recommendation exists.
- visible location, ACK GridPos, residue GridPos and future avoidance agree.
- deterministic behavior for same state/seed.
- regression coverage + Preflight + UE compile.

### Dagyeom lane — Character Appearance v1 — ACTIVE / PR #67 CLOSEOUT

Owner: 다겸 / 다겸 AI
PR: #67 `dagyeom/character-appearance-v1`
Latest checked head: `b7941b951df8147627580ff5d4a55a851a705454`

Latest actual workflow lookup for head `b7941b9`:
- no workflow runs returned at the reconciliation checkpoint.

Previously validated/reported checkpoints in PR #67:
- Preflight: PASS.
- Unreal Linux Compile: PASS.
- local PIE / SaveLoad appearance continuity checks reported PASS in the PR body.

Implemented/reported:
- Quaternius CC0 humanoids.
- deterministic #65 appearance projection.
- hair / skin / body variation baseline.
- UAL animation assets.
- Peasant outfit integration.
- humanoid residents visible in PIE.

Remaining before DONE:
- final review / merge / docs sync against the actual latest head.

Known limitation:
- locomotion is not wired; Idle-looking slide remains until Motion Bootstrap.

## Completed foundation / repair slices

### #66 World Affordance Fallback — DONE
- `Preferred → Primitive → Natural → Emergency → Unavailable`.
- no implicit modern facility spawn.
- missing infrastructure remains actually missing.

### #68 Environmental Residue — DONE
- production NEW GAME modern Core SmartObjects removed.
- Core-owned HumanWaste residue.
- accumulation / decay / exposure query.
- binary snapshot v4 environment extension.
- Save/Load + Unreal read DTO.
- emergency Eat/Drink requires real provisions.

### #69 Whole Project Verification — DONE WITH FINDINGS / VERIFY-ONLY CLOSED
- 39/39 Core tests on baseline.
- Preflight PASS.
- UE compile PASS.
- report: `docs/WHOLE_PROJECT_VERIFICATION_2026-09-14.md`.

### #70 Runtime resident / dynamic affordance reconciliation — DONE
- newly projected residents such as births are reconciled after Core advances.
- ActivityAnchors are refreshed instead of BeginPlay-only discovery.

### #71 Unreal compile trigger coverage — DONE
- all `Source/LifeLens/**` and `Source/LifeLensCore/**` C++ paths are covered by the UE compile workflow trigger.
- previous `NEEDS FIX` status is obsolete.

### #72 External physical execution ACK Core contract — DONE
- Core can hold physical intent pending for an external World executor.
- physical outcome does not complete before ACK.

### #73 World physical completion ACK integration — DONE
- actual World arrival/use timing drives ACK.
- Core duration ticks are used.
- actual completion position is sent to Core.
- environmental consequence can use the same resolved position.
- Core Grid ↔ World baseline contract: 100 uu/tile.

### #74 Runtime position restore — DONE
- Core `runtime.pos` is authoritative across snapshot restore.
- newly projected resident actors spawn from Core GridPos.
- external physical execution policy is reasserted after Core replacement.

### #75 Environmental Exposure / Perception / Avoidance — DONE
- contamination perception at Core planning boundaries.
- hygiene burden and emotional discomfort baseline.
- sanitation location Memory.
- duplicate-memory suppression window.
- current exposure + remembered contamination avoidance scoring.
- deterministic low-exposure outdoor sanitation recommendation.
- Save/Load continuity.
- Bridge recommendation API.

## Character / presentation sequence

1. Character Presentation v1 — DONE via #63.
2. Appearance projection contract — DONE via #65.
3. Character Appearance v1 — ACTIVE #67 closeout.
4. Motion Bootstrap — READY_AFTER_#67.
5. World Visual Environment v1 — HIGH PRIORITY / READY_AFTER_MOTION_BOOTSTRAP.
6. Character Motion & Context remainder — AFTER WORLD VISUAL v1.
7. Observer UX Polish / Mobile Touch / presentation feedback afterward.

Motion Bootstrap scope:
- Idle / Walk / Jog or Run.
- basic orientation smoothing.
- remove Idle-sliding.
- Character presentation must not become action authority.

## Environment / civilization next chain

Canonical causal chain:

`Need → Intent → current-world affordance → movement/arrival → action result → environment consequence → exposure/Memory → changed behavior → problem recognition → experiment/discovery → better affordance → culture/civilization`

Immediate sequence:
1. WorldDirector sanitation recommendation integration — PR #76 WAITING_CI.
2. Sanitation Problem Recognition v1 — AFTER #76 DONE.
3. Primitive sanitation discovery / designated area / pit / latrine progression.
4. HumanWaste Environmental Visual Feedback.
5. Health/pathogen and water/soil contamination later.

## Open-ended invention status

Canonical: `docs/OPEN_ENDED_INVENTION_v1.md`.

Status:
- product rule/design: DONE.
- fully generic runtime artifact engine: NOT YET IMPLEMENTED.

Required eventual behavior:
- non-historical / developer-unpredicted artifact forms may emerge.
- material + component + connection + physical/use outcome determine usefulness.
- no global auto-unlock.
- failure contributes to learning.
- physical impossibilities do not become magic inventions.

## Environmental visual feedback status

Canonical: `docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md`.

Rules already fixed:
- simulation/environment state is authoritative.
- visual state follows creation / intensity / decay / removal / SaveLoad restore.
- meaningful physical consequence has a visual representation path.
- Android uses pooling / instancing / LOD / culling / clustering.

Actual visual implementation still pending:
- HumanWaste decal/material/VFX.
- resource depletion/regrowth.
- fire/smoke/scorch.
- foot traffic trails.
- construction/damage state visuals.

## Remaining structural work / known risks

### Unified facility/resource authority — PARTIAL
Core intent/ACK timing is unified, but every actual World facility/resource is not yet one exact shared target registry.

Follow-up:
- facility identity / tier / quality / backing-resource contract.
- civilization-created and authored facilities use one authority path.
- target disappearance / path failure / re-resolve semantics.

### Birth initial physical position — NEEDS FIX
New child runtime position may default to `{0,0}`.

Follow-up:
- derive child initial Core GridPos from gestational parent/household vicinity.
- presentation spawn offsets must not become authority.

### Legacy `ULLDecisionComponent` — REVIEW / REMOVE
Dormant Blueprint-callable competing chooser remains.

### Snapshot legacy migration fixtures — NEEDS TEST
Binary v1-v4 migration exists, but dedicated representative v1/v2/v3 encoded fixture regression coverage remains.

### Health/pathogen environment feedback — LATER
#75 stops at discomfort/hygiene/Memory/avoidance.

### Death presentation policy — UNDECIDED
Core death exists; Unreal body/actor/observer presentation requires an explicit policy.

### Android product validation — NOT DONE
Still required:
- smoke package path.
- Android Cook / Package.
- APK artifact.
- real-device run.
- performance / thermal / memory profiling.

Do not revive the old multi-hour PR #2 path as the default loop.

## Canonical execution order

1. #63 Character Presentation — DONE.
2. #65 Appearance contract — DONE.
3. #66 World Affordance Fallback — DONE.
4. #68 Environmental Residue — DONE.
5. #69 Verification — DONE WITH FINDINGS.
6. #70 runtime residents/dynamic affordances — DONE.
7. #71 compile trigger coverage — DONE.
8. #72 external execution Core ACK — DONE.
9. #73 World ACK integration — DONE.
10. #74 runtime position restore — DONE.
11. #75 environment exposure/perception/avoidance — DONE.
12. **#76 World sanitation recommendation integration — WAITING_CI.**
13. Sanitation Problem Recognition — AFTER #76 DONE.
14. Primitive Latrine / sanitation affordance progression.
15. HumanWaste visual feedback.
16. Character Appearance #67 — ACTIVE in parallel.
17. Motion Bootstrap — after #67.
18. World Visual Environment v1.
19. Character Motion & Context remainder.
20. Integrated runtime verification.
21. Android smoke APK + device profiling.
22. MetaHuman comparison only after mobile baseline validation.
23. deeper open-ended invention / production / health / environment simulation.

## Frozen legacy

- PR #2 / `task/03-fast-test`: FROZEN.
- old Run `34739283266` failed before Cook/Package/APK.
- do not revive the old multi-hour path as normal iteration.

## Recovery rule

If interrupted:
1. fetch actual GitHub main / open PRs / Actions.
2. read `docs/PROJECT_PROGRESS_2026-09-15.md`.
3. reconcile this file + `TEAM_BOARD.md` + `HANDOFF_LOG.md`.
4. inspect ACTIVE_LOCK.
5. choose only READY_NOW / PARALLEL_SAFE_NOW work.
6. resume from the last verified checkpoint, not from stale conversation assumptions.
