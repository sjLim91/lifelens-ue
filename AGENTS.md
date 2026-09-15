# AGENTS.md — LifeLens Agent Entry Point

이 저장소에서 작업하는 모든 AI 에이전트(Codex / ChatGPT / Claude Code 등)는 **이 파일을 가장 먼저 읽는다.**

## 0. Source of truth

우선순위는 항상 다음과 같다.

`actual GitHub main / PR / Actions > canonical repository docs > 이전 채팅 / 기억 / 로컬 추정`

작업 시작·재개 시 순서:
1. `AGENTS.md`
2. `docs/LIFELENS_SPEC_v1.1.md`
3. `docs/DEVELOPMENT_MILESTONES.md`
4. `docs/STATE_MANAGEMENT.md`
5. `docs/DECISION_LOG.md`
6. actual `main` HEAD / target branch / PR / Actions
7. `tasks/WORK_STATE.md`
8. `tasks/TEAM_BOARD.md`
9. `tasks/HANDOFF_LOG.md` 최신 의미 있는 항목

`tasks/DAGYEOM_READY_QUEUE.md`는 과거 링크 호환용이다. **새 dispatch의 canonical source로 사용하지 않는다.**

`docs/DECISION_LOG.md`는 쭌과 AI가 대화 중 확정한 설계·정리·작업 판단 원칙의 canonical log다. 제품 요구사항 자체를 바꾸는 결정은 Decision Log에만 두지 않고 `docs/LIFELENS_SPEC_v1.1.md`에도 반영한다.

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

- `docs/LIFELENS_SPEC_v1.1.md` — 제품 최상위 요구사항 / 불변조건.
- `docs/DEVELOPMENT_MILESTONES.md` — 큰 개발 단위와 gate 순서.
- `docs/DECISION_LOG.md` — 대화 중 확정된 설계·정리·작업 판단 원칙.
- `tasks/WORK_STATE.md` — **현재 active/ready/blocked state만** 기록.
- `tasks/TEAM_BOARD.md` — ownership / active locks / Integration Requests만 기록.
- `tasks/HANDOFF_LOG.md` — 의미 있는 merge/failure/design transition만 append-only 기록.
- `docs/PROJECT_PROGRESS_2026-09-15.md` — 날짜 기준 전체 진행 snapshot.

문서 역할을 섞지 않는다. 제품 요구사항 변경은 Master Spec에도 반영하고, 현재 상태 변경은 `WORK_STATE`, ownership/lock 변경은 `TEAM_BOARD`, 대화로 확정한 지속적 판단 원칙은 `DECISION_LOG`에 반영한다.

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
