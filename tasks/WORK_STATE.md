# LifeLens Canonical Work State

> 현재 진행 상태의 단일 기준판. 실제 GitHub 상태가 항상 우선한다.
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` + `docs/CIVILIZATION_PROGRESSION_v1.md`
> 캐릭터 외형/표현 순서는 `docs/CHARACTER_APPEARANCE_ROADMAP.md`가 우선한다.
> 협업 기준: `docs/STATE_MANAGEMENT.md` + `docs/INTEGRATION_SPRINT.md`

Last reconciled: 2026-09-14 KST — PR #26 UI Foundation reconcile is complete; PR #29 is next.

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
- Status: `IN_PROGRESS / NEXT: PR #29 CHARACTER PRESENTATION FOUNDATION`
- 큰 신규 Jjun Core slice는 integration checkpoint 동안 보류
- Dagyeom-owned 수정은 `docs/INTEGRATION_SPRINT.md` 규칙에 따른다.

### 2. Observer HUD v2 — DONE / MERGED

- PR #17: **MERGED** `aa194db7c5b500cdf5041fd6d43b25f97b9dd0b6`
- R1/R2/R3 complete
- corrected Unreal Run #20 `34833994155` PASS including UHT/UBT/link
- unresolved review threads: 0
- Observer assist locks released

### 3. PR #26 UI Foundation — DONE

- Original branch: `dagyeom/ui-foundation-v1`
- Original head: `70dfa5ebeabf24b661c9f9fd0bc63e3ad01ac180`
- Original PR #26: CLOSED / NOT MERGED / superseded
- `ASSIST_LOCK-26-R1`: RELEASED
- Helper: `integration/dagyeom-ui-foundation-r1-assist`
- Helper PR #61: MERGED `7826aaa917b4877bdd3b5ebbd6d5bfd139309b9b`
- Helper scope: only `LLObserverUIFoundation.h/.cpp` on current main
- Helper Preflight: PASS `34840301429`
- Verify-only PR #62: CLOSED / NOT MERGED
- Verify Preflight: PASS `34840467738`
- Unreal Linux Compile Run #21: `34840467864` PASS including UHT/UBT/link
- Next: PR #29 Character Presentation Foundation reconcile

### 4. Dagyeom integration queue

캐릭터 관련 상세 완료조건은 `docs/CHARACTER_APPEARANCE_ROADMAP.md`를 따른다.

1. PR #29 Character Presentation Foundation — **NEXT**.
2. Character Appearance v1.
3. Character Motion & Context v1 minimum.
4. PR #30 Observer UX Polish.
5. PR #36 Mobile Touch — after #30.
6. PR #38 Visual Feedback — after #36.
7. Integrated runtime verification.
8. Android smoke APK.
9. Appearance Genetics & Lifecycle.
10. Clothing/Equipment civilization linkage.
11. Resume deeper civilization production chains.

Parent-first; mass force-rebase 금지.

### 5. Old Android validation

- PR #2 / `task/03-fast-test`: **FROZEN**
- old Run `34739283266` failed before Cook/Package/APK
- 수정/재실행/부활/병합 금지

## Latest completed milestones

### Observer HUD Integration R1-R3
- #17 merged: `aa194db7c5b500cdf5041fd6d43b25f97b9dd0b6`
- authoritative Core activity/family/emotion/relationship/civilization data available in Observer presentation
- LEVEL 0 remains thin; no UI-side simulation authority/cache

### PR #54 — macOS clang shadow hotfix
- Merge `3b649b900c44a4e48bb89171b38f5e685e757b14`

### PR #53 — Civilization Observer Read DTOs v1
- Merge `ec30d80b2986247f0f16572efb2c082a933d796d`
- Unreal Run #16 `34821150704` PASS

### PR #52 → #46
- #52 `b90da9242003fbc0cbc553605b9abc46a17aa044`
- #51 `55d5211160c8edad32b01177e2b9326a9faa2b78`
- #50 `c31c422c305a3a79a9553d37ac86247aa31d1853`
- #49 `36bd1ac81192bc689c1e811553f068f255642508`
- #48 `a90ff6a5d870858a9e555ddf1e6e526bb7aa34e1`
- #47 `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c`
- #46 `753df19657ea634ea2fa7c2ac935f6273ce14c10`

## Recovery rule

If interrupted: fetch actual GitHub state → compare with this file/queue/board → update stale docs first → resume only from last verified checkpoint.
