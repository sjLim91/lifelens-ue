# LifeLens Canonical Work State

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
>
> Whole-source audit / current P0 fixes: `docs/SOURCE_AUDIT_2026-09-18.md`.
>
> Canonical roadmap: `docs/DEVELOPMENT_MILESTONES.md`.
>
> Earth / human expansion architecture: `docs/EARTH_AND_HUMAN_FOUNDATION.md`.
>
> Long-range civilization direction: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`.
>
> Ownership / locks / IR: `tasks/TEAM_BOARD.md`.
>
> Dagyeom presentation assist handoff: `docs/DAGYEOM_PRESENTATION_HANDOFF_2026-09-18.md`.
>
> Current late-day Presentation/Observer work state: `docs/PRESENTATION_WORK_STATE_2026-09-18.md`.
>
> Historical audit: `docs/INTEGRATED_AUDIT_2026-09-17.md` (point-in-time only; do not treat as live status).

Last reconciled: **2026-09-19 KST after #238~#243 graphics/world foundation merges; #244/#245 active validation**.

---

## 1. Current main checkpoint

Current main before this docs-only closeout sync:

- main SHA: `baa986b08d504765d8baa05ee8886b415b1eacd4`.
- #145 Character Context Motion v2 — MERGED.
- #149 Visual Catch-up v3 packaged rain/snow fallback — MERGED.
- #150 Lifecycle Presentation v2 — MERGED.
- #151 Observer Adaptive Information Density v1 — MERGED.
- #152 C1 Settlement Facility Authority Foundation v1 — MERGED.
- #153 docs reconciliation — MERGED.
- #154 explicit Dagyeom presentation assist handoff — MERGED.
- #156 AUDIT-0A Observer chrome single authority — MERGED.
- #157 AUDIT-0B resident-local environmental pressure — MERGED.
- #158 AUDIT-0C whole regression guards — MERGED.

Recent validated functional baseline:
- Core Tests #721 — PASS.
- deterministic harness — PASS.
- Preflight #789 — PASS.
- Unreal Linux Compile #241 — PASS.
- whole-source audit invariants — PASS.

Authority rule remains:

> Core / World owns simulation truth. UI / Character / Environment / WorldPresentation consumes authoritative contracts and must not invent outcomes.

---

## 2. Whole-source audit P0 — COMPLETE

### AUDIT-0A — DONE (#156)
- duplicate Canvas runtime chrome removed.
- production HUD hierarchy no longer owns a second time/weather/speed surface.
- one UI speed mutation path remains.

### AUDIT-0B — DONE (#157)
- environmental Need pressure uses resident authoritative runtime GridPos.
- GridPos -> chunk -> local DynamicEnvironment.
- regression covers residents in different climate chunks.

### AUDIT-0C — DONE (#158, automated source gate)
- Preflight #789 — PASS.
- Core Tests #721 — PASS.
- deterministic harness — PASS.
- Unreal Linux Compile #241 — PASS.
- regression guard prevents the two audit bugs from silently returning.
- snapshot restore + local-climate deterministic continuation regression added.

Runtime-only PIE/device smoke remains a QA item because CI does not execute an interactive Unreal viewport/device session.

**Stage C / C-S1 is no longer blocked by the source-audit gate.**

---

## 3. P1 audit follow-up

### AUDIT-1A — Explicit materialized chunk enumeration

Current WorldPresentation and obstacle collision proxy infer materialized chunk coordinates from:
- `MaterializedChunkCount`.
- a ring around the initial chunk.

This can miss distant/non-contiguous materialized chunks later.

Required before migration/multi-settlement:
- Core bridge exposes authoritative materialized chunk coordinate list.
- WorldPresentation consumes the list.
- obstacle collision proxy consumes the list.
- no count-based coordinate guessing.

This is not ahead of P0 and does not replace C1-B after stabilization.

---

## 4. Recent completed chain

### Time / environment
- #135 Simulation Time Authority & Variable Speed.
- #136 Calendar + Day/Night.
- #137 Seasons + Dynamic Weather Core.
- #138 Environmental Consequences.
- #139 Dynamic Environment Presentation Foundation.
- #140 Visual Catch-up v2.
- #142 Runtime Chrome.
- #147 Observer time/weather/speed controls.
- #148 Dynamic Observer Canopy Visibility.
- #149 packaged rain/snow fallback.

### Character / Observer / lifecycle
- #143 Character Context Motion v2a.
- #144 Detail Scrolling v1.
- #145 Character Context Motion v2.
- #146 lifecycle live event feed.
- #150 selected-resident persistent family/lifecycle card + observed timeline.
- #151 adaptive information density.

### C1 settlement
- #152 C1-A authority:
  - WorkSurface constructible.
  - SleepingPlace constructible.
  - Shelter constructible.
  - real materials + real work.
  - deterministic sites.
  - weather friction on construction work.
  - snapshot persistence.
  - no free NEW GAME settlement facility.

---

## 5. Stage C — Settlement, Survival & Early Civilization — ACTIVE

Legacy mapping is preserved so no planned work is lost:

### C-S0 — Facility authority — DONE (#152)
- WorkSurface / SleepingPlace / Shelter.
- deterministic sites.
- real materials + real work.
- environment-sensitive construction.
- persistence.
- no free New Game facilities.

### C-S1 — Autonomous settlement need recognition — DONE (#161)
1. SleepingPlace utility from sleep/outdoor-rest pressure.
2. Shelter utility from resident-local environmental exposure.
3. WorkSurface utility from repeated craft/build demand.
4. missing project materials -> Gather demand.
5. Plan / DeliverMaterial / Work -> authoritative spatial ContextAction.
6. Presentation only executes/visualizes Core directive.
7. same-seed / snapshot continuation deterministic.

### C-S2 — Facility effects / maintenance — DONE (#162)
- sleep benefit.
- shelter environmental protection.
- work/craft benefit.
- durability / maintenance.
- ruined/inactive facilities stop providing benefit.
- repairs require actual material/labor.

### C-S3 — Durable subsistence
- water handling/storage.
- food storage/spoilage.
- cultivation/renewable production.
- season/moisture/fertility dependency.
- local scarcity -> exploration/search/movement pressure.

### C-S4 — Emergent settlement geometry
- activity centers emerge from real use.
- household space differentiation.
- sanitation away from dense living.
- storage/fire/work/sleep clusters emerge.
- no hard-coded town-center authority.

### C-S5 — Early material expansion
- TinOre.
- Bronze.
- bronze tools/processes only after actual physical/knowledge/facility prerequisites.
- no automatic Bronze Age switch.

---

## 6. Macro roadmap after Stage C

### Stage D — Long-Run Simulation & Civilization Engine
- long-run deterministic multi-generation simulation.
- History/Fast-forward and event-aware slowdown.
- CPU/memory/snapshot/per-frame work budgets.
- inactive/distant simulation strategy.
- Knowledge -> Capability -> Technology -> CivilizationTransformation.
- discovery -> experiment -> reproducibility -> diffusion -> adoption.
- loss, collapse and rediscovery.

### Stage E — Human Society, Health, Education, Economy & Migration
- health/disease/contamination/immunity/recovery.
- education/apprenticeship/recording/writing.
- roles/jobs/specialization.
- production, exchange, ownership/shared-resource policy.
- institutions/research/education organizations.
- exploration/migration.
- multiple settlements.
- trade routes/resource specialization.
- cooperation/conflict foundations.

### Stage F — Historical Civilization -> Open Future
- advanced metallurgy / urban / science.
- mechanical / industrial.
- electrical / chemical / modern infrastructure.
- digital / network.
- AI / robotics / automation.
- advanced energy / materials / biotechnology.
- planetary / space / interplanetary civilization.
- open future / unknown civilization.

Stage labels organize execution only. They are **not Core era gates**.

---

## 7. Runtime QA not covered by green compile/tests

Still requires runtime/device validation:
- actual HUD z-order/layout/safe area.
- touch gesture conflicts.
- camera feel.
- animation transitions / hand-tool alignment.
- Niagara/material visual quality.
- Android GPU/performance.
- long Unreal session memory/performance.
- APK packaging/device launch.

Android Gate B remains PAUSED BY USER.

---

## 8. Collaboration / merge rule

- Review source as one LifeLens product; ownership is for coordination, not quality silos.
- Correct responsibility beats artificial file separation.
- Same responsibility -> same canonical file.
- coordinate/rebase actual overlaps.
- Dagyeom #98 was selectively ported by #202 onto current main and the original #98 is closed as superseded.

Standing Jjun merge rule remains:
- refresh exact head.
- required CI green.
- refresh active PRs/files.
- no unresolved overlap/conflict.
- expected_head_sha on merge.

---

## 9. Immediate next implementation target

> **Earth & Human Foundation / EH-0 -> EH-1.**

Immediate structural sequence:
1. hierarchy-compatible Planet / Surface Region / Chunk identity contract.
2. authoritative materialized region/chunk enumeration.
3. deterministic hydrology foundation.
4. explicit water bodies and freshwater/saltwater observations.
5. real thirst source selection and collection.
6. only then continue water-dependent settlement/agriculture expansion.


---

## 10. Late 2026-09-18 Presentation / Observer sprint

The earlier sections preserve Stage C / audit context, but the active screen lane moved substantially beyond the #158 snapshot.

Canonical detailed snapshot:
- `docs/PRESENTATION_WORK_STATE_2026-09-18.md`.

Merged since the earlier work-state snapshot:
- #159~#215, including environment, character appearance/motion, facility readability, observer camera/HUD, lifecycle/social/family navigation and Dagyeom #98 integration via #202.

Latest merged wave:
- #216 Detail resident navigation v1.
- #217 Sleep site posture v7.
- #218 Resource depletion visual v2.
- #219 Selected social counterpart v7.
- #220 Snow cover accumulation v6.
- #221 Detailed daypart chrome v5.
- #222 Resident identity badge v7.
- #223 Offscreen action cue v4.
- #224 World event focus return v2.

Each merged only after its exact-head Preflight + Unreal Linux Compile succeeded.
#225 docs reconciliation is merged. #226/#227 Local Surface environment fixes are merged.

The previous practical four-PR batch cap is removed. Independent canonical files may now be developed/compiled in larger parallel batches while same-file responsibilities are sequenced or combined.


---

## 11. Earth & Human Foundation — ACTIVE

Canonical design:
- `docs/EARTH_AND_HUMAN_FOUNDATION.md`.

Preserved:
- current Core authority.
- deterministic WorldSeed/chunks.
- residents, Needs, emotions, relationships, family/lifecycle.
- facilities, civilization, weather/time.
- Save/Load and Observer contracts.

Structural expansion:
- `Planet -> Surface Region -> Chunk -> Local Surface`.
- Observer scale: `Local -> Regional -> Planetary -> Orbital -> Interplanetary`.
- deterministic Simulation LOD for distant regions/populations.
- terrain/hydrology before deeper settlement expansion.
- ecology and richer human body/culture/society layers.

Latest local-surface presentation:
- #226 Far world visual envelope — MERGED.
- #227 Horizon atmosphere blend — MERGED.

These fixes hide the bootstrap square/void at Local scale only; they are not the final planetary geometry.


---

## 12. Cinematic / photoreal graphics foundation — ACTIVE

Merged foundation:
- #238 zero-cost Poly Haven photoreal nature pipeline v2.
- #239 Windows cinematic renderer tier v1.
- #240 Unreal-native environment stack v1 (PCG + Water enabled).
- #241 photoreal primitive-facility asset wave v2.
- #242 Hydrology -> Unreal Water presentation contract v1.
- #243 deterministic PCG presentation seeds v1.

Current renderer contract:
- Windows PC: DX12 + SM6 + Lumen GI/Reflections + VSM + TSR + Nanite project support + Mesh Distance Fields.
- Android: separate mobile-safe renderer tier; same world truth with reduced presentation cost.
- production local-view hero art must not silently fall back to obvious Engine primitives when approved art is missing.

Active validation:
- #244 photoreal WorldPresentation consumers v1 — Preflight PASS, Unreal compile pending at this checkpoint; Dagyeom review requested.
- #245 SkyAtmosphere presentation provider v1 — Preflight PASS, Unreal compile pending at this checkpoint.

Parallel follow-up branches started:
- runtime renderer diagnostics: verify the effective renderer CVars that actually boot, rather than treating config/compile as runtime proof.
- seam-compatible terrain presentation contract: expose deterministic shared corner elevation samples for future non-flat local terrain without creating a second terrain authority.

Android Gate B remains paused until the user explicitly resumes device/APK validation.
