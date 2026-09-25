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
> **Current presentation freeze / Dagyeom handoff:** `docs/DAGYEOM_PRESENTATION_HANDOFF_2026-09-19.md`.
>
> Current late-day Presentation/Observer work state: `docs/PRESENTATION_WORK_STATE_2026-09-18.md`.
>
> Historical audit: `docs/INTEGRATED_AUDIT_2026-09-17.md` (point-in-time only; do not treat as live status).

Last reconciled: **2026-09-21 KST after #370/#372/#373 merge; environment/graphics source audit reached the mandatory pre-character notification boundary**.


## 2026-09-21 CURRENT — World Architecture v2 IMPLEMENTATION ACTIVE / W2-2 + W2-3

Canonical design: `docs/WORLD_ARCHITECTURE_v2.md`.

User-approved direction:
- World-first: characters appear on an already-existing Earth surface; the spawn point is not a settlement/living-zone authority.
- continuous Earth: camera scrolling must keep revealing deterministic terrain/water/biome instead of a decorated island on a flat plate.
- settlement emerges from repeated human activity and facilities; natural terrain/vegetation is not cleared around the initial spawn just because it is the initial spawn.
- terrain, hydrology, biome, resources and persistent human deltas share one world-coordinate truth.
- SimulationInterest and ObserverInterest are separate; observer preview must not invent simulation truth.
- Local -> Regional -> Planetary -> Orbital -> Interplanetary remain one world identity at different representation levels.

**Execution state:** user start signal received on 2026-09-21.
- W2-0 provider-contract foundation merged as PR #374 / main `f311e90e2fd63f6e5e78b996c8d30fca1cc0cc3d`.
- W2-1 deterministic continuous terrain Core field merged as PR #375 / main `1336c23453b2707b40823d223aeb76d198981893`.
- W2-2 continuous-terrain drainage graph hydrology merged as PR #378 / main `5c5f18286efa89036a6d622d11a96e9c61524ac4`.
- W2-3 observer-centered streaming interest is active as PR #379.
- current main already has authoritative `GetMaterializedNaturalChunkObservations()` over `generatedNaturalChunks`; the historical count/ring concern is not being redone.
- current new work: continuous-terrain hydrology graph + observer-centered read-only streaming/presentation.
- Character appearance/motion/context implementation remains deferred until World v2 visual/physical foundation.
- implementation ownership is now integrated: after provider contracts stabilize, Jjun may continue directly into WorldPresentation/Environment/Character/UI as the milestone requires; Dagyeom remains visual QA/targeted-polish collaboration.

The previous environment source closeout is reclassified as **structural/source closeout only, not runtime visual acceptance** because the latest runtime screenshots still show flat local terrain, missing forest canopy/water continuity and visible gray/white proxy artifacts.


## 2026-09-21 CURRENT — Whole-source audit 후반부 / 지구·인간 완성도 검증

Canonical checkpoint:
- `docs/PROJECT_STATUS_2026-09-21.md`.
- **whole-source audit resume/dedup ledger: `tasks/AUDIT_LEDGER_2026-09-21.md`.**
- zero-cost external asset policy: `docs/ZERO_COST_ASSET_POLICY_v1.md`.

Current main checkpoint before this docs sync:
- terrain relief v2 and opening settlement ecology clearing v2 are landed on main.
- #364 coast nature/resource visibility recovery — MERGED.
- #360 high-speed route budget — MERGED.
- #361 dependent care after parent loss — MERGED after Core Tests / Preflight / Unreal Linux Compile PASS.
- #362 visible-water fallback — MERGED after Preflight / Unreal Linux Compile PASS and post-#361 mergeability refresh.
- #370 CC0 canopy import — MERGED.
- #372 desktop canopy runtime tier — MERGED after Preflight #1360 / Unreal Linux Compile #620 PASS.
- #373 water fallback terrain clearance — MERGED after Preflight #1355 / Unreal Linux Compile #619 PASS.
- mature-canopy provenance is recorded; Android cook exclusion and desktop-only runtime tier are guarded.

Current audit lane:
- Earth / terrain / hydrology / vegetation / ecology runtime visibility.
- source presence alone is not completion; actual runtime visibility and context correctness are required.
- zero-cost external production assets may be integrated under `docs/ZERO_COST_ASSET_POLICY_v1.md`.

Standing execution rule:
- compile/CI failure -> inspect root cause, fix, rerun.
- exact-head required CI green + latest-main mergeability/overlap check -> merge using expected head SHA.
- do not stop the whole-source audit after a successful merge.

Next audit order:
1. **notify the user before entering character audit — mandatory boundary reached.**
2. character appearance / motion / context correctness.
3. lifecycle / family / society continuity.
4. Observer UI / camera / scale-transition **consumer wiring** audit — do not re-check already confirmed scale enums/contracts.
5. Android-first packaging, Windows/macOS native QA, Save/Load, long-run/performance/crash gates.

Environment/graphics runtime-only carryovers:
- authored Rain/Snow Niagara is not yet present; packaged primitive precipitation remains a safety fallback and requires later visual QA/authored-VFX work.
- sparse high-poly hero canopy requires Windows/macOS native performance profiling before final visual acceptance.

Visual completion rule:
> source presence or green compile alone is not visual acceptance. The runtime must visibly show the intended terrain, water, vegetation, characters and interaction context.

External asset rule:
> zero-cost assets may be downloaded and integrated when they improve production quality, but every new external asset must have verified commercial/package-compatible licensing and recorded provenance. Ambiguous, NC, personal-use-only, editorial-only and ripped assets are prohibited.


## 2026-09-19 CURRENT — Presentation frozen, Core lane resumed

- user froze further Jjun-side visual polishing and handed Presentation back to Dagyeom.
- current visual baseline is main after #282: `c58c63739c13cda24cd9994e26ff6ac5b1d43e76`.
- historical note: #281 / #283 / #284 were later merged on 2026-09-19; this section is no longer live execution state.
- visual CI success is not runtime visual acceptance.
- Jjun active implementation lane is **C1-D Durable Subsistence**:
  - authoritative water carrying/storage.
  - food storage/spoilage.
  - cultivation/agriculture.
  - renewable food production under real time/land/input constraints.
  - season/moisture/fertility effects.
  - scarcity pressure hooks.
- Android Gate B remains paused until explicitly resumed.

Canonical handoff: `docs/DAGYEOM_PRESENTATION_HANDOFF_2026-09-19.md`.


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

## 10. Late 2026-09-18 Presentation / Observer sprint — HISTORICAL

The earlier sections preserve Stage C / audit context. This section is historical; current Presentation execution is frozen on the Jjun side and handed to Dagyeom per the 2026-09-19 handoff.

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

## 12. Cinematic / photoreal graphics foundation — FROZEN / DAGYEOM OWNED

Merged foundation through the handoff baseline includes:
- #238 zero-cost Poly Haven photoreal nature pipeline v2.
- #239 Windows cinematic renderer tier v1.
- #240 Unreal-native environment stack v1.
- #241 photoreal primitive-facility asset wave v2.
- #242 Hydrology -> Unreal Water presentation contract v1.
- #243 deterministic PCG presentation seeds v1.
- #269 platform content/cook boundary.
- #271 desktop PCG runtime dressing.
- #276 Water terrain alignment.
- #277 completed primitive structure photoreal path.
- #278 desktop smooth terrain overlay.
- #279 remaining completed facility visual upgrade.
- #280 whole-world environment density.
- #282 screenshot-driven runtime visual sanity hotfix.

Current visual baseline:
- main `c58c63739c13cda24cd9994e26ff6ac5b1d43e76`.

Open handoff PRs:
- #281 Character animation polish.
- #283 Lighting / atmosphere polish.
- #284 Observer UI / camera polish.

These are Dagyeom review/decision items. Jjun does not auto-merge them.
Actual latest-main runtime screenshots, not compile success alone, decide visual acceptance.

Canonical handoff:
- `docs/DAGYEOM_PRESENTATION_HANDOFF_2026-09-19.md`.

Android Gate B remains paused until the user explicitly resumes device/APK validation.


---

## Platform content / cook boundary — FOUNDATION MERGED

- one repository / one simulation truth.
- Android and Desktop packages carry only required presentation payloads.
- #269 merged the Android/mobile vs desktop photoreal hard-reference and cook boundary. Future presentation payload changes are Dagyeom-owned unless a Core/provider contract is required.
- Android Gate B remains paused; no APK artifact-size claim until actual packaging resumes.

## 2026-09-21 DESIGN ADDENDUM — Earth + Human realism hooks

Canonical human companion: `docs/HUMAN_REALISM_FOUNDATION_v1.md`.

Added future-safe hooks:
- Perception / Knowledge / Belief confidence.
- Place Memory / Attachment.
- Habit / Routine / Preference.
- embodied health/need effects on motion and decisions.
- personal space / social distance / destination reservation.
- ownership / familiarity.
- sensory sound/smell affordances.
- ecological succession / regrowth / decay.
- fauna population LOD.
- regional weather cells.
- persistent historical traces.

Motion asset decision:
- actual behavior gap이 있으면 commercial/package-compatible 비용 0원 animation asset을 선별/다운로드/retarget해 사용 가능.
- provenance / license / skeleton / root-motion / retarget / semantic-action mapping 기록 필수.

Execution lock remains unchanged: **World v2 start signal 전 구현 없음.**

## 2026-09-21 OWNERSHIP UPDATE — integrated implementation

User direction:
- do not block World/Character/Presentation work behind a separate Dagyeom ownership lane.
- Jjun handles end-to-end implementation across Core, World, WorldPresentation, Character, UI and required Content as milestones demand.
- Dagyeom remains available for actual-screen review, visual QA and explicitly delegated polish.
- architectural authority boundaries remain unchanged: Core/World truth stays authoritative; Presentation/Character do not invent simulation outcomes.
- same-file active branch conflicts are still coordinated before edits.

Effect on World v2:
- provider contract -> Presentation handoff is no longer a human ownership gate.
- runtime screenshot acceptance remains mandatory.

## 2026-09-21 ACTIVE — Web/PWA first-class client foundation

User decision:
- retain Unreal Native as high-quality client.
- add Web/PWA as a first-class client sharing the same `LifeLensCore`.

Active branch:
- `jjun/web-client-foundation-v1`

WEB-0 scope:
- platform-neutral `WebClientBridge`.
- native contract test.
- Emscripten/Embind build target.
- PWA shell.
- actual Core world overview/resident/terrain/hydrology truth preview.
- explicit fail-closed state when WASM is unavailable.
- canonical architecture/docs/preflight.

This lane is parallel to active World v2 work and must not fork simulation truth.

## 2026-09-25 Work Mode — Resident continuity / DONE

- Previous Work PR #422 is merged (actual GitHub confirmed); its old integration-pending state is superseded.
- Branch: `work/web-resident-continuity-20260925`, base `9b895a2`.
- Cache identity/coordinate expiry and authoritative removal implemented.
- Local checks: 13 new + 17 existing regression cases, typecheck, production build PASS.
- User requested merge: #447 merged as `4ddf85b`; all four exact-head CI checks passed. Existing main-triggered preview publication is separate from merge completion.
- Recovery instructions: `tasks/WEB_CONTINUITY_HANDOFF_2026-09-25.md`.

## 2026-09-25 — Web rain correction integrated (#448)

- Rain distribution, projected streak direction, DPR sizing and explicit dry-state transitions merged as `e11119280ae122879c38a0e13d496124679b9064`.
- Seven weather + 30 existing regression checks, typecheck/build and all four GitHub checks pass.
- Follow-up: real-device visual QA (cloud browser cannot create WebGL). Preview publication tracked through existing GitHub Pages workflow.
- Checkpoint: `tasks/WEB_RAIN_HANDOFF_2026-09-25.md`.

## 2026-09-25 — Web character appearance repair integrated (#449)

- Appearance range/proportion, skin/garment coloring and head-attached hair repair merged as `9f5fa4653e27f304fe5f8851dfffed9bbde78e53`.
- All four GitHub checks pass, including 43 regression checks and typecheck/build. Actual GLB rig binding checked without GPU.
- Remaining: separate female base asset and real-device appearance/gait validation; #423 remains the action-context lane.
- Checkpoint: `tasks/WEB_CHARACTER_HANDOFF_2026-09-25.md`.

## 2026-09-25 — Web touch interaction correction integrated (#450)

- Merged `ae6d90fc4abd7c907d1aa9b41d07cbe4d4e126a8`: corrected pan/vertical orbit direction, tap slop and multi-touch release transitions; selected-life-first mobile sheet.
- Four exact-head GitHub checks and 50 regression cases pass. Subjective device touch/visual acceptance remains open.
- Checkpoint: `tasks/WEB_TOUCH_HANDOFF_2026-09-25.md`.
