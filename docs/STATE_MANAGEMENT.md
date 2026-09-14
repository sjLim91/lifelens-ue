# LifeLens Shared State Management Protocol

이 문서는 쭌(sjLim91), 다겸(STILLofficial), 그리고 양쪽 AI가 같은 저장소를 병렬 운영할 때 사용하는 **상태관리 최상위 규칙**이다.

목표는 단순하다.

> 작업이 성공했을 때뿐 아니라 실패, CI 실패, 타임아웃, 세션 종료, 수동 중단, 브랜치 전환, 충돌, 검증 대기 상태까지 모두 상대가 저장소만 보고 복구할 수 있어야 한다.

## 0. 가장 중요한 규칙 — Sync Before Work

**상태 동기화는 기능 개발보다 우선한다.** 쭌/다겸/모든 AI는 어떤 코드·빌드·PR 작업이든 시작하기 전에 실제 GitHub 상태와 협업 문서를 먼저 맞춘다.

필수 게이트:
1. 실제 `main` HEAD를 확인한다.
2. 대상 branch HEAD / PR state·head·base·mergeability / 관련 Actions를 확인한다.
3. `WORK_STATE.md` → 역할별 READY 큐 → `TEAM_BOARD.md` → `HANDOFF_LOG.md`와 대조한다.
4. 하나라도 stale하면 **코드를 건드리기 전에 상태 문서를 먼저 실제 GitHub 기준으로 수정한다.**
5. 의미 있는 checkpoint(커밋, PR, CI 시작/완료/실패, blocker 해제, merge)마다 상태 문서를 갱신한다.
6. 작업 종료 시에도 다시 실제 GitHub와 맞춘 뒤 HANDOFF를 남긴다.

따라서 **문서가 stale한 상태에서 새 기능 작업을 시작하는 것은 금지**한다. GitHub 사실이 진실이고, 문서는 그 사실을 상대가 즉시 읽을 수 있도록 따라가야 한다.

## 1. 상태 정보의 역할

세 파일의 역할을 섞지 않는다.

- `tasks/WORK_STATE.md` — **현재 상태의 단일 기준판(canonical live state)**. 지금 무엇이 진행 중이고 어디서 재개해야 하는지 기록한다.
- `tasks/TEAM_BOARD.md` — 담당 영역, 파일 잠금, Integration Request, 역할 분배를 기록한다.
- 역할별 READY 큐 — 지금 바로 가능한 작업, dependency, BLOCKED-BY 소유자를 기록한다.
- `tasks/HANDOFF_LOG.md` — 완료/실패/중단/정정의 원인과 변경 이력을 append-only로 남긴다.

현재 상태가 서로 다르면 **실제 GitHub branch/commit/PR/Actions 상태가 최우선 진실**이다. 그 다음 `WORK_STATE.md`와 관련 큐/보드를 실제 상태에 맞게 고친 뒤 작업한다.

## 2. 모든 AI의 작업 시작 순서

어떤 브랜치에서든 코드 수정 전에 반드시 다음 순서로 확인한다.

1. `AGENTS.md`
2. `docs/LIFELENS_SPEC_v1.1.md`
3. `docs/STATE_MANAGEMENT.md`
4. 실제 `main` HEAD / 대상 branch HEAD / PR / CI
5. `tasks/WORK_STATE.md`
6. 역할별 READY 큐 (`tasks/DAGYEOM_READY_QUEUE.md` 등)
7. `tasks/TEAM_BOARD.md`
8. `tasks/HANDOFF_LOG.md`
9. **불일치가 있으면 여기서 문서부터 고친다.**
10. 그 다음에만 기능 코드/빌드 작업을 시작한다.

기억, 이전 채팅, 로컬 작업 폴더만 믿고 이어가지 않는다.

## 2.1 Lock-aware Dispatch — 작업순서 꼬임 방지

작업 시작 순서 확인만으로는 부족하다. **현재 작업 권한을 명시적으로 판정한 뒤에만 파일을 수정한다.**

### A. ACTIVE_LOCK 우선

- `tasks/TEAM_BOARD.md`의 `Current Assist Locks`에서 `ACTIVE`인 lock을 먼저 확인한다.
- lock에 포함된 파일/scope는 **lock owner 외 수정 금지**다.
- 원래 CODEOWNER라도 ACTIVE assist lock이 있으면 lock이 우선한다.
- 비소유자는 같은 파일에 rebase/cherry-pick/helper branch/대체 구현을 만들지 않는다.

### B. 상태 표현의 의미

- `ACTIVE` / `DOING` / `WAITING_CI`: 해당 lane은 이미 누군가 진행 중이다.
- `PARALLEL_SAFE_NOW`: 다른 lane과 충돌 없이 지금 바로 병렬 수행 가능하다.
- `READY_NOW`: 선행조건이 모두 끝났고 즉시 코드 착수 가능하다.
- `PREP_ONLY`: 설계/자산/문서/조사만 가능하며 locked product code는 수정하지 않는다.
- `NEXT`, `AFTER`, `HIGH PRIORITY`: **순서 표시일 뿐 작업 허가가 아니다.**

### C. CI 대기 중 규칙

한 lane이 `WAITING_CI`라고 해서 다음 canonical milestone을 자동 시작하지 않는다.

- lock owner는 해당 CI/검증/merge를 끝낸다.
- 다른 작업자는 READY queue에서 명시된 `PARALLEL_SAFE_NOW`만 수행한다.
- `AFTER` 항목은 선행 checkpoint가 `DONE`이 되기 전 코드 착수 금지다.
- 예외가 필요하면 `WORK_STATE`와 `TEAM_BOARD`를 먼저 수정하여 병렬 안전 범위를 명시한 뒤 시작한다.

### D. 작업 착수 승인 판정

각 AI는 코드 수정 직전에 아래 4문항을 스스로 통과해야 한다.

1. 이 파일이 ACTIVE_LOCK에 포함되어 있지 않은가?
2. 내 작업이 `READY_NOW` 또는 `PARALLEL_SAFE_NOW`로 명시되어 있는가?
3. 선행 PR/CI/merge checkpoint가 요구된다면 실제 GitHub에서 완료되었는가?
4. 같은 목적의 다른 branch/PR이 이미 진행 중이지 않은가?

하나라도 `아니오`면 코드 수정 금지. 상태 문서 정정 또는 Integration Request가 먼저다.

### E. lock 해제

ASSIST_LOCK은 다음 조건을 모두 만족해야 RELEASED로 바꾼다.

1. 필요한 검증 완료.
2. temporary validation-only 변경 제거.
3. product diff 범위 확인.
4. merge 또는 명시적 종료 완료.
5. `WORK_STATE` + READY queue + `TEAM_BOARD` 동기화.

lock 해제 전에는 다음 code-integration milestone로 승격하지 않는다.

## 3. 공통 상태값

`WORK_STATE.md`의 상태는 아래 중 하나를 사용한다.

- `PLANNED` — 아직 코드 작업 전
- `IN_PROGRESS` — 실제 수정 중
- `WAITING_CI` — 커밋/PR은 존재하고 CI 결과 대기
- `READY_TO_MERGE` — 필요한 검증 통과, 병합 가능
- `BLOCKED` — 외부 원인/컴파일 오류/권한/충돌 때문에 진행 불가
- `INTERRUPTED` — 타임아웃/세션 종료/도구 오류 등으로 작업 흐름이 중간에 끊김
- `RECOVERING` — 중단 후 실제 GitHub 상태를 다시 대조하는 중
- `FROZEN` — 의도적으로 더 진행하지 않기로 한 브랜치/작업
- `DONE` — 필요한 검증 후 main 반영과 상태 문서 동기화까지 완료
- `PREP_ONLY` — 병렬 준비는 가능하지만 선행 checkpoint/lock 때문에 product code 통합은 아직 금지

`DONE`은 계획 완료나 코드 작성 완료가 아니라 **정의된 검증 + 병합 + 협업문서 동기화까지 실제 완료된 경우에만** 쓴다.

## 4. WORK_STATE 필수 필드

활성 작업은 최소 다음 정보를 가진다.

- Owner
- Branch
- Work item / 목적
- Last known HEAD SHA
- PR 번호
- Status
- CI / Run / Job 상태
- Last verified fact
- Blocker 또는 interruption reason
- Exact next action
- Handoff safety (`SAFE`, `CONDITIONAL`, `NOT_SAFE`)
- Shared-file impact
- Active lock ID / locked scope if applicable
- Parallel-safe scope for the other owner if applicable

다른 AI가 이 항목만 읽고도 **첫 번째로 실행해야 할 행동을 알 수 있어야 한다.**

## 5. Checkpoint-first 규칙

타임아웃은 예고 없이 발생할 수 있으므로 작업을 한 번에 크게 쌓지 않는다.

- 의미 있는 최소 단위마다 커밋을 만든다.
- 새 PR 생성, CI 시작, 실패 원인 확정, 수정 커밋 생성, 병합 직전 등 복구 포인트마다 `WORK_STATE.md`를 갱신한다.
- blocker가 해제되거나 상대에게 새 API가 제공되면 역할별 READY 큐와 `TEAM_BOARD.md`를 즉시 갱신한다.
- 장시간 빌드/외부 작업 전에 반드시 현재 HEAD, Run ID, 다음 행동을 상태판에 남긴다.
- 아직 검증되지 않은 코드를 `PASS`, `DONE`, `READY`라고 쓰지 않는다.
- ASSIST_LOCK 생성/변경/해제는 반드시 같은 checkpoint에서 `WORK_STATE` + READY queue + `TEAM_BOARD`를 함께 갱신한다.

상태관리 전용 변경은 병렬 작업 동기화를 위해 `main`에 직접 기록할 수 있다. 단, **상태/문서 파일만** 해당하며 기능 코드는 기존 PR 규칙을 따른다.

## 6. 타임아웃 / 세션 중단 복구

도구 타임아웃, 응답 타임아웃, 앱 종료, AI 세션 종료처럼 정상 종료 처리를 못 한 경우 다음 세션은 아래 순서로 복구한다.

1. 해당 작업을 즉시 계속 수정하지 않는다.
2. `main`과 branch HEAD를 조회한다.
3. 열린 PR과 PR head/base/mergeability를 조회한다.
4. 관련 Actions Run / Job / 첫 실패 원인을 조회한다.
5. `WORK_STATE.md` / READY 큐 / `TEAM_BOARD.md`와 실제 상태를 비교한다.
6. 불일치하면 실제 GitHub 상태를 기준으로 문서를 먼저 정정한다.
7. ACTIVE_LOCK 여부를 확인하고 non-owner는 locked scope를 건드리지 않는다.
8. 마지막으로 성공한 checkpoint와 미완료 작업을 명시한다.
9. 그 다음에만 `IN_PROGRESS`, `WAITING_CI`, `READY_TO_MERGE` 등 실제 상태로 전환한다.

중단 당시 상태 업데이트를 하지 못했더라도 **다음 AI가 반드시 reconciliation을 수행**하므로 작업이 유실된 것으로 간주하지 않는다.

## 7. 실패 처리

실패가 발생하면 `WORK_STATE`에는 다음을 남긴다.

- 실패한 Run/Job/Step
- 첫 실제 root cause
- 코드 실패인지 환경/빌드 실패인지
- 재시도 여부
- 다음 수정 대상

같은 실패를 원인 확인 없이 반복 실행하지 않는다.

사용자가 더 진행하지 말라고 한 실패 브랜치는 `FROZEN`으로 기록하고 재실행하지 않는다.

## 8. 병합 / 브랜치 전환

- PR이 병합되면 해당 작업을 `DONE`으로 바꾸고 merge SHA를 기록한다.
- 제공 API/blocker 상태가 바뀌면 상대 READY 큐와 Integration Request 상태를 함께 갱신한다.
- 다음 기능은 새 브랜치에서 시작한다.
- 새 브랜치 생성 후 첫 코드 수정 전에 새 행을 `IN_PROGRESS`로 등록한다.
- 상대 브랜치가 shared file을 수정 중이면 직접 덮어쓰지 않는다.
- shared file 충돌 가능성이 있으면 `Handoff safety=CONDITIONAL` 또는 `NOT_SAFE`로 기록한다.
- ACTIVE_LOCK이 걸린 milestone은 merge/종료 + 문서 동기화 전에 다음 code-integration milestone로 자동 승격하지 않는다.

## 9. 다겸/쭌 상호 인수인계

상대 AI가 작업을 이어받을 수 있는 상태는 `SAFE`다.

`SAFE` 조건:
- HEAD SHA 확인됨
- 미커밋 로컬 변경에 의존하지 않음
- 다음 행동이 명확함
- 필요한 CI/PR 상태가 기록됨
- 상대의 BLOCKED/READY 상태가 실제 최신 API 상태와 맞음
- 현재 ACTIVE_LOCK과 locked scope가 문서에 일치함

`CONDITIONAL`은 CI 대기나 shared-file 동기화처럼 조건 하나를 확인하면 이어갈 수 있는 경우다.

`NOT_SAFE`는 로컬에만 존재하는 변경, 충돌 미해결, 원인 미확인 실패처럼 그대로 이어받으면 손실 위험이 있는 경우다.

## 10. PR 규칙

모든 PR은 현재 `WORK_STATE.md` 항목과 연결되어야 한다.

PR 생성 시:
- branch/HEAD/PR 번호를 WORK_STATE에 기록
- `WAITING_CI`로 전환
- 상대 소유 scope를 실제 수정하는 assist PR이면 ASSIST_LOCK을 TEAM_BOARD에 활성화

CI 통과 시:
- `READY_TO_MERGE`

병합 시:
- `DONE` + merge SHA
- 관련 ASSIST_LOCK 해제
- 상대에게 제공되는 API/데이터가 생겼다면 READY 큐와 TEAM_BOARD를 같은 checkpoint에서 갱신

PR이 닫히거나 보류되면 그 이유와 재개 조건을 상태판에 남긴다.

## 11. No-idle / Capacity 규칙

한 사람의 현재 PR이나 기능이 `BLOCKED`, `WAITING_CI`, `FROZEN`이라고 해서 그 사람 전체가 유휴 상태가 되어서는 안 된다.

- 각 담당자는 자신의 소유 범위에서 **최소 1개의 `READY NOW` 또는 `PARALLEL_SAFE_NOW` 작업**을 유지한다.
- API/데이터 의존 때문에 진행 불가한 항목은 담당자의 `할 일 없음`으로 처리하지 않고 `BLOCKED-BY-<OWNER>`로 분리한다.
- 다겸 측 READY 작업은 `tasks/DAGYEOM_READY_QUEUE.md`를 기준으로 한다.
- 다겸 READY NOW/PARALLEL_SAFE_NOW가 0개면 `NEEDS_ASSIGNMENT`로 간주하고, SPEC의 Observer/UI/Character presentation 범위에서 **현재 lock과 겹치지 않는** 다음 작업을 즉시 채운다.
- 쭌 측은 자신 때문에 막힌 Integration Request를 별도 backlog로 유지하고, API 제공/merge 시 해당 UI 작업을 **같은 상태 동기화 checkpoint에서** READY NOW로 승격시킨다.
- 대기 중인 PR에 무관한 새 작업은 별도 브랜치로 진행한다. 기존 PR에 무한히 기능을 누적하지 않는다.
- 상대가 "할 일 없음"이라고 보고하면, 새 기능 구현보다 먼저 READY 큐 / ACTIVE_LOCK / BLOCKED-BY 소유자를 점검한다.

## 12. 절대 하지 않는 것

- 이전 채팅만 보고 현재 branch 상태를 추측하지 않는다.
- `TEAM_BOARD`의 오래된 상태만 믿고 실제 PR/branch 확인을 생략하지 않는다.
- **stale한 협업 문서를 발견하고도 그대로 기능 작업을 시작하지 않는다.**
- ACTIVE_LOCK 파일을 lock owner 외 AI가 수정하지 않는다.
- `NEXT`라는 이유만으로 선행 checkpoint/lock을 건너뛰지 않는다.
- CI 대기라는 이유만으로 다음 canonical code milestone을 자동 시작하지 않는다.
- 다른 AI의 shared-file 변경을 최신 내용 확인 없이 덮어쓰지 않는다.
- CI가 실패했는데 `DONE`으로 표시하지 않는다.
- 타임아웃 후 동일 명령을 무조건 다시 실행하지 않는다.
- 오래된 branch를 최신 main인 것처럼 취급하지 않는다.
- 한 작업이 BLOCKED라는 이유만으로 담당자 전체를 `할 일 없음`으로 종료하지 않는다.
