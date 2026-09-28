#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/CivilizationSnapshotCodec.h"
#include "lifelens/CultivationProgression.h"
#include "lifelens/Simulation.h"
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
        population.push_back({
            world.characters[i].id,
            {activity.x+static_cast<int>(i),activity.y}
        });
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
        population.push_back({extra.id,{activity.x+i,activity.y+1}});
    }
    demand=observeCultivationDemand(world,activity,&population);
    CHECK(demand.localResidents==9);
    CHECK(demand.desiredPlots==3);
    CHECK(demand.unmet());

    std::cout << "cultivation progression + persistence passed\n";
    return 0;
}
