# LifeLens Team Board

이 파일은 **ownership / active locks / Integration Requests**만 기록한다. 진행 상태는 `tasks/WORK_STATE.md`, 큰 순서는 `docs/DEVELOPMENT_MILESTONES.md`를 따른다.

## Ownership

| Lane | Owner | Default scope |
|---|---|---|
| Core / AI / Simulation / World / Save / Bridge | Jjun | `Source/LifeLensCore/**`, `Source/LifeLens/AI/**`, `Source/LifeLens/Simulation/**`, `Source/LifeLens/World/**` |
| Build / CI / Android / Config integration | Jjun | `.github/workflows/**`, build pipeline, `Config/**`, startup/default map, project plugin integration |
| UI / Observer presentation | Dagyeom | `Source/LifeLens/UI/**`, `Content/UI/**` |
| Character presentation | Dagyeom | `Source/LifeLens/Characters/**`, `Content/Characters/**` |
| Environment / maps / world presentation content | Dagyeom | `Content/Environment/**`, `Content/Maps/**`, `Content/WorldPresentation/**` |

Authority rule: Core/World owns simulation truth. UI/Character/Environment presents it and must not duplicate authority.

## Current milestone

`World Visual Milestone A — READY_NOW / START AUTHORIZED`

- Gate A is closed by project-owner acceptance using the successful Core/World runtime evidence.
- the additional manual PIE visual inspection was waived for Gate A, not falsely recorded as PASS.
- skipped visual concerns are now validation targets inside World Visual Milestone A.
- Dagyeom may begin World Visual product work now.
- Jjun remains integration support for Config/Bridge/project-level requests and verified authority blockers.

## Current Assist Locks

**0 active locks.**

Jjun helping Dagyeom defaults to REVIEW_ONLY. Do not direct-push `dagyeom/*`; use assist branch/PR when a real code change is required.

## Integration Requests

### Open formal requests

**0.**

### Expected World Visual config handoff — READY WHEN MAP EXISTS

When Dagyeom creates the production map:
1. Dagyeom provides the exact asset path, e.g. `/Game/Maps/LifeLensWorld`.
2. Jjun owns `Config/DefaultEngine.ini` default/startup map integration.
3. If Unreal Water or another plugin requires `LifeLens.uproject` changes, Dagyeom opens an explicit Integration Request with the required plugin and reason.
4. Content work should not silently edit Jjun-owned Config/project integration files.

Remember: an asset stored as `Content/Maps/MyMap.umap` is referenced by Unreal as `/Game/Maps/MyMap`, not `/Content/Maps/MyMap`.

## Request format

An Integration Request must contain:
- requester / owner needed.
- exact file/API/config needed.
- why existing contract is insufficient.
- target branch/PR or asset path.
- whether it blocks current milestone.

Requests are closed when the owning lane merges the required integration and both sides can consume it.
