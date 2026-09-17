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

### ASSIST_LOCK-LIFECYCLE-PRESENTATION-1 — ACTIVE
- requester/implementer: Jjun, 사용자의 계속 진행 승인 범위에서 Lifecycle Presentation Milestone을 종단 구현.
- owner lane: Dagyeom — Character presentation.
- implementation branch: `jjun/lifecycle-presentation-v1`.
- locked Character scope: `Source/LifeLens/Characters/LLResidentCharacter.*`, `Source/LifeLens/Characters/LLResidentAppearanceComponent.*` 중 Core `LifeStage` 소비와 외형/캡슐 스케일 동기화.
- Jjun-owned integration scope: `Source/LifeLens/Simulation/LLSimulationSubsystem.cpp`의 physical compatibility projection에서 Core `bAlive=false` 주민을 제외해 기존 WorldDirector cleanup이 actor/예약/runtime을 제거하도록 연결.
- non-goals: 새 LifeStage/사망 규칙 생성, genetics/identity 재추첨, 새 애니메이션/에셋 추가, UI 미관 변경.
- authority rule: `FLLCoreResidentObservation.bAlive`와 `LifeStage`만 진실로 사용하며 Presentation은 생존/성장 상태를 추정하지 않는다.
- release condition: Preflight + Unreal Linux Compile 통과 및 branch PR 병합 후 해제.

Jjun helping Dagyeom defaults to REVIEW_ONLY지만, 위 ACTIVE Assist Lock 범위에서는 사용자 승인에 따라 직접 구현한다. `dagyeom/*` 브랜치에는 직접 push하지 않고 별도 `jjun/*` 브랜치/PR을 사용한다.

## Recently released Assist Locks

### ASSIST_LOCK-FACILITY-TECH-1 — RELEASED
- requester/implementer: Jjun, 사용자 승인으로 Core + World + Character + UI + WorldPresentation을 한 흐름으로 공동 구현.
- branch history: `jjun/facilities-tools-technology-v1` → `jjun/tool-effectiveness-v1` → `jjun/dig-strike-tools-v1` → `jjun/fire-heat-firepit-v1` → `jjun/furnace-smelting-v1`.
- delivered PRs: #118 PrimitiveStorage, #119 tool effectiveness, #120 DiggingStick/StoneHammer, #121 FirePit/Heat/Charcoal, #122 Furnace/CopperSmelting/CopperMetal.
- final PR #122 merged to `main` as `72e90d5e94085b82806e0b0071f2b81b39cd6f16`.
- final validation: Preflight #692 PASS, Core Tests #649 PASS, Unreal Linux Compile #191 PASS.
- delivered authority chain: 필요 인식 → 실험/발견 → 재현 → 실제 제작/건설 → 실제 효율/세계 변화 → Bridge/Character/UI/WorldPresentation → Save/Load.
- New Game 시설 0개 원칙을 유지하며 PrimitiveStorage / FirePit / Furnace는 모두 Core progression을 통해서만 생성된다.
- ToolCapability / quality / durability는 실제 Gather 효율·마모와 연결되고, Pending Gather DTO가 실제 사용할 손 도구 표현을 공급한다.
- FirePit은 Fuel → Ignite → minute burn → Heat → Charcoal → Collect를 Core 권위로 수행한다.
- Furnace는 CopperOre + Charcoal → CopperSmelting 발견 → LoadSmeltCharge → Ignite → 45-minute smelt → CopperMetal → CollectMetal을 Core 권위로 수행한다.
- snapshot extension은 facility/fire/furnace runtime을 보존하며 이전 extension version read compatibility를 유지한다.
- Presentation은 시설·자원·도구·기술 상태를 생성하거나 보정하지 않고 Core read contract만 소비한다.
- dedicated Korean font asset/Android glyph 확인은 Gate B 실기기 검증 항목으로 남는다.
- release condition satisfied; UI/Character/WorldPresentation ownership은 Dagyeom 기본 규칙으로 복귀한다.

### ASSIST_LOCK-SOCIAL-COMM-1 — RELEASED
- requester/implementer: Jjun, 사용자 승인으로 Core + Presentation 통합 구현.
- implementation branch: `jjun/social-communication-localization-v1`.
- PR #117 merged to `main` as `250b3c2ed37ebb7b49e78f5305d429fa3781b7ba`.
- delivered: ACK 이후 Core 권위 recent social feed, Everyday/Meaningful/Important presentation level, 한국어 Observer 라벨/말풍선/Event Feed, 최근 상호작용 detail, social target facing, `Idle_Talking_Loop` context motion.
- validation: Preflight #656 PASS, Core Tests #572 PASS, Unreal Linux Compile #160 UHT/UBT PASS.
- dedicated Korean font asset is not yet present; actual Korean glyph rendering remains an Android Gate B visual-validation item rather than a Core authority blocker.
- release condition satisfied after final-head CI and merge.

### ASSIST_LOCK-CHARACTER-OBSTACLE-1 — RELEASED
- requester/implementer: Jjun.
- owner lane: Dagyeom — Character presentation.
- locked file was `Source/LifeLens/Characters/LLResidentCharacter.cpp` only.
- purpose: query-only tree/meaningful-rock collision proxies, constant-speed swept movement, bounded slide/side-step, explicit pebble traversal, stable opposite-side fallback.
- Core action/target authority was not changed by the obstacle PR.
- release condition satisfied after final-head compile completion and merge; normal Character ownership returns to Dagyeom.
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
- Observer Level 2 consumes authoritative Needs, Personality, Traits/Preferences, Skills, Relationships, Family, Knowledge/Gear data.

### ASSIST_LOCK-UI-CAMERA-1 — RELEASED
- requester/implementer: Jjun.
- owner lane: Dagyeom — UI / Observer presentation.
- locked files were `Source/LifeLens/UI/LLObserverPlayerController.h`, `Source/LifeLens/UI/LLObserverPlayerController.cpp`.
- purpose: Observer Camera Control v1 input routing only — PC wheel/right-drag/middle-drag and Android tap-vs-drag/pinch/two-finger pan.
- canonical contract: `docs/OBSERVER_CAMERA_CONTROL_v1.md`.
- original PR #105 was superseded by refreshed PR #109.
- PR #109 merged to `main` as `47f75d2cb7fd93b26b8b2082bf9f30cfafe67bed`.
- release condition satisfied.

## Open Integration Requests

현재 open Integration Request 없음. `ASSIST_LOCK-LIFECYCLE-PRESENTATION-1` 범위의 cross-owner Character 수정은 해당 lock으로 직접 구현한다.

## Recently resolved Integration Requests

### IR-D — typed Context Action consumer — RESOLVED
- provider PR #103, spatial follow-up #111, Action Completion #116이 병합됨.
- Social Context Motion consumer는 PR #117에서 `ASSIST_LOCK-SOCIAL-COMM-1` 범위로 완료됨.
- Character Presentation은 Core target/action truth를 소비하며 ACK를 우회하거나 결과를 만들지 않는다.

### IR-B — character facing / backwards-walk issue — RESOLVED
- PR #102 corrected Quaternius visual forward axis and merged as `d3c87996499b353c742ce97fd90978633b9b7ca0`.

### IR-A — WorldPresentation owner path — RESOLVED
- `Source/LifeLens/WorldPresentation/**` established as Dagyeom-owned presentation path.
- World Visual implementation merged in PR #90.
- presentation consumes authoritative read contracts only.

### IR-C — production map + observer framing — RESOLVED
- production map `/Game/Maps/LifeLensWorld` supplied by Dagyeom lane.
- default/startup map and observer camera integration completed in PR #96.
- squash merge: `60432fd102fed327d9e7ae8b4c3ad8727aff476c`.

## Integration Request rule

Open a new request only when one owner actually needs another owner to change a file/API/config outside the requester's lane and no active scoped Assist Lock covers it.

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
