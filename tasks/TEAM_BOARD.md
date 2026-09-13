# LifeLens Team Board

이 파일은 쭌(sjLim91)과 다겸(STILLofficial), 그리고 각자의 AI 에이전트가 동시에 작업할 때 사용하는 작업 잠금/분배 보드다.

**필수 동기화:** 작업 시작 전에 `tasks/HANDOFF_LOG.md` 최신 기록을 읽고, 의미 있는 코드/설정/워크플로우/API 변경이 끝나면 같은 파일에 append-only 인수인계를 남긴다. 상대 AI가 모르는 변경을 남기지 않는다.

상태: `TODO` / `DOING` / `REVIEW` / `DONE` / `BLOCKED`

## Active Work

| 담당 | 브랜치 | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 쭌 AI | `task/03-fast-test` | LifeLensCore ↔ Unreal bridge 실제 UE/Android 검증 + 재빌드 방지 파이프라인 | `Source/LifeLens/Simulation/**`, build/CI | DOING |
| 쭌 + 쭌 AI | `jjun/observer-read-model-v1` | 다겸 UI/Observer용 안전한 Core read DTO + 현재행동/관계 요약 | `Source/LifeLensCore/**` 중 ObserverReadModel/Simulation read API | DOING |
| 다겸 + 다겸 AI | `dagyeom/observer-ui-v2` | Observer HUD v2 + 선택 주민 상세 패널 + 관찰 UX | `Source/LifeLens/UI/**`, UI/Character presentation | TODO |

## 완료된 병렬 작업

| 담당 | 브랜치/PR | 작업 | 결과 |
|---|---|---|---|
| 쭌 + 쭌 AI | `jjun/relationship-core-v1`, PR #4 | SPEC 31 다차원 Relationship Core v1 | Core CI + Preflight PASS, `main` 병합 완료 |
| 쭌 + 쭌 AI | `jjun/emotion-core-v1`, PR #6 | SPEC 25 다차원 Emotion Core v1 | Core CI + Preflight PASS, `main` 병합 완료 |
| 쭌 + 쭌 AI | `jjun/memory-core-v1`, PR #7 | SPEC 29~30 구조화 Memory Core v1 | Core CI + Preflight PASS, `main` 병합 완료 |
| 쭌 + 쭌 AI | `jjun/belief-core-v1`, PR #8 | SPEC 30 Memory→Belief Core v1 | Core CI + Preflight PASS, `main` 병합 완료 |
| 쭌 + 쭌 AI | `jjun/social-cognition-v1`, PR #9 | Social Event→Emotion→Memory→Belief→Relationship 연결 | Core CI + Preflight PASS, `main` 병합 완료 |
| 쭌 + 쭌 AI | `jjun/social-utility-v1`, PR #10 | Relationship/Emotion/Memory/Belief를 Unified Utility Decision에 반영 | Core CI + Preflight PASS, `main` 병합 완료 |
| 쭌 + 쭌 AI | `jjun/social-execution-v1`, PR #11 | Approach/Repair/Comfort/Avoid 실제 Core 상태/사회 사건 실행 | Core CI + Preflight PASS, `main` 병합 완료 |
| 쭌 + 쭌 AI | `jjun/simulation-social-loop-v1`, PR #12 | SocialIntent를 실제 `Simulation::step()`에 연결 | Core CI + Preflight PASS, `main` 병합 완료 |

## 쭌 측 현재 공유사항

- PR #1 TASK_02 LifeLensCore는 `main` 병합 완료, Core CI PASS.
- PR #2 TASK_03 Core ↔ Unreal Bridge는 structural preflight PASS, 실제 Android UHT/UBT 검증 중.
- Android Run `34739283266`은 SHA `4a8b8d494d7e953d0eb98c2f322cdec3595a45e2` 기준이며 여전히 `in_progress`; 이후 Core/CI 변경은 포함하지 않는다.
- PR #3 Android fast-reuse는 `task/03-fast-test`에 병합 완료. `seed / fast / full` 모드로 분리하고 fast에서는 엔진 전체 재컴파일을 금지한다.
- PR #4 Relationship, #6 Emotion, #7 Memory, #8 Belief, #9 Social Cognition, #10 Social Utility, #11 Social Execution, #12 Simulation Social Loop는 모두 Core CI + Preflight PASS 후 `main` 병합 완료.
- TASK_03 장시간 빌드와 별개로 `jjun/observer-read-model-v1`에서 다겸 UI가 안전하게 소비할 read DTO를 병렬 진행한다.
- 자세한 변경 이유/검증 상태는 `tasks/HANDOFF_LOG.md`를 기준으로 한다.

## 쭌 측 다음 작업

1. 현재 TASK_03 실제 UHT/UBT 결과 확인.
2. 컴파일 오류가 있으면 첫 실제 compiler error만 수정하고 재검증.
3. `jjun/observer-read-model-v1`에서 이름/Needs/감정/현재행동/관계 13차원 요약 DTO와 Simulation read API 검증.
4. Core Bridge 검증 후 PR #2 통합.
5. Android fast pipeline의 `seed` 1회 생성 및 `fast` 실제 검증.

## 다겸 측 다음 작업

1. 작업 시작 전 `tasks/HANDOFF_LOG.md`와 이 보드 최신 상태 확인.
2. `dagyeom/observer-ui-v2`에서 기본 HUD는 전체 개요만 간결하게 유지.
3. 주민 선택 시 상세 패널 확장: 이름/나이/현재행동/욕구/성격/관계.
4. Simulation/Core 데이터는 읽기 API로만 사용하고 직접 변경 금지.
5. UI에 필요한 데이터가 부족하면 아래 Integration Request에 기록.
6. PR 생성 후 structural preflight 결과와 화면/동작 확인 내용을 PR 본문에 적기.
7. 작업 종료 시 `tasks/HANDOFF_LOG.md`에 변경 파일/검증/쭌 측 영향도를 기록.

## Shared File Lock

다음 파일은 현재 기본 소유자가 쭌 측이다. 다겸 측에서 변경하지 말고 요청을 남긴다.

- `LifeLens.uproject`
- `Source/LifeLens/LifeLens.Build.cs`
- `Source/LifeLens/Core/LLTypes.h`
- `Source/LifeLens/Simulation/**`
- `Source/LifeLensCore/**`
- `Config/**`
- `.github/workflows/**`
- `Tools/validate_bootstrap.py`

반대로 아래는 다겸 측 작업 중 쭌 측에서 가능한 한 건드리지 않는다.

- `Source/LifeLens/UI/**`
- Character의 외형/표현 전용 코드
- `Content/UI/**`
- `Content/Characters/**`

## Integration Requests

새 요청은 아래 형식으로 추가한다.

```text
[OPEN] 요청자: 쭌/다겸
필요 API/데이터:
사용 목적:
희망 반환형/방향:
관련 브랜치:
```

현재 열린 요청: 없음.

## Merge Queue

- PR #2 `[UE] Bridge LifeLensCore into Unreal runtime` — 쭌 측, 실제 UHT/UBT 검증 후 merge.
- PR #3 `[CI] Reuse compiled UE Android engine outputs` — PR #2 브랜치에 병합 완료, main 반영은 PR #2와 함께 진행.
- `jjun/observer-read-model-v1` Observer Read Model PR — 구현/코어 테스트 후 생성 예정.
- 다겸 Observer UI PR — 아직 생성 전.

## 완료/인수인계 규칙

작업이 merge되면 해당 행을 `DONE`으로 바꾸고 다음 작업은 새 브랜치에서 시작한다. 오래된 `DOING` 항목이 있으면 새 작업 전에 실제 브랜치/PR 상태를 확인한다.

모든 실제 변경은 상대 AI가 추적 가능하도록 `tasks/HANDOFF_LOG.md`에 남긴다. **커밋/PR이 있는데 HANDOFF가 없으면 작업 완료로 보지 않는다.**
