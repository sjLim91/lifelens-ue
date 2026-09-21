#include "lifelens/Hydrology.h"

#include <cassert>
#include <set>
#include <utility>

using namespace lifelens;

int main()
{
    const WorldSeed seed = 0x72a14f339ULL;
    const WorldGenesisIdentity a =
        makeWorldGenesisIdentity(seed, 111, ContinuousTerrainGenerationVersion);
    const WorldGenesisIdentity b =
        makeWorldGenesisIdentity(seed, 999, ContinuousTerrainGenerationVersion);

    int graphEdges = 0;
    int flowingSurface = 0;
    int freshwaterSurface = 0;
    int oceanCount = 0;
    int coastCount = 0;
    int accumulatedCells = 0;

    for(int y = -40; y <= 40; ++y){
        for(int x = -40; x <= 40; ++x){
            const ChunkCoord coord{x,y};
            const HydrologyFacts ha = deriveHydrologyFacts(a, coord);
            const HydrologyFacts hb = deriveHydrologyFacts(b, coord);

            // Population randomization cannot move drainage or water.
            assert(ha.coord == hb.coord);
            assert(ha.surfaceKind == hb.surfaceKind);
            assert(ha.salinity == hb.salinity);
            assert(ha.surfaceWaterId == hb.surfaceWaterId);
            assert(ha.drainageSystemId == hb.drainageSystemId);
            assert(ha.surfaceAvailability == hb.surfaceAvailability);
            assert(ha.flowPotential == hb.flowPotential);
            assert(ha.groundwaterPotential == hb.groundwaterPotential);
            assert(ha.rechargePotential == hb.rechargePotential);
            assert(ha.runoffPotential == hb.runoffPotential);
            assert(ha.drainageAccumulationPotential
                == hb.drainageAccumulationPotential);
            assert(ha.hasDownstream == hb.hasDownstream);
            assert(ha.downstream == hb.downstream);
            assert(validHydrologyFacts(ha));

            ChunkCoord graphDownstream{};
            double graphDrop = 0.0;
            if(tryDeriveContinuousHydrologyDownstream(
                    a, coord, graphDownstream, &graphDrop)){
                ++graphEdges;
                assert(isCardinalNeighbour(coord, graphDownstream));
                assert(graphDrop > 0.0);
                assert(
                    continuousHydrologyElevation01(a, graphDownstream)
                    < continuousHydrologyElevation01(a, coord));
            }

            if(ha.drainageAccumulationPotential > 0.0){
                ++accumulatedCells;
            }

            if(ha.surfaceKind == SurfaceWaterKind::Stream
               || ha.surfaceKind == SurfaceWaterKind::River){
                ++flowingSurface;
                assert(ha.hasDownstream);
                assert(ha.drainageSystemId != 0);
                assert(
                    continuousHydrologyElevation01(a, ha.downstream)
                    < continuousHydrologyElevation01(a, coord));
            }
            if(isFreshSurfaceWater(ha)){
                ++freshwaterSurface;
                assert(ha.drainageSystemId != 0);
            }
            if(ha.surfaceKind == SurfaceWaterKind::Ocean){
                ++oceanCount;
                assert(ha.salinity == WaterSalinity::Salt);
            }
            if(ha.surfaceKind == SurfaceWaterKind::Coast){
                ++coastCount;
                assert(ha.salinity == WaterSalinity::Brackish);
                assert(ha.hasMarineNeighbour);
                assert(
                    continuousHydrologyElevation01(a, ha.marineNeighbour)
                    < MacroSeaLevel01);
            }
        }
    }

    // The continuous graph must be real, varied geography rather than enum-only
    // scaffolding.
    assert(graphEdges > 0);
    assert(accumulatedCells > 0);
    assert(flowingSurface > 0);
    assert(freshwaterSurface > 0);
    assert(oceanCount > 0);
    assert(coastCount > 0);

    // Drainage direction cannot cycle because every graph step must strictly
    // lower the continuous terrain elevation.
    for(int sy = -16; sy <= 16; sy += 4){
        for(int sx = -16; sx <= 16; sx += 4){
            ChunkCoord current{sx,sy};
            std::set<std::pair<int,int>> visited;
            for(int step = 0; step < 256; ++step){
                assert(visited.insert({current.x,current.y}).second);
                if(continuousHydrologyElevation01(a, current)
                    < MacroSeaLevel01){
                    break;
                }

                ChunkCoord next{};
                if(!tryDeriveContinuousHydrologyDownstream(a,current,next)){
                    break;
                }
                assert(
                    continuousHydrologyElevation01(a,next)
                    < continuousHydrologyElevation01(a,current));
                current = next;
            }
        }
    }

    // The same connected drainage path must resolve to the same system id at
    // its immediate next step whenever both chunks expose surface water.
    int checkedConnectedIds = 0;
    for(int y = -24; y <= 24; ++y){
        for(int x = -24; x <= 24; ++x){
            const HydrologyFacts here = deriveHydrologyFacts(a,{x,y});
            if(!here.hasDownstream || here.drainageSystemId == 0) continue;
            const HydrologyFacts down =
                deriveHydrologyFacts(a,here.downstream);
            if(down.drainageSystemId == 0) continue;
            assert(here.drainageSystemId == down.drainageSystemId);
            ++checkedConnectedIds;
        }
    }
    assert(checkedConnectedIds > 0);

    return 0;
}
