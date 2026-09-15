# LifeLens Development Milestones

이 문서는 **큰 개발 단위와 integration gate의 canonical roadmap**이다. 작은 TODO 리스트가 아니다.

## Delivery rule

> Same purpose + same layer + same validation scope = one milestone-sized PR.

각 milestone은 내부적으로 여러 작은 commit을 가질 수 있지만, 무거운 UE 검증과 canonical doc closeout은 의미 있는 끝점에서 한 번 수행한다.

## Completed foundations

- Character Presentation #63 — DONE.
- Appearance projection #65 — DONE.
- World Affordance Fallback #66 — DONE.
- Environmental Residue #68 — DONE.
- Runtime resident / physical ACK / restore #70~#74 — DONE.
- Exposure / sanitation causal chain #75~#80 — DONE.
- HumanWaste Environmental Visual Feedback #82 — DONE.
- World Genesis WG-1 #83 — DONE.
- World Genesis WG-2 #85 — DONE.
- Character Appearance #67 — DONE.
- **Character Motion Bootstrap #84 — DONE.**
- **World Generation Milestone A #87 — DONE.**

## Gate A — Integrated Runtime Checkpoint A — READY_NOW

Owner: joint integration; Jjun coordinates runtime/authority verification, Dagyeom presentation lane participates as needed.

Purpose: 처음으로 `main`의 실제 vertical slice를 한 번에 확인한다.

Acceptance:
- NEW GAME starts successfully.
- WG-2 selected initial region is the region materialized by Milestone A.
- four founders are physically projected inside that materialized region.
- no starting house/toilet/farm/storage/road/tool/modern infrastructure appears.
- Character Appearance + Idle/Walk/Jog + orientation presentation works in the integrated runtime.
- existing AI movement/action flow still follows Core/World authority.
- HumanWaste/environmental visual feedback can be reconstructed from authoritative Core state.
- Save/Load does not silently reroll generated natural state and restores resident/world projection coherently.
- Observer can inspect residents after start/load.

Until this gate is checked, **Dagyeom does not start an additional product milestone.**

## Milestone B — World Visual Milestone A — AFTER GATE A

Owner: Dagyeom visual/content lane; Jjun supplies Config/Bridge integration when requested.

Scope:
- production-oriented generated-world presentation consuming real world-generation contracts.
- terrain/biome presentation.
- vegetation / rocks / water / natural dressing.
- chunk presentation/materialization consumption.
- Android-friendly HISM/instancing/LOD/culling/material budget.
- observer readability.
- no visual-only second authority.

Important Config boundary:
- Dagyeom may create maps under `Content/Maps/**` (`/Game/Maps/...`).
- default startup map / `Config/DefaultEngine.ini` changes are Jjun-owned Integration work.
- Water/plugin changes to `LifeLens.uproject` are also Integration Requests, not silent content-lane edits.

## Milestone C — Character Context Motion Milestone

Owner: Dagyeom Character presentation.

Group together instead of one PR per animation:
- Sit / Stand / Lie / Wake.
- PickUp / Use / Work.
- sanitation / digging / crafting context animation hooks.
- locomotion ↔ context transition.
- basic gaze/body orientation layering.
- later AnimBP/state-machine migration if needed.

Animation remains presentation of Core/World action authority.

## Gate B — Android Smoke / Real-device Baseline

After World Visual Milestone A:
- cheap Android smoke/package path.
- actual APK artifact.
- real-device boot/play.
- performance / thermal / memory baseline.
- chunk/render budget tuning.

MetaHuman comparison happens only after this gate.

## Later simulation milestones

These remain product direction, not immediate dispatch:
- generic facility/resource authority beyond sanitation.
- health/pathogen + water/soil contamination.
- birth physical position fix and lifecycle presentation policy.
- open-ended material/component/connection artifact runtime.
- carrying-capacity pressure / exploration / migration.
- multiple settlements / regional society / politics / generational civilization.

## Current dispatch

**Only Gate A — Integrated Runtime Checkpoint A is READY_NOW.**

`tasks/WORK_STATE.md` contains the current live execution state. `tasks/TEAM_BOARD.md` contains ownership/locks/Integration Requests.
