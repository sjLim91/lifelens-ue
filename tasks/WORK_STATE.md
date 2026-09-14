# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — PR #41 merged as `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68` after Core Tests + deterministic harness + Structural Preflight PASS.

## Mandatory sync gate

1. 작업 시작/재개 전에 actual `main` HEAD, 대상 branch HEAD, PR state/head/base, Actions를 확인한다.
2. 이 문서 → 역할별 READY queue → `TEAM_BOARD.md` → `HANDOFF_LOG.md`와 대조한다.
3. 다르면 **코드보다 문서를 먼저 갱신**한다.
4. branch 생성, PR 생성, CI 결과, 실패 원인 확정, merge 등 의미 있는 checkpoint마다 즉시 갱신한다.
5. GitHub 실제 상태가 항상 stale 문서보다 우선한다.

---

## Active / unresolved work

### 1. Production NEW GAME Unreal Runtime Integration — Jjun

- Owner: 쭌 + 쭌 AI
- Status: `PLANNED`
- Branch: next latest-main branch to be created after this DONE checkpoint.
- Goal: PR #41의 authoritative Core `Simulation::setupNewGame()`을 Unreal runtime의 실제 NEW GAME 시작 경로에 연결하고, legacy `ULLSimulationSubsystem`의 별도 2M+2F 생성기가 두 번째 진실이 되지 않도록 정리한다.
- Required contract:
  - WorldSeed 하나가 Core Simulation의 유일한 founder 생성 원천.
  - Core CharacterId + WorldSeed → 기존 stable Unreal `FGuid` 규칙 유지.
  - Unreal Observer/Character layer는 Core founder read DTO를 소비하며 독립적으로 다른 주민을 재생성하지 않음.
  - Save/Load 영속화는 별도 bounded slice로 분리 가능; 이 integration에서 fake save success를 만들지 않음.
  - Dagyeom `Source/LifeLens/UI/**` / Character presentation 파일은 수정하지 않음.
- Exact next action: actual latest main 확인 → 새 branch 생성 → current Bridge/NewGame/legacy subsystem 경계 분석 → 가장 작은 runtime integration 구현 및 UHT/UBT 검증.

### 2. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Last verified HEAD: `dc3351ea9025ee33ea70f8ce1026100070c20092`
- Status: `RECOVERING`
- Fact: former six `BLOCKED-BY-JJUN` Observer read requirements are available on main via #37/#39/#40.
- Exact next action: reconcile actual latest main, bind current Core Observer Bridge, address Dagyeom-owned UI review findings, verify UHT/UBT + PIE, update state before merge.

### 3. Dagyeom stacked UI / presentation chain

- #26 `dagyeom/ui-foundation-v1` — independent main-target PR; reconcile latest main.
- #29 `dagyeom/character-presentation-v1` — stacked on #17.
- #30 `dagyeom/observer-ux-polish-v1` — stacked on #17.
- #36 `dagyeom/mobile-touch-v1` — stacked on #30.
- #38 `dagyeom/visual-feedback-v1` — stacked on #36.
- Jjun does not modify/flatten these branches.

### 4. Old Android validation

- Branch/PR: `task/03-fast-test`, PR #2
- Status: `FROZEN`
- Run `34739283266`: FAILURE; Cook/Package/APK not reached.
- Rule: **do not modify, rerun, revive, or merge.**

---

## Latest completed milestone — Production NEW GAME Core v1

### PR #41 `[CORE] Add authoritative production New Game founders`

- Branch: `jjun/production-new-game-core-v1`
- Feature HEAD: `fd94b6b9c2c9d4ae23a6d65abf59ff4686218788`
- Status: `DONE`
- Merge SHA: `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68`
- Scope: 6 files under `Source/LifeLensCore/**`; no Dagyeom UI/Character files.
- Implemented:
  - Core `Sex` identity.
  - WorldSeed deterministic founder generator.
  - exactly 4 founders = 2 male + 2 female.
  - unique Core IDs + unique display names.
  - generated Personality / Genetics / Needs / metabolism / sleep tendency.
  - 25~34-year authoritative birthMinute compatible with existing Growth/Aging.
  - `Simulation::setupNewGame()` resets relationship/family/runtime books and reseeds RNG from WorldSeed.
  - all directed founder relationships begin stranger/very-low-familiarity; no forced couple/marriage/household/pregnancy/family links.
  - four-person minimum Needs objects so existing Planner loop can run.
- Validation:
  - Core Tests Run `34798427843` — PASS: Configure / Build / Test / Deterministic harness smoke.
  - Structural Preflight Run `34798427848` — PASS.
  - New `test_production_new_game`: exact sex composition, unique ID/name, adult lifecycle consistency, neutral family/social start, same-seed determinism, different-seed variation, repeated-setup determinism.
- Remaining: Unreal runtime still has a legacy duplicate New Game population generator; next task replaces that runtime ownership with Core-backed generation.

---

## Completed integration chain relevant to Dagyeom

- #37 Observer Runtime Bridge — DONE, merge `938d0a2798e600929b4ccc755b48bcd39026ac75`; Unreal UHT/UBT `34796067278` PASS.
- #39 Authoritative Family Runtime State — DONE, merge `612229cc610d2ea6283e080309bdee58ef42d1db`; Core `34796213647` PASS.
- #40 Family + World Observer Bridge — DONE, merge `9261581df3abd5332d92855628fd7d03203748af`; Preflight `34796892593` + Unreal UHT/UBT `34796892609` PASS.

Former Dagyeom blockers now READY: Relationship 13D, Emotion detail, SocialIntent+target, Family summary, World family/lifecycle aggregates, Blueprint/USTRUCT read-only Bridge.

---

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/board/queue → update stale docs first → resume only from last verified GitHub checkpoint.
