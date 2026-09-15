# LifeLens Canonical Work State

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
> Long-term order: `docs/DEVELOPMENT_MILESTONES.md`.

Last reconciled: 2026-09-15 KST after Gate A owner acceptance and World Visual dispatch.

## Product baseline

### Character Motion Bootstrap — DONE #84
- PR: #84
- validated product head before docs-only review reconciliation: `fa781b0829a29f8b29b99fe19fe699cc53792966`
- Preflight `34940159290`: PASS
- Unreal Linux Compile `34940159300`: PASS
- PIE visual verification: PASS — T-pose resolved, Idle/Walk visible, no sliding, smooth turning
- all three review threads resolved before merge
- docs-only helper reconciliation did not change product code
- main merge: `5f0af986728e717c274482915d5f8b5a9ee31195`

### World Generation Milestone A — DONE #87
- PR: #87
- final head: `9de52c43c3048c9260c4d6fe2de2be0f97bed082`
- Preflight `34940403254`: PASS
- Core Tests `34940403079`: PASS, 48/48 + deterministic harness
- Unreal Linux Compile `34940402948`: PASS including UE 5.6 image verification / UHT / UBT / link
- PR comments: 0
- unresolved review threads: 0
- main squash merge: `5a543b7392eca722e794b5504241d46669ac23ab`

Delivered by #87:
- deterministic detailed natural chunk baseline from WorldSeed + GenerationVersion + ChunkCoord.
- WG-2 selected start-region materialization.
- founder physical Core positions inside the selected region.
- generated-chunk registry and snapshot v6 no-reroll persistence boundary.
- read-only Unreal World Generation observation path for presentation.
- safe Core-global → Unreal-local presentation origin mapping.
- zero starting civilization infrastructure.

Documentation/validation-only commits may advance `main` beyond the product merge SHA above without changing product runtime code.

## Integrated Runtime Checkpoint A — DONE

Owner: joint integration; Jjun coordinated runtime/authority evidence.
Status: `DONE — OWNER ACCEPTED WITHOUT ADDITIONAL MANUAL PIE VISUAL QA`
Handoff safety: `SAFE`
Active locks: 0

### Runtime evidence — PASS

Corrected Gate A run:
- Run `34943197031`: **SUCCESS**
- Job `104296410531`: **SUCCESS**
- integrated harness output: `GATE_A_CORE_RUNTIME_PASS`
- founders: 4 total, 2 male / 2 female
- selected/materialized start chunk in replay: `(-12, 6)`
- generated natural chunks at initial slice: 1
- authoritative HumanWaste residues produced by missing-toilet fallback: 1
- snapshot bytes in replay: 13980

Verified in one runtime flow:
- production NEW GAME creates exactly 4 founders.
- all founders initially occupy authoritative positions inside the selected/materialized start region.
- no starting Core objects/storage/primitive sanitation infrastructure.
- Observer world/resident read models expose all 4 residents.
- autonomous missing-toilet fallback creates authoritative HumanWaste.
- Save/Load preserves generated natural chunk baseline without reroll.
- Save/Load restores exact resident positions from the save checkpoint.
- environmental residue and Observer-readable state survive restore.
- immediate snapshot re-encode is byte-identical.
- source/restored simulations remain byte-identical after another 120 minutes.

Supporting runtime contracts also PASS:
- `test_production_new_game`
- `test_world_generation_milestone_a`
- `test_external_physical_execution`
- `test_environmental_residue`
- `test_environmental_exposure`
- `test_core_save_load`
- `test_civilization_observer_read_model`

Evidence artifact from Run `34943197031`: `Gate-A-Core-Runtime-Evidence` / artifact ID `10385548493`.

Run `34943075204` was a **validation-harness expectation failure, not a product failure**. The first harness incorrectly required residents to remain inside the initial chunk after 20 minutes of autonomous movement. The corrected contract compares exact save-checkpoint positions across restore; no product code was changed for this correction.

### Manual visual acceptance policy for this gate

On 2026-09-15, the project owner explicitly chose to **waive the additional Dagyeom PIE visual inspection for Gate A** and proceed.

This is **not** a claim that the skipped visual checks were observed to pass. Instead:
- Gate A is accepted on the available automated/runtime evidence.
- any appearance/spawn/animation/environment visual regression not covered by that evidence is carried forward as Milestone B visual validation risk.
- future visual failures should be fixed in the owning lane without reopening already-proven Core/World runtime contracts unless evidence shows an authority/runtime regression.

## Dagyeom 측 상태 동기화 (2026-09-15, 다겸 측 AI 추가)

아래는 실제 GitHub 상태다. 위/아래 절의 쭌 측 서술은 수정하지 않고 현재 사실만 덧붙인다.

### World Visual Milestone A — 구현분 main 병합됨

- PR #90 `[Environment] World Visual Milestone A` — MERGED, main squash merge `2c4eef487298`.
  - 최종 head `7c56986c` Preflight PASS (`34962229119`), Unreal Linux Compile PASS (`34962229115`).
- 함께 병합된 쭌 측 통합: PR #95 `0b5c1fd8fc0d` (WorldDirector runtime config), PR #96 `60432fd102fe` (LifeLensWorld 연결 + observer camera 상향).

#90이 전달한 것:
- `Source/LifeLens/WorldPresentation/LLWorldPresentationActor` — 생성 월드 관측을 읽어 지면과 자연물을 런타임 생성. 시뮬레이션 authority 미생성.
- 좌표는 `LLWorldSpatialContract` 기준, 시작 청크가 표현 원점.
- 밀도는 청크의 fertility / moisture / traversal에서 도출. `ResourcePatches`는 Core 좌표에 표시.
- 결정론: WorldSeed + GenerationVersion + 청크 좌표. PopulationSeed 비영향.
- Android 예산: HISM 인스턴싱, 종류별 컬 거리, 인스턴스 상한.
- 시작 지점 가독성 반경(presentation-only, 반경 800 / 감쇠 700 / 자원 축소 0.55).
- 에셋: Quaternius Stylized Nature MegaKit [Standard] 68종, ambientCG 지면 3세트. provenance는 `Content/Environment/PROVENANCE.md`.

### Milestone B 수용 조건 중 다겸 측 확인 상태

확인됨:
- 생성 월드 표현이 Core 사실을 소비한다.
- Quaternius 외형 정상.
- 지면·식생·바위 렌더링 정상, 시작 지점 가독성 반경 동작.
- 시작 시 문명 인프라를 표현이 만들어내지 않는다.

미확인 (다음 작업 대상):
- 주민 4명의 스폰 겹침 / 부유 / 매몰 / 맵 이탈 여부.
- HumanWaste 등 환경 피드백 배치의 공간적 타당성.
- Observer 라벨 / 선택 / 상세의 사용성.
- Save → Load 후 표현 일관성.

회귀로 기록된 것:
- 이동 시 몸 방향과 이동 방향 불일치(문워크). 쭌 lane 처리 범위이며 다겸 측은 수정하지 않는다.

### 다겸 측 Integration Request

미해결 **0**. IR-A / IR-B / IR-C 종결 내역은 `tasks/TEAM_BOARD.md`의 종결 기록 절을 따른다.

## Current dispatch — World Visual Milestone A — READY_NOW / START AUTHORIZED

Owner: Dagyeom visual/content lane.
Jjun role: Config / Bridge / integration support only when requested.
Status: `READY_NOW — START AUTHORIZED`

Scope:
- production-oriented generated-world presentation consuming the merged World Generation contracts.
- terrain/biome presentation.
- vegetation / rocks / water / natural dressing.
- chunk presentation/materialization consumption.
- Android-friendly HISM/instancing/LOD/culling/material budget.
- observer readability.
- no visual-only second authority.

Acceptance direction:
- use Core/World read contracts as source of truth.
- no fixed gray-world assumptions where generated-world facts are available.
- no starting modern/civilization infrastructure invented by presentation.
- manual visual inspection now belongs to this milestone and should validate the skipped Gate A visual concerns together with the new world presentation.
- milestone close requires meaningful validation, not just asset creation.

## Dagyeom lane

Status: `WORLD VISUAL MILESTONE A — READY_NOW / START AUTHORIZED`

Dagyeom may begin the product milestone now. When a production map is created, provide the exact `/Game/Maps/<MapName>` asset path through an Integration Request rather than editing Jjun-owned Config.

## Jjun lane

Status: `INTEGRATION SUPPORT / WAIT FOR REQUEST`

Do not start an unrelated World Generation feature in parallel. Support World Visual through existing read contracts and handle only explicit Config/Bridge/project integration requests or verified authority blockers.

## Known near-term integration boundary

Formal Integration Request currently open: **0**.

Expected upcoming handoff once Dagyeom creates a production map:
- Dagyeom supplies `/Game/Maps/<MapName>` asset path.
- Jjun updates `Config/DefaultEngine.ini` (`GameDefaultMap`, and `EditorStartupMap` if needed).
- Water/plugin changes to `LifeLens.uproject` require a separate explicit Integration Request.

## Long compile rule

When a long UE compile/package is started, record HEAD + Run ID and continue other safe work. Do not continuously wait/poll; the user reports completion/failure and then closeout resumes.
