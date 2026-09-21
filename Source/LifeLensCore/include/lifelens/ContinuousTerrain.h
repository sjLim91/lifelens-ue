#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "MacroWorldGenesis.h"

namespace lifelens {

// World Architecture v2 continuous-terrain contract. The current production
// generation version remains unchanged until terrain + hydrology + presentation
// migration are ready to switch together. Version 3 is the first version that
// may consume this field as authoritative natural terrain.
inline constexpr WorldGenerationVersion ContinuousTerrainGenerationVersion = 3;

struct ContinuousTerrainSample {
    GridPos grid{};
    ChunkCoord chunk{};
    double elevation01 = 0.5;
    double macroElevation01 = 0.5;
    double gradientXPerGrid = 0.0;
    double gradientYPerGrid = 0.0;
};

inline double surfaceValueNoiseAtGrid(
    const WorldGenesisIdentity& identity,
    GridPos grid,
    std::uint64_t domain,
    int latticeSpanGridCells)
{
    const int span = std::max(1, latticeSpanGridCells);
    const int cellX = floorDivWorldGrid(grid.x, span);
    const int cellY = floorDivWorldGrid(grid.y, span);
    const int originX = cellX * span;
    const int originY = cellY * span;

    const double fx =
        static_cast<double>(grid.x - originX) / static_cast<double>(span);
    const double fy =
        static_cast<double>(grid.y - originY) / static_cast<double>(span);
    const double sx = macroSmoothStep(fx);
    const double sy = macroSmoothStep(fy);

    const double v00 = macroLatticeValue(identity, cellX, cellY, domain);
    const double v10 = macroLatticeValue(identity, cellX + 1, cellY, domain);
    const double v01 = macroLatticeValue(identity, cellX, cellY + 1, domain);
    const double v11 = macroLatticeValue(identity, cellX + 1, cellY + 1, domain);

    return macroLerp(
        macroLerp(v00, v10, sx),
        macroLerp(v01, v11, sx),
        sy);
}

inline double deriveContinuousSurfaceElevation01(
    const WorldGenesisIdentity& identity,
    GridPos grid)
{
    // Preserve old save/replay semantics exactly until a generation-version
    // migration explicitly opts into the continuous field.
    if (identity.generationVersion < ContinuousTerrainGenerationVersion)
    {
        return deriveMacroRegionFacts(
            identity,
            chunkCoordForGrid(grid)).elevation;
    }

    constexpr std::uint64_t ContinentalDomain = 0x573254455252434fULL; // W2TERRCO
    constexpr std::uint64_t RegionalDomain    = 0x5732544552525247ULL; // W2TERRRG
    constexpr std::uint64_t HillDomain        = 0x573254455252484cULL; // W2TERRHL
    constexpr std::uint64_t RidgeDomain       = 0x5732544552525244ULL; // W2TERRRD
    constexpr std::uint64_t DetailDomain      = 0x5732544552524454ULL; // W2TERRDT

    constexpr int Chunk = WorldChunkSpanGridCells;
    const double continental =
        surfaceValueNoiseAtGrid(identity, grid, ContinentalDomain, Chunk * 48);
    const double regional =
        surfaceValueNoiseAtGrid(identity, grid, RegionalDomain, Chunk * 16);
    const double hills =
        surfaceValueNoiseAtGrid(identity, grid, HillDomain, Chunk * 6);
    const double ridgeNoise =
        surfaceValueNoiseAtGrid(identity, grid, RidgeDomain, Chunk * 9);
    const double detail =
        surfaceValueNoiseAtGrid(identity, grid, DetailDomain, Chunk * 2);

    // Broad continents dominate. Regional/hill/detail bands add readable
    // valleys and slopes without allowing a chunk boundary to become a shape
    // boundary. Ridge contribution is centered so it adds relief, not a global
    // elevation bias.
    const double ridge =
        1.0 - std::abs(ridgeNoise * 2.0 - 1.0);
    const double elevation =
        continental * 0.48
        + regional * 0.27
        + hills * 0.16
        + detail * 0.09
        + (ridge - 0.5) * 0.10;

    return clampMacro01(elevation);
}

inline ContinuousTerrainSample deriveContinuousTerrainSample(
    const WorldGenesisIdentity& identity,
    GridPos grid)
{
    ContinuousTerrainSample result;
    result.grid = grid;
    result.chunk = chunkCoordForGrid(grid);
    result.macroElevation01 =
        deriveMacroRegionFacts(identity, result.chunk).elevation;
    result.elevation01 =
        deriveContinuousSurfaceElevation01(identity, grid);

    const GridPos west{grid.x - 1, grid.y};
    const GridPos east{grid.x + 1, grid.y};
    const GridPos south{grid.x, grid.y - 1};
    const GridPos north{grid.x, grid.y + 1};

    result.gradientXPerGrid =
        (deriveContinuousSurfaceElevation01(identity, east)
            - deriveContinuousSurfaceElevation01(identity, west)) * 0.5;
    result.gradientYPerGrid =
        (deriveContinuousSurfaceElevation01(identity, north)
            - deriveContinuousSurfaceElevation01(identity, south)) * 0.5;
    return result;
}

} // namespace lifelens
