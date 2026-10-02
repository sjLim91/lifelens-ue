#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

#include "Character.h"
#include "Facility.h"
#include "SettlementDemand.h"
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

inline bool hasOperationalFurnaceNear(
    const World& world,
    GridPos anchor,
    int maxDistance=SettlementServiceRadiusGrid)
{
    for(const auto& facility:world.facilities){
        if(facility.kind==FacilityKind::Furnace
           && facilityOperationalAndActive(facility)
           && manhattan(facility.pos,anchor)<=std::max(0,maxDistance)){
            return true;
        }
    }
    return false;
}

inline bool hasOperationalFirePitForSmeltingNear(
    const World& world,
    GridPos anchor,
    int maxDistance=SettlementServiceRadiusGrid)
{
    for(const auto& facility:world.facilities){
        if(facility.kind==FacilityKind::FirePit
           && facilityOperationalAndActive(facility)
           && manhattan(facility.pos,anchor)<=std::max(0,maxDistance)){
            return true;
        }
    }
    return false;
}

inline const ConstructedFacility* primitiveFurnaceProjectNear(
    const World& world,
    GridPos anchor,
    int maxDistance=SettlementServiceRadiusGrid)
{
    const ConstructedFacility* best=nullptr;
    int bestDistance=std::max(0,maxDistance)+1;
    for(const auto& facility:world.facilities){
        if(facility.kind!=FacilityKind::Furnace
           || facility.state==FacilityState::Ruined) continue;
        const int distance=manhattan(facility.pos,anchor);
        if(distance>std::max(0,maxDistance)) continue;
        if(best==nullptr || distance<bestDistance
           || (distance==bestDistance && facility.id<best->id)){
            best=&facility;
            bestDistance=distance;
        }
    }
    return best;
}

inline ConstructedFacility* primitiveFurnaceProjectNear(
    World& world,
    GridPos anchor,
    int maxDistance=SettlementServiceRadiusGrid)
{
    return const_cast<ConstructedFacility*>(
        primitiveFurnaceProjectNear(
            static_cast<const World&>(world),anchor,maxDistance));
}

inline bool primitiveFurnaceSiteBlocked(const World& world,GridPos pos)
{
    for(const auto& facility:world.facilities){
        if(facility.state!=FacilityState::Ruined
           && facilityFootprintsConflict(
               FacilityKind::Furnace,pos,facility.kind,facility.pos)) return true;
    }
    const int footprintRadius=facilityFootprintRadiusGrid(FacilityKind::Furnace);
    for(const auto& sanitation:world.primitiveSanitationSites){
        if(sanitation.active
           && manhattan(sanitation.pos,pos)<=5+footprintRadius) return true;
    }
    for(const auto& node:world.resourceNodes){
        if(node.quantity>0
           && manhattan(node.pos,pos)<=1+footprintRadius) return true;
    }
    return false;
}

inline PrimitiveFurnaceSiteOpportunity choosePrimitiveFurnaceSite(
    const World& world,
    CharacterId planner,
    GridPos activityAnchor)
{
    PrimitiveFurnaceSiteOpportunity result;
    if(planner==0 || primitiveFurnaceProjectNear(
            world,activityAnchor)!=nullptr
       || !hasOperationalFirePitForSmeltingNear(
            world,activityAnchor)) return result;

    const GridPos center=activityAnchor;
    constexpr std::array<GridPos,24> offsets={
        GridPos{6,4},GridPos{-6,4},GridPos{-6,-4},GridPos{6,-4},
        GridPos{7,2},GridPos{-7,2},GridPos{-7,-2},GridPos{7,-2},
        GridPos{4,7},GridPos{-4,7},GridPos{-4,-7},GridPos{4,-7},
        GridPos{10,6},GridPos{-10,6},GridPos{-10,-6},GridPos{10,-6},
        GridPos{13,4},GridPos{-13,4},GridPos{-13,-4},GridPos{13,-4},
        GridPos{16,0},GridPos{0,16},GridPos{-16,0},GridPos{0,-16}
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
       || !hasOperationalFirePitForSmeltingNear(world,pos)
       || primitiveFurnaceProjectNear(world,pos)!=nullptr
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
    FacilityId facilityId,
    double workAmount)
{
    PrimitiveFurnaceWorkResult result;
    ConstructedFacility* project=nullptr;
    for(auto& facility:world.facilities){
        if(facility.id==facilityId && facility.kind==FacilityKind::Furnace){
            project=&facility;
            break;
        }
    }
    if(project==nullptr || project->state==FacilityState::Operational
       || project->state==FacilityState::Ruined
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

inline PrimitiveFurnaceWorkResult workOnPrimitiveFurnace(
    World& world,
    Character& worker,
    double workAmount)
{
    ConstructedFacility* project=primitiveFurnaceProject(world);
    return project==nullptr
        ? PrimitiveFurnaceWorkResult{}
        : workOnPrimitiveFurnace(
            world,worker,project->id,workAmount);
}

inline bool smeltingOpportunityAvailable(
    const World& world,
    const Character& resident,
    TechniqueId technique)
{
    if(!hasOperationalFurnace(world)) return false;
    const Inventory& inventory=resident.civilization.inventory;
    const int charcoal=inventory.count(
        ItemKind::RawMaterial,MaterialKind::Charcoal);
    switch(technique){
        case TechniqueId::CopperSmelting:
            return charcoal>0
                && inventory.count(
                    ItemKind::RawMaterial,MaterialKind::CopperOre)>0;
        case TechniqueId::TinSmelting:
            return charcoal>0
                && inventory.count(
                    ItemKind::RawMaterial,MaterialKind::TinOre)>0;
        case TechniqueId::BronzeAlloying:
            return charcoal>0
                && inventory.count(
                    ItemKind::RawMaterial,MaterialKind::CopperMetal)>=2
                && inventory.count(
                    ItemKind::RawMaterial,MaterialKind::TinMetal)>=1;
        default:
            return false;
    }
}

inline bool copperSmeltingOpportunityAvailable(
    const World& world,
    const Character& resident)
{
    return smeltingOpportunityAvailable(
        world,resident,TechniqueId::CopperSmelting);
}

inline int loadPrimitiveFurnaceCharge(
    World& world,
    Character& worker,
    FacilityId facilityId,
    TechniqueId technique,
    int requested)
{
    if(!worker.civilization.knowledge.knowsAtLeast(
        technique,KnowledgeLevel::Reproducible)) return 0;
    for(auto& facility:world.facilities){
        if(facility.id!=facilityId || facility.kind!=FacilityKind::Furnace) continue;
        switch(technique){
            case TechniqueId::CopperSmelting:
                return loadFurnaceCopperCharge(
                    facility,worker.civilization.inventory,requested);
            case TechniqueId::TinSmelting:
                return loadFurnaceTinCharge(
                    facility,worker.civilization.inventory,requested);
            case TechniqueId::BronzeAlloying:
                return loadFurnaceBronzeAlloyCharge(
                    facility,worker.civilization.inventory,requested);
            default:
                return 0;
        }
    }
    return 0;
}

inline int loadPrimitiveFurnaceCopperCharge(
    World& world,
    Character& worker,
    FacilityId facilityId,
    int requested)
{
    return loadPrimitiveFurnaceCharge(
        world,worker,facilityId,TechniqueId::CopperSmelting,requested);
}

inline int loadPrimitiveFurnaceTinCharge(
    World& world,
    Character& worker,
    FacilityId facilityId,
    int requested)
{
    return loadPrimitiveFurnaceCharge(
        world,worker,facilityId,TechniqueId::TinSmelting,requested);
}

inline int loadPrimitiveFurnaceBronzeAlloyCharge(
    World& world,
    Character& worker,
    FacilityId facilityId,
    int requested)
{
    return loadPrimitiveFurnaceCharge(
        world,worker,facilityId,TechniqueId::BronzeAlloying,requested);
}

inline TechniqueId furnaceTechniqueForOutput(MaterialKind output)
{
    switch(output){
        case MaterialKind::CopperMetal: return TechniqueId::CopperSmelting;
        case MaterialKind::TinMetal: return TechniqueId::TinSmelting;
        case MaterialKind::Bronze: return TechniqueId::BronzeAlloying;
        default: return TechniqueId::None;
    }
}

inline bool ignitePrimitiveFurnace(
    World& world,
    const Character& worker,
    FacilityId facilityId)
{
    for(auto& facility:world.facilities){
        if(facility.id!=facilityId || facility.kind!=FacilityKind::Furnace) continue;
        const TechniqueId technique=
            furnaceTechniqueForOutput(facility.furnaceOutputMaterial);
        if(technique==TechniqueId::None
           || !worker.civilization.knowledge.knowsAtLeast(
                technique,KnowledgeLevel::Reproducible)) return false;
        return igniteFurnace(facility,world.minute);
    }
    return false;
}

inline int collectPrimitiveFurnaceMetal(
    World& world,
    Character& worker,
    FacilityId facilityId,
    int requested)
{
    for(auto& facility:world.facilities){
        if(facility.id!=facilityId || facility.kind!=FacilityKind::Furnace) continue;
        return collectFurnaceMetal(
            facility,worker.civilization.inventory,requested);
    }
    return 0;
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

inline int collectPrimitiveFurnaceTin(
    World& world,
    Character& worker,
    FacilityId facilityId,
    int requested)
{
    for(auto& facility:world.facilities){
        if(facility.id!=facilityId || facility.kind!=FacilityKind::Furnace) continue;
        return collectFurnaceTin(
            facility,worker.civilization.inventory,requested);
    }
    return 0;
}

inline int collectPrimitiveFurnaceBronze(
    World& world,
    Character& worker,
    FacilityId facilityId,
    int requested)
{
    for(auto& facility:world.facilities){
        if(facility.id!=facilityId || facility.kind!=FacilityKind::Furnace) continue;
        return collectFurnaceBronze(
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
