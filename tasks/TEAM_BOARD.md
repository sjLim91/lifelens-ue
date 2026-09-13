# LifeLens Team Board

이 파일은 쭌(sjLim91)과 다겸(STILLofficial), 그리고 각자의 AI 에이전트가 동시에 작업할 때 사용하는 간단한 작업 잠금/분배 보드다.

상태: `TODO` / `DOING` / `REVIEW` / `DONE` / `BLOCKED`

## Active Work

| 담당 | 브랜치 | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 쭌 AI | `task/03-fast-test` | LifeLensCore ↔ Unreal bridge, 실제 UE 컴파일, Android FAST TEST | `Source/LifeLensCore/**`, `Source/LifeLens/Simulation/**`, build/CI | DOING |
| 다겸 + 다겸 AI | `dagyeom/observer-ui-v2` | Observer HUD v2 + 선택 주민 상세 패널 + 관찰 UX | `Source/LifeLens/UI/**`, UI/Character presentation | TODO |

## 쭌 측 다음 작업

1. 현재 `task/03-fast-test` UHT/UBT 검증 완료.
2. 컴파일 오류가 있으면 첫 실제 compiler error만 수정하고 재검증.
3. Core Bridge 검증 후 PR #2 통합.
4. Android FAST TEST APK 산출물 확인.
5. 이후 Core 관계/감정/기억 확장 작업을 새 브랜치에서 시작.

## 다겸 측 다음 작업

1. `main` 최신 기준으로 `dagyeom/observer-ui-v2` 브랜치 생성.
2. 기본 HUD는 전체 개요만 간결하게 유지.
3. 주민 선택 시 상세 패널 확장: 이름/나이/현재행동/욕구/성격/관계.
4. Simulation/Core 데이터는 읽기 API로만 사용하고 직접 변경 금지.
5. UI에 필요한 데이터가 부족하면 아래 Integration Request에 기록.
6. PR 생성 후 structural preflight 결과와 화면/동작 확인 내용을 PR 본문에 적기.

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
- 다겸 Observer UI PR — 아직 생성 전.

## 완료 후 규칙

작업이 merge되면 해당 행을 `DONE`으로 바꾸고 다음 작업은 새 브랜치에서 시작한다. 오래된 `DOING` 항목이 있으면 새 작업 전에 실제 브랜치/PR 상태를 확인한다.
