# LifeLens Team Board

이 파일은 **ownership / active locks / Integration Requests**만 기록한다.

- live execution state: `tasks/WORK_STATE.md`
- canonical roadmap: `docs/DEVELOPMENT_MILESTONES.md`
- integrated audit: `docs/INTEGRATED_AUDIT_2026-09-17.md`
- durable decisions: `docs/DECISION_LOG.md`

## Ownership

| Lane | Owner | Default scope |
|---|---|---|
| Core / AI / Simulation / World / Save / Bridge | Jjun | `Source/LifeLensCore/**`, `Source/LifeLens/AI/**`, `Source/LifeLens/Simulation/**`, `Source/LifeLens/World/**` |
| Build / CI / Android / Config integration | Jjun | `.github/workflows/**`, build pipeline, `Config/**`, startup/default map, project plugin integration |
| UI / Observer presentation | Dagyeom | `Source/LifeLens/UI/**`, `Content/UI/**` |
| Character presentation | Dagyeom | `Source/LifeLens/Characters/**`, `Content/Characters/**` |
| Environment / maps / world presentation | Dagyeom | `Source/LifeLens/WorldPresentation/**`, `Content/Environment/**`, `Content/Maps/**`, `Content/WorldPresentation/**` |

Authority rule:

> Core / World owns simulation truth. UI / Character / Environment / WorldPresentation presents it and must not duplicate or invent authority.

## Collaboration model

LifeLens now uses **one integrated roadmap with parallel ownership lanes**.

Rules:
- Jjun establishes Core/World truth and read/interaction contracts.
- Dagyeom consumes those contracts for Character/UI/WorldPresentation.
- each owner works in their own lane by default.
- if one owner must directly modify another owner's lane, first create an Integration Request or explicit scoped Assist Lock.
- Jjun does not push directly to `dagyeom/*` branches.
- old branches are not merged wholesale; missing functionality is reimplemented from current `main` when needed.

## Current Assist Locks

**None.**

Normal ownership is fully restored to each lane after PR #125.

## Current owner work references

These are not locks; they are coordination references only.

### Dagyeom — PR #100 World Readability Envelope
- branch: `dagyeom/world-visual-readability-envelope`.
- owner lane: Environment / WorldPresentation.
- based on current #125 main.
- presentation-only settlement readability envelope.
- Core resource/position/action authority is unchanged.
- merge only after latest exact-head required CI passes.

### Dagyeom — PR #98 Observer Readability / QA View
- branch is stale relative to current main.
- **do not merge as-is**.
- useful ideas may be selectively reimplemented on latest main under normal Dagyeom UI ownership.

### Jjun — next functional provider milestone
- Knowledge Transmission Spatial Authority v1.
- no cross-owner lock required unless the implementation later needs Character/UI/WorldPresentation changes beyond existing contracts.

## Recently released Assist Locks

### ASSIST_LOCK-LIFECYCLE-PRESENTATION-1 — RELEASED
- requester/implementer: Jjun under explicit user continue approval.
- owner lane temporarily touched: Dagyeom Character presentation.
- implementation branch: `jjun/lifecycle-presentation-v1`.
- PR #125 merged to `main` as `e6e005ba1180a4152476ab9b7a19ad9953c07287`.
- delivered:
  - living-only physical compatibility projection;
  - deceased actor/runtime cleanup through WorldDirector reconciliation;
  - LifeStage-driven body scale synchronization;
  - capsule scale synchronization while preserving ground contact;
  - no identity/genetics reroll.
- authority source remains `FLLCoreResidentObservation.bAlive` / `LifeStage`.
- release condition satisfied after Preflight #701 PASS + Unreal Linux Compile #197 PASS + merge.
- Character ownership returns to Dagyeom.

### ASSIST_LOCK-FACILITY-TECH-1 — RELEASED
- requester/implementer: Jjun, user-approved Core + World + Character + UI + WorldPresentation vertical slice.
- branch history: `jjun/facilities-tools-technology-v1` -> `jjun/tool-effectiveness-v1` -> `jjun/dig-strike-tools-v1` -> `jjun/fire-heat-firepit-v1` -> `jjun/furnace-smelting-v1`.
- delivered PRs: #118~#122.
- final PR #122 merged as `72e90d5e94085b82806e0b0071f2b81b39cd6f16`.
- delivered authority chain: need/problem -> experiment/discovery -> reproducibility -> actual craft/build -> real efficiency/world effect -> Bridge/Character/UI/WorldPresentation -> Save/Load.
- New Game starts with zero fabricated facilities.
- ownership returned to normal lanes after merge.

### ASSIST_LOCK-SOCIAL-COMM-1 — RELEASED
- requester/implementer: Jjun.
- PR #117 merged as `250b3c2ed37ebb7b49e78f5305d429fa3781b7ba`.
- delivered: ACK-backed social event feed, Korean Observer labels/bubbles, recent interaction detail, target facing, `Idle_Talking_Loop` context motion.
- UI/Character ownership returned to Dagyeom.

### ASSIST_LOCK-CHARACTER-OBSTACLE-1 — RELEASED
- requester/implementer: Jjun.
- owner lane temporarily touched: Dagyeom Character presentation.
- purpose: immediate obstacle clipping mitigation before final Core obstacle authority normalization.
- later #123 replaced presentation-derived obstacle truth with Core/World-owned obstacle read models.
- Character ownership returned to Dagyeom.

### ASSIST_LOCK-UI-OBSERVER-DATA-1 — RELEASED
- requester/implementer: Jjun.
- PR #112 merged as `27ba0aa147fc38ad05cf388e9390dd2dcaccdf30`.
- delivered authoritative Needs / Personality / Traits / Preferences / Skills / Relationships / Family / Knowledge/Gear detail data.
- Observer ownership returned to Dagyeom.

### ASSIST_LOCK-UI-CAMERA-1 — RELEASED
- requester/implementer: Jjun.
- PR #109 merged as `47f75d2cb7fd93b26b8b2082bf9f30cfafe67bed`.
- delivered PC and Android Observer camera input routing.
- UI ownership returned to Dagyeom.

## Open Integration Requests

**None.**

Open a new Integration Request only when one owner actually needs another owner to change a file/API/config outside the requester's lane and no active scoped Assist Lock covers it.

## Recently resolved Integration Requests

### IR-D — typed Context Action consumer — RESOLVED
- provider #103, spatial follow-up #111, completion unification #116 merged.
- social presentation follow-up completed in #117.
- Character Presentation consumes Core action/target truth and cannot bypass ACK.

### IR-B — character facing / backwards-walk issue — RESOLVED
- PR #102 corrected Quaternius visual forward axis and merged as `d3c87996499b353c742ce97fd90978633b9b7ca0`.

### IR-A — WorldPresentation owner path — RESOLVED
- `Source/LifeLens/WorldPresentation/**` established as Dagyeom-owned presentation path.
- presentation consumes authoritative read contracts only.

### IR-C — production map + observer framing — RESOLVED
- production map `/Game/Maps/LifeLensWorld` supplied by Dagyeom lane.
- default/startup map and observer camera integration completed in PR #96.

## Integration Request rule

A new request must contain:
- requester / needed owner;
- exact file/API/config needed;
- why the existing contract is insufficient;
- target branch/PR or asset path;
- whether it blocks the current milestone.

Shared/config change alone does not mechanically require cross-owner review. Request review when the change materially touches another owner's behavior, a cross-owner contract, or genuinely needs visual/content validation.

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
