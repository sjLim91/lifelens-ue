#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>

#include "Facility.h"
#include "PrimitiveSanitation.h"
#include "World.h"

namespace lifelens {

// Primitive storage is intentionally kept as a facility progression contract
// instead of a free starting object. A resident must first experience carrying
// pressure, discover the idea, plan a Core-authored site, deliver real material,
// and then perform construction work.
constexpr int PrimitiveStorageRecognitionInventoryUnits = 6;
constexpr double PrimitiveStorageRecognitionBaseChance = 0.34;

struct PrimitiveStorageNeedObservation {
    bool recognized=false;
    int carriedUnits=0;
    int threshold=PrimitiveStorageRecognitionInventoryUnits;
    double pressure=0.0;
};

struct PrimitiveStorageSiteOpportunity {
    bool available=false;
    GridPos pos{};
};

inline bool hasOperationalPrimitiveStorage(const World& world)
{
    if(!world.storageSites.empty()) return true;
    for(const auto& facility:world.facilities){
        if(facility.kind==FacilityKind::PrimitiveStorage
           && facility.state==FacilityState::Operational
           && facility.active) return true;
    }
    return false;
}

inline const ConstructedFacility* primitiveStorageProject(const World& world)
{
    for(const auto& facility:world.facilities){
        if(facility.kind==FacilityKind::PrimitiveStorage
           && facility.state!=FacilityState::Ruined) return &facility;
    }
    return nullptr;
}

inline ConstructedFacility* primitiveStorageProject(World& world)
{
    for(auto& facility:world.facilities){
        if(facility.kind==FacilityKind::PrimitiveStorage
           && facility.state!=FacilityState::Ruined) return &facility;
    }
    return nullptr;
}

inline PrimitiveStorageNeedObservation observePrimitiveStorageNeed(
    const World& world,
    const Character& resident)
{
    PrimitiveStorageNeedObservation result;
    for(const auto& stack:resident.civilization.inventory.stacks()){
        result.carriedUnits+=std::max(0,stack.quantity);
    }
    result.pressure=std::max(0.0,std::min(1.0,
        static_cast<double>(result.carriedUnits-PrimitiveStorageRecognitionInventoryUnits+1)/6.0));
    result.recognized=!hasOperationalPrimitiveStorage(world)
        && primitiveStorageProject(world)==nullptr
        && result.carriedUnits>=PrimitiveStorageRecognitionInventoryUnits;
    return result;
}

inline bool primitiveStorageSiteBlocked(const World& world,GridPos pos)
{
    for(const auto& facility:world.facilities){
        if(facility.state!=FacilityState::Ruined && manhattan(facility.pos,pos)<=2) return true;
    }
    for(const auto& sanitation:world.primitiveSanitationSites){
        if(sanitation.active && manhattan(sanitation.pos,pos)<=3) return true;
    }
    // Do not place a construction footprint directly on an authoritative
    // resource node. A one-cell interaction buffer keeps future presentation
    // meshes from visually swallowing the resource target.
    for(const auto& node:world.resourceNodes){
        if(node.quantity>0 && manhattan(node.pos,pos)<=1) return true;
    }
    return false;
}

inline PrimitiveStorageSiteOpportunity choosePrimitiveStorageSite(
    const World& world,
    CharacterId planner,
    GridPos activityAnchor)
{
    PrimitiveStorageSiteOpportunity result;
    if(planner==0 || hasOperationalPrimitiveStorage(world)
       || primitiveStorageProject(world)!=nullptr) return result;

    const GridPos center=activityAnchor;
    constexpr std::array<GridPos,12> offsets={
        GridPos{4,0},GridPos{0,4},GridPos{-4,0},GridPos{0,-4},
        GridPos{4,3},GridPos{-4,3},GridPos{-4,-3},GridPos{4,-3},
        GridPos{6,0},GridPos{0,6},GridPos{-6,0},GridPos{0,-6}
    };

    // Stable rotation prevents every possible planner from preferring the same
    // compass direction while keeping the same world+resident deterministic.
    const std::uint64_t mixed=civilizationMix((world.seed ? world.seed : 1)^planner);
    const std::size_t start=static_cast<std::size_t>(mixed%offsets.size());
    for(std::size_t i=0;i<offsets.size();++i){
        const GridPos offset=offsets[(start+i)%offsets.size()];
        const GridPos candidate{center.x+offset.x,center.y+offset.y};
        if(primitiveStorageSiteBlocked(world,candidate)) continue;
        result.available=true;
        result.pos=candidate;
        return result;
    }
    return result;
}

inline int primitiveStorageMissingMaterial(const World& world,MaterialKind material)
{
    const ConstructedFacility* project=primitiveStorageProject(world);
    if(project==nullptr || project->state==FacilityState::Operational) return 0;
    return facilityMissingMaterial(*project,material);
}

inline StorageId nextStorageSiteId(const World& world)
{
    StorageId next=1;
    for(const auto& storage:world.storageSites){
        if(storage.id>=next) next=storage.id+1;
    }
    return next==0 ? 1 : next;
}

inline ConstructedFacility* establishPrimitiveStorageProject(
    World& world,
    CharacterId planner,
    GridPos pos)
{
    if(planner==0 || hasOperationalPrimitiveStorage(world)
       || primitiveStorageProject(world)!=nullptr
       || primitiveStorageSiteBlocked(world,pos)) return nullptr;

    ConstructedFacility facility=makeFacilityConstructionSite(
        nextFacilityId(world.facilities),FacilityKind::PrimitiveStorage,
        pos,planner,world.minute);
    if(facility.id==0) return nullptr;
    world.facilities.push_back(std::move(facility));
    return &world.facilities.back();
}

struct PrimitiveStorageWorkResult {
    bool worked=false;
    bool completed=false;
    FacilityId facilityId=0;
    StorageId storageId=0;
    GridPos pos{};
    double workBefore=0.0;
    double workAfter=0.0;
};

inline PrimitiveStorageWorkResult workOnPrimitiveStorage(
    World& world,
    Character& worker,
    double workAmount)
{
    PrimitiveStorageWorkResult result;
    ConstructedFacility* project=primitiveStorageProject(world);
    if(project==nullptr || project->state==FacilityState::Operational
       || !facilityMaterialsComplete(*project) || workAmount<=0.0) return result;

    result.facilityId=project->id;
    result.pos=project->pos;
    result.workBefore=project->constructionWork;
    const bool reachedCompletion=applyFacilityConstructionWork(
        *project,worker.id,workAmount);
    result.workAfter=project->constructionWork;
    result.worked=result.workAfter>result.workBefore;
    if(!result.worked) return result;

    if(reachedCompletion){
        const StorageId storageId=nextStorageSiteId(world);
        StorageSite storage;
        storage.id=storageId;
        storage.pos=project->pos;
        world.storageSites.push_back(std::move(storage));
        if(!activateConstructedFacility(*project,storageId,world.minute)){
            world.storageSites.pop_back();
            return result;
        }
        result.completed=true;
        result.storageId=storageId;
    }
    return result;
}

} // namespace lifelens
