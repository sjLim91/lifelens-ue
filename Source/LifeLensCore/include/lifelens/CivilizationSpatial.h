#pragma once

#include <array>
#include <cmath>
#include <vector>

#include "World.h"
#include "Hydrology.h"
#include "NaturalPhysicalObstacle.h"

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

    std::vector<NaturalPhysicalObstacle> naturalObstacles;
    if(const GeneratedNaturalChunk* chunk=world.findGeneratedNaturalChunk(coord)){
        naturalObstacles=deriveNaturalPhysicalObstacles(*chunk);
    }

    const auto isEnvironmentBlocked=[&](GridPos candidate){
        if(chunkCoordForGrid(candidate)!=coord) return true;
        for(const NaturalPhysicalObstacle& obstacle:naturalObstacles){
            if(obstacle.grid.x==candidate.x && obstacle.grid.y==candidate.y){
                return true;
            }
        }
        for(const ConstructedFacility& facility:world.facilities){
            if(facility.pos.x==candidate.x && facility.pos.y==candidate.y){
                return true;
            }
        }
        return false;
    };

    const auto validAccess=[&](GridPos candidate){
        return !surfaceWaterGroundContainsGrid(facts,candidate)
            && !isEnvironmentBlocked(candidate);
    };

    const GridPos preferred=surfaceWaterGroundAccessGrid(facts);
    if(validAccess(preferred)){
        outPosition=preferred;
        return true;
    }

    const SurfaceWaterGroundTraversalProfile profile=
        deriveSurfaceWaterGroundTraversalProfile(facts);
    const GridPos center=surfaceWaterCenterGrid(facts);
    const double footprintRadius=profile.linearChannel
        ? profile.halfWidthCells
        : profile.radiusCells;
    const int baseDistance=std::max(
        1,
        static_cast<int>(std::ceil(footprintRadius))+1);

    constexpr std::array<GridPos,8> directions={
        GridPos{1,0},GridPos{1,1},GridPos{0,1},GridPos{-1,1},
        GridPos{-1,0},GridPos{-1,-1},GridPos{0,-1},GridPos{1,-1}
    };
    const std::size_t startDirection=static_cast<std::size_t>(
        facts.surfaceWaterId % directions.size());

    // Search a few deterministic dry-bank rings. The water footprint is small
    // relative to the 32-cell materialized chunk, so this remains local and
    // cannot silently move gathering into an unmaterialized neighbour.
    for(int extra=0;extra<=4;++extra){
        const int distance=baseDistance+extra;
        for(std::size_t ordinal=0;ordinal<directions.size();++ordinal){
            const GridPos direction=
                directions[(startDirection+ordinal)%directions.size()];
            const GridPos candidate{
                center.x+direction.x*distance,
                center.y+direction.y*distance
            };
            if(validAccess(candidate)){
                outPosition=candidate;
                return true;
            }
        }
    }

    return false;
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
