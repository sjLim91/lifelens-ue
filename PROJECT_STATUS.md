# LifeLens Project Status

Date: 2026-09-18
Current main checkpoint before this docs sync: `87020ec85787f1dfb3d03c30dd8fafaeb7b2aafe`
Current product phase: **Earth & Human Foundation — Earth-scale world hierarchy, hydrology, ecology and human depth**

## Whole-source audit closeout

AUDIT-0A / 0B / 0C automated source gates are complete.

- #156 **AUDIT-0A** — duplicate Observer time/weather/speed UI removed; one canonical production control surface remains.
- #157 **AUDIT-0B** — per-minute environmental Need pressure now uses each resident's authoritative Core runtime GridPos -> chunk -> DynamicEnvironment.
- #158 **AUDIT-0C** — regression guards added and combined post-fix validation passed.

#158 exact-head validation:
- Preflight #789 — PASS.
- Core Tests #721 — PASS.
- Unreal Linux Compile #241 — PASS.
- deterministic harness runs inside Core Tests — PASS.

Runtime-only visual/input smoke (actual PIE/device layout, touch feel, animation/VFX/performance) remains a QA track and is not represented as CI-verified.

**Current execution focus: Earth & Human Foundation. #226 Far world visual envelope and #227 Horizon atmosphere blend are merged as Local Surface presentation fixes; next structural work is world hierarchy + hydrology.**

Canonical audit:
- `docs/SOURCE_AUDIT_2026-09-18.md`

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

## Completed foundations through PR #158

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

## Current macro roadmap

LifeLens execution is grouped into four large stages. Existing detailed work is preserved inside them; this is a roadmap simplification, not a scope reduction.

- **Earth & Human Foundation — ACTIVE**
  - EH-0 World hierarchy / observer scale / simulation LOD contracts.
  - EH-1 Terrain + hydrology: explicit rivers/lakes/coast/ocean/freshwater/saltwater.
  - EH-2 Water-driven survival and settlement pressure.
  - EH-3 Ecology.
  - EH-4 Human physiology / health.
  - EH-5 Language / culture / ordinary life.
  - EH-6 Society / economy / institutions.
  - EH-7 Planetary/orbital observer architecture.
  - EH-8 Interplanetary civilization after real prerequisites.
- **Stage C — Settlement, Survival & Early Civilization — preserved beneath the new foundation**
  - C-S0 Facility Authority — DONE via #152.
  - C-S1 Autonomous Settlement Need Recognition — DONE via #161.
  - C-S2 Facility Effects & Maintenance — DONE via #162.
  - C-S3 water scope expands into EH-1/EH-2; food/storage/cultivation continues afterward.
  - C-S4 Emergent Settlement Geometry follows real terrain/hydrology.
  - C-S5 Tin/Bronze follows real geology/resource prerequisites.
- **Stage D — Long-Run Simulation & Civilization Engine**
  - former C2 Long-Run Scale/History/Fast-forward/Cleanup.
  - former C3 Knowledge/Capability/Technology/CivilizationTransformation framework.
- **Stage E — Human Society, Health, Education, Economy & Migration**
  - former C4 Health/Disease/Population Resilience.
  - former C5 Education/Recording/Specialization/Economy/Institutions.
  - former C6 Migration/Multiple Settlements/Trade.
- **Stage F — Historical Civilization to Open Future**
  - former F1~F8, from advanced metallurgy/urbanization through industrial/electrical/digital/AI/biotech/space to unknown future civilization.

Common tracks remain outside the letters: Presentation quality, PIE/device QA, Android delivery, CI, and AUDIT follow-ups.

## Stage C sequence

### C-S1 — Autonomous settlement need recognition — DONE (#161)
- high sleep pressure / repeated outdoor sleeping -> SleepingPlace utility.
- repeated rain/cold/heat exposure -> Shelter utility.
- repeated crafting/building demand -> WorkSurface utility.
- residents gather missing construction materials.
- all build actions use Core-authored spatial targets and ContextAction ACK.

### C-S2 — Facility effects / maintenance — DONE (#162)
- SleepingPlace improves sleep efficiency/comfort.
- Shelter reduces appropriate environmental penalties.
- WorkSurface improves relevant work/crafting.
- condition/maintenance becomes causal.

### Earth & Human Foundation insertion before remaining C-S3/C-S4/C-S5
- hierarchy-compatible Planet -> Region -> Chunk -> Local Surface world model.
- explicit hydrology and freshwater/saltwater sources.
- simulation LOD for Earth-scale growth.
- ecology and deeper human systems.
- C-S3 food/storage/cultivation then consumes real water/terrain.
- C-S4 settlement shape emerges from geography and activity.
- C-S5 Tin/Bronze consumes real geology/resource prerequisites.

## Build / validation state

Recent validated baseline:
- Core Tests #721 — PASS.
- deterministic harness — PASS.
- Preflight #789 — PASS.
- Unreal Linux Compile #241 — PASS.
- whole-source audit invariants — PASS.

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
- Dagyeom PR #98 was selectively ported onto current main by #202 and the original PR was closed as superseded.

## Late 2026-09-18 work-state reconciliation

The Presentation/Observer sprint advanced far beyond the earlier #158 audit snapshot. Merged work through #215 and active #216~#224 are recorded in:

- `docs/PRESENTATION_WORK_STATE_2026-09-18.md`

For current PR/CI truth, actual GitHub main / open PR / Actions remains authoritative.

### Latest merged Presentation checkpoint

Merged after exact-head Preflight + Unreal Linux Compile:
- #216 Detail resident navigation v1.
- #217 Sleep site posture v7.
- #218 Resource depletion visual v2.
- #219 Selected social counterpart v7.
- #220 Snow cover accumulation v6.
- #221 Detailed daypart chrome v5.
- #222 Resident identity badge v7.
- #223 Offscreen action cue v4.
- #224 World event focus return v2.

#225 is the docs-only synchronization PR for this checkpoint.

## Earth & Human Foundation

Canonical expansion architecture:
- `docs/EARTH_AND_HUMAN_FOUNDATION.md`

Important: #226/#227 expand only the **Local Surface visual envelope**. They do not define Earth as a large flat plane. Future observer scale is Local -> Regional -> Planetary -> Orbital -> Interplanetary.

## Canonical documents

- Earth & Human Foundation: `docs/EARTH_AND_HUMAN_FOUNDATION.md`
- Current Presentation/Observer sprint: `docs/PRESENTATION_WORK_STATE_2026-09-18.md`
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
