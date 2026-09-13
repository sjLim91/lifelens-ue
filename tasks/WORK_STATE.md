# LifeLens Canonical Work State

> 이 파일은 **현재 진행 상태의 단일 기준판**이다. 쭌/다겸/양쪽 AI는 작업 시작 전에 반드시 읽고 실제 GitHub 상태와 대조한다.
>
> 제품 요구사항은 `docs/LIFELENS_SPEC_v1.1.md`, 상태관리 규칙은 `docs/STATE_MANAGEMENT.md`, 역할/잠금은 `tasks/TEAM_BOARD.md`, 변경 이력은 `tasks/HANDOFF_LOG.md`를 따른다.

Last reconciled: 2026-09-13 KST

## Status legend

`PLANNED` / `IN_PROGRESS` / `WAITING_CI` / `READY_TO_MERGE` / `BLOCKED` / `INTERRUPTED` / `RECOVERING` / `FROZEN` / `DONE`

---

## Active / unresolved work

### 1. P20 Death Core v1

- Owner: 쭌 + 쭌 AI
- Branch: `jjun/death-core-v1`
- Work item: SPEC 43 Death — 사망 상태, LifeHistory, grief/memory/social impact, 배우자 사별, 세대교체 기반
- Last known HEAD: `82b4001058332656b08f13a47728079172b10962` (branch creation checkpoint)
- PR: 없음 — 구현 전
- Status: `IN_PROGRESS`
- CI: 미실행
- Last verified fact: P19 Aging PR #23 was squash-merged into `main` as `82b4001058332656b08f13a47728079172b10962`; `jjun/death-core-v1` was created from that exact merge SHA.
- Blocker / interruption: 없음.
- Exact next action: Death state/model 구현 → grief/LifeHistory/relationship-family effects 연결 → C++17 test 등록 → PR → Core CI + Preflight.
- Handoff safety: `SAFE`
- Shared-file impact: `Source/LifeLensCore/**` + Core tests only. TASK_03 / 다겸 UI 파일 건드리지 않음.

### 2. Observer HUD v2

- Owner: 다겸 + 다겸 AI
- Branch: `dagyeom/observer-ui-v2`
- Work item: Observer LEVEL 0 overview / LEVEL 1 quick inspector / LEVEL 2 detail tabs
- Last known HEAD: `3b578c67528c564064c73a76c1eb4f16f25e489f`
- PR: #17 `[UI] Observer HUD v2: LEVEL 0 overview, LEVEL 1 quick inspector, LEVEL 2 detail tabs`
- Status: `IN_PROGRESS`
- CI:
  - latest observed LifeLens Preflight Run `34756163064` — PASS
  - actual UHT/UBT / 화면 동작 검증 — PENDING
- Last verified fact:
  - PR #17은 열려 있고 structural preflight는 통과함
  - UI branch는 실제 기능 커밋 13개를 포함하며 다겸 쪽 작업은 존재함
  - 감정/관계/가족/SocialIntent/월드 집계 일부는 쭌 측 Unreal read API 대기
  - 이 API 대기는 다겸 전체 작업 BLOCK이 아니라 `BLOCKED-BY-JJUN` 항목으로 분리함
- Blocker / interruption: PR #17 merge 자체는 actual UHT/UBT 및 shared-doc reconciliation이 필요하지만, 다겸은 별도 READY NOW 작업을 계속할 수 있음.
- Exact next action: `tasks/DAGYEOM_READY_QUEUE.md`에서 READY NOW 작업을 하나 선택해 새 `dagyeom/*` 브랜치에서 착수. PR #17 파일과 겹치면 stacked branch로 분리.
- Handoff safety: `CONDITIONAL`
- Shared-file impact: PR #17에 `tasks/TEAM_BOARD.md`, `tasks/HANDOFF_LOG.md` 변경 있음. 최신 main 상태 문서를 덮어쓰지 않도록 병합 전 reconciliation 필요.

### 3. TASK_03 Core ↔ Unreal / Android validation

- Owner: 쭌 + 쭌 AI
- Branch: `task/03-fast-test`
- Work item: LifeLensCore ↔ Unreal bridge, Android compile/package 검증, no-engine-rebuild pipeline
- Last known HEAD: `f8f461a8ec669ba65ad6dd669e6bac45f186d230`
- PR: #2 `[UE] Bridge LifeLensCore into Unreal runtime`
- Status: `FROZEN`
- CI / Build:
  - Android Run `34739283266` — FAILURE
  - UHT passed before UBT compile failure
  - failure occurred in `Compile LifeLens Android Development`
  - Cook/Package never started
  - APK artifact 없음
- Last verified root cause: 오래된 TASK_03 branch의 `LLCoreBridgeSubsystem`에서 Core `lifelens` namespace와 Unreal `LifeLens` symbol 경계 관련 실제 UBT compile conflict가 발생.
- Blocker / interruption: 사용자가 TASK_03 실패 상태를 그대로 두고 다음 기능으로 진행하라고 명시함.
- Exact next action: **없음. 자동 재실행/수정 금지.** 사용자가 TASK_03 재개를 명시하거나 최신 main 기반 새 integration task를 시작할 때만 별도 복구 계획 작성.
- Handoff safety: `SAFE`
- Shared-file impact: branch가 오래되었고 Build/Simulation/shared files를 포함하므로 main에 그대로 병합 금지.

### 4. UI Foundation v1 (Issue #24 / DQ-01)

- Owner: 다겸 + 다겸 AI
- Branch: `dagyeom/ui-foundation-v1`
- Work item: Issue #24 `[DAGYEOM] UI Foundation v1 — Android landscape observer foundation`
- Last known HEAD: `b512f54`
- PR: #26 `[UI] UI Foundation v1 — Android landscape observer foundation (Issue #24)`
- Status: `WAITING_CI`
- CI:
  - 로컬 `Build.sh LifeLensEditor Mac Development` — Result: Succeeded (UE 5.6.1, Xcode 허용 범위)
  - 로컬 `Tools/validate_bootstrap.py` — PASS
  - GitHub Actions — 대기
- Last verified fact: `Source/LifeLens/UI/LLObserverUIFoundation.h/.cpp` 추가. 기존 파일 수정 없음.
- Blocker / interruption: 없음
- Exact next action: PR 생성 → `WAITING_CI` → structural preflight 결과 기록.
- Handoff safety: `SAFE`
- Shared-file impact: `tasks/WORK_STATE.md`, `tasks/TEAM_BOARD.md`, `tasks/HANDOFF_LOG.md`에 행/항목 추가만.

---

## Dagyeom READY NOW

정식 큐: `tasks/DAGYEOM_READY_QUEUE.md`

현재 즉시 가능한 작업:

1. `dagyeom/ui-foundation-v1` — Android landscape typography / spacing / safe-area / icon / font foundation
2. `dagyeom/character-presentation-v1` — nameplate / LifeStage badge / selected-focus feedback / display LOD
3. `dagyeom/observer-ux-polish-v1` — World→Quick→Detail 전환, 뒤로가기/선택해제/empty-state 정리
4. `dagyeom/mobile-touch-v1` — hit target / safe-area / 작은 화면 scroll/overflow/tab UX
5. `dagyeom/visual-feedback-v1` — 선택/관찰 레벨/주목 대상의 비침투적 시각 피드백

`BLOCKED-BY-JJUN`: Relationship / Emotion / SocialIntent / Family summary / World aggregate Unreal read APIs. 이것들은 쭌 측 backlog이며 다겸이 Core를 직접 수정하지 않는다.

READY NOW가 0개가 되면 다겸 측은 `할 일 없음`으로 종료하지 않고 `NEEDS_ASSIGNMENT`로 보고 SPEC의 UI/Observer/Character presentation 범위에서 다음 작업을 즉시 채운다.

---

## Recently completed product milestones

- P11 Romance Core — `DONE`, main merged.
- P12 Household / Cohabitation — `DONE`, main merged.
- P13 Marriage — `DONE`, main merged.
- P14 Pregnancy — `DONE`, main merged.
- P15 Birth / Genetics — `DONE`, main merged.
- P16 Lifecycle Growth — `DONE`, main merged.
- P17 Parenting / Child Development — `DONE`, main merged.
- P18 Genealogy / Kinship — `DONE`, main merged as `d1324c766749847c9fc85ea69af60c23c293da31`.
- P19 Aging Core — `DONE`, Core CI + Preflight PASS; PR #23 squash-merged as `82b4001058332656b08f13a47728079172b10962`.

P18 first Core test attempt failed only in `test_genealogy` because `std::vector` growth invalidated stored references in spouse/birth registration. The bug was fixed, rerun Core Tests + deterministic harness + Preflight all passed, then PR #22 was merged.

---

## Shared-state system

- `docs/STATE_MANAGEMENT.md` — current protocol.
- `tasks/WORK_STATE.md` — this live state file.
- `tasks/DAGYEOM_READY_QUEUE.md` — 다겸 즉시 실행 큐.
- `tasks/TEAM_BOARD.md` — ownership / locks / integration requests.
- `tasks/HANDOFF_LOG.md` — append-only history.

### Mandatory recovery rule

If a session/tool/chat times out before normal completion:

1. Do not assume the previous action succeeded or failed.
2. Fetch actual branch HEAD.
3. Fetch PR state/head SHA.
4. Fetch related CI/Actions state.
5. Compare with this file.
6. Mark/reconcile state before changing code.
7. Resume only from the last verified GitHub checkpoint.

GitHub facts always override stale text in this file; when they differ, update this file first.
