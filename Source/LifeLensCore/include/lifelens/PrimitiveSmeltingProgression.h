#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

#include "Character.h"
#include "Facility.h"
#include "World.h"

namespace lifelens {

struct PrimitiveFurnaceSiteOpportunity {
    bool available=false;
    GridPos pos{};
};

inline bool hasOperationalFurnace(const World& world)
{
    for(const auto& facility:world.facilities){
        if(facility.kind==FacilityKind::Furnace
           && facility.state==FacilityState::Operational
           && facility.active) return true;
    }
    return false;
}

inline bool hasOperationalFirePitForSmelting(const World& world)
{
    for(const auto& facility:world.facilities){
        if(facility.kind==FacilityKind::FirePit
           && facility.state==FacilityState::Operational
           && facility.active) return true;
    }
    return false;
}

inline const ConstructedFacility* primitiveFurnaceProject(const World& world)
{
    for(const auto& facility:world.facilities){
        if(facility.kind==FacilityKind::Furnace
           && facility.state!=FacilityState::Ruined) return &facility;
    }
    return nullptr;
}

inline ConstructedFacility* primitiveFurnaceProject(World& world)
{
    for(auto& facility:world.facilities){
        if(facility.kind==FacilityKind::Furnace
           && facility.state!=FacilityState::Ruined) return &facility;
    }
    return nullptr;
}

inline bool primitiveFurnaceSiteBlocked(const World& world,GridPos pos)
{
    for(const auto& facility:world.facilities){
        if(facility.state!=FacilityState::Ruined && manhattan(facility.pos,pos)<=2) return true;
    }
    for(const auto& sanitation:world.primitiveSanitationSites){
        if(sanitation.active && manhattan(sanitation.pos,pos)<=5) return true;
    }
    for(const auto& node:world.resourceNodes){
        if(node.quantity>0 && manhattan(node.pos,pos)<=1) return true;
    }
    return false;
}

inline PrimitiveFurnaceSiteOpportunity choosePrimitiveFurnaceSite(
    const World& world,
    CharacterId planner,
    GridPos activityAnchor)
{
    PrimitiveFurnaceSiteOpportunity result;
    if(planner==0 || primitiveFurnaceProject(world)!=nullptr
       || !hasOperationalFirePitForSmelting(world)) return result;

    const GridPos center=activityAnchor;
    constexpr std::array<GridPos,12> offsets={
        GridPos{6,4},GridPos{-6,4},GridPos{-6,-4},GridPos{6,-4},
        GridPos{7,2},GridPos{-7,2},GridPos{-7,-2},GridPos{7,-2},
        GridPos{4,7},GridPos{-4,7},GridPos{-4,-7},GridPos{4,-7}
    };
    const std::uint64_t mixed=civilizationMix(
        (world.seed ? world.seed : 1)^planner^0x4655524E414345ULL);
    const std::size_t start=static_cast<std::size_t>(mixed%offsets.size());
    for(std::size_t i=0;i<offsets.size();++i){
        const GridPos offset=offsets[(start+i)%offsets.size()];
        const GridPos candidate{center.x+offset.x,center.y+offset.y};
        if(primitiveFurnaceSiteBlocked(world,candidate)) continue;
        result.available=true;
        result.pos=candidate;
        return result;
    }
    return result;
}

inline bool primitiveFurnaceKnowledgeReady(const Character& planner)
{
    return planner.civilization.knowledge.knowsAtLeast(
               TechniqueId::FireMaking,KnowledgeLevel::Reproducible)
        && planner.civilization.knowledge.knowsAtLeast(
               TechniqueId::StoneHammer,KnowledgeLevel::Reproducible)
        && planner.civilization.knowledge.knowsAtLeast(
               TechniqueId::SimpleContainer,KnowledgeLevel::Reproducible);
}

inline int primitiveFurnaceMissingMaterial(const World& world,MaterialKind material)
{
    const ConstructedFacility* project=primitiveFurnaceProject(world);
    if(project==nullptr || project->state==FacilityState::Operational) return 0;
    return facilityMissingMaterial(*project,material);
}

inline ConstructedFacility* establishPrimitiveFurnaceProject(
    World& world,
    const Character& planner,
    GridPos pos)
{
    if(planner.id==0 || !primitiveFurnaceKnowledgeReady(planner)
       || !hasOperationalFirePitForSmelting(world)
       || primitiveFurnaceProject(world)!=nullptr
       || primitiveFurnaceSiteBlocked(world,pos)) return nullptr;

    ConstructedFacility facility=makeFacilityConstructionSite(
        nextFacilityId(world.facilities),FacilityKind::Furnace,
        pos,planner.id,world.minute);
    if(facility.id==0) return nullptr;
    world.facilities.push_back(std::move(facility));
    return &world.facilities.back();
}

struct PrimitiveFurnaceWorkResult {
    bool worked=false;
    bool completed=false;
    FacilityId facilityId=0;
    GridPos pos{};
    double workBefore=0.0;
    double workAfter=0.0;
};

inline PrimitiveFurnaceWorkResult workOnPrimitiveFurnace(
    World& world,
    Character& worker,
    double workAmount)
{
    PrimitiveFurnaceWorkResult result;
    ConstructedFacility* project=primitiveFurnaceProject(world);
    if(project==nullptr || project->state==FacilityState::Operational
       || !primitiveFurnaceKnowledgeReady(worker)
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

inline bool copperSmeltingOpportunityAvailable(
    const World& world,
    const Character& resident)
{
    return hasOperationalFurnace(world)
        && resident.civilization.inventory.count(
            ItemKind::RawMaterial,MaterialKind::CopperOre)>0
        && resident.civilization.inventory.count(
            ItemKind::RawMaterial,MaterialKind::Charcoal)>0;
}

inline int loadPrimitiveFurnaceCopperCharge(
    World& world,
    Character& worker,
    FacilityId facilityId,
    int requested)
{
    if(!worker.civilization.knowledge.knowsAtLeast(
        TechniqueId::CopperSmelting,KnowledgeLevel::Reproducible)) return 0;
    for(auto& facility:world.facilities){
        if(facility.id!=facilityId || facility.kind!=FacilityKind::Furnace) continue;
        return loadFurnaceCopperCharge(
            facility,worker.civilization.inventory,requested);
    }
    return 0;
}

inline bool ignitePrimitiveFurnace(
    World& world,
    const Character& worker,
    FacilityId facilityId)
{
    if(!worker.civilization.knowledge.knowsAtLeast(
        TechniqueId::CopperSmelting,KnowledgeLevel::Reproducible)) return false;
    for(auto& facility:world.facilities){
        if(facility.id!=facilityId || facility.kind!=FacilityKind::Furnace) continue;
        return igniteFurnace(facility,world.minute);
    }
    return false;
}

inline int collectPrimitiveFurnaceCopper(
    World& world,
    Character& worker,
    FacilityId facilityId,
    int requested)
{
    for(auto& facility:world.facilities){
        if(facility.id!=facilityId || facility.kind!=FacilityKind::Furnace) continue;
        return collectFurnaceCopper(
            facility,worker.civilization.inventory,requested);
    }
    return 0;
}

inline void advancePrimitiveFurnaceOneMinute(World& world)
{
    for(auto& facility:world.facilities){
        if(facility.kind==FacilityKind::Furnace){
            advanceFurnaceOneMinute(facility,world.minute);
        }
    }
}

} // namespace lifelens
