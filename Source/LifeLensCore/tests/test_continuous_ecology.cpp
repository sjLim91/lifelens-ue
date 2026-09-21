#include "lifelens/ContinuousEcology.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>

using namespace lifelens;

int main()
{
    const WorldSeed seed = 0x32541abcULL;
    const WorldGenesisIdentity a =
        makeWorldGenesisIdentity(seed, 111, ContinuousTerrainGenerationVersion);
    const WorldGenesisIdentity b =
        makeWorldGenesisIdentity(seed, 999, ContinuousTerrainGenerationVersion);

    std::map<ContinuousEcologyBiome,int> biomeCounts;
    double minForest = 1.0;
    double maxForest = 0.0;
    double minRock = 1.0;
    double maxRock = 0.0;

    for (int cy = -28; cy <= 28; cy += 2) {
        for (int cx = -28; cx <= 28; cx += 2) {
            const GridPos p{
                cx * WorldChunkSpanGridCells + WorldChunkSpanGridCells / 2,
                cy * WorldChunkSpanGridCells + WorldChunkSpanGridCells / 2
            };
            const ContinuousEcologySample ea =
                deriveContinuousEcologySample(a, p);
            const ContinuousEcologySample eb =
                deriveContinuousEcologySample(b, p);

            assert(validContinuousEcologySample(ea));

            // Population randomization must not alter the natural baseline.
            assert(ea.elevation01 == eb.elevation01);
            assert(ea.slope01 == eb.slope01);
            assert(ea.moisture01 == eb.moisture01);
            assert(ea.temperature01 == eb.temperature01);
            assert(ea.waterInfluence01 == eb.waterInfluence01);
            assert(ea.forestCoverage01 == eb.forestCoverage01);
            assert(ea.grassCoverage01 == eb.grassCoverage01);
            assert(ea.shrubCoverage01 == eb.shrubCoverage01);
            assert(ea.rockCoverage01 == eb.rockCoverage01);
            assert(ea.wetlandCoverage01 == eb.wetlandCoverage01);
            assert(ea.biome == eb.biome);

            ++biomeCounts[ea.biome];
            minForest = std::min(minForest, ea.forestCoverage01);
            maxForest = std::max(maxForest, ea.forestCoverage01);
            minRock = std::min(minRock, ea.rockCoverage01);
            maxRock = std::max(maxRock, ea.rockCoverage01);

            if (ea.biome == ContinuousEcologyBiome::Ocean) {
                assert(ea.forestCoverage01 == 0.0);
                assert(ea.grassCoverage01 == 0.0);
                assert(ea.shrubCoverage01 == 0.0);
            }
        }
    }

    // Coverage must actually vary. A constant green carpet is a regression.
    assert(maxForest - minForest > 0.15);
    assert(maxRock - minRock > 0.10);
    assert(biomeCounts.size() >= 3);
    assert(biomeCounts[ContinuousEcologyBiome::Ocean] > 0);
    assert(
        biomeCounts[ContinuousEcologyBiome::TemperateForest]
        + biomeCounts[ContinuousEcologyBiome::Meadow]
        + biomeCounts[ContinuousEcologyBiome::Plains]
        + biomeCounts[ContinuousEcologyBiome::DryScrub]
        + biomeCounts[ContinuousEcologyBiome::ColdSteppe]
        + biomeCounts[ContinuousEcologyBiome::RockyHighland]
        > 0);

    // Inland coverage must cross a logical chunk border smoothly. Marine/land
    // topology can legitimately introduce a hard ecological boundary, so find
    // one land-land edge and validate the continuous fields there.
    bool checkedLandEdge = false;
    for (int cy = -16; cy <= 16 && !checkedLandEdge; ++cy) {
        for (int cx = -16; cx <= 16 && !checkedLandEdge; ++cx) {
            const ChunkCoord leftChunk{cx,cy};
            const ChunkCoord rightChunk{cx+1,cy};
            if (deriveContinuousHydrologySurfaceFacts(a,leftChunk).surfaceClass
                    != MacroSurfaceClass::Land
                || deriveContinuousHydrologySurfaceFacts(a,rightChunk).surfaceClass
                    != MacroSurfaceClass::Land) {
                continue;
            }

            const int edgeX = (cx + 1) * WorldChunkSpanGridCells;
            const int y = cy * WorldChunkSpanGridCells
                + WorldChunkSpanGridCells / 2;
            const ContinuousEcologySample left =
                deriveContinuousEcologySample(a,{edgeX-1,y});
            const ContinuousEcologySample right =
                deriveContinuousEcologySample(a,{edgeX,y});

            assert(std::abs(left.moisture01-right.moisture01) < 0.08);
            assert(std::abs(left.temperature01-right.temperature01) < 0.08);
            assert(std::abs(left.forestCoverage01-right.forestCoverage01) < 0.18);
            assert(std::abs(left.grassCoverage01-right.grassCoverage01) < 0.18);
            checkedLandEdge = true;
        }
    }
    assert(checkedLandEdge);

    // Different worlds must differ in ecology somewhere.
    const WorldGenesisIdentity other =
        makeWorldGenesisIdentity(seed + 1,111,ContinuousTerrainGenerationVersion);
    bool differs = false;
    for (int y = -6; y <= 6 && !differs; ++y) {
        for (int x = -6; x <= 6 && !differs; ++x) {
            const GridPos p{
                x * WorldChunkSpanGridCells,
                y * WorldChunkSpanGridCells
            };
            const ContinuousEcologySample e0 =
                deriveContinuousEcologySample(a,p);
            const ContinuousEcologySample e1 =
                deriveContinuousEcologySample(other,p);
            differs =
                e0.forestCoverage01 != e1.forestCoverage01
                || e0.moisture01 != e1.moisture01
                || e0.biome != e1.biome;
        }
    }
    assert(differs);

    return 0;
}
