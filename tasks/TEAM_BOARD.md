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

### ASSIST_LOCK-SOCIAL-COMM-1 — ACTIVE
- requester/implementer: Jjun, 사용자 승인으로 Core + Presentation 통합 구현.
- owner lanes: Dagyeom — UI / Observer presentation + Character presentation.
- implementation branch: `jjun/social-communication-localization-v1`.
- purpose: Social Communication & Localization v1을 Core 이벤트 계약부터 한국어 말풍선/Event Feed/시선·몸 방향/Context Motion 소비까지 한 브랜치에서 종단 구현한다.
- locked UI scope: `Source/LifeLens/UI/**` 중 소셜 표시, 이벤트 피드, 한국어 라벨/말풍선 관련 파일과 이번 작업에서 새로 추가되는 소셜 UI 파일.
- locked Character scope: `Source/LifeLens/Characters/LLResidentCharacter.*`, `LLResidentMotionComponent.*`, 소셜 상호작용 표현을 위해 이번 작업에서 새로 추가되는 Character presentation 파일.
- Appearance/asset import와 Environment/WorldPresentation 일반 작업은 이 락에 포함하지 않는다.
- Core/Bridge truth는 계속 Jjun lane이 소유하며 Presentation은 Core에서 발생하지 않은 고백/갈등/관계 변화를 생성하지 않는다.
- 기존 IR-D Context Motion consumer 작업은 이 락/브랜치에 **흡수**한다. 같은 파일에 대한 병렬 구현은 하지 않는다.
- release condition: Social Communication & Localization v1 최종 CI/PIE 검증 후 PR 병합.

Jjun helping Dagyeom defaults to REVIEW_ONLY지만, 위 ACTIVE Assist Lock 범위에서는 사용자 승인에 따라 직접 구현한다. `dagyeom/*` 브랜치에는 직접 push하지 않고 별도 `jjun/*` 브랜치/PR을 사용한다.

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

### IR-D — consume typed Context Action contract in Character Presentation — ABSORBED INTO ASSIST_LOCK-SOCIAL-COMM-1
- requester: Jjun / Core-Bridge lane after product Context Motion requirement.
- needed owner: Dagyeom — Character presentation.
- original provider PR #103 is **MERGED** as `cba58803c67f971eb1273aaf4e35f5c22b980f01`.
- spatial-authority follow-up PR #111 is **MERGED** as `dda35bac7cef4a88433f4f6a362b89c1b3ea2eee`.
- Action Completion Unification PR #116 is **MERGED** as `8492a5d4d3f65bce83530badf8ed74cf464e08a8`.
- consumer API: `FLLCoreActionDirective` via `ULLCoreBridgeSubsystem::GetResidentPendingContextDirective` / `GetResidentActionDirective`.
- Character Presentation must consume Core target/action truth and must never manufacture relationship/resource/care outcomes or bypass ACK.
- requested Context Motion Router and social interaction presentation are now implemented under `ASSIST_LOCK-SOCIAL-COMM-1` on `jjun/social-communication-localization-v1`.
- status: **PROVIDER DONE / CONSUMER WORK ABSORBED INTO CURRENT INTEGRATED MILESTONE**.

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