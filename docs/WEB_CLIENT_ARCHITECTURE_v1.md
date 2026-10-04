# LifeLens Web Client Architecture v1

> Status: **CANONICAL / IMPLEMENTATION ACTIVE**
>
> Updated: 2026-09-29 KST
>
> Decision: the active LifeLens product path is **LifeLensCore -> WASM -> Web Observer**. The former Unreal client is archived and is not part of active `main`.

## 1. Product shape

```text
                    LifeLensCore
          World + Human simulation truth
                         |
              explicit read/action contracts
                         |
                  Emscripten / WASM
                         |
                         v
                 Web Observer / PWA
          React + TypeScript + Three.js
```

The renderer is replaceable. LifeLensCore is not.

## 2. Authority

### LifeLensCore owns

- WorldSeed / PopulationSeed / WorldGenerationVersion.
- world generation, terrain, hydrology and ecology truth.
- resident identity/state.
- needs/emotion/personality/memory/belief.
- relationships/family/lifecycle.
- civilization/resources/facilities/history.
- authoritative save state and deterministic progression.

### Web Observer owns

- browser/PWA lifecycle.
- WebGL/Three.js presentation.
- browser input/camera.
- glTF/GLB character/prop presentation.
- browser-local persistence transport.
- HTML/CSS/React observer UI.
- visual interpolation, LOD and effects.

The Web client never forks or reimplements simulation rules.

## 3. Identity and transport

64-bit IDs/seeds cross the JavaScript boundary as lossless strings unless a BigInt-specific contract is explicitly introduced. They must not be rounded through JavaScript Number.

Browser-facing state is exported through explicit bridge/DTO contracts rather than direct C++ memory layout coupling.

## 4. Web runtime bridge

The bridge exposes authoritative Core operations and observation snapshots across the WASM ABI.

Current/future contract areas include:

- new game and deterministic replay.
- simulation time progression and bounded fast-forward.
- world/resident observation.
- terrain/hydrology/ecology windows.
- event/history feed.
- selected resident detail and genealogy.
- civilization resources/storage/facilities.
- save snapshot import/export.
- observer-interest queries.
- persistent human/world traces.
- action/motion presentation DTOs.

## 5. WASM build

`LifeLensCore` stays C++17.

Emscripten is an additional build target, not a separate Core fork.

```bash
python Tools/build_web_client.py
```

Generated JS/WASM binaries are release/build artifacts.

## 6. Presentation progression

- truth shell: actual Core world/residents, fail-closed runtime loading.
- world surface: continuous terrain, hydrology, water, free observer camera.
- ecology: biome/vegetation/ground cover from authority facts.
- residents: stable identity, semantic action/motion mapping and contextual targets.
- observer product: detail, relationships, family/genealogy, events, time controls and persistence.

## 7. No fake fallback

If Core WASM fails to load:

- do not spawn fake residents.
- do not generate a JavaScript-only authoritative terrain.
- do not synthesize events/resources/facilities.
- show an explicit Core-unavailable state.

Presentation-only placeholders must never look like durable LifeLens truth.

## 8. Performance model

The Web client may reduce mesh density, vegetation density, shadows, materials, animation complexity and effect budgets. It may not make the logical world smaller or alter simulation outcomes to fit rendering performance.

### Completed social events in the world scene

Core social truth remains authoritative. Current `ResidentPresentationDirective`
interaction connectors describe ongoing Social/Comfort/Repair/Teaching/Parenting;
the separate `SocialEventLayer` presents only completed `RecentSocialEvent` types.
`WorldSession.refresh().socialEvents` supplies both ObserverStore/Observation Feed
and observer-engine → WorldRenderer → WorldScene after resident position updates.
The Feed retains exact descriptions and directional relationship deltas.

World cues map the nine exact types to outward ripple, directional help pulse,
settling comfort arc, conflict zigzag, broken betrayal line with target aftermath,
rejection approach/return chevron, apology reconnecting line, converging intimacy
rings, and commitment double arc. These imply no marriage, item transfer or
conflict resolution. Unknown enum values are safely skipped. Current Core
`makeSocialCommunicationObservation` always exports `successful=true`; a future
false value weakens/shortens the same type without inventing failure semantics.

Observer-local memory stores at most 8 active cues and 128 seen event keys.
Sequence strings retain full precision; a monotonic numeric high-water mark
prevents replay after eviction. Zero/empty sequences use the Feed's
minute/actor/target/type/where compatibility identity. Initial presentation admits
at most 2 cues from a cadence-derived 48-minute window; stale history is consumed
without replay. Each cue expires after 1.2/2/2.8 seconds plus up to 0.35 seconds
for importance, using an injectable monotonic wall clock independent of speed
and pause. Intensity controls scale/opacity; false success uses 40% opacity and
70% lifetime. Admission prefers level, importance, then recency.

Anchors snapshot living positioned actors' current interpolated render locations.
Missing actors and pairs beyond 4.5 world units are skipped; deletion/death clears
active cues. Camera-origin changes rebase snapshots without moving residents.
Fast-forward clears active cues while preserving seen history; new worlds reset
presentation memory. One fixed ribbon buffer/mesh, one material and at most one
draw call cover the whole layer. Depth testing keeps cues integrated in the world.
No cue state is saved, sent to Core, or used for AI/movement/relationship decisions.

## 9. Persistence

Long-term Web persistence uses the same Core save codec:
- OPFS preferred when supported.
- IndexedDB fallback.
- user-controlled import/export where practical.

No incompatible browser-only simulation save format.

## 10. Deployment

The Web client is static-hostable and has no required paid backend.

The zero-cost publication path builds LifeLensCore with Emscripten and publishes the stable browser runtime. GitHub Pages is the canonical checkable preview.

## 11. Future native clients

A future Unreal or other native client can be introduced as another presentation adapter around the then-current LifeLensCore contracts. Core must remain free of renderer dependencies so that reintroduction does not require rebuilding the simulation model.

## 12. Acceptance

Web foundation is healthy when:
1. Core tests and deterministic harness pass.
2. WASM builds from the same LifeLensCore.
3. browser UI fails closed without WASM.
4. browser displays actual Core residents/world state.
5. terrain/hydrology presentation derives from Core truth.
6. TypeScript and production Web build pass.
7. the product remains functional without a paid backend.


### Facility lifecycle scene presentation (2026-10-04)

Core decides facility reality; Web only derives a visual phase. The current
HumanTrace facility read model supplies state, work ratio, delivered totals,
activation, fire and crop facts. CivilizationWorldFacility is joined by exact
id, kind and grid position for durability, material requirements and linked
storage. Both progress fields serialize constructionWork / requiredWork.
The faster trace wins when detail lags. Typed material piles require matching
state and matching required/delivered totals; otherwise their type stays generic.
Unstarted Planned sites omitted from Core HumanTrace can be projected from actual
civilization facilities inside observed chunks. They do not clear vegetation or
create save state. Existing trace selection and footprint spacing stay intact.

| Kind | Previous early/material appearance | Previous work 10 / 30 / 60 / 90% | Preserved operating facts |
|---|---|---|---|
| PrimitiveStorage | Stakes; early deliveries indistinct | Stakes / base+posts / body / cover | Linked stored goods |
| FirePit | Stakes; early deliveries indistinct | Stakes / stones / fuel / full | Actual lit glow |
| WorkSurface | Stakes; early deliveries indistinct | Stakes / legs / top / stone | Existing worker motion |
| SleepingPlace | Stakes; early deliveries indistinct | Stakes / branches / mat+edges / full | Primitive mat, native sleep support |
| Shelter | Stakes; early deliveries indistinct | Stakes / posts / frame+roof / walls | Existing roof/wall silhouette |
| Furnace | Stakes; early deliveries indistinct | Stakes / lower / chamber / upper | Actual fire |
| CultivatedPlot | Stakes; early deliveries indistinct | Stakes / soil / borders+ridges / full | Actual planted/growth/moisture/care/harvest |
| Unknown | Stakes; early deliveries indistinct | Stakes / posts / posts / top | Safe primitive fallback |

The renderer now shows delivered material independently of work, with no
structure at zero work. Existing causal part thresholds use a bounded smooth
reveal derived from actual progress, quantized to 40 presentation steps. It
never advances work by elapsed time. Operational is confirmed only by Core.
Durability bands (> .75, > .45, > .20, otherwise severe) reuse shared worn
materials. Severe wear affects structural silhouette and mat edges, preserving
occupied sleep support height. Kind-specific collapsed remnants retain the site.
Same facility id keeps the same root object through construction, ruin and restore.
No change within a visual progress/condition band rebuilds geometry.

Actual Civilization Work/Repair Interacting directives, matched by facilityId,
produce a small world scratch cue. Moving, proximity, DeliverMaterial arrival,
repair start and inactivity cannot increase delivery/work/durability or restore
a facility. Delivery and worker motion remain the existing resident presentation.
Repair highlights the activity; geometry recovers only after factual state or
condition changes. One fixed LineSegments geometry and material cover at most
8 active sites (32 vertices, at most one extra draw call), paused with simulation.
Shared primitive geometry and original per-facility construction meshes remain;
SleepingPlace retains its three instance batches. Existing HumanTrace visibility
budget remains 64 entries; a 150-facility input exercises that existing bound.
Presentation caches and cosmetic phase live only in the Observer and never feed
Core decisions or save authority. Social events, emissions, snow, wetness and
crop/storage facts keep their existing lanes.


### Resident lifecycle scene presentation

Core decides life. Web derives silhouette, posture and bounded world cues from
Resident DTOs; it never creates people, kinship, households, pregnancy or events.
The existing age/genetics model is retained. Core Baby/Toddler/Child/Teen stages
control juvenile silhouettes; YoungAdult/Adult/MiddleAge share the adult profile;
Elderly uses a subtle spine lean, shorter animation cadence and softened scalp
pigment. Unknown future stages stay neutral. Age only refines size; it never
corrects an explicit stage. Same resident root persists and scale settles over
0.9 observer seconds around the existing feet pivot, freezing on pause. Actual
position interpolation, travel speed, targets and Core directives are unchanged.

Pregnancy is a small torso deformation on existing skinned materials, selected
only by pregnancy.role=GestationalParent and First/Second/ThirdTrimester or Due.
First has no bump; Second/Third/Due use 0.035/0.065/0.075 of normalized body height.
Completed, GeneticPartner and missing/unknown pregnancy remove it. The shader
uses rest-model coordinates and a spine/pelvis mask, follows existing skinning,
and allocates no pregnancy mesh/material. Existing phenotype geometry is only
rebuilt when actual coloring/proportion facts change, not on height-only refresh.

Parenting uses the exact current directive, actual living positioned target and
actual Feed/PutToSleep/Bathe/ToiletAssist/Hold/Play/Educate/Discipline/Comfort/
HealthCare action. Moving retains locomotion; Interacting retains the existing
care/comfort/teach motion with a small spine inclination at close range. Parent
and child connector heights follow their actual rendered stature. There is no
Carry action in current Core; Hold does not attach or reposition the child.
Child touch selection has a bounded 0.22-world-unit fallback; actual mesh hits
retain priority. Baby locomotion remains neutral rather than adult walking while
its real Core position still follows the existing renderer interpolation.

lifeHistory flows with existing residents through WorldScene; the completed
life-event layer is observed alongside social events after resident positioning.
Initial supported history is a baseline, never replayed. Append cursors retain
at most 384 residents, seen keys 128, active cues 8. Keys use resident id/type/
minute/sorted related ids; reciprocal relationship and parent/child birth records
coalesce by exact participants/site. Unchanged histories use constant-size
cursors; truncated/sliding histories fail closed at the previous minute watermark.
Old events outside the existing cadence-derived social replay window are skipped.
Birth/ChildBorn require an actual living positioned Baby; ChildBorn additionally
requires an exact parent/child family link. A missing child creates no fake mesh.
Death can use the previous rendered site captured before the actual dead actor
is hidden; bereavement is local and never joins to a deceased resident.

Cues last 2.2 or 3.5 observer seconds, pause with presentation and have one fixed
ribbon geometry/material/draw call. Missing actors clear living cues; chunk shifts
rebase site snapshots; new-world/reset clears caches. Major relationship life
cues suppress only same-pair, same-minute Intimacy/Commitment world ribbons.
Social event memory and Observation Feed are untouched. Existing social actions,
facility lifecycle, weather, sleep and emissions retain their lanes. No lifecycle
presentation state is saved or feeds Core/AI decisions. Permanent household links,
automatic follow, ceremonies and parenting success effects are excluded because
these are not current exact action/event presentation contracts.
