#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def write(path: str, text: str) -> None:
    p = ROOT / path
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text.strip() + "\n", encoding="utf-8")

AGENTS = r'''# AGENTS.md — LifeLens Agent Entry Point

이 저장소에서 작업하는 모든 AI 에이전트(Codex / ChatGPT / Claude Code 등)는 **이 파일을 가장 먼저 읽는다.**

## 0. Source of truth

우선순위는 항상 다음과 같다.

`actual GitHub main / PR / Actions > canonical repository docs > 이전 채팅 / 기억 / 로컬 추정`

작업 시작·재개 시 순서:
1. `AGENTS.md`
2. `docs/LIFELENS_SPEC_v1.1.md`
3. `docs/DEVELOPMENT_MILESTONES.md`
4. `docs/STATE_MANAGEMENT.md`
5. actual `main` HEAD / target branch / PR / Actions
6. `tasks/WORK_STATE.md`
7. `tasks/TEAM_BOARD.md`
8. `tasks/HANDOFF_LOG.md` 최신 의미 있는 항목

`tasks/DAGYEOM_READY_QUEUE.md`는 과거 링크 호환용이다. **새 dispatch의 canonical source로 사용하지 않는다.**

## 1. Development unit — milestone-sized delivery

LifeLens는 더 이상 같은 목적의 작은 계약을 PR 하나씩 쪼개지 않는다.

> **Same purpose + same layer + same validation scope = one milestone-sized PR.**

- 내부 커밋은 작게 나눠도 된다.
- 서로 다른 authority/layer/owner는 억지로 한 PR에 섞지 않는다.
- 상태 문서와 무거운 UE 검증은 의미 있는 milestone checkpoint에서 한다.
- 작은 진행마다 `WORK_STATE`/`TEAM_BOARD`/`HANDOFF`를 반복 수정하지 않는다.

Canonical milestone roadmap: `docs/DEVELOPMENT_MILESTONES.md`.

## 2. Validation / CI discipline

1. Core / structural validator / Preflight 같은 싼 검증을 먼저 사용한다.
2. Unreal Linux Compile은 milestone close 또는 실제 C++/UHT/UBT interface risk가 있을 때만 사용한다.
3. **이미 검증된 product HEAD에 docs-only review closeout을 push해서 무거운 UE Compile을 다시 발생시키지 않는다.**
4. review comment가 최신 코드에서 이미 해결되어 있으면 reply + resolve만 한다. 코드 변경이 실제 필요한 경우에만 HEAD를 움직인다.
5. docs-only closeout/state sync는 가능하면 product merge 직후 `main`에서 한 번에 정리한다.
6. 장시간 UE Compile/Package가 시작되면 Run ID와 HEAD를 기록하고 **AI가 계속 polling하며 기다리지 않는다.** 사용자가 완료/실패를 알려주면 그때 결과를 확인한다.
7. Compile PASS는 DONE이 아니다. 필요한 merge + canonical state sync까지 완료되어야 DONE이다.
8. 같은 실패를 원인 확인 없이 재실행하지 않는다.
9. 실제 artifact가 없으면 APK/패키징 성공이라고 말하지 않는다.

## 3. Absolute product rules

- Unreal-native runtime. Legacy LOCAL OBSERVER HTML/JS는 요구사항 참고자료일 뿐 런타임 기반이 아니다.
- Android가 첫 실제 제품 타깃이다.
- `LifeLensCore`는 표준 C++17이며 Unreal 타입에 의존하지 않는다.
- 초기 NEW GAME은 남자 2 + 여자 2, 자연환경, **문명 인프라 0**에서 시작한다.
- 이름/특성은 초기 시작 시 새로 부여할 수 있지만 자연 세계는 WorldSeed/GenerationVersion 계약을 따른다.
- 집/화장실/농장/도로/도구 같은 현대/문명 시설을 편의상 마법처럼 생성하지 않는다.
- 캐릭터/UI/그래픽은 presentation이며 Core/World authority를 복제하지 않는다.
- Character animation/root motion이 이동/action authority가 되면 안 된다.
- 실제 world consequence는 가능한 경우 presentation path를 가진다.
- 무료 범위를 벗어나는 서비스/자산을 필수 의존성으로 만들지 않는다.
- MetaHuman은 Android/mobile baseline 검증 뒤 upgrade path로만 둔다. 기본 캐릭터는 Quaternius CC0 Track B.

## 4. Ownership

### Jjun lane
- `Source/LifeLensCore/**`
- `Source/LifeLens/AI/**`
- `Source/LifeLens/Simulation/**`
- `Source/LifeLens/World/**`
- Save/Load / Bridge / build / CI / Android
- `Config/**`와 project startup/default map/plugin integration

### Dagyeom lane
- `Source/LifeLens/UI/**`
- Character appearance/presentation/animation
- `Content/UI/**`
- `Content/Characters/**`
- `Content/Environment/**`
- `Content/Maps/**`
- `Content/WorldPresentation/**`
- Observer visual UX

상대 영역 수정이 필요하면 `tasks/TEAM_BOARD.md`의 Integration Request를 사용한다. Jjun의 Dagyeom 지원은 기본 REVIEW_ONLY이며 `dagyeom/*` direct push 금지다.

## 5. Shared state documents

- `docs/DEVELOPMENT_MILESTONES.md` — 큰 개발 단위와 gate 순서.
- `tasks/WORK_STATE.md` — **현재 active/ready/blocked state만** 기록.
- `tasks/TEAM_BOARD.md` — ownership / active locks / Integration Requests만 기록.
- `tasks/HANDOFF_LOG.md` — 의미 있는 merge/failure/design transition만 append-only 기록.
- `docs/PROJECT_PROGRESS_2026-09-15.md` — 날짜 기준 전체 진행 snapshot.

과거 완료 이력을 `WORK_STATE`나 `TEAM_BOARD`에 길게 복제하지 않는다.

## 6. Review / merge closeout

PR closeout 시:
1. actual PR head/base/mergeability 확인.
2. comments / reviews / unresolved threads 확인.
3. 필요한 validation 확인.
4. 이미 해결된 review는 reply + resolve.
5. product code 변경이 없으면 불필요한 heavy compile을 재유발하지 않는다.
6. merge.
7. milestone/state docs를 **한 번** 동기화.
8. `HANDOFF_LOG`에는 의미 있는 완료/전환만 append.

## 7. Interruption / recovery

세션 중단이나 timeout 뒤에는 이전 행동이 성공했다고 추측하지 않는다. actual GitHub를 다시 조회하고 `WORK_STATE`/`TEAM_BOARD`를 reconcile한 뒤 마지막 검증된 checkpoint에서 이어간다.
'''

STATE = r'''# LifeLens Shared State Management Protocol

목표: 양쪽 사람/AI가 저장소만 보고 **지금 무엇을 해야 하는지** 복구할 수 있게 하되, 상태 문서 유지 비용이 제품 개발보다 커지지 않게 한다.

## 0. Truth order

`actual GitHub > repository canonical docs > chat/memory/local guess`

작업 시작 시 actual `main`, target branch/PR, 관련 Actions를 확인한다. 불일치가 있으면 코드보다 state reconciliation이 먼저다.

## 1. 문서 역할

### `docs/DEVELOPMENT_MILESTONES.md`
- 장기 순서와 milestone acceptance.
- 작은 subtask queue가 아니다.

### `tasks/WORK_STATE.md`
- 현재 ACTIVE / READY_NOW / WAITING_CI / BLOCKED / HOLD만 기록.
- 과거 완료 세부내역을 반복 누적하지 않는다.

### `tasks/TEAM_BOARD.md`
- 담당영역, locks, Integration Requests만 기록.
- 진행상황의 두 번째 복제본으로 사용하지 않는다.

### `tasks/HANDOFF_LOG.md`
- append-only.
- 의미 있는 product merge, 실제 failure/root cause, ownership/design transition만 기록.
- 모든 작은 commit/CI 시작을 기록하지 않는다.

### 역할별 READY queue
- 별도 READY queue는 deprecated.
- 과거 링크 호환 파일은 `WORK_STATE + DEVELOPMENT_MILESTONES`로 redirect한다.

## 2. Status

- `ACTIVE` — 현재 구현/수정 중.
- `WAITING_CI` — 필요한 검증 결과 대기.
- `READY_NOW` — 선행 gate가 끝났고 즉시 착수 가능.
- `READY_TO_MERGE` — 정의된 검증/리뷰 gate 완료.
- `BLOCKED` — 원인이 명시된 장애.
- `HOLD` — 의도적으로 다음 작업을 시작하지 않는 상태.
- `FROZEN` — 폐기하지 않았지만 기본 작업경로에서 제외.
- `DONE` — 검증 + merge + 필요한 canonical state sync 완료.
- `RECOVERING` — 중단 뒤 실제 상태 재대조 중.

## 3. Milestone-sized development

> Same purpose + same layer + same validation scope = one milestone-sized PR.

- 내부 commits는 작게 가능.
- Core/World와 Character/UI 같은 ownership/layer가 다르면 분리한다.
- milestone 시작 시 ACTIVE를 한 번 기록한다.
- 중간에는 Git commits/PR이 진행기록이다.
- 중간 blocker/ownership change가 실제 발생할 때만 state docs를 수정한다.
- milestone close에서 validation + merge + canonical state sync를 한 번 수행한다.

## 4. Checkpoint cadence

상태 문서를 갱신해야 하는 시점:
- milestone 시작/소유권 확정.
- 실제 blocker 또는 중요한 설계/authority 변경.
- merge 또는 명시적 종료.
- recovery 시 actual GitHub와 문서가 달라졌을 때.

다음은 기본적으로 state sync를 요구하지 않는다:
- 모든 작은 commit.
- 단순 CI start.
- review reply만 한 경우.
- 변화 없는 polling.

## 5. CI / long build

- fast gate(Core/validator/Preflight)를 먼저 사용한다.
- heavy UE compile은 milestone close 또는 명확한 interface risk에서만 수행한다.
- long compile 시작 후 Run ID/HEAD를 기록하면 AI가 계속 기다리지 않는다.
- 사용자가 완료/실패를 알려주면 결과를 확인하고 closeout을 이어간다.
- 같은 제품 HEAD가 이미 heavy compile PASS인데 docs-only closeout 때문에 HEAD를 움직여 compile을 다시 유발하지 않는다.
- product code가 실제 바뀌지 않았다면 기존 validated product HEAD 결과를 근거로 closeout할 수 있다. 그 판단 근거를 PR/state에 남긴다.

## 6. Review 처리

review마다 먼저 최신 코드와 comment를 대조한다.

- 이미 최신 코드에서 해결됨 → reply + resolve, push 없음.
- 코드 수정 필요 → owner가 수정 commit, 필요한 검증 수행.
- canonical docs의 stale 상태만 문제 → 가능하면 product PR을 건드리지 말고 merge 후 docs-only state sync.
- 상대 owner branch는 기본 direct push 금지. 필요 시 assist branch/PR.

## 7. Locks / ownership

`TEAM_BOARD`의 ACTIVE lock이 최우선이다.
- lock owner 외 해당 scope 수정 금지.
- lock 0이면 기본 CODEOWNER/ownership 규칙 적용.
- 상대 lane 데이터/API가 필요하면 Integration Request를 먼저 남긴다.

Config/build/CI/Core/World authority는 Jjun lane, UI/Character/Content presentation은 Dagyeom lane이 기본이다.

## 8. Merge closeout

DONE 조건:
1. acceptance 충족.
2. 필요한 fast/heavy validation 완료.
3. comments/reviews/unresolved thread 확인.
4. merge 완료.
5. `WORK_STATE`/`TEAM_BOARD`/milestone roadmap에 필요한 최소 sync.
6. 의미 있는 완료면 HANDOFF append.

merge 뒤 docs-only state sync가 `main` HEAD를 더 전진시킬 수 있다. product merge SHA와 docs sync SHA는 구분한다.

## 9. Recovery

중단 후:
1. actual main/branch/PR/Actions 확인.
2. 마지막 validated product HEAD 확인.
3. `WORK_STATE`/`TEAM_BOARD`와 reconcile.
4. unresolved reviews/locks 확인.
5. 마지막 안전 checkpoint에서 재개.

이전 채팅에서 "완료"라고 말했더라도 GitHub 증거가 없으면 완료로 간주하지 않는다.
'''

MILESTONES = r'''# LifeLens Development Milestones

이 문서는 **큰 개발 단위와 integration gate의 canonical roadmap**이다. 작은 TODO 리스트가 아니다.

## Delivery rule

> Same purpose + same layer + same validation scope = one milestone-sized PR.

각 milestone은 내부적으로 여러 작은 commit을 가질 수 있지만, 무거운 UE 검증과 canonical doc closeout은 의미 있는 끝점에서 한 번 수행한다.

## Completed foundations

- Character Presentation #63 — DONE.
- Appearance projection #65 — DONE.
- World Affordance Fallback #66 — DONE.
- Environmental Residue #68 — DONE.
- Runtime resident / physical ACK / restore #70~#74 — DONE.
- Exposure / sanitation causal chain #75~#80 — DONE.
- HumanWaste Environmental Visual Feedback #82 — DONE.
- World Genesis WG-1 #83 — DONE.
- World Genesis WG-2 #85 — DONE.
- Character Appearance #67 — DONE.
- **Character Motion Bootstrap #84 — DONE.**
- **World Generation Milestone A #87 — DONE.**

## Gate A — Integrated Runtime Checkpoint A — READY_NOW

Owner: joint integration; Jjun coordinates runtime/authority verification, Dagyeom presentation lane participates as needed.

Purpose: 처음으로 `main`의 실제 vertical slice를 한 번에 확인한다.

Acceptance:
- NEW GAME starts successfully.
- WG-2 selected initial region is the region materialized by Milestone A.
- four founders are physically projected inside that materialized region.
- no starting house/toilet/farm/storage/road/tool/modern infrastructure appears.
- Character Appearance + Idle/Walk/Jog + orientation presentation works in the integrated runtime.
- existing AI movement/action flow still follows Core/World authority.
- HumanWaste/environmental visual feedback can be reconstructed from authoritative Core state.
- Save/Load does not silently reroll generated natural state and restores resident/world projection coherently.
- Observer can inspect residents after start/load.

Until this gate is checked, **Dagyeom does not start an additional product milestone.**

## Milestone B — World Visual Milestone A — AFTER GATE A

Owner: Dagyeom visual/content lane; Jjun supplies Config/Bridge integration when requested.

Scope:
- production-oriented generated-world presentation consuming real world-generation contracts.
- terrain/biome presentation.
- vegetation / rocks / water / natural dressing.
- chunk presentation/materialization consumption.
- Android-friendly HISM/instancing/LOD/culling/material budget.
- observer readability.
- no visual-only second authority.

Important Config boundary:
- Dagyeom may create maps under `Content/Maps/**` (`/Game/Maps/...`).
- default startup map / `Config/DefaultEngine.ini` changes are Jjun-owned Integration work.
- Water/plugin changes to `LifeLens.uproject` are also Integration Requests, not silent content-lane edits.

## Milestone C — Character Context Motion Milestone

Owner: Dagyeom Character presentation.

Group together instead of one PR per animation:
- Sit / Stand / Lie / Wake.
- PickUp / Use / Work.
- sanitation / digging / crafting context animation hooks.
- locomotion ↔ context transition.
- basic gaze/body orientation layering.
- later AnimBP/state-machine migration if needed.

Animation remains presentation of Core/World action authority.

## Gate B — Android Smoke / Real-device Baseline

After World Visual Milestone A:
- cheap Android smoke/package path.
- actual APK artifact.
- real-device boot/play.
- performance / thermal / memory baseline.
- chunk/render budget tuning.

MetaHuman comparison happens only after this gate.

## Later simulation milestones

These remain product direction, not immediate dispatch:
- generic facility/resource authority beyond sanitation.
- health/pathogen + water/soil contamination.
- birth physical position fix and lifecycle presentation policy.
- open-ended material/component/connection artifact runtime.
- carrying-capacity pressure / exploration / migration.
- multiple settlements / regional society / politics / generational civilization.

## Current dispatch

**Only Gate A — Integrated Runtime Checkpoint A is READY_NOW.**

`tasks/WORK_STATE.md` contains the current live execution state. `tasks/TEAM_BOARD.md` contains ownership/locks/Integration Requests.
'''

WORK_STATE = r'''# LifeLens Canonical Work State

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
> Long-term order: `docs/DEVELOPMENT_MILESTONES.md`.

Last reconciled: 2026-09-15 KST after PR #84 and PR #87 merges.

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

Documentation-only state sync commits may advance `main` beyond the product merge SHA above.

## Current dispatch — Integrated Runtime Checkpoint A — READY_NOW

Owner: joint; Jjun coordinates integration evidence.
Status: `READY_NOW`
Handoff safety: `SAFE`
Active locks: 0

Acceptance:
- NEW GAME boots on current `main`.
- selected initial region is materialized.
- 4 founders appear inside it.
- no starting civilization infrastructure.
- Appearance + Motion Bootstrap present correctly.
- Core/World movement/action authority remains intact.
- HumanWaste/environment feedback reads from authoritative state.
- Save/Load preserves generated natural state and resident projection without reroll.
- Observer can inspect residents after start/load.

Exact next action:
- run Integrated Runtime Checkpoint A on latest merged `main`.
- record only actual failures/evidence; do not create speculative fixes.
- if PASS, promote Dagyeom `World Visual Milestone A` to READY_NOW.
- if a blocker appears, assign it to the owning lane in `TEAM_BOARD`.

## Dagyeom lane

Status: `HOLD — WAIT FOR INTEGRATED RUNTIME CHECKPOINT A`

Do not start a new product milestone yet. World Visual prep/research may be discussed, but product integration starts after Gate A PASS.

## Jjun lane

World Generation Milestone A is DONE. Do not start another world-generation feature before Gate A. Jjun may perform integration fixes/config work that Gate A proves necessary.

## Known near-term integration boundary

Formal Integration Request currently open: **0**.

Expected upcoming handoff once Dagyeom creates a production map:
- Dagyeom supplies `/Game/Maps/<MapName>` asset path.
- Jjun updates `Config/DefaultEngine.ini` (`GameDefaultMap`, and `EditorStartupMap` if needed).
- Water/plugin changes to `LifeLens.uproject` require a separate explicit Integration Request.

## Long compile rule

When a long UE compile/package is started, record HEAD + Run ID and continue other safe work. Do not continuously wait/poll; the user reports completion/failure and then closeout resumes.
'''

TEAM_BOARD = r'''# LifeLens Team Board

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

## Current gate

`Integrated Runtime Checkpoint A — READY_NOW`

- Jjun coordinates integrated runtime verification.
- Dagyeom starts no additional product milestone until Gate A result.
- after PASS, `World Visual Milestone A` becomes the next Dagyeom milestone.

## Current Assist Locks

**0 active locks.**

Jjun helping Dagyeom defaults to REVIEW_ONLY. Do not direct-push `dagyeom/*`; use assist branch/PR when a real code change is required.

## Integration Requests

### Open formal requests

**0.**

### Expected World Visual config handoff — NOT YET FORMAL

When Dagyeom creates the production map:
1. Dagyeom provides the exact asset path, e.g. `/Game/Maps/LifeLensWorld`.
2. Jjun owns `Config/DefaultEngine.ini` default/startup map integration.
3. If Unreal Water or another plugin requires `LifeLens.uproject` changes, Dagyeom opens an explicit Integration Request with the required plugin and reason.
4. Content work should not silently edit Jjun-owned Config/project integration files.

## Request format

An Integration Request must contain:
- requester / owner needed.
- exact file/API/config needed.
- why existing contract is insufficient.
- target branch/PR or asset path.
- whether it blocks current milestone.

Requests are closed when the owning lane merges the required integration and both sides can consume it.
'''

DAGYEOM_QUEUE = r'''# Dagyeom READY Queue — Deprecated Redirect

This file is retained only for old links.

**Do not use this file as a second dispatch board.**

Canonical sources:
- roadmap: `docs/DEVELOPMENT_MILESTONES.md`
- current execution state: `tasks/WORK_STATE.md`
- ownership / locks / Integration Requests: `tasks/TEAM_BOARD.md`

Current Dagyeom status at the 2026-09-15 reconciliation:

`HOLD — wait for Integrated Runtime Checkpoint A`

After Gate A PASS, `World Visual Milestone A` is promoted in `WORK_STATE.md`. Do not start from stale historical READY_AFTER entries in old revisions of this file.
'''

CLAUDE = r'''# CLAUDE.md

Claude Code / Cowork follows the same repository rules as every other agent.

**Read `AGENTS.md` first.**

Then read:
1. `docs/LIFELENS_SPEC_v1.1.md`
2. `docs/DEVELOPMENT_MILESTONES.md`
3. `docs/STATE_MANAGEMENT.md`
4. actual GitHub main / target PR / Actions
5. `tasks/WORK_STATE.md`
6. `tasks/TEAM_BOARD.md`
7. latest meaningful `tasks/HANDOFF_LOG.md` entry

`tasks/DAGYEOM_READY_QUEUE.md` is deprecated and is not a canonical dispatch source.

Dagyeom defaults:
- UI / Observer presentation
- Character appearance / animation / presentation
- `Content/UI/**`, `Content/Characters/**`, `Content/Environment/**`, `Content/Maps/**`, `Content/WorldPresentation/**`

Do not directly modify Core/AI/Simulation/World authority, build/CI, `Config/**`, or `LifeLens.uproject` integration for visual convenience. Use `tasks/TEAM_BOARD.md` Integration Requests.

Current gate after #84/#87: **Integrated Runtime Checkpoint A.** Do not start an additional Dagyeom product milestone until `WORK_STATE.md` promotes it.
'''

PROGRESS = r'''# LifeLens Project Progress Snapshot — 2026-09-15

> Actual GitHub is the highest-priority truth. Current dispatch is in `tasks/WORK_STATE.md`; milestone order is in `docs/DEVELOPMENT_MILESTONES.md`.

## Latest product merges

### #84 Character Motion Bootstrap — DONE
- merge: `5f0af986728e717c274482915d5f8b5a9ee31195`
- validated product head: `fa781b0829a29f8b29b99fe19fe699cc53792966`
- Preflight `34940159290` PASS
- Unreal Linux Compile `34940159300` PASS
- PIE PASS: locomotion/rigging visible, no T-pose/sliding, smooth turn
- review threads closed

### #87 World Generation Milestone A — DONE
- merge: `5a543b7392eca722e794b5504241d46669ac23ab`
- final head: `9de52c43c3048c9260c4d6fe2de2be0f97bed082`
- Preflight `34940403254` PASS
- Core `34940403079` PASS, 48/48 + deterministic
- Unreal Linux Compile `34940402948` PASS including UHT/UBT/link
- comments 0 / unresolved threads 0

## What the runtime now has

### People / society Core
Needs, Personality, Emotion, Memory, Belief, multidimensional Relationship, social cognition, romance/marriage/household/pregnancy/birth/genetics/growth/aging/death/genealogy foundations.

### Civilization / causal environment
Natural-resource baseline, gather/store/experiment/discovery/crafting/personal knowledge/transmission, missing-affordance fallback, HumanWaste residue, exposure/memory/avoidance, sanitation problem recognition, designated sanitation site, DugPit containment, environmental visual feedback.

### Core ↔ Unreal execution
Dynamic residents, physical action pending/ACK flow, actual movement/arrival/use completion, runtime GridPos restore, Observer read paths, character appearance and locomotion presentation.

### World generation
- WG-1: stable WorldSeed/PopulationSeed/GenerationVersion/ChunkCoord contracts.
- WG-2: coherent macro nature + viable initial start region.
- Milestone A: deterministic detailed natural chunk baseline, selected-region materialization, founder placement, generated-chunk registry, snapshot v6 no-reroll persistence, read-only Unreal world-generation presentation contracts.

Canonical world chain:
`WorldSeed → Macro World → deterministic lazy chunks → persistent human/environmental history → carrying-capacity pressure → migration → multiple settlements`

Initial rule remains:
`natural environment + 2 male + 2 female founders + zero civilization infrastructure`.

## Current gate

**Integrated Runtime Checkpoint A — READY_NOW.**

Check current main as one vertical slice:
`NEW GAME → generated start region → 4 founders → appearance/motion → AI/environment/HumanWaste → Save/Load → Observer`.

Dagyeom is on HOLD for new product milestones until this gate is checked.

## Next after Gate A

1. World Visual Milestone A — real generated-world visual presentation + Android-friendly LOD/instancing/culling.
2. Character Context Motion Milestone — Sit/PickUp/Use/Work/context transitions/gaze.
3. Android smoke/package/real-device performance gate.
4. MetaHuman comparison only after mobile baseline.
5. deeper health/pathogen, generic invention, migration/multi-settlement society.

## Known structural risks

- generic facility/resource authority beyond sanitation remains partial.
- child initial physical position needs explicit verification/fix.
- dormant `ULLDecisionComponent` competing chooser needs review/removal.
- representative legacy snapshot fixtures remain incomplete.
- full health/pathogen/water contamination is later.
- death-body presentation policy remains separate.
- Android APK + real-device product validation still not done.

## Collaboration/governance transition

As of this checkpoint:
- milestone-sized PRs replace micro-PR chains for same-purpose/same-layer/same-validation work.
- separate READY queues are no longer canonical.
- `WORK_STATE` is live state only.
- `TEAM_BOARD` is ownership/locks/Integration Requests only.
- `HANDOFF_LOG` records meaningful events only.
- already validated product PRs are not moved for docs-only review closeout when that would retrigger heavy UE validation.
'''

docs = {
    "AGENTS.md": AGENTS,
    "docs/STATE_MANAGEMENT.md": STATE,
    "docs/DEVELOPMENT_MILESTONES.md": MILESTONES,
    "tasks/WORK_STATE.md": WORK_STATE,
    "tasks/TEAM_BOARD.md": TEAM_BOARD,
    "tasks/DAGYEOM_READY_QUEUE.md": DAGYEOM_QUEUE,
    "CLAUDE.md": CLAUDE,
    "docs/PROJECT_PROGRESS_2026-09-15.md": PROGRESS,
}

for path, text in docs.items():
    write(path, text)

# Keep the domain-specific world-gen document but update its latest implementation status.
wg_path = ROOT / "docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md"
wg = wg_path.read_text(encoding="utf-8")
for old, new in [
    ("MILESTONE A READY_NOW", "MILESTONE A DONE"),
    ("MILESTONE A NEXT", "MILESTONE A DONE"),
    ("World Generation Milestone A — READY_NOW", "World Generation Milestone A — DONE via PR #87"),
    ("World Generation Milestone A — READY_NOW:", "World Generation Milestone A — DONE via PR #87:"),
]:
    wg = wg.replace(old, new)
marker = "## 2026-09-15 implementation checkpoint — World Generation Milestone A DONE"
if marker not in wg:
    wg += r'''

## 2026-09-15 implementation checkpoint — World Generation Milestone A DONE

- WG-1 deterministic world identity/coordinates — DONE #83.
- WG-2 macro world + viable initial start-region selector — DONE #85.
- **World Generation Milestone A — DONE #87**, merge `5a543b7392eca722e794b5504241d46669ac23ab`.
- final #87 head `9de52c43c3048c9260c4d6fe2de2be0f97bed082`.
- Preflight `34940403254` PASS.
- Core `34940403079` PASS, 48/48 + deterministic harness.
- Unreal Linux Compile `34940402948` PASS including UE 5.6 UHT/UBT/link.

Milestone A establishes deterministic detailed natural chunks, selected start-region materialization, founder placement inside that region, generated-chunk identity/registry, snapshot v6 no-reroll persistence boundary, and read-only Unreal presentation contracts without adding starting civilization infrastructure.

**Next gate is Integrated Runtime Checkpoint A**, not another isolated world-generation slice. After that gate, World Visual Milestone A consumes the real generated-world contracts.
'''
wg_path.write_text(wg.rstrip() + "\n", encoding="utf-8")

# Append-only handoff: preserve every historical entry and add one meaningful transition.
handoff_path = ROOT / "tasks/HANDOFF_LOG.md"
handoff = handoff_path.read_text(encoding="utf-8")
handoff_marker = "### #84 + #87 closeout and milestone governance transition"
if handoff_marker not in handoff:
    handoff += r'''

---

## 2026-09-15 — Jjun / joint integration checkpoint

### #84 + #87 closeout and milestone governance transition
- 작성자: Jjun side AI, reflecting merged Dagyeom + Jjun product lanes.
- Character Motion Bootstrap #84: DONE / main merge `5f0af986728e717c274482915d5f8b5a9ee31195`.
  - validated product head `fa781b0829a29f8b29b99fe19fe699cc53792966`
  - Preflight `34940159290` PASS
  - Unreal Linux Compile `34940159300` PASS
  - PIE PASS and review threads resolved
- World Generation Milestone A #87: DONE / main squash merge `5a543b7392eca722e794b5504241d46669ac23ab`.
  - head `9de52c43c3048c9260c4d6fe2de2be0f97bed082`
  - Preflight `34940403254` PASS
  - Core `34940403079` PASS, 48/48 + deterministic
  - Unreal Linux Compile `34940402948` PASS including UHT/UBT/link
  - comments/unresolved review threads 0
- 협업 규칙 전환:
  - same-purpose + same-layer + same-validation work is one milestone-sized PR.
  - `docs/DEVELOPMENT_MILESTONES.md` becomes the canonical roadmap.
  - `WORK_STATE` is live state only; `TEAM_BOARD` is ownership/locks/Integration Requests only.
  - role-specific READY queue is deprecated to prevent duplicate/stale dispatch.
  - meaningful events only go to this append-only HANDOFF log.
  - do not move an already validated product PR HEAD for docs-only closeout when that would trigger another heavy UE compile.
  - long UE compile/package runs are started and recorded, then the AI does not keep waiting/polling; closeout resumes when the user reports completion/failure.
- 다음 gate: **Integrated Runtime Checkpoint A** on merged `main`.
- Dagyeom next product milestone remains HOLD until that integrated gate is checked.
- World Visual map/config boundary: Dagyeom owns `Content/Maps/**`; Jjun owns `Config/DefaultEngine.ini` startup/default map and `LifeLens.uproject` plugin integration via explicit Integration Request.
'''
    handoff_path.write_text(handoff.rstrip() + "\n", encoding="utf-8")

print("POST_84_87_STATE_SYNC_READY")
