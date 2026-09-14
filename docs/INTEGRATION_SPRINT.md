# LifeLens Integration Sprint Protocol

이 문서는 쭌(sjLim91) / 다겸(STILLofficial) / 양쪽 AI가 각자 소유하던 작업을 한 시점에 통합할 때 사용하는 임시 협업 모드다.

목표는 **쭌이 다겸 작업을 적극적으로 도울 수 있게 하면서도, 다겸 브랜치/파일/stacked PR을 덮어쓰거나 꼬이게 하지 않는 것**이다.

이 문서는 `docs/TEAM_WORKFLOW.md`와 `docs/STATE_MANAGEMENT.md`를 보완한다. 충돌 시 GitHub 실제 상태 → STATE_MANAGEMENT → 이 문서 → TEAM_WORKFLOW 순으로 해석한다.

## 1. 현재 Integration Sprint 진입 조건

현재 쭌 측은 Civilization Core/Bridge 기반을 빠르게 앞서 구축했고, 다겸 측은 Observer/UI/Presentation PR chain을 최신 main에 재정렬해야 한다.

따라서 PR #53 Civilization Observer Read DTOs v1이 merge되면 다음 checkpoint까지 **Jjun Integration Support Mode**로 전환한다.

Integration checkpoint 완료 조건:
- PR #17 Observer HUD v2 latest-main reconcile + current Bridge binding + review fixes + validation
- PR #26 UI Foundation latest-main reconcile + validation
- stacked chain #29/#30 → #36 → #38의 base/merge order 정리
- Core civilization + Observer UI + Character Presentation이 한 runtime에서 함께 읽히는 통합 확인

이 checkpoint가 끝날 때까지 쭌 측은 blocker 수정 외의 큰 신규 Core feature slice를 잠시 보류한다.

## 2. 기본 원칙 — 도움은 가능하지만 소유권은 유지

다겸 기본 소유권은 유지한다:
- `Source/LifeLens/UI/**`
- Character appearance/presentation 계층
- `Content/UI/**`
- `Content/Characters/**`

쭌 기본 소유권도 유지한다:
- `Source/LifeLensCore/**`
- `Source/LifeLens/AI/**`
- `Source/LifeLens/Simulation/**`
- `Source/LifeLens/World/**`
- Save/Load / Core↔Unreal Bridge / Build / CI

Integration Sprint는 이 소유권을 없애는 것이 아니라 **일시적 assistance lock을 명시적으로 빌릴 수 있게 하는 절차**다.

## 3. 쭌이 다겸을 돕는 3단계 모드

### A. REVIEW_ONLY — 기본

쭌/쭌 AI는:
- 다겸 PR/branch/diff/CI를 읽는다.
- 충돌/리뷰/최신 main 차이를 분석한다.
- PR 코멘트/TEAM_BOARD에 정확한 수정 제안을 남긴다.
- 다겸 소유 파일을 직접 수정하지 않는다.

별도 잠금이 필요 없다.

### B. ASSIST_PATCH — 실제 코드 도움

사용자가 쭌에게 다겸 작업 수정을 지시하거나, 다겸 측에서 도움을 요청한 경우에만 사용한다.

절차:
1. 실제 target PR/branch HEAD와 CI를 다시 확인한다.
2. `TEAM_BOARD.md`에 `ASSIST_LOCK`을 등록한다.
3. 잠글 파일/디렉터리를 정확히 적는다.
4. **다겸 브랜치에 직접 push하지 않는다.**
5. `integration/dagyeom-<scope>-assist` 브랜치를 target branch HEAD에서 만든다.
6. 쭌은 잠긴 파일만 수정한다.
7. 검증 후 assist branch의 diff/commit을 다겸 target branch에 합치거나 PR로 전달한다.
8. merge/handoff 후 `ASSIST_LOCK`을 즉시 해제한다.

ASSIST_LOCK이 걸린 동안 다겸/다겸 AI는 동일 파일을 수정하지 않는다.

### C. OWNERSHIP_TRANSFER — 명시적 인계

사용자가 특정 다겸 작업을 쭌에게 완전히 넘기라고 명시한 경우에만 사용한다.

- TEAM_BOARD에 `OWNERSHIP_TRANSFER: DAGYEOM -> JJUN`을 기록한다.
- 대상 PR/branch/files와 종료 조건을 명시한다.
- 종료 후 원래 소유권으로 복귀할지 계속 쭌 소유인지 기록한다.
- 말로만 인계된 상태에서 파일을 수정하지 않는다.

## 4. Stacked PR 보호 규칙

현재 다겸 chain:
- #17 Observer HUD v2
- #29 Character Presentation — stacked on #17
- #30 Observer UX Polish — stacked on #17
- #36 Mobile Touch — stacked on #30
- #38 Visual Feedback — stacked on #36

Integration Sprint에서는 **부모부터 한 단계씩** 처리한다.

1. #17을 먼저 latest main과 reconcile한다.
2. #17이 안정화/merge되기 전에는 #29/#30의 내용을 대규모 재작성하지 않는다.
3. #17 이후 #29와 #30을 최신 base로 각각 정리한다.
4. #36은 #30이 정리된 뒤 처리한다.
5. #38은 #36 이후 처리한다.

#26 UI Foundation은 독립적으로 reconcile 가능하지만 #17과 같은 파일을 수정한다면 ASSIST_LOCK 충돌 여부를 먼저 확인한다.

**여러 stacked branch를 한 번에 force update/rebase하지 않는다.** 각 단계마다 GitHub HEAD/PR/base/CI를 다시 확인한다.

## 5. Branch 안전 규칙

- `dagyeom/*` branch에 쭌 AI가 직접 push 금지.
- `jjun/*` branch에 다겸 AI가 직접 push 금지.
- 협업 패치는 `integration/*-assist` 사용.
- 기능 branch에서 상태문서만 고치기 위해 불필요하게 merge/rebase하지 않는다. 상태문서는 main에 직접 동기화 가능.
- assist branch 생성 기준 SHA를 TEAM_BOARD에 기록한다.
- target branch가 assist 중 움직이면 자동으로 덮어쓰지 않고 compare 후 재조정한다.

## 6. 파일 잠금 형식

TEAM_BOARD의 Integration Request/Assist Lock에는 최소 다음을 적는다.

- Mode: REVIEW_ONLY / ASSIST_PATCH / OWNERSHIP_TRANSFER
- Target owner
- Target PR / branch
- Base HEAD SHA
- Locked paths
- Helper branch
- Reason
- Status
- Unlock condition

예시:

`ASSIST_PATCH | Dagyeom | PR #17 / dagyeom/observer-ui-v2 | HEAD abc123 | Source/LifeLens/UI/LLObserverHUD.cpp | integration/dagyeom-observer-hud-assist | narrow-canvas fixes | DOING | merge/handoff 후 unlock`

## 7. Shared files

다음은 assist 중에도 shared로 취급한다:
- `LifeLens.uproject`
- `Source/LifeLens/LifeLens.Build.cs`
- `Source/LifeLens/Core/LLTypes.h`
- `Source/LifeLens/Simulation/**` 공개 Bridge contract
- `Config/**`
- `.github/workflows/**`
- `Tools/**` 공용 validator

다겸 UI 수정 때문에 Core/Bridge 변경이 필요하면 UI branch에서 임시로 고치지 않는다.

1. TEAM_BOARD Integration Request 생성
2. 쭌 소유 branch에서 Bridge/API 수정
3. Core/Preflight/필요 시 UHT·UBT 검증
4. main merge
5. 다겸 target branch가 최신 main을 받아 API를 사용

## 8. 통합 중 신규 Core 기능 동결

Integration Sprint 동안 쭌 측 신규 대형 기능은 기본적으로 보류한다.

허용:
- 다겸 통합 blocker 해제
- 컴파일/CI/SaveLoad/Bridge 결함 수정
- 이미 공개된 API의 최소 보강
- 통합 runtime에 필요한 World/Simulation adapter 수정

보류:
- 새 대형 Civilization production chain
- 농업/금속/경제 등 별도 feature expansion
- 다겸이 아직 소비하지 못하는 추가 read model 대량 확장

목적은 쭌이 계속 앞서 나가서 통합 거리가 다시 벌어지는 것을 막는 것이다.

## 9. 완료 기준

Integration Sprint는 단순히 PR이 open/merge된 것으로 끝나지 않는다.

완료 조건:
- 관련 PR CI green
- latest main 기준 branch 관계 정리
- Observer가 authoritative Core data를 읽음
- UI가 simulation authority를 복제하지 않음
- Character presentation이 Core action을 표현만 함
- 한 통합 runtime에서 주민/행동/관계/문명 read 상태 확인
- 상태문서/HANDOFF 최신화

그 다음 Android Smoke APK milestone으로 간다.
