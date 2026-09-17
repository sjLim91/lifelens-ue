#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>

#include "EnvironmentalConsequences.h"
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
    const DynamicEnvironmentObservation environment=deriveDynamicEnvironment(
        world.genesisIdentity(),chunkCoordForGrid(project->pos),world.minute);
    const EnvironmentalConsequenceProfile consequence=deriveEnvironmentalConsequences(environment);
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

inline EnvironmentalConsequenceProfile environmentalConsequencesAt(
    const World& world,
    GridPos pos)
{
    return deriveEnvironmentalConsequences(deriveDynamicEnvironment(
        world.genesisIdentity(),chunkCoordForGrid(pos),world.minute));
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
        const DynamicEnvironmentObservation environment=deriveDynamicEnvironment(
            world.genesisIdentity(),chunkCoordForGrid(facility.pos),world.minute);
        const EnvironmentalConsequenceProfile consequence=deriveEnvironmentalConsequences(environment);
        // Primitive exposed fire can fail outright in genuinely severe wet/windy
        // weather. Requiring active precipitation keeps legacy mild-weather fire
        // behavior stable while making storms physically consequential.
        if(environment.precipitationIntensity01>=0.65
           && consequence.fireReliability01<0.22) return false;
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

inline int generatedBaselineRegenerationPerDay(
    const World& world,
    ResourceNodeId nodeId)
{
    for(const auto& chunk:world.generatedNaturalChunks){
        for(const auto& patch:chunk.resourcePatches){
            if(patch.nodeId==nodeId) return patch.regenerationPerDay;
        }
    }
    return -1;
}

inline void prepareEnvironmentalResourceRegeneration(World& world)
{
    if(world.minute%(24*60)!=0) return;
    for(auto& node:world.resourceNodes){
        if(!node.renewable) continue;
        const int baseline=generatedBaselineRegenerationPerDay(world,node.id);
        // Compatibility/demo nodes do not have an immutable natural-patch
        // baseline. Leave those legacy fixtures unchanged to prevent multiplier
        // compounding across days and snapshots.
        if(baseline<0) continue;
        const EnvironmentalConsequenceProfile consequence=environmentalConsequencesAt(world,node.pos);
        node.regenerationPerDay=environmentalRegenerationUnits(
            node.material,baseline,consequence);
    }
}

inline void applyStartRegionEnvironmentalNeedPressure(World& world)
{
    const ChunkCoord coord=world.hasInitialStartRegionSelection
        ? world.initialStartRegionCoord
        : world.initialStartRegion().region.coord;
    const EnvironmentalConsequenceProfile consequence=deriveEnvironmentalConsequences(
        deriveDynamicEnvironment(world.genesisIdentity(),coord,world.minute));
    for(auto& character:world.characters){
        if(character.alive) applyEnvironmentalNeedPressure(character.needs,consequence);
    }
}

inline void advancePrimitiveFireOneMinute(World& world)
{
    for(auto& facility:world.facilities){
        if(facility.kind==FacilityKind::FirePit){
            const DynamicEnvironmentObservation environment=deriveDynamicEnvironment(
                world.genesisIdentity(),chunkCoordForGrid(facility.pos),world.minute);
            const EnvironmentalConsequenceProfile consequence=deriveEnvironmentalConsequences(environment);
            if(facility.lit
               && environment.precipitationIntensity01>=0.75
               && consequence.fireReliability01<0.18){
                // Severe exposed weather can extinguish a primitive fire. Fuel
                // remains available for a later re-ignition attempt.
                facility.lit=false;
                facility.burnMinutesRemaining=0;
                facility.heatLevel=std::min(facility.heatLevel,0.12);
            }
            advanceFirePitOneMinute(facility,world.minute);
        }else if(facility.kind==FacilityKind::Furnace){
            // Furnace combustion is structurally sheltered in v1; weather still
            // affects the people operating it, but not the enclosed burn cycle.
            advanceFurnaceOneMinute(facility,world.minute);
        }
    }

    // Simulation.cpp already advances this hook once per authoritative minute.
    // E3 extends that existing cadence so environmental pressure is Core truth,
    // not a Presentation-only effect.
    applyStartRegionEnvironmentalNeedPressure(world);
    prepareEnvironmentalResourceRegeneration(world);
}

} // namespace lifelens
