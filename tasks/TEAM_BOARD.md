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

## Integration Requests — Dagyeom, World Visual Milestone A (2026-09-15)

다겸 측 추가. 아래 두 건은 승인 전까지 착수하지 않는다. 승인 전에는 콘텐츠 임포트 범위 안에서만 진행한다.

### IR-A — `Source/LifeLens/WorldPresentation/**` 신설 요청 (다겸 소유)

- 요청자: 다겸 / 다겸 AI. 필요한 소유자 결정: 쭌.
- 필요한 경로: `Source/LifeLens/WorldPresentation/**`를 다겸 소유 표현 전용 경로로 신설.
- 근거:
  - World Visual Milestone A 범위에 청크 데이터 소비, HISM 인스턴싱, LOD, 컬링이 포함되어 있어 C++ 코드가 필요하다.
  - 그런데 다겸 소유 범위는 `Content/Environment/**`, `Content/Maps/**`, `Content/WorldPresentation/**`로 콘텐츠뿐이다.
  - 기존 환경 표현 코드(`LLEnvironmentalResidueVisualizerComponent`)는 쭌 소유인 `Source/LifeLens/World/**`에 있다. 다겸 측이 그 경로에 코드를 추가하면 소유 경계를 침범한다.
- 대안과 평가: 블루프린트 전용 구현도 가능하나 스크립트 저작과 텍스트 기반 리뷰가 어려워 권장하지 않는다. 헤드리스 재현과 PR 리뷰 품질이 모두 떨어진다.
- 조건 (다겸 측이 지키겠다고 명시하는 범위):
  - 새 경로는 표현 전용이다. Core/World authority를 만들지 않는다.
  - `ULLCoreBridgeSubsystem`의 읽기 API(`GetWorldGenerationObservation`, `GetNaturalChunkObservation`, `GetEnvironmentObservation` 등)만 소비한다.
  - 시뮬레이션 상태를 저장하거나 복제하지 않고, 행동·이동·자원 존재 여부를 결정하지 않는다.
  - `Source/LifeLens/LifeLens.Build.cs` 변경이 필요하면 별도 Integration Request로 요청한다.
- 대상 브랜치: `dagyeom/world-visual-milestone-a`.
- 현재 마일스톤 차단 여부: **차단한다.** 승인 전까지 런타임 청크 소비 표현을 시작할 수 없다.
- 상태: `OPEN / 쭌 결정 대기`.

### IR-B — 게걸음 수정 착수 승인 요청 (QA-1)

- 요청자: 다겸 / 다겸 AI. 필요한 소유자 결정: 쭌.
- 내용: Gate A 육안 QA에서 발견한 "이동 중 몸 방향이 90도 어긋남" 증상의 수정 착수 승인.
- 증상과 원인 분석은 `## Dagyeom QA findings — Gate A visual inspection`의 QA-1 항목에 기록되어 있다. 방향 로직은 정상이며, 원인은 스켈레탈 메시의 정면 축이 UE 관례 +X가 아니라 -Y인 것이다.
- 수정 범위: `Source/LifeLens/Characters/**` 안에서 바디 컴포넌트 yaw에 상수 오프셋을 적용한다. Core/World 변경은 필요 없다.
- 부호(+90도 또는 -90도)는 PIE 1회 확인으로 확정한다.
- 현재 마일스톤 차단 여부: 차단하지 않는다. 다만 미수정 상태에서는 World Visual 육안 검증 시 캐릭터 방향이 계속 어긋나 보인다.
- 상태: `OPEN / 쭌 승인 대기`.

### IR-C — production map 경로 통합 요청

- 요청자: 다겸 / 다겸 AI. 필요한 소유자 결정: 쭌.
- 맵 애셋 경로: `/Game/Maps/LifeLensWorld` (파일 `Content/Maps/LifeLensWorld.umap`).
- 요청 내용: `Config/DefaultEngine.ini`의 `GameDefaultMap`과 필요하면 `EditorStartupMap`을 위 경로로 통합해 달라. 해당 파일은 쭌 소유라 다겸 측은 수정하지 않았다.
- 맵 구성: SkyAtmosphere / DirectionalLight / SkyLight / ExponentialHeightFog / VolumetricCloud, 24,000 유닛 자연 지면, 자연물 348개 결정론 배치. 자동 생성 문명시설 없음.
- 함께 확인이 필요한 사항 — `ALLLifeLensGameMode::SpawnRuntimeFloor()`가 런타임에 1,400 x 1,400 유닛 바닥(엔진 Cube를 14배 스케일)을 직접 스폰한다.
  - production map이 자체 지면을 제공하므로 이 임시 바닥과 중복된다.
  - 이 바닥은 QA-2(주민이 바닥 메시 밖으로 나감)의 직접 원인으로 보인다. 바닥은 원점 기준 ±700 유닛인데 헤드리스 관측에서 주민은 3,393 유닛까지 이동했다.
  - `Source/LifeLens/Core/**`는 다겸 소유 범위가 아니므로 수정하지 않았다. 제거 또는 조건부 비활성화 여부를 쭌 측이 결정해 달라.
- 현재 마일스톤 차단 여부: 부분 차단. 기본 맵이 전환되기 전까지 PIE 육안 검증은 에디터에서 맵을 직접 열어야 한다.
- 상태: `OPEN / 쭌 결정 대기`.

#### IR-C 추가 관측 2 (런타임 표현 적용 후, 2026-09-15)

- 관찰 카메라가 생성된 숲 바로 앞에 위치해 화면이 나무로 가득 차고 주민이 보이지 않는다.
- Core 생성 월드에서 선택된 시작 청크의 위치와 고정 카메라 위치가 맞지 않는 것으로 보인다. `LLWorldPresentationActor`는 `LLWorldSpatialContract` 기준으로 시작 청크를 표현 원점에 두고 그 청크의 authoritative 밀도값대로 자연물을 배치한다. 즉 표현은 계약대로 배치되어 있고, 카메라가 그 배치 안쪽에 들어가 있는 상태다.
- 카메라는 `Source/LifeLens/Core/**` 소유라 다겸 측은 수정하지 않았다. 시작 청크 기준 프레이밍을 쭌 측이 조정해 주기를 요청한다.
- 참고: 다겸 측에서 조정 가능한 완충 수단으로 표현 액터에 "주민 시작 지점 주변 자연물 제외 반경"을 둘 수 있다. 필요하면 요청해 달라. 다만 그 반경은 authoritative fact가 아니라 가독성용 표현 규칙이므로 임의로 넣지 않았다.

#### IR-C 추가 관측 (1차 육안 확인, 2026-09-15)

- 관찰 카메라 시야: `ALLLifeLensGameMode::SpawnObserverCamera()`가 카메라를 `(0, -1500, 1120)`, 피치 -36도, FOV 55로 고정 스폰한다. 240 m 규모 월드에서는 지면에 너무 가까워 배경을 볼 수 없다. 배경 품질 판정 자체가 불가능하다. `Source/LifeLens/Core/**`는 다겸 소유가 아니므로 수정하지 않았다. 카메라 거리·피치·FOV 조정 또는 Observer 카메라 제어를 쭌 측이 판단해 달라.
- 임시 바닥 재확인: production map에서도 주민이 게임모드 임시 바닥(±700 유닛) 밖으로 나가 화면에서 사라지는 현상이 재현된다. QA-2와 동일 증상이며 새 지면(±12,000 유닛)과 무관하게 발생한다.

