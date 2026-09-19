#include "lifelens/WorldHierarchy.h"

#include <cassert>

using namespace lifelens;

int main()
{
    const WorldGenesisIdentity peopleA =
        makeWorldGenesisIdentity(874213954ULL, 111ULL, CurrentWorldGenerationVersion);
    const WorldGenesisIdentity peopleB =
        makeWorldGenesisIdentity(874213954ULL, 222ULL, CurrentWorldGenerationVersion);
    const WorldGenesisIdentity otherWorld =
        makeWorldGenesisIdentity(874213955ULL, 111ULL, CurrentWorldGenerationVersion);

    const PlanetIdentity planetA = derivePrimaryPlanetIdentity(peopleA);
    const PlanetIdentity planetB = derivePrimaryPlanetIdentity(peopleB);
    const PlanetIdentity planetOther = derivePrimaryPlanetIdentity(otherWorld);

    assert(validPlanetIdentity(planetA));
    assert(planetA.id == planetB.id);
    assert(planetA.seed == planetB.seed);
    assert(planetA.id != planetOther.id || planetA.seed != planetOther.seed);

    assert(surfaceRegionCoordForChunk({0,0}) == (SurfaceRegionCoord{0,0}));
    assert(surfaceRegionCoordForChunk({31,31}) == (SurfaceRegionCoord{0,0}));
    assert(surfaceRegionCoordForChunk({32,0}) == (SurfaceRegionCoord{1,0}));
    assert(surfaceRegionCoordForChunk({-1,-1}) == (SurfaceRegionCoord{-1,-1}));
    assert(surfaceRegionCoordForChunk({-32,-32}) == (SurfaceRegionCoord{-1,-1}));
    assert(surfaceRegionCoordForChunk({-33,-1}) == (SurfaceRegionCoord{-2,-1}));

    const SurfaceRegionIdentity regionA =
        deriveSurfaceRegionIdentityForChunk(peopleA, {67,-2});
    const SurfaceRegionIdentity regionB =
        deriveSurfaceRegionIdentityForChunk(peopleB, {67,-2});
    const SurfaceRegionIdentity regionNeighbour =
        deriveSurfaceRegionIdentityForChunk(peopleA, {68,-2});

    assert(validSurfaceRegionIdentity(regionA));
    assert(regionA.id == regionB.id);
    assert(regionA.seed == regionB.seed);
    assert(regionA.coord == (SurfaceRegionCoord{2,-1}));
    assert(regionA.id == regionNeighbour.id);
    assert(regionA.seed == regionNeighbour.seed);

    static_assert(
        static_cast<int>(ObserverWorldScale::LocalSurface)
        < static_cast<int>(ObserverWorldScale::Regional));
    static_assert(
        static_cast<int>(ObserverWorldScale::Regional)
        < static_cast<int>(ObserverWorldScale::Planetary));
    static_assert(
        static_cast<int>(ObserverWorldScale::Planetary)
        < static_cast<int>(ObserverWorldScale::Orbital));
    static_assert(
        static_cast<int>(ObserverWorldScale::Orbital)
        < static_cast<int>(ObserverWorldScale::Interplanetary));

    return 0;
}
