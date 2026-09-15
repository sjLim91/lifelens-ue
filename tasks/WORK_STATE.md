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
- PR #77 `[CORE] Add sanitation problem recognition v1`
- merge SHA: `3ea6048d9e7ac6b31e75faa4f5ca55b5c46f9016`

Validated PR #77 head `26a0af034364f7ae12b636cfc5a991d57fe729ab`:
- Structural Preflight run `34919266787`: PASS
- Core Tests run `34919266571`: PASS, 42/42
- Unreal Linux Compile run `34919266720`: PASS
- UE 5.6 UHT / UBT / link: PASS

Delivered by #77:
- direct sanitation/contamination Memory evidence can become an explicit resident-level sanitation-problem Belief.
- ordinary weak exposure does not instantly create civilization knowledge.
- repeated qualifying direct evidence, or one exceptionally salient direct event, can establish recognition.
- recognition reuses existing Core Memory → Belief authority instead of a global tech/unlock flag.
- evidence re-evaluation is idempotent (`max`, not repeated `+=`).
- environmental perception reports recognized/newly-recognized/confidence.
- snapshot encode/decode/restore preserves recognized Belief.
- no pit/latrine is auto-unlocked; the next slice consumes the recognized concern as input to experimentation/progression.

Documentation-only commits may advance `main` beyond the product-code baseline above.

## Current dispatch

### Jjun lane — Primitive sanitation experimentation / progression — READY_NOW

Owner: 쭌 / 쭌 AI
Dependency: PR #77 DONE.
Handoff safety: SAFE; no uncommitted local dependency.

Goal:
- consume `hasRecognizedSanitationProblem(character)` as a causal input to the next civilization step.
- recognized sanitation concern should motivate experimentation/designation before any durable sanitation affordance exists.
- preserve the canonical causal chain: problem recognition → attempt/experiment → result/failure/discovery → personal knowledge → better sanitation affordance.
- do not create a fixed global Tech Tree or instant `LatrineUnlocked=true` path.

Implementation direction:
- start with the minimum primitive sanitation response: designated waste area and/or dug pit attempt, chosen through existing civilization intent/experiment machinery.
- discovery remains resident/personal knowledge first; propagation comes later.
- resource/material/environment feasibility must gate the result.
- failed attempts remain evidence/learning rather than disappearing.
- only after a successful discovery should a better sanitation affordance become available.

Acceptance:
- residents without recognized sanitation concern do not manufacture sanitation experiments from nothing.
- recognized concern can deterministically bias/seed a sanitation experiment opportunity.
- no global auto-unlock.
- failure and success are both representable and deterministic from the same state.
- successful discovery can become durable personal knowledge and a later affordance input.
- Core tests + Structural Preflight; UE compile only if Unreal-facing contracts change.

Exact next action:
- reconcile existing `CivilizationIntent` / experiment APIs against the new sanitation Belief.
- implement the smallest sanitation-specific experiment/discovery contract without bypassing generic civilization progression.
- add tests for precondition rejection, deterministic attempt, failure/success outcome and no global unlock.

### Dagyeom lane — Character Appearance v1 — ACTIVE / PR #67 CLOSEOUT

Owner: 다겸 / 다겸 AI
PR: #67 `dagyeom/character-appearance-v1`
Latest checked head: `e034fe785ca46dd5cb39fd7d7e8710d677994a38`

Latest actual workflow lookup for head `e034fe7`:
- Structural Preflight run `34919060096`: PASS.
- Unreal Linux Compile run `34919060115`: PASS.

Review status:
- Jjun review `CHANGES_REQUESTED` remains active.
- requested closeout items: remove committed `__pycache__/*.pyc` and add ignore rules; remove duplicate includes; verify/fix bright-skin male Peasant exposed-skin tone consistency.
- no newer remote head or Dagyeom reply was present at the latest reconciliation checkpoint.

Previously validated/reported checkpoints in PR #67:
- Core Tests: PASS on earlier final code head.
- local PIE / SaveLoad appearance continuity checks reported PASS in the PR body.

Implemented/reported:
- Quaternius CC0 humanoids.
- deterministic #65 appearance projection.
- hair / skin / body variation baseline.
- UAL animation assets.
- Peasant outfit integration.
- head-only body derivative to prevent clothing penetration.
- humanoid residents visible in PIE.
- appearance construction moved after resident identity binding, fixing four residents receiving the same invalid-id temporary appearance.

Remaining before DONE:
- close the three review items on a new remote HEAD.
- final CI / review / merge / docs sync.

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

### #76 World sanitation recommendation integration — DONE
- Core recommendation drives actual emergency sanitation target.
- resident physically moves and completes use before ACK.
- ACK/residue location agrees with visible target cell.
- no independent World sanitation target remains.
- Preflight + UE 5.6 UHT/UBT/link PASS.

### #77 Sanitation Problem Recognition v1 — DONE
- direct sanitation Memory evidence promotes to a durable resident sanitation-problem Belief.
- weak single exposure is insufficient; repeated or exceptionally salient direct evidence can qualify.
- unchanged evidence does not inflate support/confidence.
- environmental perception integration + new-recognition event.
- snapshot persistence through existing Character/Belief authority.
- no global tech unlock / no automatic latrine creation.
- Core 42/42 + Preflight + UE 5.6 UHT/UBT/link PASS.
- merge SHA `3ea6048d9e7ac6b31e75faa4f5ca55b5c46f9016`.

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
1. WorldDirector sanitation recommendation integration — DONE #76.
2. Sanitation Problem Recognition v1 — DONE #77.
3. Primitive sanitation discovery / designated area / pit / latrine progression — READY_NOW.
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
12. #76 World sanitation recommendation integration — DONE.
13. #77 Sanitation Problem Recognition — DONE.
14. **Primitive sanitation experimentation / affordance progression — READY_NOW.**
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
