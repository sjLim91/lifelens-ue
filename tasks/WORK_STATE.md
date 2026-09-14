# LifeLens Canonical Work State

> 실제 GitHub 상태가 항상 최우선 진실이다.
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` + `docs/CIVILIZATION_PROGRESSION_v1.md`
> 캐릭터 외형/표현 기준: `docs/CHARACTER_APPEARANCE_ROADMAP.md`
> 협업 기준: `docs/STATE_MANAGEMENT.md` + `docs/INTEGRATION_SPRINT.md`

Last reconciled: 2026-09-14 KST

## Mandatory sync gate

1. 작업 시작 전 actual `main`, target branch/PR, Actions 확인.
2. `WORK_STATE.md` → 역할별 READY queue → `TEAM_BOARD.md` → `HANDOFF_LOG.md` 대조.
3. stale 문서가 있으면 코드보다 먼저 갱신.
4. ACTIVE_LOCK 파일은 lock owner 외 수정 금지.
5. `NEXT`/`AFTER`는 작업 허가가 아니다. 실제 착수 가능 상태는 `READY_NOW` 또는 `PARALLEL_SAFE_NOW`다.

## Current dispatch

### Character Presentation v1 — DONE

- Source/history: `dagyeom/character-presentation-v1`, original PR #29.
- Current-main integration: PR #63.
- Cleanup head: `6d257d5f69e63eb721d2cbd36322765e0473fabf`.
- Main merge: `c9164ebf3cc70a194d3f8f6e50dcfb3c9df2a986`.
- Preflight Run `34843425475`: PASS.
- Unreal Linux Compile Run `34843425495`: PASS including UE 5.6 UHT/UBT/link.
- Temporary `Characters/**` workflow trigger was removed before merge.
- Final PR #63 diff contained exactly four Character Presentation C++ files.
- `ASSIST_LOCK-29-R1`: RELEASED.
- Original PR #29 is stale source/history only and must not be used as the product integration path.

### Character Appearance v1 — READY_NOW

Owner: 다겸 / 다겸 AI.

Start from current latest `main` at or after `c9164ebf3cc70a194d3f8f6e50dcfb3c9df2a986`.

Minimum target:
- real humanoid skeletal mesh
- skin / face / eyes / hair / default clothing
- common skeleton + modular appearance
- deterministic `AppearanceProfile` from WorldSeed + CharacterId
- NEW GAME residents visually distinct
- Save/Load appearance continuity
- Android LOD/mobile fallback
- free-use asset license/provenance documented

Rules:
- presentation must remain read-only with respect to Core action/simulation authority.
- do not add UI/Character-side authoritative caches.
- if new authoritative appearance/genetics/save data is required, add an Integration Request to `TEAM_BOARD.md` for Jjun Core/Bridge support.
- Jjun side does not push directly to `dagyeom/*`.

### Jjun lane — REVIEW/INTEGRATION SUPPORT

- Default: REVIEW_ONLY.
- Only create ASSIST_LOCK when Dagyeom-owned code actually needs integration help.
- Large unrelated Core slices are secondary until Human Character minimum is established.

## Canonical execution order

1. Character Presentation v1 — DONE via PR #63.
2. Character Appearance v1 — READY_NOW.
3. Character Motion & Context v1 minimum — AFTER Appearance.
4. PR #30 Observer UX Polish — AFTER Human Character minimum.
5. PR #36 Mobile Touch — AFTER #30.
6. PR #38 Visual Feedback — AFTER #36.
7. Core + Observer + Human Character integrated runtime verification.
8. Android smoke APK.
9. Appearance Genetics & Lifecycle.
10. Clothing/Equipment civilization linkage.
11. Resume deeper civilization production chains.

## Completed checkpoints

- Observer HUD v2 PR #17 merged; authoritative Core activity/family/emotion/relationship/civilization data is readable by Observer UI.
- UI Foundation integrated via PR #61; actual UE 5.6 compile passed.
- Character Presentation integrated via PR #63; actual UE 5.6 compile passed.
- Civilization observer read DTOs PR #53 merged.
- Core Decision → Unreal Physical Action Bridge merged.
- Full Core snapshot + Unreal SaveGame v2 merged.

## Frozen legacy

- PR #2 / `task/03-fast-test`: FROZEN.
- old Run `34739283266` failed before Cook/Package/APK.
- do not revive or rerun this old path.

## Recovery rule

If interrupted: fetch actual GitHub state → reconcile these live docs → inspect ACTIVE_LOCK → choose only READY_NOW/PARALLEL_SAFE_NOW → resume from the last verified checkpoint.
