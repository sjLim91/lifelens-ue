#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ContinuousTerrain.h"
#include "EnvironmentalConsequences.h"
#include "Hydrology.h"
#include "SimulationClimate.h"
#include "World.h"

namespace lifelens {

inline bool sameGridPos(GridPos a, GridPos b)
{
    return a.x == b.x && a.y == b.y;
}

inline bool gridWithinRadius(GridPos a, GridPos b, int radius)
{
    const int r = std::max(0, radius);
    return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y)) <= r;
}

inline std::uint64_t gridRouteKey(GridPos p)
{
    return (
        static_cast<std::uint64_t>(
            static_cast<std::uint32_t>(p.x)) << 32U)
        | static_cast<std::uint32_t>(p.y);
}

inline GridPos gridRoutePos(std::uint64_t key)
{
    return {
        static_cast<std::int32_t>(key >> 32U),
        static_cast<std::int32_t>(key & 0xffffffffULL)
    };
}

inline bool coreGroundTraversable(const World& world, GridPos position)
{
    const WorldGenesisIdentity identity = world.genesisIdentity();
    const ChunkCoord chunk = chunkCoordForGrid(position);
    const MacroSurfaceFacts surface = deriveMacroSurfaceFacts(identity, chunk);
    if(surface.surfaceClass == MacroSurfaceClass::Ocean) return false;

    const HydrologyFacts hydrology = deriveHydrologyFacts(identity, chunk);
    if(surfaceWaterGroundContainsGrid(hydrology, position)) return false;

    return true;
}

inline double coreGroundTraversalCost(
    const World& world,
    GridPos from,
    GridPos to)
{
    const WorldGenesisIdentity identity = world.genesisIdentity();
    const MacroRegionFacts region =
        deriveMacroRegionFacts(identity, chunkCoordForGrid(to));

    const double elevationFrom =
        deriveContinuousSurfaceElevation01(identity, from);
    const double elevationTo =
        deriveContinuousSurfaceElevation01(identity, to);
    const double elevationPenalty =
        std::abs(elevationTo - elevationFrom) * 42.0;
    const double terrainPenalty =
        (1.0 - clampMacro01(region.traversalEase)) * 1.75;

    return 1.0 + terrainPenalty + elevationPenalty;
}

inline int coreGroundStepIntervalMinutes(
    const World& world,
    GridPos position)
{
    const DynamicEnvironmentObservation environment =
        deriveDynamicEnvironment(
            world.genesisIdentity(),
            chunkCoordForGrid(position),
            world.minute);
    const EnvironmentalConsequenceProfile consequence =
        deriveEnvironmentalConsequences(environment);
    return std::max(
        1,
        static_cast<int>(
            std::ceil(1.0 + 1.50 * consequence.travelFriction01)));
}

inline bool tryBuildDirectCoreGroundRoute(
    const World& world,
    GridPos start,
    GridPos target,
    int arrivalRadius,
    bool horizontalFirst,
    std::vector<GridPos>& outRoute)
{
    outRoute.clear();
    const int radius=std::max(0,arrivalRadius);
    GridPos current=start;

    const auto advanceAxis=[&](
        bool horizontal,
        std::vector<GridPos>& route)->bool
    {
        while(!gridWithinRadius(current,target,radius)){
            int* coordinate=horizontal ? &current.x : &current.y;
            const int targetCoordinate=horizontal ? target.x : target.y;
            if(*coordinate==targetCoordinate) break;

            *coordinate += *coordinate<targetCoordinate ? 1 : -1;
            if(!coreGroundTraversable(world,current)){
                route.clear();
                return false;
            }
            route.push_back(current);
        }
        return true;
    };

    if(horizontalFirst){
        if(!advanceAxis(true,outRoute)) return false;
        if(!advanceAxis(false,outRoute)) return false;
    }else{
        if(!advanceAxis(false,outRoute)) return false;
        if(!advanceAxis(true,outRoute)) return false;
    }

    return gridWithinRadius(current,target,radius);
}

struct CoreGroundRouteNode {
    GridPos pos{};
    double g = 0.0;
    double f = 0.0;
    std::uint64_t sequence = 0;
};

struct CoreGroundRouteNodeGreater {
    bool operator()(
        const CoreGroundRouteNode& a,
        const CoreGroundRouteNode& b) const
    {
        if(std::abs(a.f - b.f) > 1e-9) return a.f > b.f;
        if(std::abs(a.g - b.g) > 1e-9) return a.g > b.g;
        if(a.pos.x != b.pos.x) return a.pos.x > b.pos.x;
        if(a.pos.y != b.pos.y) return a.pos.y > b.pos.y;
        return a.sequence > b.sequence;
    }
};

inline bool buildCoreGroundRoute(
    const World& world,
    GridPos start,
    GridPos target,
    int arrivalRadius,
    std::vector<GridPos>& outRoute)
{
    outRoute.clear();
    const int radius = std::max(0, arrivalRadius);

    if(gridWithinRadius(start, target, radius)) return true;
    if(!coreGroundTraversable(world, start)) return false;

    // Most everyday movement is locally unobstructed. Resolve the two
    // deterministic Manhattan-L candidates first and reserve A* for actual
    // water/terrain detours. This keeps long-run headless simulation cheap
    // without bypassing Core traversal truth.
    std::vector<GridPos> horizontalFirst;
    if(tryBuildDirectCoreGroundRoute(
            world,start,target,radius,true,horizontalFirst)){
        outRoute=std::move(horizontalFirst);
        return true;
    }

    std::vector<GridPos> verticalFirst;
    if(tryBuildDirectCoreGroundRoute(
            world,start,target,radius,false,verticalFirst)){
        outRoute=std::move(verticalFirst);
        return true;
    }

    const int directDistance = manhattan(start, target);
    const int margin = std::max(
        8,
        std::min(28, directDistance / 3 + 6));
    const int minX = std::min(start.x, target.x) - margin;
    const int maxX = std::max(start.x, target.x) + margin;
    const int minY = std::min(start.y, target.y) - margin;
    const int maxY = std::max(start.y, target.y) + margin;

    const int spanX = std::max(1, maxX - minX + 1);
    const int spanY = std::max(1, maxY - minY + 1);
    const std::size_t maxExpanded = static_cast<std::size_t>(
        std::min(
            40000LL,
            std::max(
                2048LL,
                static_cast<long long>(spanX)
                    * static_cast<long long>(spanY))));

    std::priority_queue<
        CoreGroundRouteNode,
        std::vector<CoreGroundRouteNode>,
        CoreGroundRouteNodeGreater> open;
    std::unordered_map<std::uint64_t, double> best;
    std::unordered_map<std::uint64_t, std::uint64_t> parent;

    std::uint64_t sequence = 0;
    const std::uint64_t startKey = gridRouteKey(start);
    best[startKey] = 0.0;
    open.push({
        start,
        0.0,
        static_cast<double>(manhattan(start, target)),
        sequence++
    });

    const GridPos neighbours[4] = {
        {1, 0},
        {0, 1},
        {-1, 0},
        {0, -1}
    };

    std::uint64_t foundKey = 0;
    bool found = false;
    std::size_t expanded = 0;

    while(!open.empty() && expanded < maxExpanded){
        const CoreGroundRouteNode current = open.top();
        open.pop();

        const std::uint64_t currentKey = gridRouteKey(current.pos);
        const auto bestIt = best.find(currentKey);
        if(bestIt == best.end() || current.g > bestIt->second + 1e-9){
            continue;
        }

        if(gridWithinRadius(current.pos, target, radius)){
            foundKey = currentKey;
            found = true;
            break;
        }

        ++expanded;

        for(const GridPos delta : neighbours){
            const GridPos next{
                current.pos.x + delta.x,
                current.pos.y + delta.y
            };

            if(next.x < minX || next.x > maxX
               || next.y < minY || next.y > maxY){
                continue;
            }
            if(!coreGroundTraversable(world, next)) continue;

            const double tentative =
                current.g + coreGroundTraversalCost(world, current.pos, next);
            const std::uint64_t nextKey = gridRouteKey(next);
            const auto existing = best.find(nextKey);
            if(existing != best.end()
               && tentative >= existing->second - 1e-9){
                continue;
            }

            best[nextKey] = tentative;
            parent[nextKey] = currentKey;

            const int dx = std::max(0, std::abs(next.x - target.x) - radius);
            const int dy = std::max(0, std::abs(next.y - target.y) - radius);
            const double heuristic = static_cast<double>(dx + dy);

            open.push({
                next,
                tentative,
                tentative + heuristic,
                sequence++
            });
        }
    }

    if(!found) return false;

    std::vector<GridPos> reversed;
    std::uint64_t cursor = foundKey;
    while(cursor != startKey){
        reversed.push_back(gridRoutePos(cursor));
        const auto it = parent.find(cursor);
        if(it == parent.end()){
            outRoute.clear();
            return false;
        }
        cursor = it->second;
    }

    outRoute.assign(reversed.rbegin(), reversed.rend());
    return true;
}

} // namespace lifelens
