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

inline const NaturalResourcePatch* findGeneratedNaturalResourcePatch(
    const World& world,
    const ResourceNode& node)
{
    if(node.id==0) return nullptr;

    // Current generated worlds persist ResourceNode::pos from the owning
    // NaturalResourcePatch. Use that coordinate to jump directly to the one
    // possible owner chunk instead of rescanning every materialized chunk for
    // every resource candidate. If an old save has no serialized node position,
    // or a compatibility fixture deliberately diverges, retain the full scan
    // fallback so world-generation authority still wins.
    if(const GeneratedNaturalChunk* owner=
        world.findGeneratedNaturalChunk(chunkCoordForGrid(node.pos))){
        for(const auto& patch:owner->resourcePatches){
            if(patch.nodeId==node.id
               && patch.pos.x==node.pos.x
               && patch.pos.y==node.pos.y){
                return &patch;
            }
        }
    }

    for(const auto& chunk:world.generatedNaturalChunks){
        for(const auto& patch:chunk.resourcePatches){
            if(patch.nodeId==node.id) return &patch;
        }
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
    // authority. Current worlds hit the owner-chunk fast path; legacy v1 saves
    // that did not serialize ResourceNode::pos still use the compatibility
    // fallback inside findGeneratedNaturalResourcePatch().
    if(const NaturalResourcePatch* patch=
        findGeneratedNaturalResourcePatch(world,*node)){
        outPosition=patch->pos;
        return true;
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

    // Generation v1 predates authoritative hydrology/access separation.
    if(world.generationVersion < 2){
        outPosition=exact;
        return true;
    }

    const ChunkCoord coord=chunkCoordForGrid(exact);
    const HydrologyFacts facts=deriveHydrologyFacts(world.genesisIdentity(),coord);

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

    constexpr std::array<GridPos,8> directions={
        GridPos{1,0},GridPos{1,1},GridPos{0,1},GridPos{-1,1},
        GridPos{-1,0},GridPos{-1,-1},GridPos{0,-1},GridPos{1,-1}
    };

    // Non-water resources normally use their generated patch center. If an
    // older/current generated patch happens to overlap water or another
    // physical blocker, keep the immutable ResourceNode where it is but expose
    // a deterministic nearby dry interaction point. This preserves seed/save
    // identity while preventing residents from gathering from unreachable
    // ground.
    if(node->material != MaterialKind::Water){
        if(validAccess(exact)){
            outPosition=exact;
            return true;
        }

        const std::size_t startDirection=static_cast<std::size_t>(
            node->id % directions.size());
        for(int distance=1;distance<=10;++distance){
            for(std::size_t ordinal=0;ordinal<directions.size();++ordinal){
                const GridPos direction=
                    directions[(startDirection+ordinal)%directions.size()];
                const GridPos candidate{
                    exact.x+direction.x*distance,
                    exact.y+direction.y*distance
                };
                if(validAccess(candidate)){
                    outPosition=candidate;
                    return true;
                }
            }
        }
        return false;
    }

    // Generated v2+ Water stays logically at the hydrology center. Residents
    // interact from a deterministic dry bank cell that also avoids natural
    // obstacles and constructed facilities.
    if(!isFreshSurfaceWater(facts)){
        if(validAccess(exact)){
            outPosition=exact;
            return true;
        }
        return false;
    }

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

    const std::size_t startDirection=static_cast<std::size_t>(
        facts.surfaceWaterId % directions.size());

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
