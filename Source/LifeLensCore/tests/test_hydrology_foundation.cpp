#include "lifelens/Hydrology.h"

#include <cassert>
#include <map>
#include <utility>

using namespace lifelens;

namespace {

bool sameHydrology(const HydrologyFacts& a, const HydrologyFacts& b)
{
    return a.coord == b.coord
        && a.surfaceKind == b.surfaceKind
        && a.salinity == b.salinity
        && a.surfaceWaterId == b.surfaceWaterId
        && a.surfaceAvailability == b.surfaceAvailability
        && a.flowPotential == b.flowPotential
        && a.groundwaterPotential == b.groundwaterPotential
        && a.rechargePotential == b.rechargePotential
        && a.runoffPotential == b.runoffPotential
        && a.hasDownstream == b.hasDownstream
        && a.downstream == b.downstream;
}

} // namespace

int main()
{
    const WorldSeed seed = 874213954ULL;
    const WorldGenesisIdentity peopleA =
        makeWorldGenesisIdentity(seed, 111ULL, CurrentWorldGenerationVersion);
    const WorldGenesisIdentity peopleB =
        makeWorldGenesisIdentity(seed, 222ULL, CurrentWorldGenerationVersion);

    int surfaceCount = 0;
    int flowingCount = 0;
    int freshCount = 0;
    std::map<std::pair<int,int>, WaterBodyId> ids;

    for(int y = -24; y <= 24; ++y){
        for(int x = -24; x <= 24; ++x){
            const ChunkCoord coord{x,y};
            const HydrologyFacts a = deriveHydrologyFacts(peopleA, coord);
            const HydrologyFacts b = deriveHydrologyFacts(peopleB, coord);

            // Population randomization must never move geography/hydrology.
            assert(sameHydrology(a,b));
            assert(validHydrologyFacts(a));

            if(a.surfaceKind != SurfaceWaterKind::None){
                ++surfaceCount;
                assert(a.surfaceWaterId != 0);
                ids[{x,y}] = a.surfaceWaterId;
            }
            if(a.surfaceKind == SurfaceWaterKind::Stream
               || a.surfaceKind == SurfaceWaterKind::River){
                ++flowingCount;
                assert(a.hasDownstream);
            }
            if(isFreshSurfaceWater(a)){
                ++freshCount;
            }
        }
    }

    // The foundation must actually produce usable natural surface water over a
    // broad deterministic area, not merely expose enum names.
    assert(surfaceCount > 0);
    assert(flowingCount > 0);
    assert(freshCount > 0);

    // Stable ids reproduce exactly for the same seed/coord/kind.
    for(const auto& entry : ids){
        const ChunkCoord coord{entry.first.first, entry.first.second};
        const HydrologyFacts replay = deriveHydrologyFacts(peopleA, coord);
        assert(replay.surfaceWaterId == entry.second);
    }

    // A different world must differ in at least one hydrology fact.
    const WorldGenesisIdentity other =
        makeWorldGenesisIdentity(seed + 1ULL, 111ULL, CurrentWorldGenerationVersion);
    bool differs = false;
    for(int y = -8; y <= 8 && !differs; ++y){
        for(int x = -8; x <= 8 && !differs; ++x){
            differs = !sameHydrology(
                deriveHydrologyFacts(peopleA,{x,y}),
                deriveHydrologyFacts(other,{x,y}));
        }
    }
    assert(differs);

    // Coast/Ocean are reserved until the planetary sea-level contract lands.
    // Their salinity rule is already enforced by the validation contract.
    HydrologyFacts invalidOcean;
    invalidOcean.coord = {0,0};
    invalidOcean.surfaceKind = SurfaceWaterKind::Ocean;
    invalidOcean.surfaceWaterId = 1;
    invalidOcean.salinity = WaterSalinity::Fresh;
    assert(!validHydrologyFacts(invalidOcean));

    return 0;
}
