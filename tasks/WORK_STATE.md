# LifeLens Canonical Work State

> **현재 진행 상태의 단일 기준판.** 쭌/다겸/양쪽 AI는 기능 작업보다 먼저 실제 GitHub 상태와 이 문서를 맞춘다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` · 상태 규칙: `docs/STATE_MANAGEMENT.md` · 역할/잠금: `tasks/TEAM_BOARD.md` · 이력: `tasks/HANDOFF_LOG.md`

Last reconciled: 2026-09-14 KST — PR #48 Physical Interaction / Smart Object Execution v1 is OPEN on `jjun/physical-smart-object-v1`, latest head `505e356d277cb0896109f6d4cdd75bcf06cdab54`. Latest Structural Preflight PASS; latest Unreal Linux Compile Run #14 is pending. Previous intermediate compile runs from earlier PR heads are superseded.

## Mandatory sync gate

1. 작업 시작/재개 전에 actual `main` HEAD, 대상 branch HEAD, PR state/head/base, Actions를 확인한다.
2. 이 문서 → 역할별 READY queue → `TEAM_BOARD.md` → `HANDOFF_LOG.md`와 대조한다.
3. 다르면 **코드보다 문서를 먼저 갱신**한다.
4. branch 생성, PR 생성, CI 결과, 실패 원인 확정, merge 등 의미 있는 checkpoint마다 즉시 갱신한다.
5. GitHub 실제 상태가 항상 stale 문서보다 우선한다.

---

## Active / unresolved work

### 1. Physical Interaction / Smart Object Execution v1 — Jjun

- Owner: 쭌 + 쭌 AI
- Branch: `jjun/physical-smart-object-v1`
- PR: #48 `[UE] Add reservable physical smart-object execution v1`
- Head: `505e356d277cb0896109f6d4cdd75bcf06cdab54`
- Status: `REVIEW / WAITING_UNREAL_COMPILE`
- Implemented:
  - `ALLActivityAnchor` lifecycle `Available → Reserved → InUse`;
  - stable ResidentId exclusive claim/release;
  - use transform separated from actor origin with local offset + rotation;
  - multi-capability anchors via primary + additional intents;
  - nearest usable deterministic anchor selection with stable path tie-break;
  - reservation held across approach/use and released on directive change, idle/death, bridge loss, invalidation, despawn, or action transition;
  - covered intents: Eat / Drink / Sleep / Toilet / Hygiene;
  - generic coordinate fallback removed for covered physical intents;
  - current Entry runtime bootstrap mirrors Core New Game object capacity: 4 Sleep, 2 Toilet, 2 Sink-like anchors shared by Hygiene+Drink, 1 Eat/Fridge anchor;
  - placed enabled anchors count toward capacity and are reused instead of blindly spawning duplicates;
  - Core remains WHAT authority; Unreal remains WHERE/HOW presentation/execution;
  - Unreal Linux Compile workflow now includes `Source/LifeLens/World/**`, fixing a validation blind spot discovered during this PR.
- Scope: 4 Jjun-owned World files + `.github/workflows/unreal-linux-compile.yml`; no UI, Character presentation, Content, pure Core, or frozen PR #2 edits.
- Validation:
  - latest Structural Preflight `34808292399` PASS;
  - latest Unreal Linux Compile `34808292682` / Run #14 PENDING;
  - earlier Run #11–#13 are superseded by later PR heads and must not be used as merge evidence.
- Merge gate: actual UE 5.6 image verification + UHT + UBT PASS on head `505e356d...`, then verify PR scope/head/mergeability and merge.

### 2. Observer HUD v2 — Dagyeom

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Status: `RECOVERING`
- Existing `BLOCKED-BY-JJUN` Observer requirements remain 0.
- Exact next action: reconcile latest main, bind current Core Observer Bridge, address Dagyeom-owned UI review findings, verify UHT/UBT + PIE.

### 3. Dagyeom stacked UI / presentation chain

- #26 `dagyeom/ui-foundation-v1` — reconcile latest main independently.
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

## Latest completed milestones

### PR #46 Core-authoritative physical action bridge
- Merge: `753df19657ea634ea2fa7c2ac935f6273ce14c10`
- Core `34805882778` PASS; Preflight `34805882776` PASS; Unreal Run #10 `34805882789` PASS incl actual UE 5.6 UHT+UBT.

### PR #47 Witness / Rumor / Social Knowledge v1
- Merge: `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c`
- Core `34806559374` PASS incl deterministic harness; Preflight `34806559379` PASS.

### Previous
- PR #45 Unreal SaveGame Adapter v1 — merge `3c646ba331b8199a295fd6f2e9cac1235d844679`.
- PR #44 Full Core Save/Load v1 — merge `2b3f9882703ed73cb8318ae262f26bebb995c209`.
- PR #43 Autonomous Family Progression v1 — merge `179e3a65aaa6ff8d2243117c7aebfd760812c73d`.
- PR #42 Production NEW GAME Unreal Runtime Integration — merge `5c6f2c071ca00bcd4b26d6e002bf5c618d02ff1e`.
- PR #41 Production NEW GAME Core v1 — merge `1b6f349f03db6f3bb1f95cf9387ef994a68c5d68`.

## Next Jjun candidates after #48

1. Animation / IK / context pose layer for sit/lie/use interactions.
2. Android FAST smoke APK / real-device verification once the physical runtime slice is observable.
3. Witness / Rumor Runtime Wiring v1.

---

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/board/queue → update stale docs first → resume only from last verified GitHub checkpoint.
