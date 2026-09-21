#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>

#include "MacroWorldGenesis.h"
#include "ContinuousTerrain.h"

namespace lifelens {

using WaterBodyId = std::uint64_t;

enum class SurfaceWaterKind : std::uint8_t {
    None = 0,
    Spring,
    Stream,
    River,
    Lake,
    Wetland,
    Coast,
    Ocean
};

enum class WaterSalinity : std::uint8_t {
    Fresh = 0,
    Brackish,
    Salt
};

struct HydrologyFacts {
    ChunkCoord coord{};
    SurfaceWaterKind surfaceKind = SurfaceWaterKind::None;
    WaterSalinity salinity = WaterSalinity::Fresh;
    WaterBodyId surfaceWaterId = 0;

    // Stable baseline potentials in [0,1]. Dynamic water quantity,
    // contamination and infrastructure use are separate mutable state later.
    double surfaceAvailability = 0.0;
    double flowPotential = 0.0;
    double groundwaterPotential = 0.0;
    double rechargePotential = 0.0;
    double runoffPotential = 0.0;

    // World v2 drainage-graph signal. Legacy generation versions leave this at
    // zero; v3+ derives it from upstream cells that actually drain through this
    // chunk on the continuous terrain field.
    double drainageAccumulationPotential = 0.0;
    WaterBodyId drainageSystemId = 0;

    bool hasDownstream = false;
    ChunkCoord downstream{};

    // Coastline orientation hint derived from the same authoritative macro
    // topology. Presentation may use it to clip a local coastal water surface.
    bool hasMarineNeighbour = false;
    ChunkCoord marineNeighbour{};
};

inline const char* surfaceWaterKindName(SurfaceWaterKind kind)
{
    switch(kind){
        case SurfaceWaterKind::None: return "None";
        case SurfaceWaterKind::Spring: return "Spring";
        case SurfaceWaterKind::Stream: return "Stream";
        case SurfaceWaterKind::River: return "River";
        case SurfaceWaterKind::Lake: return "Lake";
        case SurfaceWaterKind::Wetland: return "Wetland";
        case SurfaceWaterKind::Coast: return "Coast";
        case SurfaceWaterKind::Ocean: return "Ocean";
    }
    return "None";
}

inline const char* waterSalinityName(WaterSalinity salinity)
{
    switch(salinity){
        case WaterSalinity::Fresh: return "Fresh";
        case WaterSalinity::Brackish: return "Brackish";
        case WaterSalinity::Salt: return "Salt";
    }
    return "Fresh";
}

inline WaterBodyId deriveSurfaceWaterId(
    const WorldGenesisIdentity& identity,
    ChunkCoord coord,
    SurfaceWaterKind kind)
{
    if(kind == SurfaceWaterKind::None) return 0;

    constexpr std::uint64_t HydrologyDomain = 0x485944524f4c4f47ULL; // "HYDROLOG"
    std::uint64_t state = worldGenesisMix64(
        normalizeWorldSeed(identity.worldSeed) ^ HydrologyDomain);
    state = combineWorldGenesisWord(
        state,
        normalizeWorldGenerationVersion(identity.generationVersion));
    state = combineWorldGenesisWord(state, stableSignedCoordinateWord(coord.x));
    state = combineWorldGenesisWord(state, stableSignedCoordinateWord(coord.y));
    state = combineWorldGenesisWord(
        state,
        static_cast<std::uint64_t>(static_cast<std::uint8_t>(kind)));

    WaterBodyId id = worldGenesisMix64(state) & 0x3fffffffffffffffULL;
    id |= 0x2000000000000000ULL;
    return id;
}

inline bool isFreshSurfaceWater(const HydrologyFacts& facts)
{
    return facts.surfaceKind != SurfaceWaterKind::None
        && facts.surfaceKind != SurfaceWaterKind::Ocean
        && facts.salinity == WaterSalinity::Fresh
        && facts.surfaceAvailability > 0.0;
}

struct SurfaceWaterGroundTraversalProfile {
    // Gameplay/ground-locomotion authority. This is intentionally separate
    // from Unreal presentation width/radius hints.
    bool blocksGroundTraversal = false;
    bool linearChannel = false;
    double halfWidthCells = 0.0;
    double radiusCells = 0.0;
};

inline GridPos surfaceWaterCenterGrid(const HydrologyFacts& facts)
{
    const GridPos origin = chunkOriginGrid(facts.coord);
    const int half = WorldChunkSpanGridCells / 2;
    return {origin.x + half, origin.y + half};
}

inline SurfaceWaterGroundTraversalProfile
deriveSurfaceWaterGroundTraversalProfile(const HydrologyFacts& facts)
{
    SurfaceWaterGroundTraversalProfile result;
    if(!isFreshSurfaceWater(facts)) return result;

    switch(facts.surfaceKind){
        case SurfaceWaterKind::Spring:
            result.blocksGroundTraversal = true;
            result.linearChannel = true;
            result.halfWidthCells =
                0.5 * (0.65 + 0.85 * facts.surfaceAvailability);
            break;
        case SurfaceWaterKind::Stream:
            result.blocksGroundTraversal = true;
            result.linearChannel = true;
            result.halfWidthCells =
                0.5 * (0.95
                    + 1.35 * facts.surfaceAvailability
                    + 0.45 * facts.flowPotential);
            break;
        case SurfaceWaterKind::River:
            result.blocksGroundTraversal = true;
            result.linearChannel = true;
            result.halfWidthCells =
                0.5 * (1.85
                    + 2.75 * facts.surfaceAvailability
                    + 1.10 * facts.flowPotential);
            break;
        case SurfaceWaterKind::Lake:
            result.blocksGroundTraversal = true;
            result.radiusCells =
                2.50 + 4.25 * facts.surfaceAvailability;
            break;
        case SurfaceWaterKind::Wetland:
            // Until terrain-cost movement exists, visible standing wetland
            // water is treated as a hard ground-traversal footprint.
            result.blocksGroundTraversal = true;
            result.radiusCells =
                3.00 + 4.00 * facts.surfaceAvailability;
            break;
        case SurfaceWaterKind::Coast:
        case SurfaceWaterKind::Ocean:
        case SurfaceWaterKind::None:
        default:
            break;
    }

    result.halfWidthCells = std::max(0.0, result.halfWidthCells);
    result.radiusCells = std::max(0.0, result.radiusCells);
    return result;
}

inline bool surfaceWaterGroundContainsGrid(
    const HydrologyFacts& facts,
    GridPos position)
{
    const SurfaceWaterGroundTraversalProfile profile =
        deriveSurfaceWaterGroundTraversalProfile(facts);
    if(!profile.blocksGroundTraversal) return false;

    const GridPos center = surfaceWaterCenterGrid(facts);
    const double px = static_cast<double>(position.x);
    const double py = static_cast<double>(position.y);
    const double ax = static_cast<double>(center.x);
    const double ay = static_cast<double>(center.y);

    if(profile.linearChannel && facts.hasDownstream){
        const GridPos downstreamOrigin = chunkOriginGrid(facts.downstream);
        const int half = WorldChunkSpanGridCells / 2;
        const double bx = static_cast<double>(downstreamOrigin.x + half);
        const double by = static_cast<double>(downstreamOrigin.y + half);
        const double abx = bx - ax;
        const double aby = by - ay;
        const double ab2 = abx * abx + aby * aby;
        const double t = ab2 > 0.0
            ? std::max(0.0, std::min(
                1.0,
                ((px - ax) * abx + (py - ay) * aby) / ab2))
            : 0.0;
        const double dx = px - (ax + t * abx);
        const double dy = py - (ay + t * aby);
        return dx * dx + dy * dy
            <= profile.halfWidthCells * profile.halfWidthCells;
    }

    const double radius = profile.linearChannel
        ? profile.halfWidthCells
        : profile.radiusCells;
    const double dx = px - ax;
    const double dy = py - ay;
    return dx * dx + dy * dy <= radius * radius;
}

inline GridPos surfaceWaterGroundAccessGrid(const HydrologyFacts& facts)
{
    const SurfaceWaterGroundTraversalProfile profile =
        deriveSurfaceWaterGroundTraversalProfile(facts);
    const GridPos center = surfaceWaterCenterGrid(facts);
    if(!profile.blocksGroundTraversal) return center;

    const double footprintRadius = profile.linearChannel
        ? profile.halfWidthCells
        : profile.radiusCells;
    const int offset = std::max(
        1,
        static_cast<int>(std::ceil(footprintRadius)) + 1);

    int dx = 0;
    int dy = 0;
    if(profile.linearChannel && facts.hasDownstream){
        const int flowX = facts.downstream.x - facts.coord.x;
        const int flowY = facts.downstream.y - facts.coord.y;
        // Choose one deterministic bank. Downstream is cardinal by contract.
        const bool opposite =
            (facts.surfaceWaterId & 1ULL) != 0ULL;
        dx = -flowY;
        dy = flowX;
        if(opposite){
            dx = -dx;
            dy = -dy;
        }
    }else{
        switch(static_cast<int>(facts.surfaceWaterId & 3ULL)){
            case 0: dx = 1; break;
            case 1: dx = -1; break;
            case 2: dy = 1; break;
            default: dy = -1; break;
        }
    }

    GridPos access{center.x + dx * offset, center.y + dy * offset};
    if(surfaceWaterGroundContainsGrid(facts, access)){
        // Defensive deterministic fallback: the access point must never be
        // inside the authoritative water footprint.
        access = {center.x + offset, center.y};
    }
    return access;
}

inline bool isCardinalNeighbour(ChunkCoord a, ChunkCoord b)
{
    const int dx = a.x - b.x;
    const int dy = a.y - b.y;
    return (dx == 0 && (dy == 1 || dy == -1))
        || (dy == 0 && (dx == 1 || dx == -1));
}

inline HydrologyFacts deriveLegacyHydrologyFacts(
    const WorldGenesisIdentity& identity,
    ChunkCoord coord)
{
    const MacroRegionFacts center = deriveMacroRegionFacts(identity, coord);

    const std::array<ChunkCoord, 4> neighbours = {{
        {coord.x + 1, coord.y},
        {coord.x - 1, coord.y},
        {coord.x, coord.y + 1},
        {coord.x, coord.y - 1}
    }};

    ChunkCoord lowestCoord = neighbours.front();
    double lowestElevation = deriveMacroRegionFacts(identity, lowestCoord).elevation;
    double minimumNeighbourElevation = lowestElevation;

    for(std::size_t i = 1; i < neighbours.size(); ++i){
        const double elevation =
            deriveMacroRegionFacts(identity, neighbours[i]).elevation;
        if(elevation < lowestElevation
           || (elevation == lowestElevation && neighbours[i] < lowestCoord)){
            lowestElevation = elevation;
            lowestCoord = neighbours[i];
        }
        minimumNeighbourElevation = std::min(
            minimumNeighbourElevation,
            elevation);
    }

    const double downhillDrop = std::max(
        0.0,
        center.elevation - lowestElevation);
    const double basinDepth = std::max(
        0.0,
        minimumNeighbourElevation - center.elevation);

    HydrologyFacts result;
    result.coord = coord;
    result.groundwaterPotential = clampMacro01(
        0.50 * center.waterPotential
        + 0.34 * center.moisture
        + 0.16 * (1.0 - center.elevation));
    result.rechargePotential = clampMacro01(
        0.48 * center.moisture
        + 0.34 * center.waterPotential
        + 0.18 * (1.0 - center.hazardPotential));
    result.runoffPotential = clampMacro01(
        0.40 * center.moisture
        + 0.36 * center.waterPotential
        + 0.24 * clampMacro01(downhillDrop * 7.0));

    const double channelPotential = clampMacro01(
        0.46 * result.runoffPotential
        + 0.34 * center.waterPotential
        + 0.20 * clampMacro01(downhillDrop * 9.0));

    const MacroSurfaceFacts macroSurface =
        deriveMacroSurfaceFacts(identity, coord);

    if(macroSurface.surfaceClass == MacroSurfaceClass::Ocean){
        result.surfaceKind = SurfaceWaterKind::Ocean;
        result.salinity = WaterSalinity::Salt;
    }else if(macroSurface.surfaceClass == MacroSurfaceClass::Coast){
        result.surfaceKind = SurfaceWaterKind::Coast;
        result.salinity = WaterSalinity::Brackish;
        result.hasMarineNeighbour = macroSurface.hasMarineNeighbour;
        result.marineNeighbour = macroSurface.marineNeighbour;
    }else if(center.biome == MacroBiome::Wetland
       && center.waterPotential >= 0.66
       && center.moisture >= 0.60){
        result.surfaceKind = SurfaceWaterKind::Wetland;
    }else if(basinDepth >= 0.010
             && center.waterPotential >= 0.58){
        result.surfaceKind = SurfaceWaterKind::Lake;
    }else if(channelPotential >= 0.70
             && downhillDrop >= 0.002){
        result.surfaceKind = SurfaceWaterKind::River;
    }else if(channelPotential >= 0.52
             && downhillDrop >= 0.001){
        result.surfaceKind = SurfaceWaterKind::Stream;
    }else if(result.groundwaterPotential >= 0.66
             && center.waterPotential >= 0.50){
        result.surfaceKind = SurfaceWaterKind::Spring;
    }

    if(macroSurface.surfaceClass == MacroSurfaceClass::Land){
        result.salinity = WaterSalinity::Fresh;
    }

    switch(result.surfaceKind){
        case SurfaceWaterKind::River:
            result.surfaceAvailability = clampMacro01(
                0.58 + 0.34 * center.waterPotential
                + 0.08 * result.rechargePotential);
            result.flowPotential = clampMacro01(
                0.55 * channelPotential
                + 0.45 * clampMacro01(downhillDrop * 10.0));
            break;
        case SurfaceWaterKind::Stream:
            result.surfaceAvailability = clampMacro01(
                0.35 + 0.38 * center.waterPotential
                + 0.27 * result.rechargePotential);
            result.flowPotential = clampMacro01(
                0.42 * channelPotential
                + 0.58 * clampMacro01(downhillDrop * 9.0));
            break;
        case SurfaceWaterKind::Lake:
            result.surfaceAvailability = clampMacro01(
                0.52 + 0.34 * center.waterPotential
                + 0.14 * result.rechargePotential);
            result.flowPotential = 0.08 * result.rechargePotential;
            break;
        case SurfaceWaterKind::Wetland:
            result.surfaceAvailability = clampMacro01(
                0.44 + 0.36 * center.waterPotential
                + 0.20 * center.moisture);
            result.flowPotential = 0.12 * result.runoffPotential;
            break;
        case SurfaceWaterKind::Spring:
            result.surfaceAvailability = clampMacro01(
                0.26 + 0.52 * result.groundwaterPotential
                + 0.22 * result.rechargePotential);
            result.flowPotential = 0.18 * result.rechargePotential;
            break;
        case SurfaceWaterKind::Coast:
            result.surfaceAvailability = 1.0;
            result.flowPotential = 0.16 * result.runoffPotential;
            break;
        case SurfaceWaterKind::Ocean:
            result.surfaceAvailability = 1.0;
            result.flowPotential = 0.06 * result.runoffPotential;
            break;
        case SurfaceWaterKind::None:
            break;
    }

    if(result.surfaceKind == SurfaceWaterKind::Stream
       || result.surfaceKind == SurfaceWaterKind::River
       || result.surfaceKind == SurfaceWaterKind::Spring){
        result.hasDownstream = true;
        result.downstream = lowestCoord;
    }else if(result.surfaceKind == SurfaceWaterKind::Lake
             && lowestElevation < center.elevation + 0.02){
        // A shallow deterministic outlet keeps later watershed routing possible
        // without claiming a full fluid simulation in this foundation pass.
        result.hasDownstream = true;
        result.downstream = lowestCoord;
    }

    result.surfaceWaterId = deriveSurfaceWaterId(
        identity,
        coord,
        result.surfaceKind);
    return result;
}


inline GridPos continuousHydrologyChunkCenterGrid(ChunkCoord coord)
{
    const GridPos origin = chunkOriginGrid(coord);
    const int half = WorldChunkSpanGridCells / 2;
    return {origin.x + half, origin.y + half};
}

inline double continuousHydrologyElevation01(
    const WorldGenesisIdentity& identity,
    ChunkCoord coord)
{
    return deriveContinuousSurfaceElevation01(
        identity,
        continuousHydrologyChunkCenterGrid(coord));
}

inline MacroSurfaceFacts deriveContinuousHydrologySurfaceFacts(
    const WorldGenesisIdentity& identity,
    ChunkCoord coord)
{
    MacroSurfaceFacts result;
    result.coord = coord;

    const double centerElevation =
        continuousHydrologyElevation01(identity, coord);
    if(centerElevation < MacroSeaLevel01){
        result.surfaceClass = MacroSurfaceClass::Ocean;
        return result;
    }

    const std::array<ChunkCoord,4> neighbours = {{
        {coord.x + 1, coord.y},
        {coord.x - 1, coord.y},
        {coord.x, coord.y + 1},
        {coord.x, coord.y - 1}
    }};

    bool foundMarine = false;
    double lowestMarineElevation = 2.0;
    ChunkCoord lowestMarine{};
    for(const ChunkCoord neighbour : neighbours){
        const double elevation =
            continuousHydrologyElevation01(identity, neighbour);
        if(elevation >= MacroSeaLevel01) continue;
        if(!foundMarine
           || elevation < lowestMarineElevation
           || (elevation == lowestMarineElevation && neighbour < lowestMarine)){
            foundMarine = true;
            lowestMarineElevation = elevation;
            lowestMarine = neighbour;
        }
    }

    if(foundMarine){
        result.surfaceClass = MacroSurfaceClass::Coast;
        result.hasMarineNeighbour = true;
        result.marineNeighbour = lowestMarine;
    }
    return result;
}

inline bool tryDeriveContinuousHydrologyDownstream(
    const WorldGenesisIdentity& identity,
    ChunkCoord coord,
    ChunkCoord& outDownstream,
    double* outDrop = nullptr)
{
    const double centerElevation =
        continuousHydrologyElevation01(identity, coord);
    const std::array<ChunkCoord,4> neighbours = {{
        {coord.x + 1, coord.y},
        {coord.x - 1, coord.y},
        {coord.x, coord.y + 1},
        {coord.x, coord.y - 1}
    }};

    bool found = false;
    double lowestElevation = centerElevation;
    ChunkCoord lowestCoord = coord;
    for(const ChunkCoord neighbour : neighbours){
        const double elevation =
            continuousHydrologyElevation01(identity, neighbour);
        if(elevation < lowestElevation - 1e-12
           || (found && elevation == lowestElevation && neighbour < lowestCoord)){
            found = true;
            lowestElevation = elevation;
            lowestCoord = neighbour;
        }
    }

    if(!found){
        if(outDrop) *outDrop = 0.0;
        return false;
    }

    outDownstream = lowestCoord;
    if(outDrop){
        *outDrop = std::max(0.0, centerElevation - lowestElevation);
    }
    return true;
}

inline ChunkCoord traceContinuousDrainageTerminal(
    const WorldGenesisIdentity& identity,
    ChunkCoord start,
    int maxSteps = 256)
{
    ChunkCoord current = start;
    const int steps = std::max(1, maxSteps);
    for(int i = 0; i < steps; ++i){
        if(continuousHydrologyElevation01(identity, current) < MacroSeaLevel01){
            return current;
        }
        ChunkCoord next{};
        if(!tryDeriveContinuousHydrologyDownstream(identity, current, next)){
            return current;
        }
        current = next;
    }
    return current;
}

inline WaterBodyId deriveContinuousDrainageSystemId(
    const WorldGenesisIdentity& identity,
    ChunkCoord coord)
{
    constexpr std::uint64_t DrainageDomain = 0x5732485944524f44ULL; // W2HYDROD
    const ChunkCoord terminal =
        traceContinuousDrainageTerminal(identity, coord);

    std::uint64_t state = worldGenesisMix64(
        normalizeWorldSeed(identity.worldSeed) ^ DrainageDomain);
    state = combineWorldGenesisWord(
        state,
        normalizeWorldGenerationVersion(identity.generationVersion));
    state = combineWorldGenesisWord(
        state,
        stableSignedCoordinateWord(terminal.x));
    state = combineWorldGenesisWord(
        state,
        stableSignedCoordinateWord(terminal.y));

    WaterBodyId id =
        worldGenesisMix64(state) & 0x3fffffffffffffffULL;
    id |= 0x2400000000000000ULL;
    return id;
}

inline double deriveContinuousDrainageAccumulationPotential(
    const WorldGenesisIdentity& identity,
    ChunkCoord target,
    int radiusChunks = 3)
{
    const int radius = std::max(1, radiusChunks);
    double contribution = 0.0;

    for(int dy = -radius; dy <= radius; ++dy){
        for(int dx = -radius; dx <= radius; ++dx){
            const int distance = std::abs(dx) + std::abs(dy);
            if(distance == 0 || distance > radius) continue;

            const ChunkCoord source{target.x + dx, target.y + dy};
            const MacroSurfaceFacts sourceSurface =
                deriveContinuousHydrologySurfaceFacts(identity, source);
            if(sourceSurface.surfaceClass == MacroSurfaceClass::Ocean) continue;

            ChunkCoord current = source;
            bool reachesTarget = false;
            const int maxSteps = radius * 3 + 4;
            for(int step = 0; step < maxSteps; ++step){
                ChunkCoord next{};
                if(!tryDeriveContinuousHydrologyDownstream(
                        identity, current, next)){
                    break;
                }
                if(next == target){
                    reachesTarget = true;
                    break;
                }
                if(std::abs(next.x - target.x)
                        + std::abs(next.y - target.y)
                    > radius + 2){
                    break;
                }
                current = next;
            }

            if(!reachesTarget) continue;

            const MacroRegionFacts sourceFacts =
                deriveMacroRegionFacts(identity, source);
            const double sourceRunoff = clampMacro01(
                0.55 * sourceFacts.moisture
                + 0.45 * sourceFacts.waterPotential);
            contribution +=
                sourceRunoff / (1.0 + 0.30 * static_cast<double>(distance));
        }
    }

    return clampMacro01(contribution / 4.0);
}

inline HydrologyFacts deriveContinuousHydrologyFacts(
    const WorldGenesisIdentity& identity,
    ChunkCoord coord)
{
    const MacroRegionFacts climate =
        deriveMacroRegionFacts(identity, coord);
    const MacroSurfaceFacts surface =
        deriveContinuousHydrologySurfaceFacts(identity, coord);
    const double elevation =
        continuousHydrologyElevation01(identity, coord);

    const std::array<ChunkCoord,4> neighbours = {{
        {coord.x + 1, coord.y},
        {coord.x - 1, coord.y},
        {coord.x, coord.y + 1},
        {coord.x, coord.y - 1}
    }};

    double minimumNeighbourElevation = 2.0;
    for(const ChunkCoord neighbour : neighbours){
        minimumNeighbourElevation = std::min(
            minimumNeighbourElevation,
            continuousHydrologyElevation01(identity, neighbour));
    }

    ChunkCoord downstream{};
    double downhillDrop = 0.0;
    const bool hasTerrainDownstream =
        tryDeriveContinuousHydrologyDownstream(
            identity,
            coord,
            downstream,
            &downhillDrop);
    const double basinDepth = std::max(
        0.0,
        minimumNeighbourElevation - elevation);

    HydrologyFacts result;
    result.coord = coord;
    result.groundwaterPotential = clampMacro01(
        0.48 * climate.waterPotential
        + 0.34 * climate.moisture
        + 0.18 * (1.0 - elevation));
    result.rechargePotential = clampMacro01(
        0.52 * climate.moisture
        + 0.30 * climate.waterPotential
        + 0.18 * (1.0 - climate.hazardPotential));
    result.runoffPotential = clampMacro01(
        0.38 * climate.moisture
        + 0.30 * climate.waterPotential
        + 0.32 * clampMacro01(downhillDrop * 14.0));
    result.drainageAccumulationPotential =
        deriveContinuousDrainageAccumulationPotential(identity, coord);

    const double channelPotential = clampMacro01(
        0.34 * result.runoffPotential
        + 0.24 * climate.waterPotential
        + 0.42 * result.drainageAccumulationPotential);

    if(surface.surfaceClass == MacroSurfaceClass::Ocean){
        result.surfaceKind = SurfaceWaterKind::Ocean;
        result.salinity = WaterSalinity::Salt;
    }else if(surface.surfaceClass == MacroSurfaceClass::Coast){
        result.surfaceKind = SurfaceWaterKind::Coast;
        result.salinity = WaterSalinity::Brackish;
        result.hasMarineNeighbour = surface.hasMarineNeighbour;
        result.marineNeighbour = surface.marineNeighbour;
    }else if(!hasTerrainDownstream
             && basinDepth >= 0.0015
             && climate.waterPotential >= 0.56
             && result.rechargePotential >= 0.48){
        result.surfaceKind = SurfaceWaterKind::Lake;
    }else if(climate.moisture >= 0.66
             && climate.waterPotential >= 0.66
             && downhillDrop < 0.0045){
        result.surfaceKind = SurfaceWaterKind::Wetland;
    }else if(hasTerrainDownstream
             && result.drainageAccumulationPotential >= 0.42
             && channelPotential >= 0.52){
        result.surfaceKind = SurfaceWaterKind::River;
    }else if(hasTerrainDownstream
             && result.drainageAccumulationPotential >= 0.12
             && channelPotential >= 0.38){
        result.surfaceKind = SurfaceWaterKind::Stream;
    }else if(result.groundwaterPotential >= 0.68
             && climate.waterPotential >= 0.50){
        result.surfaceKind = SurfaceWaterKind::Spring;
    }

    if(surface.surfaceClass == MacroSurfaceClass::Land){
        result.salinity = WaterSalinity::Fresh;
    }

    switch(result.surfaceKind){
        case SurfaceWaterKind::River:
            result.surfaceAvailability = clampMacro01(
                0.56
                + 0.22 * climate.waterPotential
                + 0.14 * result.rechargePotential
                + 0.08 * result.drainageAccumulationPotential);
            result.flowPotential = clampMacro01(
                0.42 * channelPotential
                + 0.30 * clampMacro01(downhillDrop * 16.0)
                + 0.28 * result.drainageAccumulationPotential);
            break;
        case SurfaceWaterKind::Stream:
            result.surfaceAvailability = clampMacro01(
                0.30
                + 0.34 * climate.waterPotential
                + 0.22 * result.rechargePotential
                + 0.14 * result.drainageAccumulationPotential);
            result.flowPotential = clampMacro01(
                0.38 * channelPotential
                + 0.38 * clampMacro01(downhillDrop * 14.0)
                + 0.24 * result.drainageAccumulationPotential);
            break;
        case SurfaceWaterKind::Lake:
            result.surfaceAvailability = clampMacro01(
                0.50
                + 0.28 * climate.waterPotential
                + 0.22 * result.rechargePotential);
            result.flowPotential = 0.05 * result.rechargePotential;
            break;
        case SurfaceWaterKind::Wetland:
            result.surfaceAvailability = clampMacro01(
                0.42
                + 0.34 * climate.waterPotential
                + 0.24 * climate.moisture);
            result.flowPotential = 0.10 * result.runoffPotential;
            break;
        case SurfaceWaterKind::Spring:
            result.surfaceAvailability = clampMacro01(
                0.24
                + 0.52 * result.groundwaterPotential
                + 0.24 * result.rechargePotential);
            result.flowPotential = hasTerrainDownstream
                ? 0.16 * result.rechargePotential
                : 0.04 * result.rechargePotential;
            break;
        case SurfaceWaterKind::Coast:
            result.surfaceAvailability = 1.0;
            result.flowPotential = 0.16 * result.runoffPotential;
            break;
        case SurfaceWaterKind::Ocean:
            result.surfaceAvailability = 1.0;
            result.flowPotential = 0.06 * result.runoffPotential;
            break;
        case SurfaceWaterKind::None:
            break;
    }

    if((result.surfaceKind == SurfaceWaterKind::Stream
        || result.surfaceKind == SurfaceWaterKind::River
        || result.surfaceKind == SurfaceWaterKind::Spring)
       && hasTerrainDownstream){
        result.hasDownstream = true;
        result.downstream = downstream;
    }

    if(result.surfaceKind != SurfaceWaterKind::None){
        result.drainageSystemId =
            deriveContinuousDrainageSystemId(identity, coord);
    }

    // Keep the existing segment id contract for compatibility while exposing a
    // stable drainage-system id for World v2 river/lake grouping.
    result.surfaceWaterId = deriveSurfaceWaterId(
        identity,
        coord,
        result.surfaceKind);
    return result;
}

inline HydrologyFacts deriveHydrologyFacts(
    const WorldGenesisIdentity& identity,
    ChunkCoord coord)
{
    if(identity.generationVersion >= ContinuousTerrainGenerationVersion){
        return deriveContinuousHydrologyFacts(identity, coord);
    }
    return deriveLegacyHydrologyFacts(identity, coord);
}

inline constexpr int FreshSurfaceStartSearchRadiusChunks =
    MacroStartSearchRadiusChunks * 3;
inline constexpr int FreshSurfaceNeighbourRadiusChunks = 2;

inline bool findNearestFreshSurfaceWaterChunk(
    const WorldGenesisIdentity& identity,
    ChunkCoord origin,
    ChunkCoord& outCoord,
    int maxDistanceChunks=FreshSurfaceNeighbourRadiusChunks)
{
    const int radius = std::max(1, maxDistanceChunks);
    bool found = false;
    int bestDistance = 0;

    for(int dy=-radius; dy<=radius; ++dy){
        for(int dx=-radius; dx<=radius; ++dx){
            if(dx==0 && dy==0) continue;
            const int absX = dx < 0 ? -dx : dx;
            const int absY = dy < 0 ? -dy : dy;
            const int distance = absX + absY;
            if(distance > radius) continue;

            const ChunkCoord candidate{origin.x+dx,origin.y+dy};
            const HydrologyFacts hydrology =
                deriveHydrologyFacts(identity,candidate);
            if(!isFreshSurfaceWater(hydrology)) continue;

            if(!found || distance < bestDistance
               || (distance == bestDistance && candidate < outCoord)){
                outCoord = candidate;
                bestDistance = distance;
                found = true;
            }
        }
    }
    return found;
}

inline InitialStartRegionSelection selectInitialFreshwaterAdjacentRegion(
    const WorldGenesisIdentity& identity,
    int searchRadiusChunks=FreshSurfaceStartSearchRadiusChunks)
{
    const int radius = std::max(1, searchRadiusChunks);
    InitialStartRegionSelection best;
    bool hasBest = false;

    for(int y=-radius; y<=radius; ++y){
        for(int x=-radius; x<=radius; ++x){
            const ChunkCoord coord{x,y};
            const MacroSurfaceFacts surface =
                deriveMacroSurfaceFacts(identity, coord);
            ++best.evaluatedCandidates;
            if(surface.surfaceClass != MacroSurfaceClass::Land){
                continue;
            }

            // Founders begin on dry local surface, never inside the centered
            // fresh-water shape rendered for this chunk.
            const HydrologyFacts localHydrology =
                deriveHydrologyFacts(identity, coord);
            if(localHydrology.surfaceKind != SurfaceWaterKind::None){
                continue;
            }

            ChunkCoord freshwaterCoord{};
            if(!findNearestFreshSurfaceWaterChunk(
                    identity,
                    coord,
                    freshwaterCoord,
                    FreshSurfaceNeighbourRadiusChunks)){
                continue;
            }

            const MacroRegionFacts facts =
                deriveMacroRegionFacts(identity, coord);
            const double viability = scoreInitialStartRegion(facts);
            if(!hasBest || viability > best.viability
               || (viability == best.viability && facts.coord < best.region.coord)){
                best.region = facts;
                best.viability = viability;
                hasBest = true;
            }
        }
    }

    return hasBest
        ? best
        : selectInitialStartRegion(identity, radius);
}

inline bool validHydrologyFacts(const HydrologyFacts& facts)
{
    const auto inRange=[](double value){
        return value >= 0.0 && value <= 1.0;
    };

    if(!inRange(facts.surfaceAvailability)
       || !inRange(facts.flowPotential)
       || !inRange(facts.groundwaterPotential)
       || !inRange(facts.rechargePotential)
       || !inRange(facts.runoffPotential)
       || !inRange(facts.drainageAccumulationPotential)){
        return false;
    }

    if(facts.surfaceKind == SurfaceWaterKind::None){
        if(facts.surfaceWaterId != 0 || facts.surfaceAvailability != 0.0){
            return false;
        }
    }else if(facts.surfaceWaterId == 0){
        return false;
    }

    if(facts.hasDownstream
       && !isCardinalNeighbour(facts.coord, facts.downstream)){
        return false;
    }

    if(facts.hasMarineNeighbour
       && !isCardinalNeighbour(facts.coord, facts.marineNeighbour)){
        return false;
    }

    if(facts.surfaceKind == SurfaceWaterKind::Coast
       && !facts.hasMarineNeighbour){
        return false;
    }

    if((facts.surfaceKind == SurfaceWaterKind::Ocean
        || facts.surfaceKind == SurfaceWaterKind::Coast)
       && facts.salinity == WaterSalinity::Fresh){
        return false;
    }

    return true;
}

} // namespace lifelens
