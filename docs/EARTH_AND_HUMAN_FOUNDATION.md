# LifeLens Earth & Human Foundation

> Status: **Canonical architecture direction**
>
> **World architecture update (2026-09-21):** Earth-scale world implementation details are now canonical in `docs/WORLD_ARCHITECTURE_v2.md`. This document remains canonical for the broader Earth + human/civilization direction; where world-generation, start-area, settlement, streaming or presentation wording conflicts, World Architecture v2 wins.
>
> This document expands the existing autonomous-life/civilization design without discarding the current Core.
>
> Truth order: actual GitHub main / PR / Actions > this document > older roadmap wording.

---

## 1. Product direction

LifeLens is not only a settlement simulator.

The long-range product is an observer simulation in which individual humans are born into an Earth-scale physical environment, form relationships and families, learn and transmit knowledge, create culture and society, transform the environment, build civilization, and may eventually leave the planet and establish life on other worlds.

The simulation must remain causal at every scale.

A date, era label, camera zoom level, or UI choice must never grant technology, infrastructure, resources, social institutions, or interplanetary capability for free.

---

## 2. What stays

The current architecture remains valuable and is preserved:

- deterministic WorldSeed.
- persistent resident identity / GUID mapping.
- exactly 2 male + 2 female adult founders at NEW GAME.
- Needs / Emotion / Personality / Memory / Belief / Relationship foundations.
- romance / household / marriage / pregnancy / birth / growth / aging / death.
- Utility AI and action execution.
- authoritative facilities, resources and spatial actions.
- calendar / day-night / season / weather.
- Core / World as sole simulation authority.
- Presentation as read-only projection.
- deterministic Save / Load.
- Observer-first UI.
- Android-first delivery.

This direction is an expansion, not a rewrite.

---

## 3. What changes structurally

The current effective hierarchy is close to:

`WorldSeed -> 2D Chunk -> Local simulation`

It must evolve toward:

`Celestial System -> Planet -> Surface Region -> Chunk -> Local Surface`

The current Chunk coordinate contract remains valid inside the initial local surface while the higher hierarchy is added above it.

No existing settlement/family/Need work should be rewritten merely to introduce hierarchy.

---

## 4. World scale model

### 4.1 Local Surface

Highest simulation detail.

Used for:
- individual residents.
- exact walking/navigation.
- facilities.
- trees, rocks and nearby resources.
- local water access.
- household life.
- physical social interactions.

Current WorldPresentation and #226 / #227 belong to this scale.

The far visual ground introduced there is a **Local Surface LOD envelope**, not the physical size of Earth.

### 4.2 Regional

Represents areas too large to simulate every blade of grass or every resident action every frame.

Contains summarized:
- terrain.
- watersheds.
- settlements.
- population.
- ecology.
- resources.
- transport/trade links.
- climate.

Local regions can materialize into full chunks when observation or simulation requires them.

### 4.3 Planetary

Represents the planet as a sphere / planetary body.

Contains:
- land/ocean distribution.
- large mountain systems.
- climate zones.
- major watersheds.
- biomes.
- regional population/civilization state.
- atmosphere and planetary resources.

A distant observer should see a globe rather than an infinite flat plane.

### 4.4 Orbital

Represents:
- the planet globe.
- atmosphere.
- moons.
- artificial satellites.
- stations.
- launch/orbital infrastructure.

Local resident simulation continues in Core while the observer uses orbital presentation.

### 4.5 Interplanetary

Represents:
- multiple celestial bodies.
- transfer routes.
- off-world settlements.
- orbital industry.
- communication delays where relevant.
- resource flows between worlds.

Each colonized world can independently expose:

`Planet -> Region -> Chunk -> Local Surface`

---

## 5. Observer scale

Canonical observer scale direction:

`Local -> Regional -> Planetary -> Orbital -> Interplanetary`

The camera must not literally travel millions of Unreal units between these scales.

Representation changes by observer scale while Core identity and simulation state remain continuous.

Desired visual continuity:

`resident -> home -> settlement -> region -> continent/ocean context -> planet curvature -> whole planet -> orbit -> planetary system`

---

## 6. Simulation LOD

Earth-scale simulation cannot run every human, animal and ecosystem element at full local fidelity continuously.

LifeLens therefore requires deterministic simulation LOD.

### Full / local
- exact resident AI.
- exact movement.
- detailed Needs.
- physical actions.
- detailed facilities/resources.

### Regional
- aggregate population and household flows.
- summarized production/consumption.
- migration.
- ecology/resource pressure.
- important individual events retained.

### Planetary / remote
- large-scale demographic, climate, ecological and civilization state.
- event-driven transitions.
- no fake local actions.

Promotion/demotion between LODs must be deterministic and preserve important people, history, resources and causal state.

---

## 7. Earth systems

### EH-E1 Terrain

Required:
- plains.
- hills.
- mountains.
- valleys.
- plateaus.
- cliffs.
- basins.
- caves where appropriate.
- elevation-driven traversal.

Terrain must be more than a flat visual plane.

### EH-E2 Hydrology

WaterPotential alone is not enough.

Core needs explicit water-system concepts:
- spring.
- stream.
- river.
- lake.
- pond.
- wetland.
- groundwater.
- coast.
- ocean.

Water state eventually includes:
- fresh / brackish / salt.
- flow.
- recharge.
- capacity/availability.
- contamination.
- seasonal variation.

Drainage should broadly follow elevation and climate rather than arbitrary decoration.

### EH-E3 Water cycle

Long-term direction:
- precipitation.
- runoff.
- infiltration.
- groundwater.
- river flow.
- lakes.
- evaporation.
- ocean return.

The first implementation may use deterministic approximations; it must leave room for this causal loop.

### EH-E4 Geology / soil

- soil types.
- fertility.
- moisture retention.
- erosion.
- rock/mineral distribution.
- clay.
- salt.
- limestone.
- copper/tin/iron/coal and later industrial resources.

Resource distribution should follow geography/geology rather than uniform random placement.

### EH-E5 Climate

Build on current time/weather:
- latitude/region effects.
- elevation effects.
- ocean influence.
- climate zones.
- seasonal regional differences.
- wind/humidity/precipitation patterns.
- drought/heat/cold/storm pressure.

### EH-E6 Natural hazards

Long-range:
- flood.
- drought.
- wildfire.
- storm.
- heat wave.
- cold wave.
- landslide.
- earthquake.
- volcanic activity where geography supports it.

No disaster should exist only as a UI event; it must have world consequences.

---

## 8. Ecology

Current vegetation/resource presentation is only a foundation.

Required direction:
- plant growth and death.
- regeneration/succession.
- species suitability by climate/soil.
- herbivores.
- predators.
- birds.
- fish.
- insects where simulation value justifies them.
- reproduction and population pressure.
- food webs.
- hunting/fishing.
- domestication.
- crop domestication.
- over-hunting/deforestation/ecosystem damage.
- ecological recovery.

Ecology also requires LOD; distant animal populations should not all be Actors.

---

## 9. Human body / health

Existing Needs remain but expand into a richer human model.

Direction:
- hydration.
- caloric balance.
- macronutrient/micronutrient abstractions where useful.
- body temperature.
- fatigue.
- sleep quality.
- strength/endurance.
- body growth and aging.
- injury.
- bleeding.
- fracture.
- burns.
- infection.
- disease.
- immunity.
- disability / long-term impairment.
- pregnancy physical state.
- postpartum recovery.
- aging-related decline.
- cause-specific mortality.

Do not over-model biology where it adds no observable behavior; prioritize causal human consequences.

---

## 10. Food / subsistence

- gathering.
- hunting.
- fishing.
- farming.
- seeds.
- sowing/harvest.
- seasonal yield.
- livestock.
- food preparation.
- cooking.
- spoilage.
- drying.
- smoking.
- salting.
- fermentation.
- refrigeration when capability exists.
- food preference/culture.
- nutrition effects.

---

## 11. Water use progression

Water becomes a real spatial need:

`thirst -> find freshwater -> travel -> drink/collect -> carry -> store`

Possible later capability progression:
- natural spring/river collection.
- containers.
- storage.
- boiling/filtration.
- wells.
- canals/irrigation.
- aqueduct/pipes.
- municipal water.
- sewage treatment.
- desalination.
- closed-loop off-world water recovery.

Saltwater is not freely drinkable freshwater.

---

## 12. Human mind

Build on Personality / Emotion / Memory / Belief:

- attachment.
- trust.
- jealousy.
- guilt.
- pride.
- shame.
- loneliness.
- grief.
- hope.
- fear.
- habit.
- preferences.
- curiosity.
- patience.
- impulsivity.
- stress.
- burnout.
- values.
- changing goals.

Different people should interpret the same event differently through memory, belief, personality and relationship context.

---

## 13. Relationships and family

Extend current foundations with:
- friendship.
- close friendship.
- rivalry.
- hostility.
- unrequited affection.
- romantic attraction.
- breakups.
- divorce.
- remarriage.
- widowhood.
- parent-child attachment.
- sibling rivalry.
- favoritism.
- generational conflict.
- mentor/student.
- neighbor/coworker relationships.
- reputation and rumor.

Relationship state must affect behavior, not only numbers in UI.

---

## 14. Language / communication

Required long-range direction:
- topic-based conversation.
- questions/answers.
- teaching.
- promises.
- secrets.
- rumors.
- persuasion.
- negotiation.
- misunderstanding.
- lying where personality/context supports it.
- records.
- writing.
- language/cultural transmission.

Knowledge should spread through actual people, records and institutions, not global unlocks.

---

## 15. Culture

Culture should emerge and change over generations.

Possible dimensions:
- naming customs.
- food traditions.
- clothing.
- hairstyle/adornment.
- family customs.
- marriage practices.
- funerary practices.
- festivals.
- music.
- dance.
- visual art.
- storytelling.
- myths.
- spiritual/religious belief.
- taboos.
- local traditions.

No single modern real-world culture is the mandatory endpoint.

---

## 16. Society / economy

Long-range systems:
- household.
- kin groups.
- settlement.
- organizations.
- roles.
- leadership.
- norms.
- rules/law.
- dispute resolution.
- punishment/restitution.
- reputation.
- social status.
- common/private resources.
- property.
- inheritance.
- gifts.
- barter.
- money.
- markets.
- prices/scarcity.
- labor.
- specialization.
- employment.
- production.
- trade.
- credit.
- institutions.

Specific political/economic systems must not be hard-coded as the universal destination.

---

## 17. Ordinary human life

LifeLens must not become a nonstop survival/task machine.

Residents need room for:
- meals together.
- casual conversation.
- rest.
- play.
- walking.
- visiting.
- hobbies.
- gifts.
- celebrations.
- childcare.
- leisure.
- solitude.
- mourning.
- anniversaries/meaningful dates.

These are important to making observed people feel human.

---

## 18. History

The simulation should retain meaningful history:
- births/deaths.
- families.
- discoveries.
- inventions.
- disasters.
- migrations.
- settlement founding/abandonment.
- cultural change.
- conflicts/reconciliation.
- major infrastructure.
- notable individuals.

Observer history must eventually support navigating centuries without storing every trivial frame.

---

## 19. Civilization / technology

Existing Knowledge -> Capability -> Technology -> CivilizationTransformation remains canonical.

Additions must preserve:
- discovery.
- experimentation.
- failure.
- reproducibility.
- teaching/recording.
- adoption.
- loss.
- collapse.
- rediscovery.

Technology is not a fixed era timer.

---

## 20. Planetary / space future

Long-range civilization may progress into:
- aviation.
- rocketry.
- satellites.
- orbital stations.
- lunar/planetary missions.
- off-world extraction.
- permanent settlements.
- closed-loop life support.
- interplanetary logistics.
- multi-planet society.

These require actual knowledge, energy, materials, infrastructure and social capacity.

The observer should eventually be able to zoom from a resident to a planet and then to other worlds without implying that all scales use the same Unreal coordinate space.

---

## 21. Execution order

### EH-0 — World hierarchy / scale contract
- Planet identity.
- Surface region identity.
- existing chunk compatibility.
- observer scale contract.
- materialized-region enumeration.
- deterministic seeds per hierarchy layer.

### EH-1 — Terrain + hydrology foundation
- elevation model.
- watershed/drainage representation.
- explicit water bodies.
- freshwater/saltwater.
- coast/ocean foundation.
- water observation contracts.

### EH-2 — Water-driven survival
- real thirst source selection.
- travel to freshwater.
- drinking/collection.
- carrying/storage.
- contamination/boiling foundation.
- settlement pressure near reliable water.

This absorbs and expands the old C-S3 water scope.

### EH-3 — Ecology
- renewable plants.
- animal population foundations.
- hunting/fishing.
- ecological pressure.

### EH-4 — Human physiology / health
- richer body state.
- nutrition/hydration consequences.
- injury/disease foundations.
- pregnancy/postpartum physical consequences.

### EH-5 — Language / culture / ordinary life
- richer communication.
- knowledge records.
- customs/traditions.
- leisure and everyday behavior.

### EH-6 — Society / economy / institutions
- specialization.
- ownership/exchange.
- markets/trade.
- organizations/education/law foundations.

### EH-7 — Planetary observer / orbital architecture
- regional and planetary visual representation.
- sphere/globe transition.
- orbital observer scale.
- celestial body identity.

### EH-8 — Interplanetary civilization
- only after physical/civilization prerequisites.
- off-world regions use the same hierarchy principles.

---

## 22. Relationship to existing roadmap

Completed work is preserved.

- C-S0 Facility Authority remains done.
- C-S1 Autonomous Settlement Need Recognition remains done.
- C-S2 Facility Effects/Maintenance remains done.
- C-S3 is expanded into EH-1/EH-2 plus later food/agriculture work.
- C-S4 Emergent Settlement Geometry remains relevant after hydrology because water/topography should shape settlement form.
- C-S5 Tin/Bronze remains relevant after geology/resource distribution.
- Stage D long-run simulation becomes tightly coupled to Simulation LOD.
- Stage E human society is expanded by EH-4~EH-6.
- Stage F open future remains the long-range outcome and now explicitly includes planetary scale architecture.

---

## 23. Immediate implementation priority

Do **not** continue adding isolated cosmetic world details before the Earth foundation exists.

Immediate priority:

1. land the current Local Surface boundary fixes (#226/#227).
2. add hierarchy-compatible world identity contracts.
3. expose authoritative materialized region/chunk enumeration.
4. add explicit hydrology Core types and deterministic water observations.
5. connect thirst to real freshwater sources.
6. render rivers/lakes/coast from authoritative water data.
7. then resume settlement/agriculture expansion on top of real geography.

---

## 24. Non-goals for the first foundation pass

Do not attempt immediately:
- fully accurate planetary fluid simulation.
- every animal species.
- every disease.
- billions of full-detail humans.
- literal real-world map data.
- seamless physical Unreal coordinates from ground to Mars.

The architecture must permit these domains to deepen later without forcing a rewrite.

---

## 25. Definition of success

Earth & Human Foundation is working when:

- one resident feels tiny relative to the surrounding world.
- water is a real place/resource, not only a numeric potential.
- settlement location is shaped by terrain/water/resources.
- distant regions exist without full Actor simulation.
- zooming outward has a defined path toward a planet representation.
- human survival, relationships, culture and civilization all consume the same authoritative world.
- future off-world civilization can reuse the hierarchy rather than replacing it.
