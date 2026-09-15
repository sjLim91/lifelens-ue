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
- **Gate A — Integrated Runtime Checkpoint A — DONE by owner acceptance on automated/runtime evidence.**

## Gate A — Integrated Runtime Checkpoint A — DONE

Owner: joint integration; Jjun coordinated runtime/authority verification.

Final gate state:
- **Jjun Core/World runtime side: PASS** — Run `34943197031`, Job `104296410531`.
- additional Dagyeom manual PIE visual inspection: **WAIVED BY PROJECT OWNER on 2026-09-15**.
- this waiver is not recorded as a visual PASS; skipped visual concerns are carried into World Visual Milestone A validation.

Purpose: 처음으로 `main`의 실제 vertical slice를 한 번에 확인한다.

Runtime acceptance proven:
- NEW GAME starts successfully.
- WG-2 selected initial region is the region materialized by Milestone A.
- four founders have authoritative initial positions inside that materialized region.
- no starting Core house/toilet/farm/storage/road/tool/modern infrastructure is fabricated.
- existing AI movement/action flow follows Core/World authority.
- HumanWaste/environmental feedback originates from authoritative Core state.
- Save/Load does not reroll generated natural state and restores exact resident positions at the save checkpoint.
- Observer Core read models expose residents after start/load.
- source/restored simulation state remains byte-identical after deterministic continuation.

Jjun runtime evidence from the corrected Gate A run:
- 4 founders: 2 male / 2 female.
- selected/materialized replay start chunk: `(-12, 6)`.
- initial generated natural chunk registry count: 1.
- autonomous missing-toilet path produced authoritative HumanWaste: 1 residue.
- Save/Load restored exact save-checkpoint resident positions.
- generated natural baseline survived restore without reroll.
- immediate snapshot roundtrip and another 120 minutes of deterministic continuation were byte-identical.
- supporting production-new-game, world-generation, external-physical-execution, environmental-residue/exposure, core-save-load and civilization-observer contracts all passed.

The earlier Run `34943075204` failed only because the temporary validation harness incorrectly assumed residents must remain in their initial chunk after autonomous movement. That expectation was corrected; no product code change was required.

Manual visual items that were not separately inspected at Gate A are **not discarded**. They become explicit validation targets inside the next visual milestone: appearance correctness, locomotion/orientation coherence, spawn overlap/floating/sinking, environmental placement, Observer usability, and Save/Load visual continuity.

## Milestone B — World Visual Milestone A — READY_NOW / START AUTHORIZED

Owner: Dagyeom visual/content lane; Jjun supplies Config/Bridge integration when requested.

Purpose: replace the bootstrap/fixed-stage presentation with a production-oriented generated-world view driven by the merged world-generation contracts.

Scope:
- production-oriented generated-world presentation consuming real world-generation contracts.
- terrain/biome presentation.
- vegetation / rocks / water / natural dressing.
- chunk presentation/materialization consumption.
- Android-friendly HISM/instancing/LOD/culling/material budget.
- observer readability.
- no visual-only second authority.

Validation carried into this milestone:
- four residents visibly project coherently in the generated-world presentation.
- Quaternius appearance remains correct.
- Idle/Walk/Jog and orientation have no T-pose/sliding regression.
- no visibly invalid spawn overlap/floating/sinking/map escape.
- no starting civilization infrastructure is invented visually as simulation truth.
- HumanWaste/environment feedback placement is spatially reasonable.
- Observer labels/selection/detail remain usable.
- Save → Load presentation remains visually coherent.

Important authority rule:
- World presentation consumes Core/World facts; it does not create a parallel terrain/resource/simulation authority.
- generated-world facts should replace fixed gray-world assumptions whenever an authoritative read contract exists.

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

**World Visual Milestone A is READY_NOW and START AUTHORIZED.**

- Dagyeom: begin World Visual Milestone A in the visual/content lane.
- Jjun: remain available for explicit Config/Bridge/project Integration Requests and verified authority blockers.
- do not split the milestone into tiny PRs; keep the coherent generated-world presentation objective together and validate at a meaningful milestone checkpoint.

`tasks/WORK_STATE.md` contains the current live execution state. `tasks/TEAM_BOARD.md` contains ownership/locks/Integration Requests.
