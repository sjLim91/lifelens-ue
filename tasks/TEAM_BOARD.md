# LifeLens Team Board

이 파일은 쭌(sjLim91)과 다겸(STILLofficial), 그리고 각자의 AI 에이전트가 동시에 작업할 때 사용하는 작업 잠금/분배 보드다.

**필수 동기화:** 작업 시작 전에 `tasks/HANDOFF_LOG.md` 최신 기록을 읽고, 의미 있는 코드/설정/워크플로우/API 변경이 끝나면 같은 파일에 append-only 인수인계를 남긴다. 상대 AI가 모르는 변경을 남기지 않는다.

상태: `TODO` / `DOING` / `REVIEW` / `DONE` / `BLOCKED`

## Active Work

| 담당 | 브랜치 | 작업 | 소유 범위 | 상태 |
|---|---|---|---|---|
| 쭌 + 쭌 AI | `jjun/genealogy-core-v1`, PR #22 | SPEC 41 Genealogy / Kinship Core v1 | `Source/LifeLensCore/**` | REVIEW |
| 쭌 + 쭌 AI | `task/03-fast-test`, PR #2 | LifeLensCore ↔ Unreal Android 검증 | Bridge/build | BLOCKED — 기존 Run 실패 상태 보존, 재실행 안 함 |
| 다겸 + 다겸 AI | `dagyeom/observer-ui-v2` | Observer HUD v2 + 선택 주민 상세 패널 + 관찰 UX | `Source/LifeLens/UI/**`, UI/Character presentation | TODO |
| 다겸 + 다겸 AI | `dagyeom/ui-foundation-v1` | Issue #24 UI Foundation v1 — Android landscape observer foundation (DQ-01) | `Source/LifeLens/UI/LLObserverUIFoundation.*`, `Content/UI/**` | DOING |

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
| 쭌 + 쭌 AI | `jjun/observer-read-model-v1`, PR #13 | Observer/UI용 Core read DTO + 현재행동/관계 13차원 노출 | Core CI + Preflight PASS, `main` 병합 완료 |
| 쭌 + 쭌 AI | `jjun/romance-core-v1`, PR #14 | SPEC 32 Romance: 독립 고백/수락 판단 및 연애 상태 | Core CI + Preflight PASS, `main` 병합 완료 |
| 쭌 + 쭌 AI | `jjun/household-core-v1`, PR #15 | SPEC 35 Household/Cohabitation | Core CI + Preflight PASS, `main` 병합 완료 |
| 쭌 + 쭌 AI | `jjun/marriage-core-v1`, PR #16 | SPEC 34 약혼/결혼/별거/이혼/사별 상태 | Core CI + Preflight PASS, `main` 병합 완료 |
| 쭌 + 쭌 AI | `jjun/pregnancy-core-v1`, PR #18 | SPEC 36 Pregnancy: 의향/생물학/임신 진행/Needs 영향 | Core CI + Preflight PASS, `main` 병합 완료 |
| 쭌 + 쭌 AI | `jjun/birth-genetics-core-v1`, PR #19 | SPEC 37~38 Birth/Genetics + 부모/Household/LifeHistory 연결 | Core CI + Preflight PASS, `main` 병합 완료 |
| 쭌 + 쭌 AI | `jjun/lifecycle-growth-v1`, PR #20 | SPEC 39 Baby→Elderly 성장 단계/행동 권한/LifeHistory | Core CI + Preflight PASS, `main` 병합 완료 |
| 쭌 + 쭌 AI | `jjun/parenting-core-v1`, PR #21 | SPEC 40 자율 Parenting + 아동 발달 상태 | Core CI + Preflight PASS, `main` 병합 완료 |

## 쭌 측 현재 공유사항

- TASK_03 Android Run `34739283266`은 실제 Android UBT에서 실패했고 Cook/Package/APK 단계에는 도달하지 못했다. 사용자 지시에 따라 실패 상태 그대로 보존하며 재실행하지 않는다.
- P11~P17 life progression Core가 `main`에 병합되어 Romance → Cohabitation → Marriage → Pregnancy → Birth/Genetics → Growth → Parenting 기반이 연결되어 있다.
- `Character`에는 Genetics, 부모/자녀 링크, LifeHistory, LifeStage, ChildDevelopment가 추가되어 있다.
- Parenting은 Feed/Sleep/Bathe/Hold/Play/Educate/Discipline/Comfort/HealthCare를 상황에 따라 평가하며 양육 품질이 Attachment/Trust/Confidence/Stress/SocialSkill/Personality Development에 영향을 준다.
- 현재 PR #22에서 Genealogy / Kinship graph를 검증 중이다.
- Core/Simulation 데이터는 다겸 UI에서 직접 수정하지 않고 읽기 API/DTO를 통해 소비한다.

## 쭌 측 다음 작업

1. PR #22 Genealogy Core CI/Preflight 통과 시 `main` 병합.
2. SPEC 42 Aging: Health/Energy/Movement/Fertility/LifeGoal/FamilyRole 영향 확장.
3. SPEC 43 Death: 사망 상태, grief/memory/social impact, 세대교체 기반.
4. SPEC 44 LifeHistory 이벤트 범위 확장.
5. 이후 최신 Core를 Unreal Bridge에 다시 연결할 때 기존 TASK_03 실패 원인을 반영한 새 검증 브랜치를 사용하고, 실패 Run 자체는 재실행하지 않는다.

## 다겸 측 다음 작업

1. 작업 시작 전 `tasks/HANDOFF_LOG.md`와 이 보드 최신 상태 확인.
2. `dagyeom/observer-ui-v2`에서 기본 HUD는 전체 개요만 간결하게 유지.
3. 주민 선택 시 상세 패널 확장: 이름/나이/LifeStage/현재행동/욕구/성격/감정/관계/가족 요약.
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

- PR #22 `[CORE] Add genealogy and kinship graph` — 쭌 측, Core CI/Preflight 검증 중.
- PR #2 `[UE] Bridge LifeLensCore into Unreal runtime` — 기존 Android 검증 실패로 BLOCKED. 실패 Run 재실행 금지.
- 다겸 Observer UI PR — 아직 생성 전.

## 완료/인수인계 규칙

작업이 merge되면 해당 행을 `DONE`으로 바꾸고 다음 작업은 새 브랜치에서 시작한다. 오래된 `DOING` 항목이 있으면 새 작업 전에 실제 브랜치/PR 상태를 확인한다.

모든 실제 변경은 상대 AI가 추적 가능하도록 `tasks/HANDOFF_LOG.md`에 남긴다. **커밋/PR이 있는데 HANDOFF가 없으면 작업 완료로 보지 않는다.**
