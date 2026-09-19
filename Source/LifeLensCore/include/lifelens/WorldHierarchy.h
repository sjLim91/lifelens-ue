#pragma once

#include <algorithm>
#include <cstdint>

#include "WorldGenesis.h"

namespace lifelens {

using CelestialBodyId = std::uint64_t;
using SurfaceRegionId = std::uint64_t;

inline constexpr int SurfaceRegionSpanChunks = 32;

enum class ObserverWorldScale : std::uint8_t {
    LocalSurface = 0,
    Regional,
    Planetary,
    Orbital,
    Interplanetary
};

struct PlanetIdentity {
    CelestialBodyId id = 0;
    std::uint64_t seed = 0;
};

struct SurfaceRegionCoord {
    int x = 0;
    int y = 0;

    friend bool operator==(SurfaceRegionCoord a, SurfaceRegionCoord b)
    {
        return a.x == b.x && a.y == b.y;
    }

    friend bool operator!=(SurfaceRegionCoord a, SurfaceRegionCoord b)
    {
        return !(a == b);
    }

    friend bool operator<(SurfaceRegionCoord a, SurfaceRegionCoord b)
    {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
    }
};

struct SurfaceRegionIdentity {
    SurfaceRegionId id = 0;
    SurfaceRegionCoord coord{};
    std::uint64_t seed = 0;
};

inline PlanetIdentity derivePrimaryPlanetIdentity(
    const WorldGenesisIdentity& identity)
{
    constexpr std::uint64_t PlanetDomain = 0x504c414e45545f31ULL; // "PLANET_1"
    std::uint64_t state = worldGenesisMix64(
        normalizeWorldSeed(identity.worldSeed) ^ PlanetDomain);
    state = combineWorldGenesisWord(
        state,
        normalizeWorldGenerationVersion(identity.generationVersion));

    PlanetIdentity result;
    result.seed = worldGenesisMix64(state ^ 0x534545445f503031ULL);
    result.id = worldGenesisMix64(state ^ 0x49445f5052494d31ULL)
        & 0x3fffffffffffffffULL;
    result.id |= 0x1000000000000000ULL;
    return result;
}

inline SurfaceRegionCoord surfaceRegionCoordForChunk(ChunkCoord chunk)
{
    return {
        floorDivWorldGrid(chunk.x, SurfaceRegionSpanChunks),
        floorDivWorldGrid(chunk.y, SurfaceRegionSpanChunks)
    };
}

inline SurfaceRegionIdentity deriveSurfaceRegionIdentity(
    const WorldGenesisIdentity& identity,
    SurfaceRegionCoord coord)
{
    constexpr std::uint64_t RegionDomain = 0x5355524652454731ULL; // "SURFREG1"
    const PlanetIdentity planet = derivePrimaryPlanetIdentity(identity);

    std::uint64_t state = worldGenesisMix64(planet.seed ^ RegionDomain);
    state = combineWorldGenesisWord(state, stableSignedCoordinateWord(coord.x));
    state = combineWorldGenesisWord(state, stableSignedCoordinateWord(coord.y));

    SurfaceRegionIdentity result;
    result.coord = coord;
    result.seed = worldGenesisMix64(state ^ 0x534545445f523031ULL);
    result.id = worldGenesisMix64(state ^ 0x49445f5245473031ULL)
        & 0x3fffffffffffffffULL;
    result.id |= 0x1800000000000000ULL;
    return result;
}

inline SurfaceRegionIdentity deriveSurfaceRegionIdentityForChunk(
    const WorldGenesisIdentity& identity,
    ChunkCoord chunk)
{
    return deriveSurfaceRegionIdentity(
        identity,
        surfaceRegionCoordForChunk(chunk));
}

inline bool validPlanetIdentity(const PlanetIdentity& identity)
{
    return identity.id != 0 && identity.seed != 0;
}

inline bool validSurfaceRegionIdentity(const SurfaceRegionIdentity& identity)
{
    return identity.id != 0 && identity.seed != 0;
}

} // namespace lifelens
