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
- PR #78 `[CORE] Add primitive sanitation experimentation progression v1`
- merge SHA: `3a9739682034ad7c5009da5f75a11b0f07fed55b`

Validated PR #78 head `e78f6d211589b74008ff0fdf73e9be1048e2d807`:
- Structural Preflight run `34921390473`: PASS
- Core Tests run `34921390510`: PASS, 43/43
- Unreal Linux Compile run `34921390491`: PASS
- UE 5.6 image verification / UHT / UBT / link: PASS

Delivered by #78:
- resident sanitation-problem Belief becomes a causal input to primitive sanitation experimentation.
- `DesignatedSanitationArea` is a personal civilization technique, not a global tech flag.
- sanitation experiment requires BOTH recognized sanitation concern and a sufficiently clean candidate site.
- existing low-exposure recommendation supplies deterministic environmental feasibility.
- deterministic failure becomes `Hypothesized`; deterministic success becomes personal `Reproducible` knowledge.
- existing witness/teaching provenance and civilization observer/read-model ranges cover the new technique without global auto-unlock.
- existing civilization snapshot extension persists the technique.
- no Toilet/Latrine SmartObject or `LatrineUnlocked` state is spawned by discovery.

Documentation-only commits may advance `main` beyond the product-code baseline above.

## Current dispatch

### Jjun lane — Designated sanitation area affordance integration v1 — IN_PROGRESS

Owner: 쭌 / 쭌 AI
Branch: `jjun/designated-sanitation-affordance-v1`
Dependency: PR #78 DONE.
Handoff safety: SAFE; no uncommitted local dependency.

Goal:
- consume `TechniqueId::DesignatedSanitationArea` knowledge plus the deterministic low-exposure site contract to create an actual authoritative primitive sanitation affordance.
- move the causal chain from discovery/knowledge to a reusable physical behavior target.
- keep the site Core-owned/persisted; Unreal presentation/world execution must consume it rather than inventing a second target.
- do not jump directly to a dug pit or modern latrine before the designated-area behavior is authoritative and working.

Implementation direction:
- define the smallest Core-owned designated sanitation site state with stable identity/location/quality or equivalent facility facts.
- creation requires a resident with reproducible `DesignatedSanitationArea` knowledge and a currently feasible clean site.
- route future Toilet affordance resolution to an existing designated site before natural/emergency outdoor fallback.
- actual use must still complete through the existing World movement/ACK path and deposit environmental residue at the real completion location.
- Save/Load must preserve the created site and its authoritative GridPos.
- target disappearance/invalidation must fail/re-resolve rather than create magic infrastructure.

Acceptance:
- technique knowledge alone does not silently create a site until the creation/use contract executes.
- one authoritative GridPos drives Core preference, World movement and completion consequence.
- no duplicate World-only sanitation target.
- Save/Load restores the site.
- invalid/unavailable site falls back through the existing affordance hierarchy.
- Core tests + Structural Preflight + Unreal compile for Bridge/World contract changes.

Exact next action:
- inspect current SmartObject/facility and external physical execution contracts for the narrowest primitive-site authority representation.
- implement Core-owned designated sanitation site creation/lookup/persistence.
- expose/consume the site through the existing Toilet resolution path without adding a competing World chooser.
- add regression coverage before moving to dug pit/latrine progression.

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
4. Designated sanitation area authoritative affordance integration — IN_PROGRESS.
5. Dug pit / latrine progression after the designated-area loop is physical and persistent.
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
14. #78 Primitive sanitation experimentation progression — DONE.
15. **Designated sanitation area authoritative affordance integration — IN_PROGRESS (`jjun/designated-sanitation-affordance-v1`).**
16. Dug pit / latrine progression.
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
