#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <utility>

#include "ContinuousEcology.h"

#include "EnvironmentalConsequences.h"
#include "Facility.h"
#include "PrimitiveSanitation.h"
#include "SettlementDemand.h"
#include "World.h"

namespace lifelens {

// C1 settlement facilities are deliberately not free bootstrap objects. They are
// Core-authored construction projects with real sites, materials and work.
inline bool isSettlementFoundationFacility(FacilityKind kind)
{
    return kind==FacilityKind::WorkSurface
        || kind==FacilityKind::SleepingPlace
        || kind==FacilityKind::Shelter;
}

inline bool hasOperationalSettlementFacility(const World& world,FacilityKind kind)
{
    if(!isSettlementFoundationFacility(kind)) return false;
    for(const auto& facility:world.facilities){
        if(facility.kind==kind
           && facility.state==FacilityState::Operational
           && facility.active) return true;
    }
    return false;
}

inline const ConstructedFacility* settlementFacilityProject(
    const World& world,
    FacilityKind kind)
{
    if(!isSettlementFoundationFacility(kind)) return nullptr;
    for(const auto& facility:world.facilities){
        if(facility.kind==kind && facility.state!=FacilityState::Ruined) return &facility;
    }
    return nullptr;
}

inline ConstructedFacility* settlementFacilityProject(
    World& world,
    FacilityKind kind)
{
    if(!isSettlementFoundationFacility(kind)) return nullptr;
    for(auto& facility:world.facilities){
        if(facility.kind==kind && facility.state!=FacilityState::Ruined) return &facility;
    }
    return nullptr;
}

inline const ConstructedFacility* operationalSettlementFacility(
    const World& world,
    FacilityKind kind)
{
    if(!isSettlementFoundationFacility(kind)) return nullptr;
    for(const auto& facility:world.facilities){
        if(facility.kind==kind && facilityOperationalAndActive(facility)) return &facility;
    }
    return nullptr;
}

inline ConstructedFacility* operationalSettlementFacility(
    World& world,
    FacilityKind kind)
{
    if(!isSettlementFoundationFacility(kind)) return nullptr;
    for(auto& facility:world.facilities){
        if(facility.kind==kind && facilityOperationalAndActive(facility)) return &facility;
    }
    return nullptr;
}

inline const ConstructedFacility* operationalSettlementFacilityNear(
    const World& world,
    FacilityKind kind,
    GridPos pos,
    int maxDistance=1)
{
    const ConstructedFacility* best=nullptr;
    int bestDistance=std::max(0,maxDistance)+1;
    for(const auto& facility:world.facilities){
        if(facility.kind!=kind || !facilityOperationalAndActive(facility)) continue;
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

inline ConstructedFacility* operationalSettlementFacilityNear(
    World& world,
    FacilityKind kind,
    GridPos pos,
    int maxDistance=1)
{
    ConstructedFacility* best=nullptr;
    int bestDistance=std::max(0,maxDistance)+1;
    for(auto& facility:world.facilities){
        if(facility.kind!=kind || !facilityOperationalAndActive(facility)) continue;
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

inline const ConstructedFacility* bestOperationalSleepFacility(
    const World& world,
    GridPos pos,
    int maxDistance=1)
{
    const ConstructedFacility* sleeping=
        operationalSettlementFacilityNear(
            world,FacilityKind::SleepingPlace,pos,maxDistance);
    const ConstructedFacility* shelter=
        operationalSettlementFacilityNear(
            world,FacilityKind::Shelter,pos,maxDistance);

    if(sleeping==nullptr) return shelter;
    if(shelter==nullptr) return sleeping;

    const int sleepingDistance=manhattan(sleeping->pos,pos);
    const int shelterDistance=manhattan(shelter->pos,pos);
    if(sleepingDistance!=shelterDistance){
        return sleepingDistance<shelterDistance ? sleeping : shelter;
    }

    // Dedicated bedding wins an equal-distance tie.
    return sleeping;
}

inline ConstructedFacility* bestOperationalSleepFacility(
    World& world,
    GridPos pos,
    int maxDistance=1)
{
    ConstructedFacility* sleeping=
        operationalSettlementFacilityNear(
            world,FacilityKind::SleepingPlace,pos,maxDistance);
    ConstructedFacility* shelter=
        operationalSettlementFacilityNear(
            world,FacilityKind::Shelter,pos,maxDistance);

    if(sleeping==nullptr) return shelter;
    if(shelter==nullptr) return sleeping;

    const int sleepingDistance=manhattan(sleeping->pos,pos);
    const int shelterDistance=manhattan(shelter->pos,pos);
    if(sleepingDistance!=shelterDistance){
        return sleepingDistance<shelterDistance ? sleeping : shelter;
    }
    return sleeping;
}

inline const ConstructedFacility* nearestOperationalSleepFacility(
    const World& world,
    GridPos pos)
{
    const ConstructedFacility* best=nullptr;
    int bestDistance=0;
    for(const auto& facility:world.facilities){
        if(!facilityProvidesSleep(facility.kind)
           || !facilityOperationalAndActive(facility)) continue;
        const int distance=manhattan(facility.pos,pos);
        if(best==nullptr || distance<bestDistance
           || (distance==bestDistance
               && facility.kind==FacilityKind::SleepingPlace
               && best->kind!=FacilityKind::SleepingPlace)
           || (distance==bestDistance && facility.kind==best->kind
               && facility.id<best->id)){
            best=&facility;
            bestDistance=distance;
        }
    }
    return best;
}

inline double settlementSleepRecoveryPerTick(
    const ConstructedFacility& facility)
{
    if(!facilityOperationalAndActive(facility)
       || !facilityProvidesSleep(facility.kind)) return 0.0;

    // Simulation::step is one simulation minute. Recovery is intentionally
    // minute-scale so fatigue falls with actual time asleep rather than because
    // a one-shot "Sleep" action completed.
    const double effectiveness=facilityEffectiveness01(facility);
    if(facility.kind==FacilityKind::SleepingPlace){
        return 0.00300+0.00020*effectiveness;
    }
    if(facility.kind==FacilityKind::Shelter){
        return 0.00255+0.00020*effectiveness;
    }
    return 0.0;
}

inline double settlementShelterProtection01(
    const World& world,
    GridPos pos)
{
    const ConstructedFacility* shelter=
        operationalSettlementFacilityNear(
            world,FacilityKind::Shelter,pos,1);
    if(shelter==nullptr) return 0.0;
    return std::clamp(0.68*facilityEffectiveness01(*shelter),0.0,0.68);
}

inline double sleepRecoveryPerMinuteAt(
    const World& world,
    GridPos pos,
    const ConstructedFacility* facility=nullptr)
{
    const EnvironmentalConsequenceProfile consequence=
        deriveEnvironmentalConsequences(
            deriveDynamicEnvironment(
                world.genesisIdentity(),
                chunkCoordForGrid(pos),
                world.minute));

    const double environmentalStress=std::clamp(
        0.40*consequence.wetStress01
        +0.30*consequence.coldStress01
        +0.20*consequence.heatStress01
        +0.10*consequence.travelFriction01,
        0.0,
        1.0);

    double baseRecovery=0.00230;
    double protection=settlementShelterProtection01(world,pos);
    if(facility!=nullptr && facilityOperationalAndActive(*facility)
       && facilityProvidesSleep(facility->kind)){
        baseRecovery=settlementSleepRecoveryPerTick(*facility);
        if(facility->kind==FacilityKind::Shelter){
            protection=std::max(
                protection,
                0.68*facilityEffectiveness01(*facility));
        }
    }

    // Sleeping in rain/cold/heat still helps, but much less. A nearby shelter
    // absorbs most of that penalty without turning it into a free full reset.
    const double exposedStress=
        environmentalStress*(1.0-std::clamp(protection,0.0,0.85));
    return std::max(
        0.00095,
        baseRecovery*(1.0-0.42*exposedStress));
}

inline double settlementWorkSurfaceSkillBonus(
    const ConstructedFacility& facility)
{
    if(!facilityOperationalAndActive(facility)
       || !facilitySupportsCrafting(facility.kind)) return 0.0;
    return 0.06+0.16*facilityEffectiveness01(facility);
}

inline bool settlementFacilityNeedsMaintenance(
    const ConstructedFacility& facility)
{
    return isSettlementFoundationFacility(facility.kind)
        && facilityOperationalAndActive(facility)
        && facilitySupportsMaintenance(facility.kind)
        && facility.durability<0.72;
}

inline bool settlementFacilityCanRestore(
    const ConstructedFacility& facility)
{
    return isSettlementFoundationFacility(facility.kind)
        && facility.state==FacilityState::Ruined
        && facilitySupportsMaintenance(facility.kind);
}

inline int settlementRepairMaterialDemand(
    const World& world,
    MaterialKind material)
{
    if(material==MaterialKind::Unknown) return 0;
    int demand=0;
    for(const auto& facility:world.facilities){
        if(settlementFacilityNeedsMaintenance(facility)){
            if(facilityRepairMaterial(facility.kind)==material) ++demand;
            continue;
        }
        if(settlementFacilityCanRestore(facility)){
            demand+=facilityRestorationMaterialRequirement(
                facility.kind,material);
        }
    }
    return demand;
}

inline FacilityRepairResult repairSettlementFacility(
    World& world,
    Character& worker,
    FacilityId facilityId)
{
    for(auto& facility:world.facilities){
        if(facility.id!=facilityId
           || !isSettlementFoundationFacility(facility.kind)) continue;
        if(settlementFacilityCanRestore(facility)){
            return restoreRuinedConstructedFacility(
                facility,
                worker.id,
                worker.civilization.inventory,
                worker.civilization.craftingSkill,
                world.minute);
        }
        return repairConstructedFacility(
            facility,
            worker.id,
            worker.civilization.inventory,
            worker.civilization.craftingSkill);
    }
    return {};
}

inline void advanceSettlementFacilityWearOneMinute(World& world)
{
    // Shelter is continuously exposed to weather. Other foundation facilities
    // wear primarily when used and are handled at the action that consumed them.
    if(world.minute%60!=0) return;

    for(auto& facility:world.facilities){
        if(facility.kind!=FacilityKind::Shelter
           || !facilityOperationalAndActive(facility)) continue;

        const EnvironmentalConsequenceProfile consequence=
            deriveEnvironmentalConsequences(
                deriveDynamicEnvironment(
                    world.genesisIdentity(),
                    chunkCoordForGrid(facility.pos),
                    world.minute));
        const double hourlyWear=
            0.00025+0.00075*consequence.outdoorWorkFriction01;
        applyFacilityWear(facility,hourlyWear);
    }
}

inline int settlementConstructionMissingMaterial(
    const World& world,
    MaterialKind material)
{
    int missing=0;
    for(const auto& facility:world.facilities){
        if(!isSettlementFoundationFacility(facility.kind)
           || facility.state==FacilityState::Operational
           || facility.state==FacilityState::Ruined) continue;
        missing+=std::max(0,facilityMissingMaterial(facility,material));
    }
    return missing;
}

inline bool settlementFacilitySiteBlocked(
    const World& world,
    GridPos pos,
    FacilityKind plannedKind)
{
    for(const auto& facility:world.facilities){
        // A ruin is still physical matter occupying the site. Restoration uses
        // the existing facility directly and never needs the site planner, so
        // excluding ruins here only allows a different new structure to be
        // built through the surviving frame.
        if(facilityFootprintsConflict(
               plannedKind,pos,facility.kind,facility.pos)) return true;
    }
    const int footprintRadius=facilityFootprintRadiusGrid(plannedKind);
    for(const auto& sanitation:world.primitiveSanitationSites){
        if(sanitation.active
           && manhattan(sanitation.pos,pos)<=4+footprintRadius) return true;
    }
    for(const auto& node:world.resourceNodes){
        if(node.quantity>0
           && manhattan(node.pos,pos)<=1+footprintRadius) return true;
    }
    return false;
}

struct SettlementFacilitySiteOpportunity {
    bool available=false;
    GridPos pos{};
};

inline double settlementFunctionalAffinity(
    FacilityKind planned,
    FacilityKind existing)
{
    if(planned==FacilityKind::SleepingPlace){
        if(existing==FacilityKind::Shelter) return 1.00;
        if(existing==FacilityKind::FirePit) return 0.42;
        if(existing==FacilityKind::PrimitiveStorage) return 0.30;
        return 0.16;
    }
    if(planned==FacilityKind::Shelter){
        if(existing==FacilityKind::SleepingPlace) return 1.00;
        if(existing==FacilityKind::PrimitiveStorage) return 0.44;
        if(existing==FacilityKind::FirePit) return 0.34;
        return 0.18;
    }
    if(planned==FacilityKind::WorkSurface){
        if(existing==FacilityKind::PrimitiveStorage) return 1.00;
        if(existing==FacilityKind::FirePit) return 0.72;
        if(existing==FacilityKind::Furnace) return 0.66;
        if(existing==FacilityKind::Shelter) return 0.26;
        return 0.14;
    }
    return 0.0;
}

inline double settlementFacilityLivedUseSignal(
    const World& world,
    const ConstructedFacility& facility)
{
    if(!facilityOperationalAndActive(facility)) return 0.0;

    const double repeatedUse=std::clamp(
        static_cast<double>(facility.usageCount)/8.0,
        0.0,
        1.0);
    const int lastActivityMinute=std::max({
        facility.lastUsedMinute,
        facility.lastFireMinute,
        facility.lastCultivationMinute
    });
    double recency=0.0;
    if(lastActivityMinute>=0){
        constexpr int ActivityMemoryMinutes=14*24*60;
        const int age=std::max(0,world.minute-lastActivityMinute);
        recency=1.0-std::clamp(
            static_cast<double>(age)
                /static_cast<double>(ActivityMemoryMinutes),
            0.0,
            1.0);
    }

    // Repetition is the durable signal; recent fire/farm/use activity keeps a
    // place temporarily attractive without inventing a permanent town center.
    return std::clamp(
        0.65*repeatedUse+0.35*recency,
        0.0,
        1.0);
}

inline double settlementResidentActivityScore(
    const World& world,
    GridPos candidate,
    const SettlementPopulation* population)
{
    if(population==nullptr || population->empty()) return 0.0;

    constexpr int ResidentActivityRadiusGrid=18;
    double score=0.0;
    for(const auto& entry:*population){
        const Character* resident=nullptr;
        for(const auto& candidateResident:world.characters){
            if(candidateResident.id==entry.first){
                resident=&candidateResident;
                break;
            }
        }
        if(resident==nullptr || !resident->alive) continue;

        const int distance=manhattan(candidate,entry.second);
        if(distance>ResidentActivityRadiusGrid) continue;
        const double proximity=1.0-std::clamp(
            static_cast<double>(distance)
                /static_cast<double>(ResidentActivityRadiusGrid),
            0.0,
            1.0);
        score+=0.10*proximity;
    }
    return std::min(0.40,score);
}

inline double settlementActivityCenterScore(
    const World& world,
    GridPos candidate,
    FacilityKind planned)
{
    double score=0.0;
    for(const auto& facility:world.facilities){
        if(!facilityOperationalAndActive(facility)) continue;
        const int distance=manhattan(candidate,facility.pos);
        const int safeDistance=
            facilityMinimumCenterDistanceGrid(planned,facility.kind);
        if(distance>safeDistance+10) continue;
        const double proximity=
            1.0-static_cast<double>(
                std::max(0,distance-safeDistance))/10.0;
        const double livedUse=settlementFacilityLivedUseSignal(
            world,facility);
        const double activityWeight=0.70+0.60*livedUse;
        score+=settlementFunctionalAffinity(planned,facility.kind)
            *activityWeight
            *std::max(0.0,proximity);
    }

    // Compatibility/early stockpiles may exist before a linked storage
    // facility. Once linked, actual storage use strengthens the activity center.
    for(const auto& storage:world.storageSites){
        const int distance=manhattan(candidate,storage.pos);
        if(distance>12) continue;
        const double proximity=
            1.0-static_cast<double>(std::max(0,distance-3))/10.0;
        const double affinity=
            planned==FacilityKind::WorkSurface ? 0.72 : 0.24;
        double activityWeight=0.80;
        for(const auto& facility:world.facilities){
            if(facility.linkedStorage!=storage.id
               || !facilityOperationalAndActive(facility)) continue;
            activityWeight=0.70+0.60*settlementFacilityLivedUseSignal(
                world,facility);
            break;
        }
        score+=affinity*activityWeight*std::max(0.0,proximity);
    }

    // The hard block already keeps sanitation out of the immediate living core.
    // This softer ring discourages dense living/work growth directly beside it.
    for(const auto& sanitation:world.primitiveSanitationSites){
        if(!sanitation.active) continue;
        const int distance=manhattan(candidate,sanitation.pos);
        if(distance<9){
            score-=0.10*static_cast<double>(9-distance);
        }
    }
    return score;
}

inline double settlementTerrainHabitabilityScore(
    const World& world,
    GridPos candidate,
    FacilityKind planned)
{
    const WorldGenesisIdentity identity=world.genesisIdentity();
    const ContinuousTerrainSample terrain=
        deriveContinuousTerrainSample(identity,candidate);

    const double slopePerGrid=std::hypot(
        terrain.gradientXPerGrid,
        terrain.gradientYPerGrid);
    const double slopeSeverity=std::clamp(
        slopePerGrid*static_cast<double>(WorldChunkSpanGridCells)*8.0,
        0.0,
        1.0);
    const double flatness=1.0-slopeSeverity;
    const double flatnessWeight=
        planned==FacilityKind::SleepingPlace ? 0.90
        : planned==FacilityKind::Shelter ? 0.82
        : 0.62;

    double score=flatnessWeight*flatness;

    // Extreme high/low ground is possible, but early settlement prefers a
    // usable middle band unless another strong survival signal compensates.
    const double elevationComfort=1.0-std::clamp(
        std::abs(terrain.elevation01-0.52)*1.8,
        0.0,
        1.0);
    score+=0.16*elevationComfort;

    // Vegetation offers building material, shade and food opportunity, while
    // saturated ground is a poor default foundation. Work areas tolerate more
    // exposed rock than sleeping/shelter sites.
    const ContinuousEcologySample ecology=
        deriveContinuousEcologySample(identity,candidate);
    const double vegetation=
        0.55*ecology.forestCoverage01
        +0.25*ecology.grassCoverage01
        +0.20*ecology.shrubCoverage01;
    score+=0.14*vegetation;
    score-=0.22*ecology.wetlandCoverage01;
    if(planned==FacilityKind::WorkSurface){
        score+=0.08*ecology.rockCoverage01;
    }

    const ChunkCoord localChunk=chunkCoordForGrid(candidate);
    const HydrologyFacts localWater=
        deriveHydrologyFacts(identity,localChunk);

    // Never place a settlement foundation in the authoritative ground-water
    // footprint itself. Wetland/coastal ground is allowed only with a strong
    // penalty so later societies can still deliberately inhabit it.
    if(localWater.surfaceKind==SurfaceWaterKind::Ocean
       || surfaceWaterGroundContainsGrid(localWater,candidate)){
        return -1000.0;
    }
    if(localWater.surfaceKind==SurfaceWaterKind::Wetland) score-=0.55;
    if(localWater.surfaceKind==SurfaceWaterKind::Coast) score-=0.20;

    bool foundFreshWater=false;
    int nearestFreshDistance=1000000;
    double nearestFreshAvailability=0.0;
    for(int dy=-FreshSurfaceNeighbourRadiusChunks;
        dy<=FreshSurfaceNeighbourRadiusChunks;
        ++dy){
        for(int dx=-FreshSurfaceNeighbourRadiusChunks;
            dx<=FreshSurfaceNeighbourRadiusChunks;
            ++dx){
            const ChunkCoord coord{localChunk.x+dx,localChunk.y+dy};
            const HydrologyFacts water=deriveHydrologyFacts(identity,coord);
            if(!isFreshSurfaceWater(water)) continue;

            const GridPos access=surfaceWaterGroundAccessGrid(water);
            const int distance=manhattan(candidate,access);
            if(!foundFreshWater
               || distance<nearestFreshDistance
               || (distance==nearestFreshDistance
                   && water.surfaceAvailability>nearestFreshAvailability)){
                foundFreshWater=true;
                nearestFreshDistance=distance;
                nearestFreshAvailability=water.surfaceAvailability;
            }
        }
    }

    if(foundFreshWater){
        const double proximity=1.0-std::clamp(
            static_cast<double>(std::max(0,nearestFreshDistance-3))/80.0,
            0.0,
            1.0);
        score+=0.48*proximity+0.16*nearestFreshAvailability;
    }else{
        score-=0.35;
    }

    return score;
}

inline SettlementFacilitySiteOpportunity chooseSettlementFacilitySite(
    const World& world,
    CharacterId planner,
    FacilityKind kind,
    GridPos activityAnchor,
    const SettlementPopulation* population=nullptr)
{
    SettlementFacilitySiteOpportunity result;
    if(planner==0 || !isSettlementFoundationFacility(kind)
       || !observeSettlementFacilityDemand(
           world,planner,kind,activityAnchor,population).canPlan()) return result;

    // NEW GAME spawn is only an entry coordinate. Settlement candidates emerge
    // around where the resident is actually acting now, then existing facilities
    // and terrain suitability decide which nearby site wins.
    const GridPos center=activityAnchor;

    constexpr std::array<GridPos,32> offsets={
        GridPos{3,0},GridPos{0,3},GridPos{-3,0},GridPos{0,-3},
        GridPos{3,3},GridPos{-3,3},GridPos{-3,-3},GridPos{3,-3},
        GridPos{5,1},GridPos{1,5},GridPos{-5,1},GridPos{1,-5},
        GridPos{5,-2},GridPos{-2,5},GridPos{-5,-2},GridPos{-2,-5},
        GridPos{10,0},GridPos{0,10},GridPos{-10,0},GridPos{0,-10},
        GridPos{8,8},GridPos{-8,8},GridPos{-8,-8},GridPos{8,-8},
        GridPos{12,4},GridPos{-12,4},GridPos{-12,-4},GridPos{12,-4},
        GridPos{14,0},GridPos{0,14},GridPos{-14,0},GridPos{0,-14}
    };

    const std::uint64_t salt=
        0x534554544C454D54ULL
        ^ (static_cast<std::uint64_t>(kind)+1ULL)*0x9e3779b97f4a7c15ULL;
    const std::uint64_t mixed=civilizationMix((world.seed ? world.seed : 1)^planner^salt);
    const std::size_t start=static_cast<std::size_t>(mixed%offsets.size());

    bool found=false;
    double bestScore=-1.0e9;
    const auto considerSite=[&](GridPos candidate){
        if(settlementFacilitySiteBlocked(world,candidate,kind)) return;
        if(population && !observeSettlementFacilityDemand(
            world,planner,kind,candidate,population).canPlan()) return;

        const double habitability=settlementTerrainHabitabilityScore(world,candidate,kind);
        if(habitability<=-1000.0) return;
        const double score=
            settlementActivityCenterScore(world,candidate,kind)
            +settlementResidentActivityScore(world,candidate,population)
            +habitability;
        // Traversal order breaks score ties deterministically.
        if(!found || score>bestScore+1e-12){
            found=true;
            bestScore=score;
            result.available=true;
            result.pos=candidate;
        }
    };
    for(std::size_t i=0;i<offsets.size();++i){
        const GridPos offset=offsets[(start+i)%offsets.size()];
        considerSite({center.x+offset.x,center.y+offset.y});
    }
    // Real footprints/resources can fill the inner cluster before local demand
    // is met. Search bounded outer rings rather than overlap existing buildings
    // or treat the original 32 candidates as the entire buildable settlement.
    for(int radius=24; !found && radius<=SettlementServiceRadiusGrid-16; radius+=8){
        const int half=radius/2;
        const std::array<GridPos,8> ring={
            GridPos{radius,0},GridPos{half,half},GridPos{0,radius},GridPos{-half,half},
            GridPos{-radius,0},GridPos{-half,-half},GridPos{0,-radius},GridPos{half,-half}
        };
        for(std::size_t i=0;i<ring.size();++i){
            const GridPos offset=ring[(start+i)%ring.size()];
            considerSite({center.x+offset.x,center.y+offset.y});
        }
    }
    return result;
}

inline ConstructedFacility* establishSettlementFacilityProject(
    World& world,
    CharacterId planner,
    FacilityKind kind,
    GridPos pos,
    const SettlementPopulation* population=nullptr)
{
    if(planner==0 || !isSettlementFoundationFacility(kind)
       || !facilityKindConstructible(kind)
       || !observeSettlementFacilityDemand(
           world,planner,kind,pos,population).canPlan()
       || settlementFacilitySiteBlocked(world,pos,kind)) return nullptr;

    ConstructedFacility facility=makeFacilityConstructionSite(
        nextFacilityId(world.facilities),kind,pos,planner,world.minute);
    if(facility.id==0) return nullptr;
    world.facilities.push_back(std::move(facility));
    return &world.facilities.back();
}

struct SettlementFacilityWorkResult {
    bool worked=false;
    bool completed=false;
    FacilityId facilityId=0;
    FacilityKind kind=FacilityKind::WorkSurface;
    GridPos pos{};
    double workBefore=0.0;
    double workAfter=0.0;
};

inline SettlementFacilityWorkResult workOnSettlementFacility(
    World& world,
    Character& worker,
    FacilityId facilityId,
    double workAmount)
{
    SettlementFacilityWorkResult result;
    ConstructedFacility* project=nullptr;
    for(auto& facility:world.facilities){
        if(facility.id==facilityId){ project=&facility; break; }
    }
    if(project==nullptr || !isSettlementFoundationFacility(project->kind)
       || project->state==FacilityState::Operational
       || !facilityMaterialsComplete(*project) || workAmount<=0.0) return result;

    result.facilityId=project->id;
    result.kind=project->kind;
    result.pos=project->pos;
    result.workBefore=project->constructionWork;

    const EnvironmentalConsequenceProfile consequence=deriveEnvironmentalConsequences(
        deriveDynamicEnvironment(
            world.genesisIdentity(),chunkCoordForGrid(project->pos),world.minute));
    const double effectiveWork=std::max(
        0.05,workAmount*(1.0-0.55*consequence.outdoorWorkFriction01));

    const bool reachedCompletion=applyFacilityConstructionWork(
        *project,worker.id,effectiveWork);
    result.workAfter=project->constructionWork;
    result.worked=result.workAfter>result.workBefore;
    if(!result.worked) return result;

    if(reachedCompletion && activateConstructedFacility(*project,0,world.minute)){
        result.completed=true;
    }
    return result;
}

} // namespace lifelens
