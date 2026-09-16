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

## Dagyeom — IR-E-1 초기 관찰 가독성 mitigation 기록 (2026-09-16)

다겸 측 추가. 쭌 측 승인 조건(PR #98 코멘트)에 따라 구현하고 PIE로 확인했다. **RESOLVED로 처리하지 않는다.**

### 상태

`MITIGATED — Milestone B 초기 관찰 가독성용. 최종 해법 아님.`

### 구현

초기 관찰 카메라 위치에서 정착지 기준점(`InitialCenterGrid`)으로 향하는 원뿔 안의 `Canopy` 계층만 추가 억제한다. 구현 위치는 `Source/LifeLens/WorldPresentation/LLWorldPresentationActor.{h,cpp}`이며 다른 파일은 수정하지 않았다.

- 카메라 포즈는 `APlayerCameraManager::GetCameraLocation()`으로 한 번 읽어 고정한다. 게임모드가 카메라를 늦게 스폰하므로 첫 빌드에서 놓치면 다음 refresh에서 잡아 1회 재생성한다.
- 원뿔은 카메라에서 정착지까지 구간에만 적용되고 거리에 비례해 넓어져 화면상 일정 각도를 차지한다. 반각 16도, 가장자리 9도 감쇠, 중심 잔존율 0.05.
- `Canopy`만 적용한다. 관목·풀·바위는 해당 높이에서 시야를 막지 않으므로 밀도를 유지한다.

### 승인 조건 준수

- presentation-only. Core / world generation / resource authority 불변.
- `ResourcePatches` 미삭제.
- 초기 카메라와 Config 값은 읽기 전용. `const APlayerCameraManager*`로 위치만 읽으며 쓰기 호출이 없다.
- 위치 기반 결정론 유지. 동일 실행 2회 로그 완전 일치.
- 관목·풀·바위를 과도하게 비우지 않음.

### 검증

- PIE: 식생 로딩 완료 후에도 화면이 트여 있고 관찰 가능한 상태가 유지된다. 이전처럼 주민이 완전히 묻히는 현상은 해소되었다. LEVEL 1 카드와 선택 표시도 정상.
- headless: 로그에 `sightline=<억제 수>/<카메라 포착 여부>` 지표를 추가했다.

### 한계 — 이것이 RESOLVED가 아닌 이유

원뿔은 **초기 카메라 자세에 고정**된다. 플레이어가 회전하거나 패닝하면 시선이 비워 둔 쐐기를 벗어나 효과가 사라진다. 즉 게임 시작 시점의 관찰 상태만 보장한다.

또한 카메라와 정착지 사이 구간 상당 부분이 이미 activity zone이라 기존 envelope이 대부분 솎아낸 상태다. 원뿔이 추가로 걷어내는 양 자체는 적으며, 적은 수의 나무가 결정적으로 작용하는 구조다. 주민은 `CoreGridToWorldSpawnLocation` 기준 같은 셀 안 42 유닛 이내로 뭉치므로 나무 한 그루가 무리 전체를 가릴 수 있다.

### 근본 해법 방향 (쭌 측 권장, 별도 과제)

월드 생성·배치를 카메라에 따라 다시 억제하는 방식이 아니라, **이미 생성된 월드는 그대로 두고 현재 카메라와 관찰 대상 주민 사이를 실제로 가리는 `Canopy` presentation만 런타임에서 되돌릴 수 있게 fade/hide 처리**한다.

- 카메라 이동에 따른 나무 재생성·소멸 pop을 피한다.
- collision과 visual 불일치를 피한다.
- 권한 경계 오염을 피한다.
- 구현 시 카메라 transform과 관찰 대상 접근이 `PlayerCameraManager` 및 기존 Observer API로 충분하면 cross-owner 변경이 필요 없다. 부족하면 그 시점에 새 Integration Request를 올린다.
