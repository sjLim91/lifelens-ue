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

inline bool civilizationOutdoorInteractionGridValid(
    const World& world,
    GridPos candidate,
    ChunkCoord requiredChunk,
    ResourceNodeId allowedResourceNode=0,
    SanitationSiteId allowedSanitationSite=0,
    double maximumResidueExposure=-1.0)
{
    if(chunkCoordForGrid(candidate)!=requiredChunk) return false;

    const WorldGenesisIdentity identity=world.genesisIdentity();
    const MacroSurfaceFacts surface=deriveMacroSurfaceFacts(identity,requiredChunk);
    if(surface.surfaceClass==MacroSurfaceClass::Ocean) return false;

    const HydrologyFacts hydrology=deriveHydrologyFacts(identity,requiredChunk);
    if(surfaceWaterGroundContainsGrid(hydrology,candidate)) return false;

    if(const GeneratedNaturalChunk* chunk=world.findGeneratedNaturalChunk(requiredChunk)){
        for(const NaturalPhysicalObstacle& obstacle:deriveNaturalPhysicalObstacles(*chunk)){
            if(obstacle.grid.x==candidate.x && obstacle.grid.y==candidate.y){
                return false;
            }
        }
    }

    for(const ConstructedFacility& facility:world.facilities){
        const int radius=std::max(0,facilityFootprintRadiusGrid(facility.kind));
        if(std::max(
                std::abs(facility.pos.x-candidate.x),
                std::abs(facility.pos.y-candidate.y))<=radius){
            return false;
        }
    }

    for(const StorageSite& storage:world.storageSites){
        if(storage.pos.x==candidate.x && storage.pos.y==candidate.y){
            return false;
        }
    }

    for(const SmartObject& object:world.objects){
        if(object.pos.x==candidate.x && object.pos.y==candidate.y){
            return false;
        }
    }

    for(const PrimitiveSanitationSite& site:world.primitiveSanitationSites){
        if(!site.active || site.id==allowedSanitationSite) continue;
        if(site.pos.x==candidate.x && site.pos.y==candidate.y){
            return false;
        }
    }

    for(const ResourceNode& node:world.resourceNodes){
        if(node.quantity<=0 || node.id==allowedResourceNode) continue;
        if(node.pos.x==candidate.x && node.pos.y==candidate.y){
            return false;
        }

        GridPos resourceAccess{};
        if(resolveCivilizationResourceAccessGridPosition(
                world,node.id,resourceAccess)
           && resourceAccess.x==candidate.x
           && resourceAccess.y==candidate.y){
            return false;
        }
    }

    if(maximumResidueExposure>=0.0
       && world.environmentalResidues.exposureAt(candidate)>=maximumResidueExposure){
        return false;
    }

    return true;
}

inline bool resolveCivilizationOutdoorInteractionGridPosition(
    const World& world,
    GridPos preferred,
    GridPos& outPosition,
    int maxDistance=10,
    ResourceNodeId allowedResourceNode=0,
    SanitationSiteId allowedSanitationSite=0,
    double maximumResidueExposure=-1.0)
{
    const ChunkCoord requiredChunk=chunkCoordForGrid(preferred);
    const auto valid=[&](GridPos candidate){
        return civilizationOutdoorInteractionGridValid(
            world,
            candidate,
            requiredChunk,
            allowedResourceNode,
            allowedSanitationSite,
            maximumResidueExposure);
    };

    if(valid(preferred)){
        outPosition=preferred;
        return true;
    }

    const int limit=std::max(1,maxDistance);
    for(int distance=1;distance<=limit;++distance){
        for(int dx=-distance;dx<=distance;++dx){
            const GridPos top{preferred.x+dx,preferred.y-distance};
            if(valid(top)){ outPosition=top; return true; }
            const GridPos bottom{preferred.x+dx,preferred.y+distance};
            if(valid(bottom)){ outPosition=bottom; return true; }
        }
        for(int dy=-distance+1;dy<=distance-1;++dy){
            const GridPos left{preferred.x-distance,preferred.y+dy};
            if(valid(left)){ outPosition=left; return true; }
            const GridPos right{preferred.x+distance,preferred.y+dy};
            if(valid(right)){ outPosition=right; return true; }
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
