# AGENTS.md — LifeLens 작업 규칙

이 저장소에서 작업하는 모든 AI 에이전트(Codex / Astra / Claude Code 등)는 이 파일을 먼저 읽는다.

## 0. 최우선 규칙 — Sync Before Work

**모든 기능 개발·빌드·PR 작업보다 협업 상태 동기화가 먼저다.**

작업 시작/재개 시 실제 GitHub의 `main` HEAD, 대상 branch HEAD, PR state/head/base/mergeability, 관련 Actions를 확인하고 `tasks/WORK_STATE.md`, 역할별 READY 큐, `tasks/TEAM_BOARD.md`, `tasks/HANDOFF_LOG.md`와 대조한다.

- 문서가 stale이면 **코드 수정 전에 문서부터 실제 GitHub 기준으로 갱신**한다.
- blocker가 해제되거나 새 API가 merge되면 상대 READY 큐/Integration Request 상태도 같은 checkpoint에서 갱신한다.
- 의미 있는 checkpoint마다 상태 문서를 갱신한다.
- 작업 종료/merge/failure 시 실제 GitHub와 다시 맞추고 HANDOFF를 남긴다.
- **stale한 상태 문서를 알고도 기능 작업을 시작하지 않는다.**

상세 절차는 `docs/STATE_MANAGEMENT.md`를 따른다.

## 기준 문서

- `docs/LIFELENS_SPEC_v1.1.md` — 제품 설계 기준. 요구사항을 사용자에게 다시 묻지 말고 이 문서를 기준으로 진행한다.
- `docs/BUILD_STRATEGY_v1.2.md` — 빌드/검증 전략의 기본 원칙.
- `BUILD_LESSONS.md` — 실제 빌드에서 확인된 성공/실패 사실. 빌드 전략 문서와 충돌하면 **실제 검증 결과가 우선**한다.
- `docs/TEAM_WORKFLOW.md` — 쭌/다겸 2인 협업, 코드 소유권, 충돌 방지 규칙.
- `docs/STATE_MANAGEMENT.md` — 성공/실패/타임아웃/중단/브랜치 전환을 포함한 공용 상태관리 규칙.
- `tasks/WORK_STATE.md` — **현재 작업 상태의 단일 기준판**. 작업 시작/재개 전에 반드시 실제 GitHub 상태와 대조한다.
- `tasks/DAGYEOM_READY_QUEUE.md` — 다겸의 즉시 실행/의존성/BLOCKED-BY 상태.
- `tasks/TEAM_BOARD.md` — 담당자/파일 잠금/Integration Request. 상태판이 아니라 역할/잠금 기준이다.
- `tasks/HANDOFF_LOG.md` — append-only 변경/실패/중단 이력.
- `tasks/` — 실제 작업 지시서. 같은 담당 영역에서는 번호 순서를 따른다.

## 절대 규칙

1. **장시간 Unreal 빌드를 맹목적으로 반복하지 않는다.** Core 테스트와 Preflight를 먼저 사용한다. Android는 `dev-slim-5.6.0`에 Android target이 없다는 실제 실패가 확인되었으므로, APK 경로에서는 동일 방식을 반복하지 않는다. 필요할 때만 검증된 UE 5.6 source-build 경로를 사용한다.
2. **APK 빌드는 일상 로직 확인 수단이 아니다.** 로직 검증은 `Source/LifeLensCore`의 콘솔 하네스와 테스트를 우선한다. Android 패키징은 수동 마일스톤 검증으로 사용한다.
3. **`LifeLensCore`는 Unreal에 의존하지 않는다.** `#include "CoreMinimal.h"`, `UObject`, `TArray`, `FString` 등 엔진 타입을 코어에서 사용하지 않는다. 표준 C++17만 사용하고 Termux(Android clang)에서도 컴파일 가능해야 한다.
4. 캐릭터 이름을 ID로 쓰지 않는다. 모든 캐릭터는 GUID 또는 코어의 독립 ID를 가진다.
5. Relationship을 숫자 하나로 표현하지 않는다. SPEC 31절의 다차원 구조를 따른다.
6. 무료 범위를 벗어나는 서비스(유료 CI, 유료 API, 유료 클라우드)를 필수 의존성으로 넣지 않는다.
7. `main`에 직접 기능 개발하지 않는다. 자신의 작업 브랜치에서 구현하고 PR로 통합한다. 단, 병렬 협업 동기화를 위한 **상태/문서 전용 갱신**은 `docs/STATE_MANAGEMENT.md` 규칙에 따라 main에 기록할 수 있다.
8. 상대 담당 영역과 `tasks/TEAM_BOARD.md`에서 `DOING`으로 잠긴 파일을 임의로 수정하지 않는다.
9. 공동 소유 파일을 변경하면 PR에 `SHARED FILE CHANGE`와 영향 범위를 명시한다.
10. 실제 artifact가 없으면 APK/패키징 성공이라고 말하지 않는다.
11. **타임아웃/세션 중단 후 이전 행동의 성공 여부를 추측하지 않는다.** branch HEAD, PR, CI를 다시 조회하고 상태 문서를 실제 GitHub와 맞춘 후 재개한다.
12. `TEAM_BOARD`, 이전 채팅, 로컬 상태가 GitHub 실제 branch/PR/Actions와 다르면 GitHub 사실이 우선한다.
13. **기능 commit/PR이 있어도 협업 상태 문서가 최신이 아니면 협업 관점에서 완료가 아니다.**

## 작업 시작 / 재개 순서

코드 수정 전에 반드시 아래를 확인한다.

1. `AGENTS.md`
2. `docs/LIFELENS_SPEC_v1.1.md`
3. `docs/STATE_MANAGEMENT.md`
4. 실제 `main` HEAD / 대상 branch HEAD / PR / CI
5. `tasks/WORK_STATE.md`
6. 역할별 READY 큐 (`tasks/DAGYEOM_READY_QUEUE.md` 등)
7. `tasks/TEAM_BOARD.md`
8. `tasks/HANDOFF_LOG.md`
9. 실제 상태와 문서가 다르면 **문서를 먼저 갱신**
10. 그 다음에만 코드/빌드 작업 시작

타임아웃이나 세션 교체 후에는 먼저 `RECOVERING` 관점으로 위 상태를 대조하고, 마지막 검증 checkpoint에서만 이어간다.

## 작업 방식

- 계획만 서술하지 말고 가능한 범위에서 실제 파일을 생성/수정한다.
- 한 번의 작업은 하나의 명확한 목적을 가진다. 지시서에 없는 기능을 겸사겸사 추가하지 않는다.
- 상대 영역의 데이터/API가 필요하면 직접 내부 구현을 건드리지 말고 `tasks/TEAM_BOARD.md`의 Integration Request에 남긴다.
- 의미 있는 checkpoint(커밋, PR 생성, CI 시작/실패, 수정 완료, blocker 해제, 병합 직전/완료)마다 `tasks/WORK_STATE.md`와 필요한 큐/보드를 갱신한다.
- 성공/실패/중단의 이유는 `tasks/HANDOFF_LOG.md`에 append-only로 남긴다. 상대 branch가 같은 파일을 수정 중이면 덮어쓰지 말고 상태판/PR 코멘트로 우선 공유한 뒤 merge 시 조정한다.
- 변경 후 반드시 완료 조건을 확인하고 실행한 테스트/검증 결과를 커밋 메시지 또는 PR 본문에 적는다.
- 워크플로우 YAML을 수정할 때는 무거운 잡 전에 짧은 사전 검증을 둔다.
- 에러가 나면 정확한 실패 Step과 첫 실제 원인을 확인한 뒤 고친다. 같은 실패를 확인 없이 재실행하지 않는다.
- 사용자가 특정 실패 branch를 그대로 두라고 하면 `FROZEN`으로 기록하고 자동 재실행/수정을 하지 않는다.

## 담당 영역

### 쭌 / sjLim91 / 쭌 측 AI

- `Source/LifeLensCore/**`
- `Source/LifeLens/AI/**`
- `Source/LifeLens/Simulation/**`
- `Source/LifeLens/World/**`
- Save/Load, 관계/연애/가족/세대 로직
- Core ↔ Unreal bridge
- Android build / CI / 테스트 / 통합

### 다겸 / STILLofficial / 다겸 측 AI

- `Source/LifeLens/UI/**`
- Character 외형/표현 계층
- `Content/UI/**`
- `Content/Characters/**`
- Observer HUD / 주민 상세 패널 / 관찰 UX
- 카메라/표현/레이아웃/시각 피드백

세부 규칙과 공동 소유 파일은 `docs/TEAM_WORKFLOW.md`를 따른다.

## 커밋 / 브랜치

- `main`은 항상 통합 가능한 안정 상태를 유지한다.
- 쭌: `jjun/<scope>-<task>` 또는 기존 `task/<number>-...`
- 다겸: `dagyeom/<scope>-<task>`
- 커밋 메시지 접두어: `core:`, `ue:`, `ui:`, `ci:`, `docs:`, `fix:`

## 디렉터리

```text
lifelens-ue/
  AGENTS.md
  BUILD_LESSONS.md
  docs/
    LIFELENS_SPEC_v1.1.md
    BUILD_STRATEGY_v1.2.md
    TEAM_WORKFLOW.md
    STATE_MANAGEMENT.md
  tasks/
    WORK_STATE.md
    DAGYEOM_READY_QUEUE.md
    TEAM_BOARD.md
    HANDOFF_LOG.md
  Source/
    LifeLensCore/        # 순수 C++ 시뮬레이션 (Unreal 무관)
    LifeLens/            # Unreal 게임 모듈
  Config/
  Content/
  .github/
    CODEOWNERS
    workflows/
```
