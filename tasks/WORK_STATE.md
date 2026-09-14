# LifeLens Canonical Work State

> 이 파일은 **현재 진행 상태의 단일 기준판**이다. 쭌/다겸/양쪽 AI는 작업 시작 전에 반드시 읽고 실제 GitHub 상태와 대조한다.
>
> 제품 요구사항은 `docs/LIFELENS_SPEC_v1.1.md`, 상태관리 규칙은 `docs/STATE_MANAGEMENT.md`, 역할/잠금은 `tasks/TEAM_BOARD.md`, 변경 이력은 `tasks/HANDOFF_LOG.md`를 따른다.

Last reconciled: 2026-09-14 KST — Observer Runtime Bridge v1 PR #37 opened; structural Preflight PASS; Unreal compile validation pending

## Status legend

`PLANNED` / `IN_PROGRESS` / `WAITING_CI` / `READY_TO_MERGE` / `BLOCKED` / `INTERRUPTED` / `RECOVERING` / `FROZEN` / `DONE`

---

## Active / unresolved work

### 1. P24 Observer Read Model v2

- Owner: 쭌 + 쭌 AI
- Branch: `jjun/observer-read-model-v2`
- Work item: Emotion / family summary / world aggregate Core read DTOs.
- Last known HEAD: `86b916aef27e5df7c6d5e9a30f49c61a25fd971c`
- PR: #34 `[CORE] Expand observer read model for family and world overview`
- Status: `DONE`
- Merge SHA: `b4403faf138c153bcd83be266cde5026ea53b831`
- CI: refreshed Core Tests Run `34794030200` PASS; Preflight Run `34794030209` PASS.
- Last verified fact: PR #34 is merged. 27/27 local CMake Release tests (assertions enabled), deterministic seed 42 harness and structural preflight passed before merge. Remote tree `516e33a` equaled the tested local merge tree.
- Blocker / interruption: None.
- Exact next action: No further feature edits on this branch. Runtime exposure proceeds in separate `jjun/observer-runtime-bridge-v1`.
- Handoff safety: `SAFE`.
- Shared-file impact: Core DTO/test/CMake + append-only HANDOFF_LOG. No UI/TASK_03 changes. Unreal bridge is intentionally separate.
- Contract notes: `majorLifeEvents` is per-character LifeHistory record count, not unique events; GenerationContinuity still uses its separate API. Core Simulation does not yet own the family books required for live family/world family aggregates.

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
  - 감정/관계/가족/SocialIntent/월드 집계 일부는 쭌 측 Unreal read API 대기
  - API 대기는 다겸 전체 작업 BLOCK이 아니라 `BLOCKED-BY-JJUN` 항목으로 분리함
- Blocker / interruption: PR #17 merge 자체는 actual UHT/UBT 및 shared-doc reconciliation이 필요하지만, 다겸은 별도 READY NOW 작업을 계속할 수 있음.
- Exact next action: `tasks/DAGYEOM_READY_QUEUE.md` READY NOW 작업 진행. Observer Runtime Bridge v1에서 제공되는 실제 Core resident/emotion/relationship/activity read API를 소비하는 integration queue를 추가하되, family/world family aggregate는 별도 authoritative-state 작업 전까지 대기.
- Handoff safety: `CONDITIONAL`
- Shared-file impact: PR #17에 shared docs 변경 있음. 최신 main 상태 문서를 덮어쓰지 않도록 병합 전 reconciliation 필요.

### 3. TASK_03 Core ↔ Unreal / Android validation

- Owner: 쭌 + 쭌 AI
- Branch: `task/03-fast-test`
- Work item: LifeLensCore ↔ Unreal bridge, Android compile/package 검증, no-engine-rebuild pipeline
- Last known HEAD: `f8f461a8ec669ba65ad6dd669e6bac45f186d230`
- PR: #2 `[UE] Bridge LifeLensCore into Unreal runtime`
- Status: `FROZEN`
- CI / Build: Android Run `34739283266` — FAILURE; Cook/Package/APK 미도달.
- Blocker / interruption: 사용자 지시에 따라 실패 상태 그대로 보존.
- Exact next action: **없음. 자동 재실행/수정 금지.** 최신 main 기반 새 integration task는 TASK_03 자체와 분리해서 진행.
- Handoff safety: `SAFE`
- Shared-file impact: 오래된 Build/Simulation/shared files 포함. main에 그대로 병합 금지.

### 4. Core validation recovery v1

- Owner: 쭌 + 쭌 AI
- Branch: `jjun/core-validation-recovery-v1`
- Work item: Restore real Release assertions and fix invalidated RelationshipBook references.
- Last known HEAD: `612f98f4499f50d359a814df33e3d0b913259f6f`
- PR: #35 `[CORE] Fix dangling relationship references and restore Release test assertions`
- Status: `DONE`
- Merge SHA: `31b2263de3f4a9c80650d1139213d0c38acc8058`
- CI: Core Tests Run `34793799376` PASS; Preflight Run `34793799447` PASS.
- Last verified fact: 26/26 local CMake Release tests passed with assertions enabled; all test compile commands restore assertions after NDEBUG. Deterministic one-day seed 42 harness and structural preflight passed. Original heap-use-after-free reproduced with ASan; corrected 1,000-insertion test passed ASan/UBSan. Remote tree `94fe2a7` equals tested local tree.
- Blocker / interruption: fixed.
- Exact next action: No further feature edits here.
- Handoff safety: `SAFE`.
- Shared-file impact: Core headers/tests/CMake plus append-only HANDOFF_LOG. No UI/Unreal/TASK_03 changes.

### 5. Observer Runtime Bridge v1

- Owner: 쭌 + 쭌 AI
- Branch: `jjun/observer-runtime-bridge-v1`
- Work item: Expose authoritative LifeLensCore resident observation data to Unreal without modifying the frozen TASK_03 branch or 다겸 UI files.
- Base: current `main` at P24 merge `b4403faf138c153bcd83be266cde5026ea53b831`.
- Last verified feature HEAD: `1cd3139d4756386d4bd4ee3a3b59d2c74467ed7a` (subsequent commits are state/handoff docs only).
- PR: #37 `[UE] Expose LifeLensCore observer runtime read API`
- Status: `IN_PROGRESS`
- CI: LifeLens Preflight Run `34794889890` PASS at feature HEAD `1cd3139d...`; latest documentation-only head will receive the same PR Preflight gate.
- Last verified fact:
  - actual bridge implementation exists on remote branch and PR #37
  - structural preflight checks required bridge files/contracts and rejects Unreal reflection leakage into `Source/LifeLensCore/**`
  - branch was 7 commits ahead / 0 behind main at feature checkpoint; no Dagyeom UI file and no TASK_03 workflow/branch file was changed
- Implemented scope:
  - new Core bridge on latest main, not by reviving PR #2
  - deterministic `(WorldSeed, Core CharacterId)` → Unreal `FGuid`; names are display-only and never identifiers
  - read-only resident Needs; 11 Emotion axes + summaries; current Physical/Social activity and target; 13D directional Relationship + derived scores; Memory/Belief counts; simulation minute/living counts
  - `OnCoreRuntimeStateChanged` for Observer consumers
  - Core remains pure C++17; Unreal reflection/types stay in `Source/LifeLens/**`
  - no modification to `Source/LifeLens/UI/**`
- Explicit non-goal: do not present P24 family/household/romance/pregnancy aggregates as live until Core `Simulation` owns authoritative `GenealogyBook` / `RomanceBook` / `HouseholdBook` / `PregnancyBook` state.
- Validation gap: actual Unreal UHT/UBT compile has **not** been run for PR #37 yet; structural PASS is not treated as compile/APK success. Local container could not clone GitHub because outbound DNS is unavailable.
- Blocker / interruption: no product-code blocker. Need a separate latest-main Unreal compile validation path that does not modify/revive frozen TASK_03 or blindly repeat its long failed build.
- Exact next action: inspect available current-main CI/cache paths and choose the shortest isolated UHT/UBT compile validation for PR #37. Do not start full APK/package work merely to validate this bridge. After compile PASS, mark READY_TO_MERGE and publish the API contract to Dagyeom integration queue.
- Handoff safety: `SAFE` — remote branch, PR, feature SHA, Preflight Run and remaining validation gap are explicit.
- Shared-file impact: `Source/LifeLens/LifeLens.Build.cs`, `Source/LifeLens/Simulation/**`, `Tools/validate_bootstrap.py`, `tasks/WORK_STATE.md`, append-only `tasks/HANDOFF_LOG.md`. No UI/TASK_03 workflow edits.

---

## Dagyeom READY NOW

정식 큐: `tasks/DAGYEOM_READY_QUEUE.md`

1. `dagyeom/ui-foundation-v1` — Android landscape typography / spacing / safe-area / icon / font foundation
2. `dagyeom/character-presentation-v1` — nameplate / LifeStage badge / selected-focus feedback / display LOD
3. `dagyeom/observer-ux-polish-v1` — World→Quick→Detail 전환, 뒤로가기/선택해제/empty-state 정리
4. `dagyeom/mobile-touch-v1` — hit target / safe-area / 작은 화면 scroll/overflow/tab UX
5. `dagyeom/visual-feedback-v1` — 선택/관찰 레벨/주목 대상의 비침투적 시각 피드백

`BLOCKED-BY-JJUN`: Family summary / family-derived World aggregate live Unreal read APIs remain blocked on authoritative family-state ownership. Resident Relationship / Emotion / SocialIntent/current activity/basic World resident counts are implemented in PR #37 and become READY for Dagyeom binding after Unreal compile validation + merge.

---

## Recently completed product milestones

- P11 Romance Core — `DONE`.
- P12 Household / Cohabitation — `DONE`.
- P13 Marriage — `DONE`.
- P14 Pregnancy — `DONE`.
- P15 Birth / Genetics — `DONE`.
- P16 Lifecycle Growth — `DONE`.
- P17 Parenting / Child Development — `DONE`.
- P18 Genealogy / Kinship — `DONE`, merge `d1324c766749847c9fc85ea69af60c23c293da31`.
- P19 Aging Core — `DONE`, PR #23 merge `82b4001058332656b08f13a47728079172b10962`.
- P20 Death Core — `DONE`, PR #25 merge `07509a73f235928c16bf9b26f8b13c689b6a030b`.
- P21 LifeHistory Core — `DONE`, PR #27 merge `b46869acc62830c432bb5b3693f98535c37f2e0a`.
- P22 LifeHistory Wiring — `DONE`, PR #32 merge `8454c0c91e1770e2e503d52d5489446beafe6535`.
- P23 Generation Continuity — `DONE`, Core CI + deterministic harness + Preflight PASS; PR #33 merge `37bc2e7af7924fdf8a262d086426e15f2c3e2db3`.
- P24 Observer Read Model v2 — `DONE`, PR #34 merge `b4403faf138c153bcd83be266cde5026ea53b831`.

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
