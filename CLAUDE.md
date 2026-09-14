# CLAUDE.md

이 저장소의 모든 에이전트 규칙은 `AGENTS.md`를 따른다. 먼저 반드시 `AGENTS.md`를 읽는다.

## 다겸 작업의 최우선 게이트

**코드보다 상태 동기화가 먼저다.** 다겸의 로컬 PC(Cowork / Claude Code)에서 작업할 때도 기능 수정 전에 실제 GitHub 상태와 공용 문서를 먼저 맞춘다.

현재 알려진 제품 checkpoint:
- PR #37 Observer Runtime Bridge — main 병합 완료
- PR #39 authoritative family runtime state — main 병합 완료
- PR #40 family/world observer bridge — main 병합 완료
- 제품 merge checkpoint: `9261581df3abd5332d92855628fd7d03203748af`
- PR #40 Structural Preflight `34796892593` PASS
- PR #40 Unreal 5.6 Linux actual UHT/UBT `34796892609` PASS
- 기존 Observer read 관련 `BLOCKED-BY-JJUN` 6개 항목은 모두 **해제됨**. 최신 `tasks/DAGYEOM_READY_QUEUE.md`를 따른다.

다겸 작업 시작 순서:

1. `AGENTS.md`
2. `docs/LIFELENS_SPEC_v1.1.md`
3. `docs/BUILD_STRATEGY_v1.2.md`
4. `BUILD_LESSONS.md`
5. `docs/TEAM_WORKFLOW.md`
6. `docs/STATE_MANAGEMENT.md`
7. 실제 `main` HEAD / 현재 branch HEAD / PR state+head+base+mergeability / CI
8. `tasks/WORK_STATE.md`
9. `tasks/DAGYEOM_READY_QUEUE.md`
10. `tasks/TEAM_BOARD.md`
11. `tasks/HANDOFF_LOG.md`
12. 해당 `tasks/TASK_*.md`
13. 로컬 빌드 작업일 때만 `tasks/COWORK_LOCAL_BUILD.md`
14. 실제 GitHub와 문서가 다르면 **코드 수정 전에 문서부터 갱신**

`tasks/WORK_STATE.md`가 현재 작업 상태의 단일 기준판이다. 작업 시작/재개 전 branch HEAD, PR, CI 상태를 실제 GitHub와 대조한다. 타임아웃, 세션 종료, 도구 오류, 수동 중단 후에는 이전 행동이 성공했다고 가정하지 말고 `docs/STATE_MANAGEMENT.md`의 복구 절차대로 실제 GitHub 상태를 먼저 확인한다.

`tasks/DAGYEOM_READY_QUEUE.md`는 다겸 측의 **즉시 실행 가능한 작업 큐**다. 최신 큐에서 PR #17 latest-main reconciliation + Core Observer Bridge binding이 최우선이다. READY NOW가 0개라면 "할 일 없음"으로 종료하지 말고 `NEEDS_ASSIGNMENT`로 판단한다. 새로운 `BLOCKED-BY-JJUN`은 최신 main의 API를 실제 확인한 뒤 구체적으로 부족한 필드/API가 있을 때만 추가한다.

`tasks/HANDOFF_LOG.md`는 양쪽 AI의 append-only 인수인계 이력이다. 성공뿐 아니라 실패/중단/정정도 남긴다. 단, 상대 branch가 같은 shared file을 수정 중이면 내용을 덮어쓰지 말고 우선 `WORK_STATE` 및 PR 코멘트로 공유한 뒤 merge 시 조정한다.

다겸 측 기본 담당은 Observer/UI/Character presentation이다. `Source/LifeLensCore/**`, `Source/LifeLens/AI/**`, `Source/LifeLens/Simulation/**`, build/CI/Config는 쭌 측 소유 영역이므로 직접 수정하지 않는다. 필요한 데이터/API가 있으면 `tasks/TEAM_BOARD.md`의 Integration Request에 기록한다.

다겸 측 작업 브랜치는 `dagyeom/<scope>-<task>` 형식을 사용하고, `main`에 직접 기능 코드를 push하지 않는다. 상태/문서 전용 동기화는 `docs/STATE_MANAGEMENT.md` 규칙을 따른다.
