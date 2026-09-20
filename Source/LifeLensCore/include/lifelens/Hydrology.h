#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

#include "MacroWorldGenesis.h"

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

inline bool isCardinalNeighbour(ChunkCoord a, ChunkCoord b)
{
    const int dx = a.x - b.x;
    const int dy = a.y - b.y;
    return (dx == 0 && (dy == 1 || dy == -1))
        || (dy == 0 && (dx == 1 || dx == -1));
}

inline HydrologyFacts deriveHydrologyFacts(
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
       || !inRange(facts.runoffPotential)){
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
