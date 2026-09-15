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

## Dagyeom QA findings — Gate A visual inspection (2026-09-15)

다겸 측 추가. Gate A는 owner waiver로 DONE 처리되었으므로 아래 두 건은 수정하지 않고 증상만 기록한다.

### QA-1 — 주민이 옆으로 걷는다 (Characters lane / 다겸 소유, 승인 후 수정)

- 증상: 이동 중 몸 방향과 실제 이동 방향이 약 90도 어긋난다. 걷기 애니메이션은 전방 보행인데 화면상 이동은 측면이라 게걸음처럼 보인다.
- 재현: NEW GAME 후 주민이 UseToilet 등으로 이동할 때 관찰.
- 판별 결과 — 방향 로직은 정상이다. `ll.DebugMotion` 로그에서 body yaw / 이동 방향 / 액터 yaw가 모두 일치한다. 예: `speed=180.0 yaw=-3.8 travel=-3.8 actor=-3.8`, 같은 구간 위치 변화 `(1853,-274) → (3111,-357)`의 실제 방위각도 -3.8도.
- 실제 원인 — 스켈레탈 메시의 정면 축이 UE 관례(+X)와 다르다.
  - 임포트된 `Superhero_Male_FullBody`의 컴포넌트 공간 바운드는 X 92.9 / Y 14.6 / Z 91.0이다. 좌우로 벌린 팔 길이가 X축에 있으므로 X가 측면 축, Y가 전후 축이다.
  - 원본 glTF 바인드 포즈에서 좌우 손 벡터는 `(1.413, 0, 0)`, 발끝 방향 벡터는 `(0, -0.071, 0.142)`다. 즉 glTF 기준 측면은 X, 정면은 +Z이며, UE 임포트 후에는 정면이 -Y가 된다.
  - 따라서 월드 yaw를 그대로 바디 컴포넌트에 적용하면 메시는 의도한 방향의 90도 옆을 본다.
- 영향 범위: Character Appearance v1부터 존재하던 문제다. 당시에는 바디가 액터 회전을 그대로 상속했고 걷기 애니메이션이 없어 드러나지 않았다. Motion Bootstrap이 보행 사이클을 넣으면서 눈에 보이게 되었다.
- 예상 수정: `Source/LifeLens/Characters/**` 안에서 바디 컴포넌트 yaw에 상수 오프셋(+90도로 추정)을 적용한다. 부호는 PIE 1회 확인으로 확정한다. Core/World 변경 불필요.
- 상태: `OPEN / 다겸 소유 / 쭌 승인 후 착수`.

### QA-2 — UseToilet 이동 중 주민이 바닥 메시 밖으로 나감 (World lane)

- 증상: Day 1 11:36~11:59 구간에서 주민 3명이 바닥 메시 바깥으로 이동했다. HUD는 4 residents로 표시하지만 화면에는 Idle 상태인 1명만 남는다.
- 재현: NEW GAME 후 주민이 UseToilet 목표로 이동하는 구간을 관찰.
- 참고 관측: 헤드리스 재현에서도 주민이 시작 지점에서 X 방향으로 계속 이동해 3,000 유닛 이상 벗어났다. 예: 주민 0의 위치가 `(1853,-274)`에서 `(3393,-105)`까지 단조 증가. 이동 속도는 `RuntimeMoveSpeed` 180 cm/s로 일정하다.
- 다겸 측 판단: 이동 목표와 도착 판정은 `Source/LifeLens/World/**` 권한이며 표현 계층은 위치를 읽기만 한다. 따라서 World lane 소관으로 기록만 한다.
- 상태: `OPEN / 쭌 소유 / 증상 기록만`.
