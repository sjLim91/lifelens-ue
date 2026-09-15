# LifeLens Team Board

이 파일은 **ownership / active locks / Integration Requests**만 기록한다. 진행 상태는 `tasks/WORK_STATE.md`, 큰 순서는 `docs/DEVELOPMENT_MILESTONES.md`, 대화로 확정된 지속적 판단은 `docs/DECISION_LOG.md`를 따른다.

## Ownership

| Lane | Owner | Default scope |
|---|---|---|
| Core / AI / Simulation / World / Save / Bridge | Jjun | `Source/LifeLensCore/**`, `Source/LifeLens/AI/**`, `Source/LifeLens/Simulation/**`, `Source/LifeLens/World/**` |
| Build / CI / Android / Config integration | Jjun | `.github/workflows/**`, build pipeline, `Config/**`, startup/default map, project plugin integration |
| UI / Observer presentation | Dagyeom | `Source/LifeLens/UI/**`, `Content/UI/**` |
| Character presentation | Dagyeom | `Source/LifeLens/Characters/**`, `Content/Characters/**` |
| Environment / maps / world presentation | Dagyeom | `Source/LifeLens/WorldPresentation/**`, `Content/Environment/**`, `Content/Maps/**`, `Content/WorldPresentation/**` |

Authority rule: Core/World owns simulation truth. UI/Character/Environment/WorldPresentation presents it and must not duplicate authority.

## Current Assist Locks

### ASSIST_LOCK-UI-CAMERA-1 — ACTIVE
- requester/implementer: Jjun.
- owner lane: Dagyeom — UI / Observer presentation.
- exact locked files: `Source/LifeLens/UI/LLObserverPlayerController.h`, `Source/LifeLens/UI/LLObserverPlayerController.cpp`.
- purpose: Observer Camera Control v1 input routing only — PC wheel/right-drag/middle-drag and Android tap-vs-drag/pinch/two-finger pan.
- canonical contract: `docs/OBSERVER_CAMERA_CONTROL_v1.md`.
- implementation branch: `jjun/observer-camera-control-v1`.
- exclusions: HUD layout/polish, #30/#36/#38 consolidation, Character presentation, Core simulation authority.
- release condition: camera-control PR merged or abandoned and the two locked files are synchronized back to main.

Jjun helping Dagyeom defaults to REVIEW_ONLY. Do not direct-push `dagyeom/*`; use assist branch/PR when a real cross-owner code change is required.

## Open Integration Requests

### IR-D — consume typed Context Action contract in Character Presentation — OPEN
- requester: Jjun / Core-Bridge lane after product Context Motion requirement.
- needed owner: Dagyeom — Character presentation.
- provider PR #103 is **MERGED** as `cba58803c67f971eb1273aaf4e35f5c22b980f01`.
- provider validation: Core Tests #490 PASS, Preflight #587 PASS, Unreal Linux Compile #118 (`34986684115`) PASS.
- consumer API: `FLLCoreActionDirective` in `Source/LifeLens/Simulation/LLCoreActionTypes.h` via `ULLCoreBridgeSubsystem::GetResidentActionDirective`.
- actual civilization action set exposed by Core: `Gather / Store / Experiment / Craft` only.
- directive also exposes authoritative material/item/technique/quantity/result/action-minute and stable resource/storage IDs.
- `bHasCivilizationSpatialTarget` is false for ordinary Gather/Store because current Core resource/storage entities do not own positions; do not guess a nearby scenery target.
- sanitation-site work is an exception when Core actually supplies the real site position.
- requested Character work: Context Motion Router consumes the directive and selects validated talking/sit/interact/pickup/kneeling/fallback motions without creating simulation truth.
- no Jjun direct changes to `Source/LifeLens/Characters/**`; 0 assist locks outside the explicit camera assist above.
- no GitHub review request is required merely because Dagyeom will consume the API.
- status: **PROVIDER DONE / PRESENTATION CONSUMPTION READY**. Does not block Core work.

## Recently resolved Integration Requests

### IR-B — character facing / backwards-walk issue — RESOLVED
- owner: Dagyeom — Character presentation.
- PR #102 changed `MeshForwardYawOffsetDegrees` from `+90` to `-90` after imported-skeleton axis measurement proved Quaternius visual forward is local `+Y`.
- `ll.DebugMotion` A/B evidence: old `+90` produced `facing=-1.00`; corrected `-90` produced `facing=+1.00` on all sampled moving frames.
- PIE confirmed residents face the travel direction and the reproduced backwards-walk issue is resolved.
- Preflight #577: PASS.
- Unreal Linux Compile #117 (`34979601112`): PASS.
- squash merge: `d3c87996499b353c742ce97fd90978633b9b7ca0`.
- Core/World movement authority and actor yaw were not changed.
- remaining stationary-target and male/female/outfit checks are ordinary visual regression QA, not an open IR-B blocker.

### IR-A — WorldPresentation owner path — RESOLVED
- `Source/LifeLens/WorldPresentation/**` established as Dagyeom-owned presentation path.
- World Visual implementation merged in PR #90.
- presentation consumes authoritative read contracts only.

### IR-C — production map + observer framing — RESOLVED
- production map `/Game/Maps/LifeLensWorld` supplied by Dagyeom lane.
- default/startup map and observer camera integration completed in PR #96.
- #96 product head `2642e72efbc69a01c5f58cff08683b1cea793087` passed Preflight #551 and Unreal Linux Compile #103.
- squash merge: `60432fd102fed327d9e7ae8b4c3ad8727aff476c`.
- remaining camera/aesthetic confirmation is visual QA, not an open Integration Request.

## Integration Request rule

Open a new request only when one owner actually needs another owner to change a file/API/config outside the requester's lane.

A request must contain:
- requester / needed owner.
- exact file/API/config needed.
- why the existing contract is insufficient.
- target branch/PR or asset path.
- whether it blocks the current milestone.

Shared/Config change alone does **not** mechanically require Dagyeom review. Request review when the change materially touches Dagyeom-owned behavior, a cross-owner interface/contract, or genuinely needs visual/content validation.
