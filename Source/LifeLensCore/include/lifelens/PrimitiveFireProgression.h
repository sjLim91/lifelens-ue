#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>

#include "Facility.h"
#include "PrimitiveSanitation.h"
#include "PrimitiveSmeltingProgression.h"
#include "World.h"

namespace lifelens {

struct PrimitiveFirePitSiteOpportunity {
    bool available=false;
    GridPos pos{};
};

inline bool hasOperationalFirePit(const World& world)
{
    for(const auto& facility:world.facilities){
        if(facility.kind==FacilityKind::FirePit
           && facility.state==FacilityState::Operational
           && facility.active) return true;
    }
    return false;
}

inline const ConstructedFacility* primitiveFirePitProject(const World& world)
{
    for(const auto& facility:world.facilities){
        if(facility.kind==FacilityKind::FirePit
           && facility.state!=FacilityState::Ruined) return &facility;
    }
    return nullptr;
}

inline ConstructedFacility* primitiveFirePitProject(World& world)
{
    for(auto& facility:world.facilities){
        if(facility.kind==FacilityKind::FirePit
           && facility.state!=FacilityState::Ruined) return &facility;
    }
    return nullptr;
}

inline bool primitiveFirePitSiteBlocked(const World& world,GridPos pos)
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

inline bool primitiveFirePitPlanningDeferredBySanitation(const World& world)
{
    for(const auto& sanitation:world.primitiveSanitationSites){
        if(sanitation.active
           && sanitation.kind==PrimitiveSanitationSiteKind::DesignatedArea
           && sanitation.useCount>=2) return true;
    }
    return false;
}

inline PrimitiveFirePitSiteOpportunity choosePrimitiveFirePitSite(
    const World& world,
    CharacterId planner)
{
    PrimitiveFirePitSiteOpportunity result;
    if(planner==0 || primitiveFirePitProject(world)!=nullptr
       || primitiveFirePitPlanningDeferredBySanitation(world)) return result;

    const GridPos center=world.hasInitialStartRegionSelection
        ? world.initialStartRegionCenterGrid()
        : GridPos{};
    constexpr std::array<GridPos,12> offsets={
        GridPos{3,3},GridPos{-3,3},GridPos{-3,-3},GridPos{3,-3},
        GridPos{5,2},GridPos{-5,2},GridPos{-5,-2},GridPos{5,-2},
        GridPos{2,5},GridPos{-2,5},GridPos{-2,-5},GridPos{2,-5}
    };
    const std::uint64_t mixed=civilizationMix(
        (world.seed ? world.seed : 1)^planner^0x46495245504954ULL);
    const std::size_t start=static_cast<std::size_t>(mixed%offsets.size());
    for(std::size_t i=0;i<offsets.size();++i){
        const GridPos offset=offsets[(start+i)%offsets.size()];
        const GridPos candidate{center.x+offset.x,center.y+offset.y};
        if(primitiveFirePitSiteBlocked(world,candidate)) continue;
        result.available=true;
        result.pos=candidate;
        return result;
    }
    return result;
}

inline int primitiveFirePitMissingMaterial(const World& world,MaterialKind material)
{
    const ConstructedFacility* project=primitiveFirePitProject(world);
    if(project==nullptr || project->state==FacilityState::Operational) return 0;
    return facilityMissingMaterial(*project,material);
}

inline ConstructedFacility* establishPrimitiveFirePitProject(
    World& world,
    const Character& planner,
    GridPos pos)
{
    if(planner.id==0
       || !planner.civilization.knowledge.knowsAtLeast(
           TechniqueId::FireMaking,KnowledgeLevel::Reproducible)
       || primitiveFirePitProject(world)!=nullptr
       || primitiveFirePitSiteBlocked(world,pos)) return nullptr;

    ConstructedFacility facility=makeFacilityConstructionSite(
        nextFacilityId(world.facilities),FacilityKind::FirePit,
        pos,planner.id,world.minute);
    if(facility.id==0) return nullptr;
    world.facilities.push_back(std::move(facility));
    return &world.facilities.back();
}

struct PrimitiveFirePitWorkResult {
    bool worked=false;
    bool completed=false;
    FacilityId facilityId=0;
    GridPos pos{};
    double workBefore=0.0;
    double workAfter=0.0;
};

inline PrimitiveFirePitWorkResult workOnPrimitiveFirePit(
    World& world,
    Character& worker,
    double workAmount)
{
    PrimitiveFirePitWorkResult result;
    ConstructedFacility* project=primitiveFirePitProject(world);
    if(project==nullptr || project->state==FacilityState::Operational
       || !worker.civilization.knowledge.knowsAtLeast(
           TechniqueId::FireMaking,KnowledgeLevel::Reproducible)
       || !facilityMaterialsComplete(*project) || workAmount<=0.0) return result;

    result.facilityId=project->id;
    result.pos=project->pos;
    result.workBefore=project->constructionWork;
    const bool reachedCompletion=applyFacilityConstructionWork(
        *project,worker.id,workAmount);
    result.workAfter=project->constructionWork;
    result.worked=result.workAfter>result.workBefore;
    if(!result.worked) return result;

    if(reachedCompletion && activateConstructedFacility(*project,0,world.minute)){
        result.completed=true;
    }
    return result;
}

inline int fuelPrimitiveFirePit(
    World& world,
    Character& worker,
    FacilityId facilityId,
    int requested)
{
    if(!worker.civilization.knowledge.knowsAtLeast(
        TechniqueId::FireMaking,KnowledgeLevel::Reproducible)) return 0;
    for(auto& facility:world.facilities){
        if(facility.id!=facilityId || facility.kind!=FacilityKind::FirePit) continue;
        return addFirePitWoodFuel(
            facility,worker.civilization.inventory,requested);
    }
    return 0;
}

inline bool ignitePrimitiveFirePit(
    World& world,
    const Character& worker,
    FacilityId facilityId)
{
    if(!worker.civilization.knowledge.knowsAtLeast(
        TechniqueId::FireMaking,KnowledgeLevel::Reproducible)) return false;
    for(auto& facility:world.facilities){
        if(facility.id!=facilityId || facility.kind!=FacilityKind::FirePit) continue;
        return igniteFirePit(facility,world.minute);
    }
    return false;
}

inline int collectPrimitiveFirePitCharcoal(
    World& world,
    Character& worker,
    FacilityId facilityId,
    int requested)
{
    for(auto& facility:world.facilities){
        if(facility.id!=facilityId || facility.kind!=FacilityKind::FirePit) continue;
        return collectFirePitCharcoal(
            facility,worker.civilization.inventory,requested);
    }
    return 0;
}

inline void advancePrimitiveFireOneMinute(World& world)
{
    for(auto& facility:world.facilities){
        if(facility.kind==FacilityKind::FirePit){
            advanceFirePitOneMinute(facility,world.minute);
        }else if(facility.kind==FacilityKind::Furnace){
            advanceFurnaceOneMinute(facility,world.minute);
        }
    }
}

} // namespace lifelens
