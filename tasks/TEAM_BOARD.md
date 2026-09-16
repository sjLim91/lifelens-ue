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

**None.**

Current Jjun Core task `Lifecycle Core Correctness v1` is entirely inside the Jjun-owned Core/Simulation lane and does not require a cross-owner assist lock.

Jjun helping Dagyeom defaults to REVIEW_ONLY. Do not direct-push `dagyeom/*`; use assist branch/PR when a real cross-owner code change is required.

## Recently released Assist Locks

### ASSIST_LOCK-CHARACTER-OBSTACLE-1 — RELEASED
- requester/implementer: Jjun.
- owner lane: Dagyeom — Character presentation.
- implementation branch: `jjun/world-obstacle-collision-v1`.
- PR #114 merged to `main` as `3a544fc36d743d9afa1b3ec3100f4577db13ba75`.
- locked file was `Source/LifeLens/Characters/LLResidentCharacter.cpp` only.
- delivered: query-only tree/meaningful-rock collision proxies, constant-speed swept movement, bounded slide/side-step, explicit pebble traversal, stable opposite-side fallback.
- Core action/target authority was not changed by the obstacle PR.
- release condition satisfied after final-head compile completion and merge; normal Character-file ownership returns to Dagyeom.
- PIE/APK feel QA remains a product-validation item, not an ownership lock.

### ASSIST_LOCK-UI-OBSERVER-DATA-1 — RELEASED
- requester/implementer: Jjun.
- owner lane: Dagyeom — UI / Observer presentation.
- implementation branch: `jjun/observer-data-completeness-v1`.
- purpose: complete Observer resident-detail fidelity end-to-end while keeping Core/World as the only simulation authority.
- PR #112 merged to `main` as `27ba0aa147fc38ad05cf388e9390dd2dcaccdf30`.
- validation: Core Tests #515 **PASS, 52/52** including `test_traits_preferences`; Preflight #627 **PASS**; Unreal Linux Compile #137 **PASS**.
- delivered Core provider: `Source/LifeLensCore/include/lifelens/TraitsPreferences.h` -> `ULLCoreBridgeSubsystem::GetResidentTraitPreferenceObservation` -> Observer detail.
- delivered trait dimensions: Resilience / Creativity / Discipline / Compassion / Adaptability / Boldness / Perseverance / Resourcefulness.
- delivered preference dimensions: Socializing / Solitude / Exploration / Crafting / Gathering / Comfort / Novelty / Order.
- Traits/Preferences are deterministic Core read models derived from persistent Personality/Genetics, reproduce through Save/Load, and do not create a second mutable authority.
- the same Core profiles now influence ordinary Social/Civilization utility with bounded disposition effects; urgent Hunger/Thirst provision gathering remains outside ordinary preference modulation so survival priority is preserved.
- Observer Level 2 also consumes authoritative numeric Needs, all 14 Core personality dimensions, civilization Skills, directional Relationships, Family, and Knowledge/Gear data.
- release condition satisfied: normal Observer layout/styling/mobile-maintenance ownership returns to Dagyeom while the authoritative data contract remains fixed by `docs/OBSERVER_RESIDENT_DETAIL_DATA_v1.md`.

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
- obstacle-collision follow-up: the authoritative `GridPos` is the target locus, not a requirement for the resident capsule to overlap the exact visual/collider centre. If a tree/rock/storage visual has a blocking collision proxy, Character Presentation must stop within a truthful interaction radius outside that blocker, face/interact with the same Core target, and must not move, replace, or fabricate the Core coordinates.
- civilization snapshot extension v2 persists resource/storage positions while retaining v1 read compatibility; generated resource positions can still resolve from immutable natural patches for legacy v1 data.
- sanitation-site work continues to use the real Core-supplied site position.
- requested Character work: Context Motion Router consumes the directive and selects validated talking/sit/interact/pickup/kneeling/fallback motions without creating simulation truth.
- no Jjun direct changes to `Source/LifeLens/Characters/**` outside an explicit assist lock.
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

## Dagyeom Integration Request — IR-E 관찰 시야 (2026-09-16, 갱신)

다겸 측 추가. 2026-09-16 최신 `main`(`0a80144`)에서 재확인한 결과로 갱신한다. 최초 제출 당시에는 카메라 제어 PR #109 / #110 병합 이전이었다.

### IR-E-1 — 식생이 카메라와 주민 사이 시선을 가린다 (증상 구체화)

- 갱신된 증상: **Play 직후에는 주민이 보인다.** 이후 식생이 로딩되면서 주민이 가려진다.
- 따라서 초기 카메라 위치·각도 자체의 문제가 아니라, **카메라와 주민 사이 시선 경로에 식생이 생성되는 것**이 원인이다. 최초 제출 시의 "초기 프레이밍이 활동 구역을 담지 못한다"는 서술은 이 관측으로 대체한다.
- 현재 초기 카메라는 `Config/DefaultGame.ini` 기준으로 거리 1.25 청크, 높이 2.0 청크, FOV 55도이며 `(0, -4000, 6400)`에서 `(0, 0, 100)`을 바라본다. 즉 시선은 -Y 쪽에서 정착지 중심으로 들어온다.
- 소유권 판단: 증상이 시선 경로 위 식생이므로 **다겸 소유 WorldPresentation 범위에서 해결 가능하다.** 카메라 쪽 변경 없이 처리할 수 있어 보인다. 쭌 측 판단만 받으면 다겸이 구현한다.

### IR-E-2 — 패닝

- 최초 제출 시점에는 패닝이 없었다. 이후 `47f75d2` / `9093282` / `2249605` 등으로 Observer Camera Control v1이 병합되었고, 현재 `LLObserverPlayerController`에 `PanByScreenDelta`, 우클릭·휠클릭 핸들러, `PanWorldPerPixelDistanceFactor`, 핀치 줌 감도가 존재한다.
- 코드상으로는 해소된 것으로 보인다. **실조작 확인은 미기록.** 다음 PIE 확인 때 우클릭·휠클릭 드래그를 검증하고 결과에 따라 종결한다.

### 다겸 측 완충안 — 초기 시야 원뿔 안의 큰 수목 추가 억제

IR-E-1을 다겸 범위에서 해결하는 구체안이다. 승인 시 `ALLWorldPresentationActor`에 구현한다.

- 원뿔 정의: 꼭짓점은 초기 카메라 위치, 축은 정착지 기준점(`InitialCenterGrid`) 방향, 반각은 화각보다 약간 좁게 둔다. 길이는 카메라에서 정착지까지 구간에 한정한다.
- 억제 대상: `Canopy` 계층만. 큰 수목이 시선을 막는 주된 원인이며 관목·풀·바위는 해당 높이에서 시야를 막지 않는다.
- 억제 강도: 기존 readability envelope과 같은 방식으로 keep factor를 곱한다. 경계에서 급격히 끊기지 않도록 원뿔 가장자리에서 감쇠한다.
- 조건 준수:
  - presentation-only. Core / world generation / resource authority 미변경.
  - `ResourcePatches`는 제거하지 않는다. 필요하면 크기만 줄인다.
  - 결정론 유지. 억제 판정은 위치의 순수 함수이며 기존 해시 스트림을 그대로 쓴다.
  - 기준점은 고정 world origin이 아니라 Core `InitialCenterGrid`.
  - 카메라 프레이밍 값은 `Config/DefaultGame.ini` 소유이므로 읽기만 한다.
- **한계 명시**: 원뿔은 초기 카메라 자세를 기준으로 고정된다. 플레이어가 회전하거나 패닝하면 시선이 비워 둔 쐐기를 벗어나 효과가 사라진다. 즉 초기 관찰 상태를 보장하는 수단이며 일반적인 시야 확보 해법이 아니다. 일반 해법이 필요하면 런타임 카메라 방향을 소비하는 동적 억제나 카메라 쪽 처리(예: 수목 페이드)가 필요하며, 그것은 별도 판단 대상이다.

- 상태: `OPEN / 쭌 결정 대기`. 착수는 회신 후.
