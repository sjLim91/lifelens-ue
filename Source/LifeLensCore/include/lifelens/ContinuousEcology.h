#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "ContinuousTerrain.h"
#include "Hydrology.h"

namespace lifelens {

enum class ContinuousEcologyBiome : std::uint8_t {
    Ocean = 0,
    Coast,
    Wetland,
    TemperateForest,
    Meadow,
    Plains,
    DryScrub,
    ColdSteppe,
    RockyHighland
};

struct ContinuousEcologySample {
    GridPos grid{};
    ChunkCoord chunk{};

    double elevation01 = 0.5;
    double slope01 = 0.0;
    double moisture01 = 0.5;
    double temperature01 = 0.5;
    double waterInfluence01 = 0.5;

    // Coverage fields are ecological likelihood/visual-density contracts, not
    // individual resource inventory. Presentation may materialize vegetation
    // from them without inventing Core ResourceNodes.
    double forestCoverage01 = 0.0;
    double grassCoverage01 = 0.0;
    double shrubCoverage01 = 0.0;
    double rockCoverage01 = 0.0;
    double wetlandCoverage01 = 0.0;

    ContinuousEcologyBiome biome = ContinuousEcologyBiome::Plains;
};

inline const char* continuousEcologyBiomeName(ContinuousEcologyBiome biome)
{
    switch (biome) {
        case ContinuousEcologyBiome::Ocean: return "Ocean";
        case ContinuousEcologyBiome::Coast: return "Coast";
        case ContinuousEcologyBiome::Wetland: return "Wetland";
        case ContinuousEcologyBiome::TemperateForest: return "TemperateForest";
        case ContinuousEcologyBiome::Meadow: return "Meadow";
        case ContinuousEcologyBiome::Plains: return "Plains";
        case ContinuousEcologyBiome::DryScrub: return "DryScrub";
        case ContinuousEcologyBiome::ColdSteppe: return "ColdSteppe";
        case ContinuousEcologyBiome::RockyHighland: return "RockyHighland";
    }
    return "Plains";
}

inline double continuousMoistureAtGrid(
    const WorldGenesisIdentity& identity,
    GridPos grid)
{
    constexpr std::uint64_t BroadDomain = 0x573245434f4d4f42ULL; // W2ECOMOB
    constexpr std::uint64_t LocalDomain = 0x573245434f4d4f4cULL; // W2ECOMOL
    constexpr int Chunk = WorldChunkSpanGridCells;

    const double broad =
        surfaceValueNoiseAtGrid(identity, grid, BroadDomain, Chunk * 24);
    const double local =
        surfaceValueNoiseAtGrid(identity, grid, LocalDomain, Chunk * 6);

    return clampMacro01(0.72 * broad + 0.28 * local);
}

inline double continuousTemperatureAtGrid(
    const WorldGenesisIdentity& identity,
    GridPos grid,
    double elevation01)
{
    constexpr std::uint64_t BroadDomain = 0x573245434f544d42ULL; // W2ECOTMB
    constexpr std::uint64_t LocalDomain = 0x573245434f544d4cULL; // W2ECOTML
    constexpr int Chunk = WorldChunkSpanGridCells;

    const double broad =
        surfaceValueNoiseAtGrid(identity, grid, BroadDomain, Chunk * 32);
    const double local =
        surfaceValueNoiseAtGrid(identity, grid, LocalDomain, Chunk * 8);
    const double base = 0.78 * broad + 0.22 * local;

    // Simple lapse-rate analogue. This is deliberately a normalized climate
    // field, not a claim of real Celsius at a specific Earth latitude.
    const double altitudeCooling =
        std::max(0.0, elevation01 - 0.45) * 0.58;
    return clampMacro01(base - altitudeCooling);
}

inline double continuousWaterInfluenceAtGrid(
    const WorldGenesisIdentity& identity,
    GridPos grid)
{
    const ChunkCoord chunk = chunkCoordForGrid(grid);
    const MacroRegionFacts macro =
        deriveMacroRegionFacts(identity, chunk);

    // Smooth potential remains the baseline so ecology does not acquire hard
    // square edges from chunk classification. Actual local surface water gives
    // only an additional bounded boost.
    double influence = clampMacro01(
        0.62 * continuousMoistureAtGrid(identity, grid)
        + 0.38 * macro.waterPotential);

    if (identity.generationVersion >= ContinuousTerrainGenerationVersion) {
        const HydrologyFacts hydro = deriveHydrologyFacts(identity, chunk);
        if (hydro.surfaceKind == SurfaceWaterKind::River
            || hydro.surfaceKind == SurfaceWaterKind::Stream
            || hydro.surfaceKind == SurfaceWaterKind::Spring
            || hydro.surfaceKind == SurfaceWaterKind::Lake
            || hydro.surfaceKind == SurfaceWaterKind::Wetland) {
            influence = clampMacro01(
                influence + 0.16 + 0.12 * hydro.surfaceAvailability);
        } else if (hydro.surfaceKind == SurfaceWaterKind::Coast) {
            influence = clampMacro01(influence + 0.08);
        }
    }
    return influence;
}

inline ContinuousEcologyBiome classifyContinuousEcologyBiome(
    const ContinuousEcologySample& sample,
    MacroSurfaceClass surfaceClass)
{
    if (surfaceClass == MacroSurfaceClass::Ocean) {
        return ContinuousEcologyBiome::Ocean;
    }
    if (surfaceClass == MacroSurfaceClass::Coast) {
        return ContinuousEcologyBiome::Coast;
    }
    if (sample.wetlandCoverage01 >= 0.58) {
        return ContinuousEcologyBiome::Wetland;
    }
    if (sample.elevation01 >= 0.76 || sample.rockCoverage01 >= 0.66) {
        return ContinuousEcologyBiome::RockyHighland;
    }
    if (sample.temperature01 <= 0.25) {
        return ContinuousEcologyBiome::ColdSteppe;
    }
    if (sample.forestCoverage01 >= 0.48) {
        return ContinuousEcologyBiome::TemperateForest;
    }
    if (sample.moisture01 <= 0.28 && sample.shrubCoverage01 >= 0.28) {
        return ContinuousEcologyBiome::DryScrub;
    }
    if (sample.grassCoverage01 >= 0.54) {
        return ContinuousEcologyBiome::Meadow;
    }
    return ContinuousEcologyBiome::Plains;
}

inline ContinuousEcologySample deriveContinuousEcologySample(
    const WorldGenesisIdentity& identity,
    GridPos grid)
{
    ContinuousEcologySample result;
    result.grid = grid;
    result.chunk = chunkCoordForGrid(grid);

    const ContinuousTerrainSample terrain =
        deriveContinuousTerrainSample(identity, grid);
    result.elevation01 = terrain.elevation01;

    const double gradientMagnitude = std::sqrt(
        terrain.gradientXPerGrid * terrain.gradientXPerGrid
        + terrain.gradientYPerGrid * terrain.gradientYPerGrid);
    result.slope01 = clampMacro01(gradientMagnitude * 96.0);

    result.moisture01 = continuousMoistureAtGrid(identity, grid);
    result.temperature01 =
        continuousTemperatureAtGrid(identity, grid, result.elevation01);
    result.waterInfluence01 =
        continuousWaterInfluenceAtGrid(identity, grid);

    constexpr std::uint64_t PatchDomain = 0x573245434f504154ULL; // W2ECOPAT
    constexpr int Chunk = WorldChunkSpanGridCells;
    const double patch =
        surfaceValueNoiseAtGrid(identity, grid, PatchDomain, Chunk * 3);

    const double temperatureComfort = clampMacro01(
        1.0 - std::abs(result.temperature01 - 0.56) * 1.8);
    const double slopeVegetationPenalty =
        clampMacro01(1.0 - result.slope01 * 0.72);
    const double elevationForestPenalty =
        clampMacro01(1.0 - std::max(0.0, result.elevation01 - 0.62) * 1.9);

    result.forestCoverage01 = clampMacro01(
        (0.50 * result.moisture01
         + 0.22 * result.waterInfluence01
         + 0.18 * temperatureComfort
         + 0.10 * patch)
        * slopeVegetationPenalty
        * elevationForestPenalty);

    result.grassCoverage01 = clampMacro01(
        (0.34 * result.moisture01
         + 0.24 * temperatureComfort
         + 0.24 * (1.0 - result.forestCoverage01)
         + 0.18 * (1.0 - std::abs(patch - 0.52)))
        * clampMacro01(1.0 - result.slope01 * 0.52));

    result.shrubCoverage01 = clampMacro01(
        0.30 * (1.0 - result.moisture01)
        + 0.24 * temperatureComfort
        + 0.24 * (1.0 - result.forestCoverage01)
        + 0.22 * patch);

    result.rockCoverage01 = clampMacro01(
        0.48 * result.slope01
        + 0.30 * result.elevation01
        + 0.14 * (1.0 - result.moisture01)
        + 0.08 * (1.0 - patch));

    result.wetlandCoverage01 = clampMacro01(
        0.44 * result.moisture01
        + 0.40 * result.waterInfluence01
        + 0.16 * (1.0 - result.slope01)
        - 0.42);

    MacroSurfaceClass surfaceClass =
        deriveMacroSurfaceFacts(identity, result.chunk).surfaceClass;
    if (identity.generationVersion >= ContinuousTerrainGenerationVersion) {
        surfaceClass =
            deriveContinuousHydrologySurfaceFacts(identity, result.chunk)
                .surfaceClass;
    }

    if (surfaceClass == MacroSurfaceClass::Ocean) {
        result.forestCoverage01 = 0.0;
        result.grassCoverage01 = 0.0;
        result.shrubCoverage01 = 0.0;
        result.rockCoverage01 = 0.0;
        result.wetlandCoverage01 = 0.0;
    } else if (surfaceClass == MacroSurfaceClass::Coast) {
        result.forestCoverage01 *= 0.38;
        result.grassCoverage01 *= 0.72;
        result.shrubCoverage01 *= 0.66;
        result.rockCoverage01 = clampMacro01(
            result.rockCoverage01 + 0.12);
        result.wetlandCoverage01 *= 0.55;
    }

    result.biome =
        classifyContinuousEcologyBiome(result, surfaceClass);
    return result;
}

inline bool validContinuousEcologySample(
    const ContinuousEcologySample& sample)
{
    const auto inRange = [](double value) {
        return value >= 0.0 && value <= 1.0 && std::isfinite(value);
    };

    return inRange(sample.elevation01)
        && inRange(sample.slope01)
        && inRange(sample.moisture01)
        && inRange(sample.temperature01)
        && inRange(sample.waterInfluence01)
        && inRange(sample.forestCoverage01)
        && inRange(sample.grassCoverage01)
        && inRange(sample.shrubCoverage01)
        && inRange(sample.rockCoverage01)
        && inRange(sample.wetlandCoverage01);
}

} // namespace lifelens
