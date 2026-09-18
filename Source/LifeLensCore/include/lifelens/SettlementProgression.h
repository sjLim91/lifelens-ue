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
