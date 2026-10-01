#include <iostream>

#include "lifelens/CivilizationDecision.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character makeResident(CharacterId id)
{
    Character c;
    c.id=id;
    c.name="Innovator";
    c.alive=true;
    c.civilization.character=id;
    c.personality.curiosity=0.72;
    c.personality.openness=0.70;
    c.personality.patience=0.58;
    c.civilization.learningSkill=0.62;
    c.civilization.craftingSkill=0.55;
    return c;
}

static void addWater(World& world,GridPos pos)
{
    ResourceNode water;
    water.id=1;
    water.pos=pos;
    water.material=MaterialKind::Water;
    water.quantity=100;
    water.maxQuantity=100;
    water.renewable=true;
    water.regenerationPerDay=10;
    world.resourceNodes.push_back(water);
}

static World waterWorld(bool distant)
{
    World world(444444);
    world.characters.clear();
    world.resourceNodes.clear();
    world.storageSites.clear();
    world.facilities.clear();
    world.generatedNaturalChunks.clear();

    Character resident=makeResident(1);
    resident.needs.thirst=0.80;
    resident.civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::Clay,3,0.5,1.0});
    world.characters.push_back(resident);

    addWater(
        world,
        distant
            ? GridPos{WorldChunkSpanGridCells*2,0}
            : GridPos{1,0});
    return world;
}

int main()
{
    const GridPos origin{0,0};

    World far=waterWorld(true);
    World near=waterWorld(false);

    const InnovationPressureObservation farContainer=
        observeTechnologyInnovationPressure(
            far,far.characters[0],
            TechnologyId::SimpleContainer,origin,nullptr);
    const InnovationPressureObservation nearContainer=
        observeTechnologyInnovationPressure(
            near,near.characters[0],
            TechnologyId::SimpleContainer,origin,nullptr);

    CHECK(farContainer.technology==TechnologyId::SimpleContainer);
    CHECK(farContainer.dominantDriver==InnovationPressureDriver::Logistics);
    CHECK(farContainer.logistics01>nearContainer.logistics01+0.40);
    CHECK(farContainer.pressure01>nearContainer.pressure01+0.20);

    // Only ShapeClay is materially feasible here. The same personality, seed,
    // inventory and minute should value it more when water transport is a real
    // long-distance burden.
    const CivilizationUtilityDecision farExperiment=
        bestExperimentDecisionAtPosition(
            far,far.characters[0],origin,nullptr);
    const CivilizationUtilityDecision nearExperiment=
        bestExperimentDecisionAtPosition(
            near,near.characters[0],origin,nullptr);

    CHECK(farExperiment.intent==CivilizationIntent::Experiment);
    CHECK(farExperiment.experiment==ExperimentKind::ShapeClay);
    CHECK(farExperiment.technique==TechniqueId::SimpleContainer);
    CHECK(farExperiment.hasInnovationPressure);
    CHECK(farExperiment.innovationPressure.technology==
        TechnologyId::SimpleContainer);
    CHECK(farExperiment.innovationPressure.dominantDriver==
        InnovationPressureDriver::Logistics);
    CHECK(farExperiment.utility>nearExperiment.utility+0.05);

    // Once the physical transport problem is solved by a vessel, the logistics
    // driver collapses without deleting curiosity or the generic research path.
    far.characters[0].civilization.inventory.add(
        {ItemKind::SimpleContainer,MaterialKind::Clay,1,0.7,1.0});
    const InnovationPressureObservation solvedContainer=
        observeTechnologyInnovationPressure(
            far,far.characters[0],
            TechnologyId::SimpleContainer,origin,nullptr);
    CHECK(solvedContainer.logistics01<farContainer.logistics01-0.40);
    CHECK(solvedContainer.pressure01<farContainer.pressure01);

    // Food insecurity should independently drive cultivation research.
    World foodWorld(555555);
    foodWorld.characters.clear();
    foodWorld.resourceNodes.clear();
    foodWorld.storageSites.clear();
    foodWorld.facilities.clear();

    Character farmer=makeResident(2);
    farmer.needs.hunger=0.80;
    farmer.civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::PlantFood,1,0.6,1.0});
    foodWorld.characters.push_back(farmer);

    const InnovationPressureObservation hungryCultivation=
        observeTechnologyInnovationPressure(
            foodWorld,foodWorld.characters[0],
            TechnologyId::Cultivation,origin,nullptr);
    CHECK(hungryCultivation.dominantDriver==
        InnovationPressureDriver::FoodSecurity);
    CHECK(hungryCultivation.foodSecurity01>0.75);

    // An operational planted plot plus a real local food reserve resolves the
    // specific food-security pressure while other personal Needs can remain.
    ConstructedFacility plot;
    plot.id=10;
    plot.kind=FacilityKind::CultivatedPlot;
    plot.state=FacilityState::Operational;
    plot.active=true;
    plot.durability=1.0;
    plot.cropPlanted=true;
    plot.cropHarvestUnits=4;
    foodWorld.facilities.push_back(plot);
    foodWorld.characters[0].civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::PlantFood,20,0.6,1.0});

    const InnovationPressureObservation securedCultivation=
        observeTechnologyInnovationPressure(
            foodWorld,foodWorld.characters[0],
            TechnologyId::Cultivation,origin,nullptr);
    CHECK(securedCultivation.foodSecurity01<
        hungryCultivation.foodSecurity01-0.60);
    CHECK(securedCultivation.pressure01<
        hungryCultivation.pressure01-0.30);

    // The reusable context snapshot and convenience path must produce the same
    // deterministic result.
    const InnovationPressureContextSnapshot context=
        observeInnovationPressureContext(
            near,near.characters[0],origin,nullptr);
    const InnovationPressureObservation fromContext=
        observeTechnologyInnovationPressure(
            near,near.characters[0],
            TechnologyId::SimpleContainer,origin,context);
    CHECK(fromContext.pressure01==nearContainer.pressure01);
    CHECK(fromContext.dominantDriver==nearContainer.dominantDriver);

    std::cout << "civilization innovation pressure engine passed\n";
    return 0;
}
