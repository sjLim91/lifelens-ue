# CLAUDE.md

이 저장소의 모든 에이전트 규칙은 `AGENTS.md`를 따른다. 먼저 반드시 `AGENTS.md`를 읽는다.

다겸의 로컬 PC(Cowork / Claude Code)에서 작업할 때도 쭌 측 AI와 동일하게 아래 순서로 확인한다.

1. `AGENTS.md`
2. `docs/LIFELENS_SPEC_v1.1.md`
3. `docs/BUILD_STRATEGY_v1.2.md`
4. `BUILD_LESSONS.md`
5. `docs/TEAM_WORKFLOW.md`
6. `tasks/TEAM_BOARD.md`
7. `tasks/HANDOFF_LOG.md`
8. 해당 `tasks/TASK_*.md`
9. 로컬 빌드 작업일 때만 `tasks/COWORK_LOCAL_BUILD.md`

`tasks/HANDOFF_LOG.md`는 양쪽 AI의 공용 인수인계장이다. 작업 시작 전에 최신 기록을 읽고, 코드/설정/워크플로우/API를 의미 있게 변경했으면 작업 종료 전에 자신의 변경 파일, 브랜치/PR, 검증 상태, 상대 영향도를 append-only로 기록한다. 상대 AI가 모르는 변경을 남기지 않는다.

다겸 측 기본 담당은 Observer/UI/Character presentation이다. `Source/LifeLensCore/**`, `Source/LifeLens/AI/**`, `Source/LifeLens/Simulation/**`, build/CI/Config는 쭌 측 소유 영역이므로 직접 수정하지 않는다. 필요한 데이터/API가 있으면 `tasks/TEAM_BOARD.md`의 Integration Request에 기록한다.

다겸 측 작업 브랜치는 `dagyeom/<scope>-<task>` 형식을 사용하고, `main`에 직접 기능 코드를 push하지 않는다.
