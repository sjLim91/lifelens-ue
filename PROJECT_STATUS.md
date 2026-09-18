# LifeLens Project Status

Date: 2026-09-18
Current main checkpoint before this docs sync: `6cceafca5233e9ba472a2fee04139ebf41887aa4`
Current product phase: **C1 — Settlement & Subsistence**

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

## Completed foundations through PR #152

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

Settlement:
- #152 **C1-A Settlement Facility Authority Foundation**.
  - WorkSurface = Wood 3 + Stone 2 + Work 7.
  - SleepingPlace = Fiber 4 + Wood 2 + Work 5.
  - Shelter = Wood 8 + Fiber 5 + Work 14.
  - Deterministic Core site selection.
  - Real inventory material delivery.
  - Real construction work.
  - Weather-sensitive outdoor work friction.
  - Save/Load persistence.
  - No free starting facility.

## Current next implementation

### C1-B — Autonomous settlement need recognition

Connect the new facility authority to actual resident decisions:

- high sleep pressure / repeated outdoor sleeping -> SleepingPlace utility.
- repeated rain/cold/heat exposure -> Shelter utility.
- repeated crafting/building demand -> WorkSurface utility.
- residents must gather missing construction materials instead of receiving them.
- all build actions use Core-authored spatial targets and ContextAction ACK.
- Presentation must not create or complete a facility.

### C1-C — Facility effects

After autonomous construction is stable:

- SleepingPlace improves sleep efficiency/comfort versus outdoor fallback.
- Shelter reduces appropriate environmental penalties.
- WorkSurface improves relevant crafting/work efficiency.
- facility condition and maintenance begin to matter.

### C1-D / C1-E — Subsistence and emergent settlement shape

- water handling and storage.
- food storage and spoilage pressure.
- renewable food / cultivation foundation.
- seasonal production constraints.
- migration/search when local resources are insufficient.
- frequently used facilities naturally form settlement centers.
- sanitation remains separated from dense living areas.
- household space can later differentiate without a hard-coded town-center object.

## Long-range execution path

After C1:

**C2 long-run reliability**
-> multi-century deterministic/headless scale, population/resource/snapshot cost.

**C3 open-ended civilization framework**
-> stable Capability / Technology / CivilizationTransformation identities and prerequisite/effect graph.

**C4~C6**
-> health/disease/population resilience
-> education/recording/specialization/economy/institutions
-> migration/multiple settlements/trade.

**F1~F8**
-> advanced metallurgy / cities / science
-> mechanical-industrial
-> electrical-modern infrastructure
-> digital/network
-> AI/robotics
-> advanced energy/materials/biotech
-> planetary/space
-> open future / unknown civilization.

These are capability bands, not forced era unlocks.

## Build / validation state

Default functional gates while Android Gate B is paused:

- Core change: Core Tests + deterministic checks + Preflight.
- Unreal C++ change: Preflight + Unreal Linux Compile.
- exact-head validation before merge.
- docs-only changes must not intentionally launch a heavy build solely for documentation.

Android:
- Gate B / device APK validation remains **PAUSED BY USER** until explicitly resumed.
- PR #141 proved a prebuilt UE 5.6 Linux host image contains the required UnrealEditor / RunUAT / UBT and Android platform payload.
- Do not return to repeatedly source-building the whole Unreal Editor for routine Android attempts.
- #141 itself is a stale probe branch, not a production merge target as-is.

## Collaboration state

- Jjun: Core / AI / Simulation / World authority / Save / Bridge / CI / Android.
- Dagyeom: Character presentation / Animation / Observer UI / Camera/visual UX / WorldPresentation.
- Do not distort architecture merely to avoid touching the same file.
- If the same responsibility belongs in the same file, modify the correct file and coordinate/rebase conflicts.
- Jjun-owned PRs may merge after exact-head required CI is green when there is no unresolved overlap with Dagyeom's active work.
- Cross-owner overlap requires coordination first.

Open stale item:
- PR #98 remains selective-salvage only. Its useful panel-opacity / QA-view intent must be rebased onto current main rather than merged as an old whole-file snapshot.

## Canonical documents

- Product spec: `docs/LIFELENS_SPEC_v1.1.md`
- Execution roadmap: `docs/DEVELOPMENT_MILESTONES.md`
- Live execution state: `tasks/WORK_STATE.md`
- Open-ended civilization direction: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`
- Civilization design: `docs/CIVILIZATION_PROGRESSION_v1.md`
- Time/environment contract: `docs/TIME_AND_DYNAMIC_ENVIRONMENT.md`
- Decisions: `docs/DECISION_LOG.md`
- Ownership/locks/IR: `tasks/TEAM_BOARD.md`
- Dagyeom presentation assist handoff: `docs/DAGYEOM_PRESENTATION_HANDOFF_2026-09-18.md`
