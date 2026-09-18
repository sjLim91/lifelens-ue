#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>

#include "EnvironmentalConsequences.h"
#include "Facility.h"
#include "PrimitiveSanitation.h"
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

    const double effectiveness=facilityEffectiveness01(facility);
    if(facility.kind==FacilityKind::SleepingPlace){
        return 0.044+0.018*effectiveness;
    }
    if(facility.kind==FacilityKind::Shelter){
        return 0.040+0.012*effectiveness;
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

inline int settlementRepairMaterialDemand(
    const World& world,
    MaterialKind material)
{
    if(material==MaterialKind::Unknown) return 0;
    int demand=0;
    for(const auto& facility:world.facilities){
        if(!settlementFacilityNeedsMaintenance(facility)) continue;
        if(facilityRepairMaterial(facility.kind)==material) ++demand;
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

inline bool settlementFacilitySiteBlocked(const World& world,GridPos pos)
{
    for(const auto& facility:world.facilities){
        if(facility.state!=FacilityState::Ruined && manhattan(facility.pos,pos)<=2) return true;
    }
    for(const auto& sanitation:world.primitiveSanitationSites){
        if(sanitation.active && manhattan(sanitation.pos,pos)<=4) return true;
    }
    for(const auto& node:world.resourceNodes){
        if(node.quantity>0 && manhattan(node.pos,pos)<=1) return true;
    }
    return false;
}

struct SettlementFacilitySiteOpportunity {
    bool available=false;
    GridPos pos{};
};

inline SettlementFacilitySiteOpportunity chooseSettlementFacilitySite(
    const World& world,
    CharacterId planner,
    FacilityKind kind)
{
    SettlementFacilitySiteOpportunity result;
    if(planner==0 || !isSettlementFoundationFacility(kind)
       || hasOperationalSettlementFacility(world,kind)
       || settlementFacilityProject(world,kind)!=nullptr) return result;

    const GridPos center=world.hasInitialStartRegionSelection
        ? world.initialStartRegionCenterGrid()
        : GridPos{};

    constexpr std::array<GridPos,16> offsets={
        GridPos{3,0},GridPos{0,3},GridPos{-3,0},GridPos{0,-3},
        GridPos{3,3},GridPos{-3,3},GridPos{-3,-3},GridPos{3,-3},
        GridPos{5,1},GridPos{1,5},GridPos{-5,1},GridPos{1,-5},
        GridPos{5,-2},GridPos{-2,5},GridPos{-5,-2},GridPos{-2,-5}
    };

    const std::uint64_t salt=
        0x534554544C454D54ULL
        ^ (static_cast<std::uint64_t>(kind)+1ULL)*0x9e3779b97f4a7c15ULL;
    const std::uint64_t mixed=civilizationMix((world.seed ? world.seed : 1)^planner^salt);
    const std::size_t start=static_cast<std::size_t>(mixed%offsets.size());
    for(std::size_t i=0;i<offsets.size();++i){
        const GridPos offset=offsets[(start+i)%offsets.size()];
        const GridPos candidate{center.x+offset.x,center.y+offset.y};
        if(settlementFacilitySiteBlocked(world,candidate)) continue;
        result.available=true;
        result.pos=candidate;
        return result;
    }
    return result;
}

inline ConstructedFacility* establishSettlementFacilityProject(
    World& world,
    CharacterId planner,
    FacilityKind kind,
    GridPos pos)
{
    if(planner==0 || !isSettlementFoundationFacility(kind)
       || !facilityKindConstructible(kind)
       || hasOperationalSettlementFacility(world,kind)
       || settlementFacilityProject(world,kind)!=nullptr
       || settlementFacilitySiteBlocked(world,pos)) return nullptr;

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
