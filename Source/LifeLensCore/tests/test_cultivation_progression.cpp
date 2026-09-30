#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/CivilizationSnapshotCodec.h"
#include "lifelens/CultivationProgression.h"
#include "lifelens/Simulation.h"
#include "lifelens/SocialUtility.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

int main()
{
    Simulation simulation(26092841,0,3);
    simulation.setupNewGame();
    World& world=simulation.world();
    CHECK(world.characters.size()==4);

    Character& actor=world.characters.front();
    actor.civilization.knowledge.learn(
        TechniqueId::DiggingStick,KnowledgeLevel::Reproducible,0.95);
    actor.civilization.knowledge.learn(
        TechniqueId::Cultivation,KnowledgeLevel::Reproducible,0.92);
    actor.civilization.inventory.add({
        ItemKind::DiggingStick,MaterialKind::Wood,1,0.8,1.0});

    const GridPos activity=world.initialStartRegionCenterGrid();
    const CultivatedPlotSiteOpportunity site=
        chooseCultivatedPlotSite(world,actor.id,activity);
    CHECK(site.available);
    CHECK(cultivationTerrainSuitable(world,site.pos));
    CHECK(site.environment.fertility01>=0.24);

    ConstructedFacility* plot=
        establishCultivatedPlotProject(world,actor.id,site.pos);
    CHECK(plot!=nullptr);
    CHECK(plot->kind==FacilityKind::CultivatedPlot);
    CHECK(plot->state==FacilityState::Planned);

    // Physical construction is not free.
    for(const auto& requirement:plot->requirements){
        const int missing=facilityMissingMaterial(*plot,requirement.material);
        CHECK(missing>0);
        actor.civilization.inventory.add({
            ItemKind::RawMaterial,requirement.material,missing,0.6,1.0});
        CHECK(deliverFacilityMaterial(
            *plot,actor.civilization.inventory,requirement.material,missing)==missing);
    }
    for(int i=0;i<16 && plot->state!=FacilityState::Operational;++i){
        const CultivatedPlotWorkResult work=
            workOnCultivatedPlot(world,actor,plot->id,2.0);
        CHECK(work.worked);
        plot=&world.facilities.front();
    }
    CHECK(plot->state==FacilityState::Operational);
    CHECK(plot->active);

    // Planting consumes a real PlantFood unit as seed.
    CHECK(!plantCultivatedPlot(world,actor,*plot));
    actor.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::PlantFood,2,0.7,1.0});
    const int seedBefore=actor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood);
    CHECK(plantCultivatedPlot(world,actor,*plot));
    CHECK(plot->cropPlanted);
    CHECK(actor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)==seedBefore-1);

    // Cultivation state is authoritative and survives the binary snapshot.
    actor.civilization.inventory.add({
        ItemKind::SimpleContainer,MaterialKind::Clay,2,0.7,1.0});
    actor.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Water,2,0.8,1.0});
    CHECK(waterCultivatedPlot(world,actor,*plot));
    CHECK(tendCultivatedPlot(world,actor,*plot));
    advanceCultivatedPlotOneDay(world,*plot);
    CHECK(plot->cropGrowth01>0.0);
    CHECK(plot->cropGrowth01<1.0);

    std::string error;
    std::vector<std::uint8_t> bytes;
    CHECK(encodeSimulationSnapshot(
        simulation.captureSnapshot(),bytes,&error));
    CHECK(error.empty());

    SimulationStateSnapshot decoded;
    CHECK(decodeSimulationSnapshot(bytes,decoded,&error));
    CHECK(error.empty());
    CHECK(decoded.world.facilities.size()==world.facilities.size());
    const ConstructedFacility& decodedPlot=decoded.world.facilities.front();
    CHECK(decodedPlot.kind==FacilityKind::CultivatedPlot);
    CHECK(decodedPlot.cropPlanted);
    CHECK(decodedPlot.cropPlantedMinute==plot->cropPlantedMinute);
    CHECK(decodedPlot.cropGrowth01==plot->cropGrowth01);
    CHECK(decodedPlot.cropMoisture01==plot->cropMoisture01);
    CHECK(decodedPlot.cropCare01==plot->cropCare01);
    CHECK(decodedPlot.cropHarvestUnits==plot->cropHarvestUnits);

    // Sustained labor/water + suitable land eventually yields actual food.
    for(int day=0;day<365 && plot->cropHarvestUnits==0;++day){
        if(plot->cropMoisture01<0.46){
            actor.civilization.inventory.add({
                ItemKind::RawMaterial,MaterialKind::Water,1,0.8,1.0});
            CHECK(waterCultivatedPlot(world,actor,*plot));
        }
        if(plot->cropCare01<0.58){
            CHECK(tendCultivatedPlot(world,actor,*plot));
        }
        advanceCultivatedPlotOneDay(world,*plot);
    }
    CHECK(plot->cropGrowth01>=1.0);
    CHECK(plot->cropHarvestUnits>=2);

    const int foodBeforeHarvest=actor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood);
    const int harvested=harvestCultivatedPlot(world,actor,*plot);
    CHECK(harvested>=2);
    CHECK(actor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)==foodBeforeHarvest+harvested);
    CHECK(!plot->cropPlanted);
    CHECK(plot->cropGrowth01==0.0);
    CHECK(plot->cropHarvestUnits==0);

    // Local population pressure scales desired plot capacity rather than
    // treating one global plot as sufficient forever.
    SettlementPopulation population;
    for(std::size_t i=0;i<world.characters.size();++i){
        population.emplace(
            world.characters[i].id,
            GridPos{activity.x+static_cast<int>(i),activity.y});
    }
    CultivationDemandObservation demand=
        observeCultivationDemand(world,activity,&population);
    CHECK(demand.localResidents==4);
    CHECK(demand.desiredPlots==1);

    for(int i=0;i<5;++i){
        Character extra=world.characters.front();
        extra.id=9000+i;
        extra.alive=true;
        extra.civilization.character=extra.id;
        world.characters.push_back(extra);
        population.emplace(
            extra.id,
            GridPos{activity.x+i,activity.y+1});
    }
    demand=observeCultivationDemand(world,activity,&population);
    CHECK(demand.localResidents==9);
    CHECK(demand.desiredPlots==3);
    CHECK(demand.unmet());

    // Production civilization decisions must follow the resident's lived
    // position, not the NEW GAME entry coordinate. Put a thirsty crop and a
    // water stockpile well outside the original settlement service radius.
    Simulation movedSimulation(26092842,0,3);
    movedSimulation.setupNewGame();
    World& movedWorld=movedSimulation.world();
    Character& movedActor=movedWorld.characters.front();
    movedActor.needs={0.08,0.08,0.08,0.08,0.08};
    movedActor.civilization.inventory=Inventory{};
    movedActor.civilization.knowledge.learn(
        TechniqueId::DiggingStick,KnowledgeLevel::Reproducible,0.95);
    movedActor.civilization.knowledge.learn(
        TechniqueId::Cultivation,KnowledgeLevel::Reproducible,0.95);
    movedActor.civilization.inventory.add({
        ItemKind::DiggingStick,MaterialKind::Wood,1,0.8,1.0});

    const GridPos movedStart=movedWorld.initialStartRegionCenterGrid();
    const GridPos livedFarm{
        movedStart.x+CultivationServiceRadiusGrid+24,
        movedStart.y
    };

    ConstructedFacility livedPlot=makeFacilityConstructionSite(
        nextFacilityId(movedWorld.facilities),
        FacilityKind::CultivatedPlot,
        livedFarm,
        movedActor.id,
        movedWorld.minute);
    CHECK(livedPlot.id!=0);
    for(auto& requirement:livedPlot.requirements){
        requirement.delivered=requirement.required;
    }
    livedPlot.constructionWork=livedPlot.requiredWork;
    CHECK(activateConstructedFacility(livedPlot,0,movedWorld.minute));
    livedPlot.cropPlanted=true;
    livedPlot.cropPlantedMinute=movedWorld.minute;
    livedPlot.cropGrowth01=0.30;
    livedPlot.cropMoisture01=0.10;
    livedPlot.cropCare01=0.90;
    movedWorld.facilities.push_back(livedPlot);

    StorageSite livedStorage;
    livedStorage.id=1;
    livedStorage.pos={livedFarm.x+1,livedFarm.y};
    livedStorage.inventory.add({
        ItemKind::SimpleContainer,MaterialKind::Clay,1,0.8,1.0});
    livedStorage.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Water,1,0.8,1.0});
    movedWorld.storageSites.push_back(livedStorage);

    CHECK(cultivationInputNeededNear(
        movedWorld,livedFarm,MaterialKind::Water));
    CHECK(!cultivationInputNeededNear(
        movedWorld,movedStart,MaterialKind::Water));

    const CivilizationUtilityDecision legacyOriginRetrieve=
        bestRetrieveDecision(movedWorld,movedActor);
    CHECK(legacyOriginRetrieve.intent==CivilizationIntent::None);

    const CivilizationUtilityDecision livedRetrieve=
        bestRetrieveDecisionAtPosition(
            movedWorld,movedActor,livedFarm);
    CHECK(livedRetrieve.intent==CivilizationIntent::Retrieve);
    CHECK(livedRetrieve.material==MaterialKind::Water);
    CHECK(livedRetrieve.storage==movedWorld.storageSites.front().id);

    const CivilizationUtilityDecision dispositionAware=
        chooseDispositionAwareCivilizationDecisionAtPosition(
            movedWorld,movedActor,livedFarm);
    CHECK(dispositionAware.intent==CivilizationIntent::Retrieve);
    CHECK(dispositionAware.material==MaterialKind::Water);
    CHECK(dispositionAware.storage==movedWorld.storageSites.front().id);

    // Same physical affordances must not collapse every resident into the same
    // development role. A patient, conscientious grower and an exploratory
    // forager both know cultivation and can use the same empty plot, but their
    // deterministic Utility competition should produce different choices.
    Simulation diversitySimulation(26092843,0,3);
    diversitySimulation.setupNewGame();
    World& diversityWorld=diversitySimulation.world();
    diversityWorld.facilities.clear();
    diversityWorld.storageSites.clear();
    diversityWorld.resourceNodes.clear();
    diversityWorld.characters.resize(2);

    GridPos diversityAnchor{};
    CHECK(diversitySimulation.runtimePosition(
        diversityWorld.characters.front().id,diversityAnchor));

    Character& grower=diversityWorld.characters[0];
    Character& forager=diversityWorld.characters[1];
    grower.needs={0.34,0.08,0.08,0.08,0.08};
    forager.needs=grower.needs;

    for(Character* resident:{&grower,&forager}){
        resident->civilization.character=resident->id;
        resident->civilization.inventory=Inventory{};
        resident->civilization.knowledge.learn(
            TechniqueId::DiggingStick,KnowledgeLevel::Reproducible,0.98);
        resident->civilization.knowledge.learn(
            TechniqueId::Cultivation,KnowledgeLevel::Reproducible,0.98);
        resident->civilization.inventory.add({
            ItemKind::DiggingStick,MaterialKind::Wood,1,0.9,1.0});
        resident->civilization.inventory.add({
            ItemKind::RawMaterial,MaterialKind::PlantFood,1,0.8,1.0});
    }

    grower.personality.patience=1.0;
    grower.personality.conscientiousness=1.0;
    grower.personality.orderliness=0.9;
    grower.personality.adaptability=0.55;
    grower.personality.curiosity=0.08;
    grower.personality.openness=0.18;
    grower.personality.riskTolerance=0.08;
    grower.civilization.gatheringSkill=0.10;
    grower.civilization.craftingSkill=0.55;

    forager.personality.patience=0.05;
    forager.personality.conscientiousness=0.05;
    forager.personality.orderliness=0.05;
    forager.personality.adaptability=1.0;
    forager.personality.curiosity=1.0;
    forager.personality.openness=0.90;
    forager.personality.riskTolerance=0.85;
    forager.civilization.gatheringSkill=1.0;
    forager.civilization.craftingSkill=0.20;

    ConstructedFacility diversityPlot=makeFacilityConstructionSite(
        990201,
        FacilityKind::CultivatedPlot,
        {diversityAnchor.x+1,diversityAnchor.y},
        grower.id,
        diversityWorld.minute);
    CHECK(diversityPlot.id!=0);
    for(auto& requirement:diversityPlot.requirements){
        requirement.delivered=requirement.required;
    }
    diversityPlot.constructionWork=diversityPlot.requiredWork;
    CHECK(activateConstructedFacility(
        diversityPlot,0,diversityWorld.minute));
    diversityWorld.facilities.push_back(diversityPlot);

    ResourceNode wildFood;
    wildFood.id=990301;
    wildFood.material=MaterialKind::PlantFood;
    wildFood.quantity=12;
    wildFood.maxQuantity=12;
    wildFood.pos={diversityAnchor.x+3,diversityAnchor.y};
    diversityWorld.resourceNodes.push_back(wildFood);

    SettlementPopulation diversityPopulation;
    diversityPopulation.emplace(grower.id,diversityAnchor);
    diversityPopulation.emplace(forager.id,diversityAnchor);

    const CivilizationUtilityDecision growerChoice=
        chooseDispositionAwareCivilizationDecisionAtPosition(
            diversityWorld,grower,diversityAnchor,&diversityPopulation);
    const CivilizationUtilityDecision foragerChoice=
        chooseDispositionAwareCivilizationDecisionAtPosition(
            diversityWorld,forager,diversityAnchor,&diversityPopulation);

    CHECK(growerChoice.intent==CivilizationIntent::Craft);
    CHECK(growerChoice.technique==TechniqueId::Cultivation);
    CHECK(growerChoice.facilityAction==FacilityBuildAction::Plant);
    CHECK(foragerChoice.intent==CivilizationIntent::Gather);
    CHECK(foragerChoice.material==MaterialKind::PlantFood);
    CHECK(foragerChoice.resourceNode==wildFood.id);

    // The split is deterministic, not random refusal.
    const CivilizationUtilityDecision growerAgain=
        chooseDispositionAwareCivilizationDecisionAtPosition(
            diversityWorld,grower,diversityAnchor,&diversityPopulation);
    const CivilizationUtilityDecision foragerAgain=
        chooseDispositionAwareCivilizationDecisionAtPosition(
            diversityWorld,forager,diversityAnchor,&diversityPopulation);
    CHECK(growerAgain.intent==growerChoice.intent);
    CHECK(growerAgain.facilityAction==growerChoice.facilityAction);
    CHECK(foragerAgain.intent==foragerChoice.intent);
    CHECK(foragerAgain.resourceNode==foragerChoice.resourceNode);

    // Performing the chosen cultivation action becomes lived experience and
    // strengthens future role specialization without creating a permanent job.
    const double experienceBefore=
        civilizationTechniqueExperience01(grower,TechniqueId::Cultivation);
    const CivilizationExecutionResult planted=
        executeCivilizationDecisionAtPosition(
            diversityWorld,
            grower,
            growerChoice,
            diversityWorld.facilities.front().pos,
            &diversityPopulation);
    CHECK(planted.executed && planted.success);
    CHECK(civilizationTechniqueExperience01(
        grower,TechniqueId::Cultivation)>experienceBefore);

    std::cout << "cultivation progression + lived-position economy + autonomous role diversity passed\n";
    return 0;
}
