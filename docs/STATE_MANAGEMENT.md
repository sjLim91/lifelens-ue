# LifeLens Shared State Management Protocol

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
