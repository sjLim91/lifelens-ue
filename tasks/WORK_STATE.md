# LifeLens Canonical Work State

> 현재 진행 상태의 단일 기준판. 실제 GitHub 상태가 항상 우선한다.
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` + `docs/CIVILIZATION_PROGRESSION_v1.md`
> 협업 기준: `docs/STATE_MANAGEMENT.md` + `docs/INTEGRATION_SPRINT.md`

Last reconciled: 2026-09-14 KST.

## Mandatory sync gate

1. 작업 시작/재개 전 actual `main`, target branch/PR, Actions를 확인한다.
2. `WORK_STATE.md` → 역할별 READY queue → `TEAM_BOARD.md` → `HANDOFF_LOG.md`를 맞춘다.
3. stale 문서가 있으면 코드보다 먼저 갱신한다.
4. 의미 있는 checkpoint마다 상태를 갱신한다.

## Canonical product direction

초기 4명 → 자연 자원 채집 → 저장/소유 → 실험/실패/발견 → 개인 지식 → 제작/도구 → 목격·모방·교육 → 전문화/교환 → 세대 누적 → emergent civilization.

- 전역 recipe/tech 자동 unlock 금지
- 강제 시대 gate 금지
- 개인 지식/출처/전파를 Core가 권위 상태로 소유
- 현대형 bootstrap anchor는 개발용 affordance일 뿐 canonical 시작세계가 아님

## Active work

### 1. Integration Sprint — ACTIVE

- Owner: 쭌 + 다겸, 기존 소유권 유지
- Status: `IN_PROGRESS / R2 BINDING`
- 큰 신규 Jjun Core slice는 integration checkpoint 동안 보류
- Dagyeom-owned 수정은 `docs/INTEGRATION_SPRINT.md` 규칙에 따른다.

### 2. Dagyeom PR #17 — Observer HUD v2

- Branch: `dagyeom/observer-ui-v2`
- Current HEAD: `0716a95eb4b6000c7098f8c0bb812bd622afa6a3`
- PR #17: OPEN / mergeable
- R1 latest-main reconciliation: **DONE**
  - helper PR #56 merged into Dagyeom branch
  - verify-only PR #57 closed without merge
  - Preflight `34824371968` PASS
  - Unreal Linux Compile Run #17 `34824371965` PASS including UHT + UBT + link
  - former Codex review findings resolved
- R2 current Observer Bridge / civilization binding: **DOING**
  - `ASSIST_LOCK-17-R2`
  - helper branch: `integration/dagyeom-observer-r2-assist`
  - locked paths: `LLObserverHUD.cpp`, `LLObserverHUD.h`, `LLObserverLabels.h`
- R3 after R2: PR description/state docs + final validation + merge readiness

### 3. Dagyeom stacked chain

- #26 UI Foundation: independent reconcile where safe
- #29 Character Presentation: after #17
- #30 Observer UX Polish: after #17
- #36 Mobile Touch: after #30
- #38 Visual Feedback: after #36
- parent-first; mass force-rebase 금지

### 4. Old Android validation

- PR #2 / `task/03-fast-test`: **FROZEN**
- old Run `34739283266` failed before Cook/Package/APK
- 수정/재실행/부활/병합 금지

## Latest completed milestones

### Integration R1
- PR #56 merge into `dagyeom/observer-ui-v2`: `0716a95eb4b6000c7098f8c0bb812bd622afa6a3`
- Verify-only PR #57: CLOSED / NOT MERGED
- Preflight `34824371968` PASS
- Unreal Run #17 `34824371965` PASS including UHT + UBT + link

### PR #54 — macOS clang shadow hotfix
- Merge `3b649b900c44a4e48bb89171b38f5e685e757b14`
- Preflight + Core tests + deterministic harness PASS

### PR #53 — Civilization Observer Read DTOs v1
- Merge `ec30d80b2986247f0f16572efb2c082a933d796d`
- Core `34821150702` PASS
- Preflight `34821150693` PASS
- Unreal Run #16 `34821150704` PASS
- published `GetResidentCivilizationObservation(...)`
- published `GetCivilizationWorldObservation(...)`

### PR #52 → #46
- #52 `b90da9242003fbc0cbc553605b9abc46a17aa044`
- #51 `55d5211160c8edad32b01177e2b9326a9faa2b78`
- #50 `c31c422c305a3a79a9553d37ac86247aa31d1853`
- #49 `36bd1ac81192bc689c1e811553f068f255642508`
- #48 `a90ff6a5d870858a9e555ddf1e6e526bb7aa34e1`
- #47 `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c`
- #46 `753df19657ea634ea2fa7c2ac935f6273ce14c10`

## Next sequencing

1. Complete PR #17 R2 Bridge/civilization binding.
2. Close R3 docs/review state and final validation.
3. Reconcile #26, then #29/#30 → #36 → #38 parent-first.
4. Integrated runtime verification.
5. Android smoke APK.
6. Resume deeper civilization production chains.

## Recovery rule

If interrupted: fetch actual GitHub state → compare with this file/queue/board → update stale docs first → resume only from last verified checkpoint.
