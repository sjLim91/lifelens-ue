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

None.

Jjun helping Dagyeom defaults to REVIEW_ONLY. Do not direct-push `dagyeom/*`; use assist branch/PR when a real cross-owner code change is required.

## Recently released Assist Locks

### ASSIST_LOCK-UI-CAMERA-1 — RELEASED
- requester/implementer: Jjun.
- owner lane: Dagyeom — UI / Observer presentation.
- locked files were `Source/LifeLens/UI/LLObserverPlayerController.h`, `Source/LifeLens/UI/LLObserverPlayerController.cpp`.
- purpose: Observer Camera Control v1 input routing only — PC wheel/right-drag/middle-drag and Android tap-vs-drag/pinch/two-finger pan.
- canonical contract: `docs/OBSERVER_CAMERA_CONTROL_v1.md`.
- original PR #105 was superseded by refreshed PR #109.
- PR #109 merged to `main` as `47f75d2cb7fd93b26b8b2082bf9f30cfafe67bed`.
- release condition satisfied: camera-control changes are synchronized to main and the stale #105 branch is closed.

## Open Integration Requests

### IR-D — consume typed Context Action contract in Character Presentation — OPEN
- requester: Jjun / Core-Bridge lane after product Context Motion requirement.
- needed owner: Dagyeom — Character presentation.
- original provider PR #103 is **MERGED** as `cba58803c67f971eb1273aaf4e35f5c22b980f01`.
- spatial-authority follow-up PR #111 is **MERGED** as `dda35bac7cef4a88433f4f6a362b89c1b3ea2eee`.
- #111 validation: Core Tests #502 PASS, Preflight #617 PASS, Unreal Linux Compile #130 PASS.
- consumer API: `FLLCoreActionDirective` in `Source/LifeLens/Simulation/LLCoreActionTypes.h` via `ULLCoreBridgeSubsystem::GetResidentActionDirective`.
- actual civilization action set exposed by Core: `Gather / Store / Experiment / Craft` only.
- directive exposes authoritative material/item/technique/quantity/result/action-minute, stable resource/storage IDs, and Core-owned spatial targets when available.
- `ResourceNode` and `StorageSite` now own authoritative `GridPos`; generated resource nodes preserve the exact `NaturalResourcePatch.pos`.
- ordinary Gather resolves to the actual resource-node position; Store resolves to the actual storage-site position. Character Presentation must consume those targets rather than guess nearby scenery.
- civilization snapshot extension v2 persists resource/storage positions while retaining v1 read compatibility; generated resource positions can still resolve from immutable natural patches for legacy v1 data.
- sanitation-site work continues to use the real Core-supplied site position.
- requested Character work: Context Motion Router consumes the directive and selects validated talking/sit/interact/pickup/kneeling/fallback motions without creating simulation truth.
- no Jjun direct changes to `Source/LifeLens/Characters/**`.
- no GitHub review request is required merely because Dagyeom will consume the API.
- status: **PROVIDER FULLY DONE / PRESENTATION CONSUMPTION READY**. Does not block Core work.

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
