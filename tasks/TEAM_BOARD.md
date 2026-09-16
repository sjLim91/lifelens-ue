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

## Dagyeom Integration Request — IR-E 관찰 카메라 (2026-09-16)

다겸 측 추가. 요청 기록이며 카메라 소유는 쭌 lane이다.

### 현재 동작 확인 결과

PIE에서 다시 확인했다. **휠 줌과 회전은 동작한다.** 아래 두 가지가 문제다.

### IR-E-1 — 초기 카메라 각도가 주민 활동 구역을 담지 못한다

- 증상: Play 시작 시 주민이 나무에 가려 보이지 않는다. 플레이어가 시작하자마자 줌과 회전을 조작해야 주민을 찾을 수 있다.
- PR #100의 vegetation density 조정 이후에도 동일하다. 정착지 중심 1200 UU는 비어 있지만 초기 카메라가 그 지점을 정면으로 담지 않는다.
- 요청: 초기 프레이밍이 주민 활동 구역을 담도록 조정해 달라.
- 참고: `ll.ViewResidents`를 입력하면 주민 4명이 정상적으로 잡힌다. 즉 월드 배치나 표현 문제가 아니라 초기 카메라 위치·각도 문제다. 다만 디버그 명령 없이 기본 화면에서 주민이 보이는 것이 정상 상태여야 한다.

### IR-E-2 — 패닝이 없어 시야를 옆으로 옮길 수 없다

- 증상: 줌과 회전은 되지만 카메라 이동(패닝)이 없다. 초기 지점을 중심으로만 볼 수 있다.
- Observer 관점에서 세계를 둘러보려면 패닝이 필요해 보인다.
- 요청: 관찰 카메라에 패닝 입력 추가 검토.

### 다겸 측 완충안 (요청 수락 전까지의 대안)

카메라를 건드리지 않고 표현 계층에서 할 수 있는 것은 **초기 카메라 시선 방향의 큰 수목을 추가로 억제**하는 것이다.

- `ALLWorldPresentationActor`의 readability envelope에 방향 가중치를 추가해, 정착지 기준 초기 카메라 방향 쪽 canopy만 더 강하게 솎아낸다.
- presentation-only이며 Core authority와 ResourcePatch는 그대로 둔다. 결정론도 유지한다.
- 다만 이것은 대칭적이지 않은 표현이 되고, 카메라가 회전하면 반대쪽에서는 효과가 없다. **근본 해결이 아니라 임시 완충이다.**
- 쭌 측이 IR-E-1을 조정하면 이 완충안은 불필요하다. 필요 여부를 알려 주면 구현한다.

- 상태: `OPEN / 쭌 결정 대기`.
