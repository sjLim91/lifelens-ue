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

### ASSIST_LOCK-FACILITY-TECH-1 — ACTIVE
- requester/implementer: Jjun, 사용자 승인으로 Core + World + Character + UI + WorldPresentation을 한 흐름으로 공동 구현.
- owner lanes: Dagyeom — UI / Observer presentation, Character presentation, Environment / maps / world presentation.
- milestone branch history: `jjun/facilities-tools-technology-v1` → `jjun/tool-effectiveness-v1` → `jjun/dig-strike-tools-v1` → `jjun/fire-heat-firepit-v1` → `jjun/furnace-smelting-v1`.
- current PR: #122 `원시 제련 v1: Furnace→CopperSmelting→Copper 종단 연결`.
- purpose: 시설·도구·기술 발전을 `필요 인식 -> 관찰/실험 -> 발견 -> 재현 -> 실제 제작/건설 -> 실제 효율/세계 변화 -> 표현/Observer -> Save/Load`의 종단 흐름으로 구현한다.
- delivered first slice: PR #118 merged to `main` as `c3de18790bd19ff7068971b5edc1f475c016048c`; New Game 시설 0개 원칙을 유지하면서 PrimitiveStorage를 필요 인식 → 발견 → Core 위치 선정 → 재료 운반 → 건설 ACK → Storage 활성화 → Save/Load → World/Character/UI 표현까지 연결했다.
- delivered tool-effectiveness slice: PR #119 merged to `main` as `c59a3e2338f23f658a4bde18e9fabd1ebec13687`; ToolCapability / quality / durability를 실제 채집 효율과 마모/파손에 연결하고, Pending Gather가 실제 사용할 도구를 Bridge DTO에 노출해 Gather 모션과 손 도구 proxy를 연결했다.
- delivered dig/strike slice: PR #120 merged to `main` as `28d2f44ae81c7eb140015cb664b4e9d40a4ed16c`; `DiggingStick` / `StoneHammer`를 무료 지급 없이 실험·발견·재현·제작 루프에 추가하고 Dig / Strike capability를 Clay 및 Stone/Flint/Bone/Ore 채집 효율·마모와 손 도구 표현에 연결했다.
- delivered fire slice: PR #121 merged to `main` as `da40073d327ef00aff9646000fcdc30eda9d93d8`; FireMaking `Reproducible` 지식을 전제로 FirePit을 Stone 5 + Wood 2 + Work 6으로 실제 건설하고, Fuel → Ignite → simulation-minute burn → Heat → Charcoal → Collect를 Core 권위로 연결했다. fire runtime은 snapshot v4로 저장되고 Bridge/Context/WorldPresentation은 authoritative 상태만 소비한다.
- current furnace/smelting slice: 운영 FirePit + FireMaking / StoneHammer / SimpleContainer 지식을 전제로 Furnace를 Stone 8 + Clay 6 + Work 12로 실제 건설한다. 운영 Furnace 앞에서 CopperOre + Charcoal을 소비하는 `SmeltCopperOre` 실험으로 `CopperSmelting`을 발견한 뒤, 반복 생산은 Furnace의 LoadSmeltCharge → Ignite → 45 simulation-minute smelt → CopperMetal → CollectMetal runtime만 사용한다. Tin/Iron은 다음 합금·고온 제련 단계까지 ore 상태로 남긴다.
- furnace persistence v1: civilization snapshot extension v5가 Furnace `oreUnits / metalUnits`를 v4 fire runtime 뒤에 append하며 v1~v4 read compatibility를 유지한다.
- hand-tool presentation v1: SharpFlake / StoneCuttingTool / SimpleContainer / DiggingStick / StoneHammer를 authoritative pending-tool DTO로만 표시한다. 현재 engine basic mesh proxy는 추후 art asset 교체 지점이다.
- fire presentation v1: 기존 Android-safe facility HISM을 재사용해 FirePit 돌 고리 / 장작 / 목탄 / lit flame proxy를 표시한다. 불/연료/열/목탄 상태는 Presentation에서 생성하거나 보정하지 않는다.
- furnace presentation v1: 같은 Android-safe HISM 계층을 재사용해 Furnace 몸체/굴뚝/광석 charge/완성 금속/가동 flame proxy를 표시한다. `OreUnits / MetalUnits / HeatLevel / bLit`는 Core read DTO에서만 온다.
- observer labels: DiggingStick / StoneHammer 기존 누락과 CopperMetal / CopperSmelting 한글 라벨을 #122에서 보강한다. 전용 한글 glyph 실기기 검증은 Android Gate B에 유지한다.
- locked Core/World scope: `Source/LifeLensCore/**`, `Source/LifeLens/Simulation/**`, `Source/LifeLens/World/**` 중 시설/건설/도구/기술/자원/제작/공간 권위와 Bridge 계약.
- locked UI scope: `Source/LifeLens/UI/**`, `Content/UI/**` 중 시설·제작·기술 발견·인벤토리·건설·도구 상태 표현. 한글 glyph 실기기 보장은 Android Gate B에서 검증하며 전용 UI 폰트 보강을 Presentation 체크리스트에 포함한다.
- locked Character scope: `Source/LifeLens/Characters/**`, `Content/Characters/**` 중 채집/제작/건설/도구 사용 모션과 손 도구 표현.
- locked WorldPresentation scope: `Source/LifeLens/WorldPresentation/**`, `Content/Environment/**`, `Content/WorldPresentation/**` 중 Core 권위 자원 감소/재생, 건설 시설, 불/연료/작업대/광맥 등 문명 결과 표현.
- Environment 일반 미관 작업과 #8 무관한 캐릭터 외형 작업은 이 락에 포함하지 않는다.
- Presentation은 시설·자원·도구·기술 상태를 생성하거나 보정하지 않고 Core read contract만 소비한다.
- release condition: #8 시설/도구 v1의 Core + Bridge + Character/UI presentation 종단 범위가 Core/Preflight/Unreal 검증을 통과하고 병합된 뒤 해제한다.

Jjun helping Dagyeom defaults to REVIEW_ONLY지만, 위 ACTIVE Assist Lock 범위에서는 사용자 승인에 따라 직접 구현한다. `dagyeom/*` 브랜치에는 직접 push하지 않고 별도 `jjun/*` 브랜치/PR을 사용한다.

## Recently released Assist Locks

### ASSIST_LOCK-SOCIAL-COMM-1 — RELEASED
- requester/implementer: Jjun, 사용자 승인으로 Core + Presentation 통합 구현.
- implementation branch: `jjun/social-communication-localization-v1`.
- PR #117 merged to `main` as `250b3c2ed37ebb7b49e78f5305d429fa3781b7ba`.
- delivered: ACK 이후 Core 권위 recent social feed, Everyday/Meaningful/Important presentation level, 한국어 Observer 라벨/말풍선/Event Feed, 최근 상호작용 detail, social target facing, `Idle_Talking_Loop` context motion.
- validation: Preflight #656 PASS, Core Tests #572 PASS, Unreal Linux Compile #160 UHT/UBT PASS.
- dedicated Korean font asset is not yet present; actual Korean glyph rendering remains an Android Gate B visual-validation item rather than a Core authority blocker.
- release condition satisfied after final-head CI and merge; normal ownership returns except for the new scoped #8 assist lock above.

### ASSIST_LOCK-CHARACTER-OBSTACLE-1 — RELEASED
- requester/implementer: Jjun.
- owner lane: Dagyeom — Character presentation.
- locked file was `Source/LifeLens/Characters/LLResidentCharacter.cpp` only.
- purpose: query-only tree/meaningful-rock collision proxies, constant-speed swept movement, bounded slide/side-step, explicit pebble traversal, stable opposite-side fallback.
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

현재 #8 범위의 Core↔Presentation 변경은 `ASSIST_LOCK-FACILITY-TECH-1` 아래에서 직접 종단 구현하므로 별도 신규 IR 없이 진행한다. 기존 ownership을 벗어나는 #8 무관 작업이 필요할 때만 새 IR을 연다.

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
