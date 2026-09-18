# LifeLens Project Status

Date: 2026-09-18
Current main checkpoint before this docs sync: `87020ec85787f1dfb3d03c30dd8fafaeb7b2aafe`
Current product phase: **P0 Whole-Source Audit Stabilization -> C1 Settlement & Subsistence**

## STOP-THE-LINE CURRENT PRIORITY

Whole-source audit found two issues that must be fixed **before C1-B**.

### AUDIT-0A — Time / Weather / Speed UI duplication

Current main has both:
- `ALLRuntimeObserverHUD::DrawRuntimeChrome()`
- `ULLObserverTimeWeatherOverlay` + auto-created presentation subsystem

Both show time/weather/speed and both can mutate `ULLSimulationSubsystem::SetSimulationSpeedPreset()`.

Required:
- consolidate to exactly one canonical Observer control surface.
- remove duplicate hit/input path.
- keep Core time authority unchanged.
- verify PC/Android interaction and PIE layout.

### AUDIT-0B — Environmental Need Pressure must follow each resident's authoritative location

Current per-minute environment pressure uses one start-region environment for every living resident.

Required:
- resident runtime GridPos -> resident chunk -> DynamicEnvironment.
- two residents in different chunks must receive their own local environmental pressure.
- preserve deterministic Save/Load continuation.

### AUDIT-0C — Whole regression after fixes

Required before resuming C1-B:
- Core Tests.
- deterministic harness.
- Structural Preflight.
- Unreal Linux Compile.
- PIE visual/input smoke checklist.

Canonical audit:
- `docs/SOURCE_AUDIT_2026-09-18.md`

After AUDIT-0A/B/C are green, resume:
**C1-B Autonomous Settlement Need Recognition**.

---

## Product direction

LifeLens is an Unreal-native autonomous life and civilization simulation.

- NEW GAME starts with exactly 4 adult founders: 2 male + 2 female.
- Founders receive persistent GUIDs and deterministic seed-driven identity/state.
- The player is an Observer, not a direct controller.
- Core / World owns simulation truth. Presentation only consumes authoritative state.
- No free civilization infrastructure is granted because an "era" changed.
- Civilization development is open-ended: survival, knowledge, resources, environment and society may eventually progress beyond present-day civilization into unknown future forms.
- Historical "eras" are observer summaries, not Core unlock timers.
- Android remains the first deployment target; Windows follows.
- Paid runtime/API/cloud dependencies are not required for the simulation to function.

## Completed foundations through PR #154

Major correctness / authority:
- #115 Lifecycle Core Correctness.
- #116 Action Completion Unification.
- #117 Social Communication & Korean localization.
- #118~#122 primitive civilization: storage, tools, FirePit, heat/charcoal, furnace/copper.
- #123 World / Facility / Obstacle Authority Normalization.
- #124 Legacy Authority Removal.
- #130 spatially-authoritative knowledge transmission.
- #132 Emotion Runtime Integration.

Time / environment:
- #135 Simulation Time Authority: Pause / 1x / 4x / 16x / 64x.
- #136 Calendar + Day/Night.
- #137 Seasons + Dynamic Weather Core.
- #138 Environmental Consequences.
- #139 Dynamic Environment Presentation foundation.
- #140 surface weather / Niagara hooks / post-process / HISM budget.
- #149 packaged rain/snow fallback when authored Niagara assets are absent.

Observer / presentation catch-up:
- #142 runtime time/weather/speed chrome.
- #143 tool-specific gather motion.
- #144 PC wheel + Android drag Detail scrolling.
- #145 Character Context Motion v2.
- #146 lifecycle birth/growth/pregnancy/death feed.
- #147 time/weather/speed controls.
- #148 dynamic camera-to-resident canopy visibility.
- #150 persistent selected-resident family/lifecycle state + observed life timeline.
- #151 adaptive information density by viewport/camera distance.
- #154 explicit Dagyeom presentation assist handoff.

Settlement:
- #152 **C1-A Settlement Facility Authority Foundation**.
  - WorkSurface = Wood 3 + Stone 2 + Work 7.
  - SleepingPlace = Fiber 4 + Wood 2 + Work 5.
  - Shelter = Wood 8 + Fiber 5 + Work 14.
  - deterministic Core site selection.
  - real inventory material delivery.
  - real construction work.
  - weather-sensitive outdoor work friction.
  - Save/Load persistence.
  - no free starting facility.

## Audit P1 structural follow-up

### AUDIT-1A — Materialized chunk enumeration

Current WorldPresentation and obstacle collision proxy infer chunk coordinates by scanning a ring around the initial chunk using `MaterializedChunkCount`.

This is acceptable for the current contiguous start-region phase but is not sufficient for future distant exploration/migration.

Before multi-settlement / migration:
- expose authoritative materialized chunk coordinate list from Core bridge.
- WorldPresentation and collision proxy consume that list directly.

This is **not ahead of AUDIT-0A/B**, and does not replace C1-B after P0 stabilization.

## C1 sequence after P0 audit fixes

### C1-B — Autonomous settlement need recognition
- high sleep pressure / repeated outdoor sleeping -> SleepingPlace utility.
- repeated rain/cold/heat exposure -> Shelter utility.
- repeated crafting/building demand -> WorkSurface utility.
- residents gather missing construction materials.
- all build actions use Core-authored spatial targets and ContextAction ACK.

### C1-C — Facility effects / maintenance
- SleepingPlace improves sleep efficiency/comfort.
- Shelter reduces appropriate environmental penalties.
- WorkSurface improves relevant work/crafting.
- condition/maintenance becomes causal.

### C1-D / C1-E / C1-F
- water/food persistence, storage/spoilage, cultivation.
- emergent settlement shape.
- Tin/Bronze only after real prerequisites.

## Build / validation state

Recent validated baseline:
- Core Tests #707 — PASS.
- 67/67 Core tests — PASS.
- deterministic harness — PASS.
- Preflight #774 — PASS.
- Unreal Linux Compile #235 — PASS.

Important:
- green compile/tests prove structural and regression health, not visual/device perfection.
- PIE/Android input/layout/animation/performance still require runtime QA.

Android:
- Gate B / device APK validation remains **PAUSED BY USER** until explicitly resumed.
- PR #141 proved the prebuilt UE 5.6 Linux host direction.
- do not restart long Android builds until explicitly requested.

## Collaboration state

- Ownership is useful for coordination, but source correctness is reviewed as one LifeLens product.
- Do not distort architecture merely to avoid touching the same file.
- If the same responsibility belongs in the same file, modify the correct file and coordinate/rebase conflicts.
- stale PR #98 remains selective-salvage only.

## Canonical documents

- Whole-source audit / current P0 fixes: `docs/SOURCE_AUDIT_2026-09-18.md`
- Product spec: `docs/LIFELENS_SPEC_v1.1.md`
- Execution roadmap: `docs/DEVELOPMENT_MILESTONES.md`
- Live execution state: `tasks/WORK_STATE.md`
- Open-ended civilization direction: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`
- Civilization design: `docs/CIVILIZATION_PROGRESSION_v1.md`
- Time/environment contract: `docs/TIME_AND_DYNAMIC_ENVIRONMENT.md`
- Decisions: `docs/DECISION_LOG.md`
- Ownership/locks/IR: `tasks/TEAM_BOARD.md`
- Dagyeom presentation assist handoff: `docs/DAGYEOM_PRESENTATION_HANDOFF_2026-09-18.md`
