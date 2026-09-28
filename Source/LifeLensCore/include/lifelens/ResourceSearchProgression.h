#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include "CoreNavigation.h"
#include "NaturalWorldChunk.h"
#include "World.h"

namespace lifelens {

inline constexpr int ResourceSearchMaximumFrontierDistanceChunks=8;

struct ResourceSearchOpportunity {
    bool available=false;
    MaterialKind soughtMaterial=MaterialKind::Unknown;
    ChunkCoord chunk{};
    GridPos targetPos{};
    int frontierDistanceChunks=0;
    double score=0.0;
};

inline std::uint64_t resourceSearchMix(std::uint64_t value)
{
    value^=value>>30;
    value*=0xbf58476d1ce4e5b9ULL;
    value^=value>>27;
    value*=0x94d049bb133111ebULL;
    value^=value>>31;
    return value;
}

inline double resourceSearchTieBreak(
    const World& world,
    CharacterId actor,
    MaterialKind material,
    ChunkCoord chunk)
{
    std::uint64_t value=resourceSearchMix(
        (world.seed ? world.seed : 1ULL)
        ^(static_cast<std::uint64_t>(actor)+0x9e3779b97f4a7c15ULL));
    value=resourceSearchMix(
        value
        ^(static_cast<std::uint64_t>(static_cast<int>(material))+1ULL)
            *0x632be59bd9b4e019ULL);
    value=resourceSearchMix(
        value
        ^static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunk.x))
        ^(static_cast<std::uint64_t>(
            static_cast<std::uint32_t>(chunk.y))<<32U));
    value=resourceSearchMix(
        value
        ^static_cast<std::uint64_t>(
            world.generatedNaturalChunks.size()+1ULL)
            *0x94d049bb133111ebULL);
    return static_cast<double>(value>>11)
        *(1.0/9007199254740992.0);
}

inline bool resourceSearchChunkIsFrontier(
    const World& world,
    ChunkCoord candidate)
{
    if(world.findGeneratedNaturalChunk(candidate)!=nullptr) return false;
    const MacroSurfaceFacts surface=
        deriveMacroSurfaceFacts(world.genesisIdentity(),candidate);
    if(surface.surfaceClass==MacroSurfaceClass::Ocean) return false;

    constexpr std::array<ChunkCoord,4> neighbours={{
        {1,0},{0,1},{-1,0},{0,-1}
    }};
    for(const ChunkCoord delta:neighbours){
        const ChunkCoord neighbour{
            candidate.x+delta.x,
            candidate.y+delta.y
        };
        if(world.findGeneratedNaturalChunk(neighbour)!=nullptr) return true;
    }
    return false;
}

inline bool chooseResourceSearchTargetInsideChunk(
    const World& world,
    ChunkCoord chunk,
    GridPos& outTarget)
{
    const GridPos origin=chunkOriginGrid(chunk);
    const int center=WorldChunkSpanGridCells/2;
    const int quarter=std::max(2,WorldChunkSpanGridCells/4);
    const std::array<GridPos,13> offsets={{
        {0,0},
        {quarter,0},{-quarter,0},{0,quarter},{0,-quarter},
        {quarter,quarter},{quarter,-quarter},
        {-quarter,quarter},{-quarter,-quarter},
        {quarter/2,0},{-quarter/2,0},{0,quarter/2},{0,-quarter/2}
    }};

    for(const GridPos offset:offsets){
        const GridPos candidate{
            origin.x+center+offset.x,
            origin.y+center+offset.y
        };
        if(chunkCoordForGrid(candidate)!=chunk) continue;
        if(!coreGroundTraversable(world,candidate)) continue;
        outTarget=candidate;
        return true;
    }
    return false;
}

inline ResourceSearchOpportunity chooseResourceSearchOpportunity(
    const World& world,
    CharacterId actor,
    MaterialKind soughtMaterial,
    GridPos authoritativePosition)
{
    ResourceSearchOpportunity best;
    if(actor==0 || soughtMaterial==MaterialKind::Unknown
       || world.generatedNaturalChunks.empty()) return best;

    const ChunkCoord currentChunk=
        chunkCoordForGrid(authoritativePosition);
    std::vector<ChunkCoord> candidates;
    candidates.reserve(world.generatedNaturalChunks.size()*4);

    constexpr std::array<ChunkCoord,4> directions={{
        {1,0},{0,1},{-1,0},{0,-1}
    }};

    for(const GeneratedNaturalChunk& known:world.generatedNaturalChunks){
        for(const ChunkCoord direction:directions){
            const ChunkCoord candidate{
                known.coord.x+direction.x,
                known.coord.y+direction.y
            };
            if(!resourceSearchChunkIsFrontier(world,candidate)) continue;

            const int distance=
                std::abs(candidate.x-currentChunk.x)
                +std::abs(candidate.y-currentChunk.y);
            if(distance<=0
               || distance>ResourceSearchMaximumFrontierDistanceChunks){
                continue;
            }

            bool duplicate=false;
            for(const ChunkCoord existing:candidates){
                if(existing==candidate){
                    duplicate=true;
                    break;
                }
            }
            if(!duplicate) candidates.push_back(candidate);
        }
    }

    for(const ChunkCoord candidate:candidates){
        GridPos target{};
        if(!chooseResourceSearchTargetInsideChunk(world,candidate,target))
            continue;

        // The route check uses physical terrain/hydrology authority only.
        // It does not inspect the candidate chunk's hidden resource catalogue.
        std::vector<GridPos> route;
        if(!buildCoreGroundRoute(
            world,authoritativePosition,target,1,route)) continue;

        const int distance=
            std::abs(candidate.x-currentChunk.x)
            +std::abs(candidate.y-currentChunk.y);
        const double tie=resourceSearchTieBreak(
            world,actor,soughtMaterial,candidate);
        const double score=
            static_cast<double>(distance)
            +0.25*tie
            +0.0025*static_cast<double>(route.size());

        if(!best.available || score<best.score-1e-12
           || (std::abs(score-best.score)<=1e-12
               && (candidate.x<best.chunk.x
                   || (candidate.x==best.chunk.x
                       && candidate.y<best.chunk.y)))){
            best.available=true;
            best.soughtMaterial=soughtMaterial;
            best.chunk=candidate;
            best.targetPos=target;
            best.frontierDistanceChunks=distance;
            best.score=score;
        }
    }
    return best;
}

inline bool validResourceSearchTarget(
    const World& world,
    GridPos target)
{
    const ChunkCoord chunk=chunkCoordForGrid(target);
    return resourceSearchChunkIsFrontier(world,chunk)
        && coreGroundTraversable(world,target);
}

inline bool materializeResourceSearchArrival(
    World& world,
    GridPos resolvedPosition)
{
    const ChunkCoord chunk=chunkCoordForGrid(resolvedPosition);
    if(world.findGeneratedNaturalChunk(chunk)!=nullptr) return true;
    if(!resourceSearchChunkIsFrontier(world,chunk)) return false;
    world.materializeNaturalChunk(chunk);
    return world.findGeneratedNaturalChunk(chunk)!=nullptr;
}

} // namespace lifelens
