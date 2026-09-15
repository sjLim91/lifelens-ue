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
- PR #79 `[CORE/WORLD] Add authoritative designated sanitation area affordance v1`
- merge SHA: `90f2b4f9cdc480e99886632e09323b2feab9625c`

Validated PR #79 head `49c76863b1957df09b7d814f7117826e92707021`:
- Structural Preflight run `34923460056`: PASS
- Core Tests run `34923459974`: PASS, 44/44
- deterministic harness smoke: PASS
- Unreal Linux Compile run `34923459949`: PASS
- UE 5.6 image verification / UHT / UBT / link: PASS

Delivered by #79:
- `DesignatedSanitationArea` reproducible knowledge can now materialize through the existing Civilization `Craft` axis into a persistent Core-owned primitive sanitation site.
- technique knowledge alone does not silently create world infrastructure.
- Core owns stable sanitation site identity, authoritative GridPos, establisher/minute, active state and use count.
- Core sanitation use resolution prefers an active designated area before unstructured emergency outdoor relief.
- Unreal Bridge carries designated-site identity plus exact Core GridPos; World movement and completion ACK consume the same target.
- designated ACK requires the exact active site id and GridPos; stale/wrong identity or position fails closed.
- actual designated use increments site usage and deposits HumanWaste at the acknowledged Core cell.
- snapshot binary format is v5 and persists designated sanitation site state; older v1-v4 snapshots decode with no designated sites.
- no global `LatrineUnlocked` flag or automatic modern Toilet/Latrine SmartObject was introduced.

Documentation-only commits may advance `main` beyond the product-code baseline above.

## Current dispatch

### Jjun lane — Dug pit / primitive latrine progression — READY_NOW

Owner: 쭌 / 쭌 AI
Branch: not created yet.
Dependency: PR #79 DONE.
Handoff safety: SAFE; no uncommitted local dependency.

Goal:
- evolve the now-authoritative reusable designated sanitation area into a more capable primitive sanitation improvement without jumping directly to modern plumbing.
- keep progression causal: observed problem → personal/shared knowledge → actual materials/work → improved physical affordance.
- preserve Core ownership of facility identity/location/state and the existing World movement/ACK contract.

Implementation direction:
- inspect the current #79 `PrimitiveSanitationSite`, Civilization recipe/material, excavation/tool and environmental consequence contracts before adding state.
- model the smallest physically meaningful improvement step, preferably a dug pit before a more developed latrine.
- require actual knowledge/material/tool/work prerequisites rather than a global unlock or instant upgrade.
- decide whether improvement mutates the existing site or creates a successor facility identity; avoid parallel duplicate authorities.
- define how containment changes HumanWaste exposure/radius/intensity without pretending primitive sanitation removes waste entirely.
- persist/restore improvement state and fail/re-resolve cleanly if the facility becomes invalid.

Acceptance:
- no reproducible knowledge/resources/work → no pit/latrine appears.
- designated area remains the fallback primitive sanitation facility until a real improvement completes.
- improved facility has one authoritative Core identity/GridPos used by World movement and ACK.
- improvement produces a measurable sanitation benefit while retaining realistic residue/consequence.
- Save/Load continuity.
- Core tests + Structural Preflight + Unreal compile for any Bridge/World contract change.

Exact next action:
- inspect `PrimitiveSanitationSite`, civilization recipes/experiments, inventory/tool representation, environmental residue deposition and snapshot extension.
- choose the narrowest dug-pit progression contract and create a new Jjun branch only after that boundary is fixed.

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

### #78 Primitive sanitation experimentation progression v1 — DONE
- recognized sanitation concern + clean candidate site gates `DesignateSanitationArea` experimentation.
- deterministic failure → Hypothesized; deterministic success → personal Reproducible knowledge.
- existing witness/teaching provenance + civilization observer coverage extended to the technique.
- existing civilization snapshot persistence carries the technique.
- no world facility/global unlock is created by discovery alone.
- Core 43/43 + Preflight + UE 5.6 UHT/UBT/link PASS.
- merge SHA `3a9739682034ad7c5009da5f75a11b0f07fed55b`.

### #79 Designated sanitation area authoritative affordance v1 — DONE
- reproducible `DesignatedSanitationArea` knowledge materializes only through actual Civilization Craft execution.
- Core-owned persistent primitive sanitation site with stable id/GridPos/active/useCount.
- one sanitation target contract drives Core preference, Unreal World movement and exact completion ACK.
- wrong/stale site id or GridPos fails closed without mutating needs/residue/useCount.
- designated use creates residue at the real acknowledged site and increments useCount.
- snapshot binary v5 persists site state and v1-v4 remain readable with no site state.
- no global unlock or automatic modern toilet/latrine.
- Core 44/44 + deterministic harness + Preflight + UE 5.6 UHT/UBT/link PASS.
- merge SHA `90f2b4f9cdc480e99886632e09323b2feab9625c`.

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
3. Primitive sanitation experiment / DesignatedSanitationArea discovery — DONE #78.
4. Designated sanitation area authoritative affordance integration — DONE #79.
5. Dug pit / primitive latrine progression — READY_NOW.
6. HumanWaste Environmental Visual Feedback.
7. Health/pathogen and water/soil contamination later.

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
#79 establishes one exact authoritative identity/GridPos path for the designated sanitation site, but every actual World facility/resource is not yet one exact shared target registry.

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
Binary v1-v5 migration support exists as applicable, but dedicated representative old-format fixture regression coverage remains incomplete.

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
14. #78 Primitive sanitation experimentation progression — DONE.
15. #79 Designated sanitation area authoritative affordance — DONE.
16. **Dug pit / primitive latrine progression — READY_NOW.**
17. HumanWaste visual feedback.
18. Character Appearance #67 — ACTIVE in parallel.
19. Motion Bootstrap — after #67.
20. World Visual Environment v1.
21. Character Motion & Context remainder.
22. Integrated runtime verification.
23. Android smoke APK + device profiling.
24. MetaHuman comparison only after mobile baseline validation.
25. deeper open-ended invention / production / health / environment simulation.

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
