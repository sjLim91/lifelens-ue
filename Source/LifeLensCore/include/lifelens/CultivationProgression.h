#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "Character.h"
#include "ContinuousEcology.h"
#include "Facility.h"
#include "SettlementDemand.h"
#include "SimulationCalendar.h"
#include "SimulationClimate.h"
#include "World.h"

namespace lifelens {

inline constexpr int CultivationServiceRadiusGrid=WorldChunkSpanGridCells*2;
inline constexpr int CultivationResidentsPerPlot=4;
inline constexpr int CultivationFoodReservePerResident=4;
inline constexpr double CultivationBaseGrowthDays=24.0;

struct CultivationEnvironmentObservation {
    double fertility01=0.0;
    double naturalMoisture01=0.0;
    double temperatureSuitability01=0.0;
    double seasonSuitability01=0.0;
    double growthSuitability01=0.0;
};

struct CultivatedPlotSiteOpportunity {
    bool available=false;
    GridPos pos{};
    double score=0.0;
    CultivationEnvironmentObservation environment{};
};

struct CultivationDemandObservation {
    int localResidents=0;
    int desiredPlots=0;
    int committedPlots=0;
    int localFoodUnits=0;
    int desiredFoodReserve=0;
    double capacityPressure=0.0;
    double foodPressure=0.0;
    double pressure=0.0;

    bool unmet() const {
        return committedPlots<desiredPlots || localFoodUnits<desiredFoodReserve;
    }
};

struct CultivatedPlotWorkResult {
    FacilityId facilityId=0;
    bool worked=false;
    bool completed=false;
    double workBefore=0.0;
    double workAfter=0.0;
};

inline double cultivationClamp01(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

inline CultivationEnvironmentObservation observeCultivationEnvironment(
    const World& world,
    GridPos pos,
    int minute)
{
    CultivationEnvironmentObservation result;
    const ContinuousEcologySample ecology=
        deriveContinuousEcologySample(world.genesisIdentity(),pos);
    const DynamicEnvironmentObservation climate=
        deriveDynamicEnvironment(
            world.genesisIdentity(),chunkCoordForGrid(pos),minute);
    const SimulationCalendarObservation calendar=
        deriveSimulationCalendar(minute);

    const double flatness=cultivationClamp01(1.0-ecology.slope01);
    const double soilTexture=cultivationClamp01(
        0.46*ecology.grassCoverage01
        +0.24*ecology.moisture01
        +0.18*ecology.waterInfluence01
        +0.12*(1.0-ecology.rockCoverage01));
    result.fertility01=cultivationClamp01(
        soilTexture*(0.40+0.60*flatness));

    result.naturalMoisture01=cultivationClamp01(
        0.42*ecology.moisture01
        +0.28*ecology.waterInfluence01
        +0.20*climate.surfaceWetness01
        +0.10*climate.humidity01);

    result.temperatureSuitability01=cultivationClamp01(
        1.0-std::abs(climate.airTemperatureC-20.0)/22.0);

    switch(calendar.season){
        case SeasonSummary::Spring: result.seasonSuitability01=1.0; break;
        case SeasonSummary::Summer: result.seasonSuitability01=0.95; break;
        case SeasonSummary::Autumn: result.seasonSuitability01=0.72; break;
        case SeasonSummary::Winter: result.seasonSuitability01=0.16; break;
    }

    result.growthSuitability01=cultivationClamp01(
        result.fertility01
        *result.temperatureSuitability01
        *result.seasonSuitability01);
    return result;
}

inline bool cultivationTerrainSuitable(const World& world,GridPos pos)
{
    const ContinuousEcologySample ecology=
        deriveContinuousEcologySample(world.genesisIdentity(),pos);
    if(ecology.biome==ContinuousEcologyBiome::Ocean
       || ecology.biome==ContinuousEcologyBiome::Coast
       || ecology.biome==ContinuousEcologyBiome::Wetland
       || ecology.biome==ContinuousEcologyBiome::RockyHighland) return false;
    if(ecology.slope01>0.56 || ecology.rockCoverage01>0.72) return false;
    return observeCultivationEnvironment(world,pos,world.minute).fertility01>=0.24;
}

inline bool cultivatedPlotSiteBlocked(const World& world,GridPos pos)
{
    for(const auto& facility:world.facilities){
        if(facility.state!=FacilityState::Ruined
           && facilityFootprintsConflict(
               FacilityKind::CultivatedPlot,pos,facility.kind,facility.pos)){
            return true;
        }
    }

    const int footprintRadius=
        facilityFootprintRadiusGrid(FacilityKind::CultivatedPlot);
    for(const auto& sanitation:world.primitiveSanitationSites){
        if(sanitation.active
           && manhattan(sanitation.pos,pos)<=5+footprintRadius) return true;
    }
    for(const auto& node:world.resourceNodes){
        if(node.quantity>0
           && manhattan(node.pos,pos)<=1+footprintRadius) return true;
    }
    return !cultivationTerrainSuitable(world,pos);
}

inline CultivatedPlotSiteOpportunity chooseCultivatedPlotSite(
    const World& world,
    CharacterId planner,
    GridPos activityAnchor)
{
    CultivatedPlotSiteOpportunity best;
    if(planner==0) return best;

    // Search outward from lived activity, not NEW GAME spawn. The score favors
    // productive flat/moist ground while still allowing the settlement to
    // expand when inner rings are occupied.
    for(int radius=9;radius<=30;radius+=3){
        for(int dx=-radius;dx<=radius;dx+=3){
            for(int dy=-radius;dy<=radius;dy+=3){
                if(std::max(std::abs(dx),std::abs(dy))!=radius) continue;
                const GridPos candidate{activityAnchor.x+dx,activityAnchor.y+dy};
                if(cultivatedPlotSiteBlocked(world,candidate)) continue;

                const CultivationEnvironmentObservation environment=
                    observeCultivationEnvironment(world,candidate,world.minute);
                const int distance=manhattan(candidate,activityAnchor);
                const double distancePenalty=cultivationClamp01(
                    static_cast<double>(distance)/48.0);
                const double score=cultivationClamp01(
                    0.58*environment.fertility01
                    +0.22*environment.naturalMoisture01
                    +0.14*environment.temperatureSuitability01
                    +0.06*(1.0-distancePenalty));

                if(!best.available || score>best.score+1e-12
                   || (std::abs(score-best.score)<=1e-12
                       && (candidate.x<best.pos.x
                           || (candidate.x==best.pos.x
                               && candidate.y<best.pos.y)))){
                    best.available=true;
                    best.pos=candidate;
                    best.score=score;
                    best.environment=environment;
                }
            }
        }
        if(best.available) break;
    }
    return best;
}

inline const ConstructedFacility* cultivatedPlotProjectNear(
    const World& world,
    GridPos pos,
    int maxDistance=CultivationServiceRadiusGrid)
{
    const ConstructedFacility* best=nullptr;
    int bestDistance=std::max(0,maxDistance)+1;
    for(const auto& facility:world.facilities){
        if(facility.kind!=FacilityKind::CultivatedPlot
           || facility.state==FacilityState::Ruined
           || facility.state==FacilityState::Operational) continue;
        const int distance=manhattan(facility.pos,pos);
        if(distance>std::max(0,maxDistance)) continue;
        if(best==nullptr || distance<bestDistance
           || (distance==bestDistance && facility.id<best->id)){
            best=&facility;
            bestDistance=distance;
        }
    }
    return best;
}

inline ConstructedFacility* cultivatedPlotProjectNear(
    World& world,
    GridPos pos,
    int maxDistance=CultivationServiceRadiusGrid)
{
    return const_cast<ConstructedFacility*>(
        cultivatedPlotProjectNear(
            static_cast<const World&>(world),pos,maxDistance));
}

inline int operationalCultivatedPlotCountNear(
    const World& world,
    GridPos pos,
    int maxDistance=CultivationServiceRadiusGrid)
{
    int count=0;
    for(const auto& facility:world.facilities){
        if(facility.kind==FacilityKind::CultivatedPlot
           && facilityOperationalAndActive(facility)
           && manhattan(facility.pos,pos)<=std::max(0,maxDistance)){
            ++count;
        }
    }
    return count;
}

inline int cultivationLocalResidentCount(
    const World& world,
    GridPos center,
    const SettlementPopulation* population)
{
    if(population==nullptr){
        int count=0;
        for(const auto& character:world.characters) if(character.alive) ++count;
        return count;
    }

    int count=0;
    for(const auto& entry:*population){
        if(manhattan(entry.second,center)>CultivationServiceRadiusGrid) continue;
        for(const auto& character:world.characters){
            if(character.id==entry.first && character.alive){
                ++count;
                break;
            }
        }
    }
    return count;
}

inline int cultivationLocalFoodUnits(
    const World& world,
    GridPos center,
    const SettlementPopulation* population)
{
    int units=0;
    if(population==nullptr){
        for(const auto& character:world.characters){
            if(!character.alive) continue;
            units+=character.civilization.inventory.count(
                ItemKind::RawMaterial,MaterialKind::PlantFood);
        }
    }else{
        for(const auto& entry:*population){
            if(manhattan(entry.second,center)>CultivationServiceRadiusGrid) continue;
            for(const auto& character:world.characters){
                if(character.id!=entry.first || !character.alive) continue;
                units+=character.civilization.inventory.count(
                    ItemKind::RawMaterial,MaterialKind::PlantFood);
                break;
            }
        }
    }

    for(const auto& storage:world.storageSites){
        if(manhattan(storage.pos,center)>CultivationServiceRadiusGrid) continue;
        units+=storage.inventory.count(
            ItemKind::RawMaterial,MaterialKind::PlantFood);
    }
    return units;
}

inline CultivationDemandObservation observeCultivationDemand(
    const World& world,
    GridPos center,
    const SettlementPopulation* population=nullptr)
{
    CultivationDemandObservation result;
    result.localResidents=
        cultivationLocalResidentCount(world,center,population);
    if(result.localResidents<=0) return result;

    result.desiredPlots=std::max(
        1,(result.localResidents+CultivationResidentsPerPlot-1)
            /CultivationResidentsPerPlot);
    result.committedPlots=operationalCultivatedPlotCountNear(world,center);
    if(cultivatedPlotProjectNear(world,center)!=nullptr) ++result.committedPlots;

    result.localFoodUnits=
        cultivationLocalFoodUnits(world,center,population);
    result.desiredFoodReserve=
        result.localResidents*CultivationFoodReservePerResident;

    result.capacityPressure=cultivationClamp01(
        static_cast<double>(
            std::max(0,result.desiredPlots-result.committedPlots))
        /static_cast<double>(std::max(1,result.desiredPlots)));
    result.foodPressure=cultivationClamp01(
        static_cast<double>(
            std::max(0,result.desiredFoodReserve-result.localFoodUnits))
        /static_cast<double>(std::max(1,result.desiredFoodReserve)));
    result.pressure=std::max(
        result.capacityPressure,
        0.35*result.capacityPressure+0.65*result.foodPressure);
    return result;
}

inline bool cultivationExperimentOpportunityAvailable(
    const World& world,
    const Character& self,
    GridPos pos,
    const SettlementPopulation* population=nullptr)
{
    if(self.id==0
       || self.civilization.inventory.count(
           ItemKind::RawMaterial,MaterialKind::PlantFood)<=0) return false;
    if(!self.civilization.knowledge.knowsAtLeast(
        TechniqueId::DiggingStick,KnowledgeLevel::Reproducible)) return false;

    const CultivationDemandObservation demand=
        observeCultivationDemand(world,pos,population);
    if(demand.pressure<0.28) return false;

    const CultivatedPlotSiteOpportunity site=
        chooseCultivatedPlotSite(world,self.id,pos);
    return site.available && site.environment.growthSuitability01>=0.18;
}

inline ConstructedFacility* establishCultivatedPlotProject(
    World& world,
    CharacterId planner,
    GridPos pos)
{
    if(planner==0 || cultivatedPlotSiteBlocked(world,pos)) return nullptr;
    ConstructedFacility facility=makeFacilityConstructionSite(
        nextFacilityId(world.facilities),
        FacilityKind::CultivatedPlot,
        pos,
        planner,
        world.minute);
    if(facility.id==0) return nullptr;
    world.facilities.push_back(std::move(facility));
    return &world.facilities.back();
}

inline CultivatedPlotWorkResult workOnCultivatedPlot(
    World& world,
    Character& worker,
    FacilityId facilityId,
    double work)
{
    CultivatedPlotWorkResult result;
    result.facilityId=facilityId;
    for(auto& facility:world.facilities){
        if(facility.id!=facilityId
           || facility.kind!=FacilityKind::CultivatedPlot) continue;
        result.workBefore=facility.constructionWork;
        const bool completedByWork=applyFacilityConstructionWork(
            facility,worker.id,std::max(0.0,work));
        result.workAfter=facility.constructionWork;
        result.worked=result.workAfter>result.workBefore+1e-12;
        if(!result.worked) return result;
        if(completedByWork || facilityWorkComplete(facility)){
            result.completed=activateConstructedFacility(
                facility,0,world.minute);
        }
        return result;
    }
    return result;
}

inline bool plantCultivatedPlot(
    World& world,
    Character& worker,
    ConstructedFacility& facility)
{
    if(facility.kind!=FacilityKind::CultivatedPlot
       || !facilityOperationalAndActive(facility)
       || facility.cropPlanted
       || !worker.civilization.knowledge.knowsAtLeast(
           TechniqueId::Cultivation,KnowledgeLevel::Reproducible)
       || worker.civilization.inventory.count(
           ItemKind::DiggingStick,MaterialKind::Wood,true)<=0
       || worker.civilization.inventory.count(
           ItemKind::RawMaterial,MaterialKind::PlantFood)<=0) return false;

    if(!worker.civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::PlantFood,1)) return false;

    const auto environment=
        observeCultivationEnvironment(world,facility.pos,world.minute);
    facility.cropPlanted=true;
    facility.cropPlantedMinute=world.minute;
    facility.cropGrowth01=0.0;
    facility.cropMoisture01=cultivationClamp01(
        0.12+0.42*environment.naturalMoisture01);
    facility.cropCare01=0.24;
    facility.cropHarvestUnits=0;
    facility.lastCultivationMinute=world.minute;
    facility.lastWorkedBy=worker.id;
    return true;
}

inline bool waterCultivatedPlot(
    World& world,
    Character& worker,
    ConstructedFacility& facility)
{
    if(facility.kind!=FacilityKind::CultivatedPlot
       || !facilityOperationalAndActive(facility)
       || !facility.cropPlanted
       || facility.cropHarvestUnits>0
       || portableWaterCount(
           worker.civilization.inventory)<=0) return false;

    if(!consumePortableWater(
        worker.civilization.inventory,1)) return false;
    facility.cropMoisture01=cultivationClamp01(
        facility.cropMoisture01+0.48);
    facility.lastCultivationMinute=world.minute;
    facility.lastWorkedBy=worker.id;
    return true;
}

inline bool tendCultivatedPlot(
    World& world,
    Character& worker,
    ConstructedFacility& facility)
{
    if(facility.kind!=FacilityKind::CultivatedPlot
       || !facilityOperationalAndActive(facility)
       || !facility.cropPlanted
       || facility.cropHarvestUnits>0
       || worker.civilization.inventory.count(
           ItemKind::DiggingStick,MaterialKind::Wood,true)<=0) return false;

    facility.cropCare01=cultivationClamp01(
        facility.cropCare01+0.34);
    facility.lastCultivationMinute=world.minute;
    facility.lastWorkedBy=worker.id;
    return true;
}

inline int harvestCultivatedPlot(
    World& world,
    Character& worker,
    ConstructedFacility& facility)
{
    if(facility.kind!=FacilityKind::CultivatedPlot
       || !facilityOperationalAndActive(facility)
       || !facility.cropPlanted
       || facility.cropGrowth01<1.0
       || facility.cropHarvestUnits<=0) return 0;

    const int harvested=facility.cropHarvestUnits;
    const auto environment=
        observeCultivationEnvironment(world,facility.pos,world.minute);
    const double quality=cultivationClamp01(
        0.42+0.30*environment.fertility01+0.28*facility.cropCare01);
    worker.civilization.inventory.add({
        ItemKind::RawMaterial,
        MaterialKind::PlantFood,
        harvested,
        quality,
        1.0
    });

    facility.cropPlanted=false;
    facility.cropPlantedMinute=-1;
    facility.cropGrowth01=0.0;
    facility.cropMoisture01=0.0;
    facility.cropCare01=0.0;
    facility.cropHarvestUnits=0;
    facility.lastCultivationMinute=world.minute;
    facility.lastWorkedBy=worker.id;
    return harvested;
}

inline void advanceCultivatedPlotOneDay(
    World& world,
    ConstructedFacility& facility)
{
    if(facility.kind!=FacilityKind::CultivatedPlot
       || !facilityOperationalAndActive(facility)
       || !facility.cropPlanted) return;

    const ContinuousEcologySample ecology=
        deriveContinuousEcologySample(world.genesisIdentity(),facility.pos);
    const DynamicEnvironmentObservation climate=
        deriveDynamicEnvironment(
            world.genesisIdentity(),
            chunkCoordForGrid(facility.pos),
            world.minute);
    const CultivationEnvironmentObservation environment=
        observeCultivationEnvironment(world,facility.pos,world.minute);

    const double rainRecharge=cultivationClamp01(
        0.62*climate.precipitationIntensity01
        +0.24*climate.surfaceWetness01
        +0.14*environment.naturalMoisture01);
    const double evaporation=cultivationClamp01(
        0.07
        +(climate.airTemperatureC>26.0
            ? (climate.airTemperatureC-26.0)/65.0 : 0.0)
        +0.05*climate.windIntensity01);
    facility.cropMoisture01=cultivationClamp01(
        facility.cropMoisture01*0.78
        +0.34*rainRecharge
        +0.08*ecology.moisture01
        -evaporation);
    facility.cropCare01=cultivationClamp01(
        facility.cropCare01-0.055);

    if(facility.cropHarvestUnits>0) return;

    const double moistureFactor=cultivationClamp01(
        (facility.cropMoisture01-0.06)/0.54);
    const double careFactor=
        0.36+0.64*facility.cropCare01;
    const double dailyGrowth=
        (1.0/CultivationBaseGrowthDays)
        *(0.28+0.72*environment.fertility01)
        *moistureFactor
        *environment.temperatureSuitability01
        *environment.seasonSuitability01
        *careFactor;

    facility.cropGrowth01=cultivationClamp01(
        facility.cropGrowth01+std::max(0.0,dailyGrowth));

    if(facility.cropGrowth01>=1.0-1e-9){
        facility.cropGrowth01=1.0;
        facility.cropHarvestUnits=std::max(
            2,
            static_cast<int>(std::lround(
                3.0
                +5.0*environment.fertility01
                +2.0*facility.cropCare01)));
    }
}

inline void advanceCultivationOneDay(World& world)
{
    for(auto& facility:world.facilities){
        advanceCultivatedPlotOneDay(world,facility);
    }
}

inline int cultivationConstructionMissingMaterial(
    const World& world,
    MaterialKind material)
{
    int missing=0;
    for(const auto& facility:world.facilities){
        if(facility.kind!=FacilityKind::CultivatedPlot
           || facility.state==FacilityState::Operational
           || facility.state==FacilityState::Ruined) continue;
        missing+=facilityMissingMaterial(facility,material);
    }
    return missing;
}

inline bool cultivationInputNeededNear(
    const World& world,
    GridPos pos,
    MaterialKind material)
{
    for(const auto& facility:world.facilities){
        if(facility.kind!=FacilityKind::CultivatedPlot
           || !facilityOperationalAndActive(facility)
           || manhattan(facility.pos,pos)>CultivationServiceRadiusGrid) continue;
        if(material==MaterialKind::PlantFood && !facility.cropPlanted) return true;
        if(material==MaterialKind::Water && facility.cropPlanted
           && facility.cropHarvestUnits==0
           && facility.cropMoisture01<0.46) return true;
    }
    return false;
}

} // namespace lifelens
