#pragma once

#include <cstdint>

#include "SmartObject.h"

namespace lifelens {

using WorldSeed = std::uint64_t;
using PopulationSeed = std::uint64_t;
using WorldGenerationVersion = std::uint32_t;

inline constexpr WorldGenerationVersion CurrentWorldGenerationVersion = 1;

// Logical Core grid cells per chunk. This is a simulation-coordinate contract,
// not a statement that one grid cell equals one physical meter in presentation.
inline constexpr int WorldChunkSpanGridCells = 32;

struct ChunkCoord {
    int x = 0;
    int y = 0;

    friend bool operator==(ChunkCoord a, ChunkCoord b)
    {
        return a.x == b.x && a.y == b.y;
    }

    friend bool operator!=(ChunkCoord a, ChunkCoord b)
    {
        return !(a == b);
    }

    friend bool operator<(ChunkCoord a, ChunkCoord b)
    {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
    }
};

struct WorldGenesisIdentity {
    WorldSeed worldSeed = 1;
    PopulationSeed populationSeed = 1;
    WorldGenerationVersion generationVersion = CurrentWorldGenerationVersion;
};

struct UntouchedChunkBaseline {
    ChunkCoord coord{};
    std::uint64_t chunkSeed = 0;
    std::uint64_t terrainStreamSeed = 0;
    std::uint64_t climateStreamSeed = 0;
    std::uint64_t resourceStreamSeed = 0;
    std::uint64_t detailStreamSeed = 0;

    friend bool operator==(const UntouchedChunkBaseline& a, const UntouchedChunkBaseline& b)
    {
        return a.coord == b.coord
            && a.chunkSeed == b.chunkSeed
            && a.terrainStreamSeed == b.terrainStreamSeed
            && a.climateStreamSeed == b.climateStreamSeed
            && a.resourceStreamSeed == b.resourceStreamSeed
            && a.detailStreamSeed == b.detailStreamSeed;
    }

    friend bool operator!=(const UntouchedChunkBaseline& a, const UntouchedChunkBaseline& b)
    {
        return !(a == b);
    }
};

inline WorldSeed normalizeWorldSeed(WorldSeed seed)
{
    return seed == 0 ? 1 : seed;
}

inline WorldGenerationVersion normalizeWorldGenerationVersion(WorldGenerationVersion version)
{
    return version == 0 ? CurrentWorldGenerationVersion : version;
}

// Fixed SplitMix64 finalizer. Unlike std::hash this contract is intentionally
// specified and therefore stable across platforms/processes for a given input.
inline std::uint64_t worldGenesisMix64(std::uint64_t value)
{
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

inline std::uint64_t stableSignedCoordinateWord(int value)
{
    const std::int64_t wide = static_cast<std::int64_t>(value);
    return wide >= 0
        ? static_cast<std::uint64_t>(wide) * 2ULL
        : static_cast<std::uint64_t>(-(wide + 1)) * 2ULL + 1ULL;
}

inline std::uint64_t combineWorldGenesisWord(std::uint64_t state, std::uint64_t value)
{
    return worldGenesisMix64(state ^ worldGenesisMix64(value));
}

inline PopulationSeed deriveDefaultPopulationSeed(WorldSeed worldSeed)
{
    constexpr std::uint64_t PopulationDomain = 0x504f50554c415449ULL; // "POPULATI"
    return worldGenesisMix64(normalizeWorldSeed(worldSeed) ^ PopulationDomain);
}

inline WorldGenesisIdentity makeWorldGenesisIdentity(
    WorldSeed worldSeed,
    PopulationSeed populationSeed = 0,
    WorldGenerationVersion generationVersion = CurrentWorldGenerationVersion)
{
    WorldGenesisIdentity identity;
    identity.worldSeed = normalizeWorldSeed(worldSeed);
    identity.populationSeed = populationSeed == 0
        ? deriveDefaultPopulationSeed(identity.worldSeed)
        : populationSeed;
    identity.generationVersion = normalizeWorldGenerationVersion(generationVersion);
    return identity;
}

inline int floorDivWorldGrid(int value, int positiveDivisor)
{
    const std::int64_t divisor = positiveDivisor > 0 ? positiveDivisor : 1;
    const std::int64_t wide = value;
    std::int64_t quotient = wide / divisor;
    const std::int64_t remainder = wide % divisor;
    if (remainder < 0) --quotient;
    return static_cast<int>(quotient);
}

inline ChunkCoord chunkCoordForGrid(GridPos position)
{
    return {
        floorDivWorldGrid(position.x, WorldChunkSpanGridCells),
        floorDivWorldGrid(position.y, WorldChunkSpanGridCells)
    };
}

inline GridPos chunkOriginGrid(ChunkCoord coord)
{
    return {
        coord.x * WorldChunkSpanGridCells,
        coord.y * WorldChunkSpanGridCells
    };
}

inline GridPos chunkLocalGrid(GridPos position)
{
    const ChunkCoord coord = chunkCoordForGrid(position);
    const GridPos origin = chunkOriginGrid(coord);
    return {position.x - origin.x, position.y - origin.y};
}

inline std::uint64_t deriveChunkSeed(
    WorldSeed worldSeed,
    ChunkCoord coord,
    WorldGenerationVersion generationVersion = CurrentWorldGenerationVersion)
{
    constexpr std::uint64_t ChunkDomain = 0x4348554e4b5f5731ULL; // "CHUNK_W1"
    std::uint64_t state = worldGenesisMix64(normalizeWorldSeed(worldSeed) ^ ChunkDomain);
    state = combineWorldGenesisWord(state, normalizeWorldGenerationVersion(generationVersion));
    state = combineWorldGenesisWord(state, stableSignedCoordinateWord(coord.x));
    state = combineWorldGenesisWord(state, stableSignedCoordinateWord(coord.y));
    return state;
}

inline std::uint64_t deriveChunkStreamSeed(std::uint64_t chunkSeed, std::uint64_t domain)
{
    return worldGenesisMix64(chunkSeed ^ domain);
}

inline UntouchedChunkBaseline deriveUntouchedChunkBaseline(
    WorldSeed worldSeed,
    ChunkCoord coord,
    WorldGenerationVersion generationVersion = CurrentWorldGenerationVersion)
{
    constexpr std::uint64_t TerrainDomain = 0x5445525241494e31ULL; // "TERRAIN1"
    constexpr std::uint64_t ClimateDomain = 0x434c494d41544531ULL; // "CLIMATE1"
    constexpr std::uint64_t ResourceDomain = 0x5245534f55524331ULL; // "RESOURC1"
    constexpr std::uint64_t DetailDomain = 0x44455441494c5f31ULL; // "DETAIL_1"

    UntouchedChunkBaseline baseline;
    baseline.coord = coord;
    baseline.chunkSeed = deriveChunkSeed(worldSeed, coord, generationVersion);
    baseline.terrainStreamSeed = deriveChunkStreamSeed(baseline.chunkSeed, TerrainDomain);
    baseline.climateStreamSeed = deriveChunkStreamSeed(baseline.chunkSeed, ClimateDomain);
    baseline.resourceStreamSeed = deriveChunkStreamSeed(baseline.chunkSeed, ResourceDomain);
    baseline.detailStreamSeed = deriveChunkStreamSeed(baseline.chunkSeed, DetailDomain);
    return baseline;
}

inline UntouchedChunkBaseline deriveUntouchedChunkBaseline(
    const WorldGenesisIdentity& identity,
    ChunkCoord coord)
{
    // PopulationSeed is deliberately excluded. Natural geography must stay the
    // same when replaying a world with different initial residents.
    return deriveUntouchedChunkBaseline(identity.worldSeed, coord, identity.generationVersion);
}

} // namespace lifelens
