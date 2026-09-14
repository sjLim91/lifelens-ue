# LifeLens Canonical Work State

> 실제 GitHub 상태가 항상 최우선 진실이다.
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` + `docs/CIVILIZATION_PROGRESSION_v1.md`
> World affordance / 환경 consequence 기준: `docs/WORLD_AFFORDANCE_ENVIRONMENT_v1.md`
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
- Main merge: `c9164ebf3cc70a194d3f8f6e50dcfb3c9df2a986`.
- Preflight Run `34843425475`: PASS.
- Unreal Linux Compile Run `34843425495`: PASS including UE 5.6 UHT/UBT/link.
- `ASSIST_LOCK-29-R1`: RELEASED.
- Original PR #29 is stale source/history only and must not be used as the product integration path.

### Character Appearance support contract — DONE

- Jjun support PR #65: `[BRIDGE] Add deterministic Character Appearance profile contract v1`.
- Main merge: `86663cb10f245bf185984a3622e4a86596e14923`.
- Structural Preflight Run `34845789763`: PASS.
- Unreal Linux Compile Run `34845789757`: PASS including actual UE 5.6 UHT/UBT/link.
- Provides vendor-independent `FLLAppearanceProfile` / `ULLAppearanceProfileLibrary` projection from stable resident identity.
- No separate appearance SaveGame authority or mutable cache was added.
- Dagyeom Character/UI/Content files were not modified.
- Jjun proactive Character Appearance support is complete; further support only on explicit Integration Request/blocker.

### World Affordance Fallback v1 — DONE

Owner: 쭌 / 쭌 AI.

- PR #66 `[WORLD] Add tiered affordance fallback v1` merged to main.
- Main merge: `0ad8d6b4c80c134832fcad9bf9b34dcfabf2b68a`.
- Latest-head Structural Preflight Run `34848772905`: PASS.
- Latest-head Unreal Linux Compile Run `34848772895`: PASS including actual UE 5.6 UHT/UBT/link.
- Canonical design: `docs/WORLD_AFFORDANCE_ENVIRONMENT_v1.md`.

Delivered:
- removed automatic bootstrap beds/toilets/eat/hygiene facilities
- actual-world affordance selection by `Preferred → Primitive → Natural → Emergency`
- no implicit civilization facility creation
- emergency Eat/Drink does not create resources
- exposes active affordance tier / emergency state

Boundary:
- #66 implements action/affordance fallback only.
- Authoritative environmental residue/contamination is intentionally the next slice.

### Environmental Residue v1 — READY_NOW

Owner: 쭌 / 쭌 AI.

Canonical requirements: `docs/WORLD_AFFORDANCE_ENVIRONMENT_v1.md`.

Minimum next slice:
- outdoor toilet completion becomes an actual environment consequence
- Core-owned residue state
- location / amount / intensity / decay
- accumulation when repeated
- Save/Load continuity
- Observer/World read path
- query contract for Hygiene / Health risk / discomfort / avoidance
- no Unreal-only authoritative contamination cache

Later extension:
- weather/water/soil influence
- cleanup/burial/disposal
- hygiene knowledge and discovery
- primitive latrine → improved toilet/drainage progression
- visual residue/VFX driven from Core state

### Character Appearance v1 — READY_NOW

Owner: 다겸 / 다겸 AI.

Start from current latest `main` at or after `86663cb10f245bf185984a3622e4a86596e14923`.

Canonical asset track: `docs/CHARACTER_ASSET_TRACK.md`.
Default: Track B — Quaternius packs whose exact source/version explicitly states CC0. MetaHuman remains a post-Android-validation upgrade/comparison path.

Minimum target:
- real humanoid skeletal mesh
- skin / face / eyes / hair / default clothing
- common skeleton + modular appearance
- deterministic AppearanceProfile from stable resident identity
- NEW GAME residents visually distinct
- Save/Load appearance continuity
- Android LOD/mobile fallback
- asset license/provenance documented

Rules:
- presentation remains read-only with respect to Core action/simulation authority.
- do not add UI/Character-side authoritative caches.
- use the merged PR #65 appearance projection contract instead of inventing a second authority.
- if new authoritative appearance/genetics/save data is required, add an Integration Request to `TEAM_BOARD.md`.

#### 다겸 진행 행 (다겸 측 AI 추가, 2026-09-14)

- Owner: 다겸 + 다겸 AI
- Branch: `dagyeom/character-appearance-v1` (base `main` `9db79b8`, latest main merge 진행)
- Work item: Character Appearance v1 — Track B Quaternius CC0
- Last known HEAD: `26ec3b9`
- PR: #67 `[UI] Character Appearance v1 — Quaternius CC0 human body, deterministic look (Track B)`
- Status: `WAITING_CI`
- CI: 로컬 `Build.sh LifeLensEditor Mac Development` Result: Succeeded; 로컬 `Tools/validate_bootstrap.py` PASS; GitHub Actions 대기
- Last verified fact: UBC/UAL 83 에셋 임포트, Appearance/Inputs 컴포넌트, PIE 화면 확인(4명 휴머노이드, 남녀 구분, 헤어·피부 차이, 라벨·링·LEVEL 1 정상; 의상 없음, Idle 외 애니 미연결, Save/Load 미확인)
- Blocker / interruption: 없음
- Exact next action: CI 확인 → PIE 재확인(외형 유지, Save/Load 연속성) → 의상 세트 보강(후속)
- Handoff safety: `SAFE`
- Shared-file impact: 없음

### Jjun lane — OWN TRACK / REVIEW SUPPORT

- Character Appearance proactive support is DONE via PR #65.
- Default for Dagyeom work returns to REVIEW_ONLY.
- Jjun owns non-overlapping Core/World/build/runtime work.
- Current Jjun product path: Environmental Residue v1.
- Create ASSIST_LOCK only when Dagyeom-owned code actually needs integration help.

## World / civilization causal rule

`Need → Intent → current-world affordance → action result → environment consequence → experience/memory/health input → problem recognition → experiment/discovery → better affordance → culture/civilization`

Rules:
- World absence must remain visible; do not hide missing infrastructure with implicit spawns.
- Emergency fallback must generally be less effective or more costly than purpose-built affordances.
- Environmental consequence with simulation impact belongs to Core authority and Save/Load.
- Technology/progress is emergent from need, observation, knowledge and resources, not a forced global tech unlock.

## Canonical execution order

1. Character Presentation v1 — DONE via PR #63.
2. Appearance data/projection support — DONE via PR #65.
3. Character Appearance v1 — READY_NOW / Track B Quaternius CC0. (Dagyeom lane)
4. World Affordance Fallback v1 — DONE via PR #66. (Jjun lane)
5. Environmental Residue v1 — READY_NOW. (Jjun lane)
6. Character Motion & Context v1 minimum — AFTER Appearance.
7. PR #30 Observer UX Polish — AFTER Human Character minimum.
8. PR #36 Mobile Touch — AFTER #30.
9. PR #38 Visual Feedback — AFTER #36.
10. Core + Observer + Human Character integrated runtime verification.
11. Android smoke APK + profiling.
12. MetaHuman comparison / upgrade decision.
13. Appearance Genetics & Lifecycle.
14. Clothing/Equipment civilization linkage.
15. Resume deeper civilization production chains.

## Completed checkpoints

- Observer HUD v2 PR #17 merged; authoritative Core activity/family/emotion/relationship/civilization data is readable by Observer UI.
- UI Foundation integrated via PR #61; actual UE 5.6 compile passed.
- Character Presentation integrated via PR #63; actual UE 5.6 compile passed.
- Character Appearance deterministic projection contract merged via PR #65; actual UE 5.6 compile passed.
- World Affordance Fallback merged via PR #66; actual UE 5.6 compile passed.
- Civilization observer read DTOs PR #53 merged.
- Core Decision → Unreal Physical Action Bridge merged.
- Full Core snapshot + Unreal SaveGame v2 merged.

## Frozen legacy

- PR #2 / `task/03-fast-test`: FROZEN.
- old Run `34739283266` failed before Cook/Package/APK.
- do not revive or rerun this old path.

## Recovery rule

If interrupted: fetch actual GitHub state → reconcile these live docs → inspect ACTIVE_LOCK → choose only READY_NOW/PARALLEL_SAFE_NOW → resume from the last verified checkpoint.
