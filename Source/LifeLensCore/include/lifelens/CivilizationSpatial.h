#pragma once

#include "World.h"
#include "Hydrology.h"

namespace lifelens {

inline const ResourceNode* findCivilizationResourceNodeSpatial(
    const World& world,
    ResourceNodeId id)
{
    if(id==0) return nullptr;
    for(const auto& node:world.resourceNodes){
        if(node.id==id) return &node;
    }
    return nullptr;
}

inline const StorageSite* findCivilizationStorageSpatial(
    const World& world,
    StorageId id)
{
    if(id==0) return nullptr;
    for(const auto& storage:world.storageSites){
        if(storage.id==id) return &storage;
    }
    return nullptr;
}

inline bool resolveCivilizationResourceGridPosition(
    const World& world,
    ResourceNodeId id,
    GridPos& outPosition)
{
    const ResourceNode* node=findCivilizationResourceNodeSpatial(world,id);
    if(node==nullptr) return false;

    // Generated natural-patch position is the immutable world-generation
    // authority. Prefer it when present so legacy v1 saves (which did not
    // serialize ResourceNode::pos) still resolve to the exact generated site.
    for(const auto& chunk:world.generatedNaturalChunks){
        for(const auto& patch:chunk.resourcePatches){
            if(patch.nodeId==id){
                outPosition=patch.pos;
                return true;
            }
        }
    }

    outPosition=node->pos;
    return true;
}

inline bool resolveCivilizationResourceAccessGridPosition(
    const World& world,
    ResourceNodeId id,
    GridPos& outPosition)
{
    const ResourceNode* node=findCivilizationResourceNodeSpatial(world,id);
    if(node==nullptr) return false;

    GridPos exact{};
    if(!resolveCivilizationResourceGridPosition(world,id,exact)) return false;

    // Generation v1 resource locations predate authoritative hydrology.
    // Preserve their exact spatial behavior for save compatibility.
    if(world.generationVersion < 2 || node->material != MaterialKind::Water){
        outPosition=exact;
        return true;
    }

    const ChunkCoord coord=chunkCoordForGrid(exact);
    const HydrologyFacts facts=deriveHydrologyFacts(world.genesisIdentity(),coord);
    if(!isFreshSurfaceWater(facts)){
        outPosition=exact;
        return true;
    }

    const GridPos access=surfaceWaterGroundAccessGrid(facts);
    if(surfaceWaterGroundContainsGrid(facts,access)){
        return false;
    }

    outPosition=access;
    return true;
}

inline bool resolveCivilizationStorageGridPosition(
    const World& world,
    StorageId id,
    GridPos& outPosition)
{
    const StorageSite* storage=findCivilizationStorageSpatial(world,id);
    if(storage==nullptr) return false;
    outPosition=storage->pos;
    return true;
}

} // namespace lifelens
