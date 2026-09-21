#include "lifelens/ContinuousTerrain.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

using namespace lifelens;

int main()
{
    const WorldSeed seed = 0x514e93ab71ULL;
    const WorldGenesisIdentity a =
        makeWorldGenesisIdentity(seed, 111, ContinuousTerrainGenerationVersion);
    const WorldGenesisIdentity b =
        makeWorldGenesisIdentity(seed, 222, ContinuousTerrainGenerationVersion);

    const std::vector<GridPos> probes = {
        {0, 0},
        {1, 1},
        {-1, -1},
        {WorldChunkSpanGridCells - 1, 7},
        {WorldChunkSpanGridCells, 7},
        {-WorldChunkSpanGridCells, -13},
        {WorldChunkSpanGridCells * 17 + 5, WorldChunkSpanGridCells * -9 + 11}
    };

    // Natural geography is deterministic and independent of PopulationSeed.
    for (const GridPos p : probes)
    {
        const double ea = deriveContinuousSurfaceElevation01(a, p);
        const double eb = deriveContinuousSurfaceElevation01(b, p);
        assert(ea == eb);
        assert(ea >= 0.0 && ea <= 1.0);

        const ContinuousTerrainSample sample =
            deriveContinuousTerrainSample(a, p);
        assert(sample.grid.x == p.x && sample.grid.y == p.y);
        assert(sample.chunk == chunkCoordForGrid(p));
        assert(sample.elevation01 == ea);
        assert(std::isfinite(sample.gradientXPerGrid));
        assert(std::isfinite(sample.gradientYPerGrid));
    }

    // Old generation versions remain replay-compatible: they still expose the
    // chunk-level macro baseline until the full v3 migration is activated.
    const WorldGenesisIdentity v2 =
        makeWorldGenesisIdentity(seed, 111, 2);
    for (const GridPos p : probes)
    {
        assert(deriveContinuousSurfaceElevation01(v2, p)
            == deriveMacroRegionFacts(v2, chunkCoordForGrid(p)).elevation);
    }

    // Crossing a logical chunk edge must not cause an artificial terrain jump.
    const int edgeX = WorldChunkSpanGridCells * 3;
    for (int y = -WorldChunkSpanGridCells * 2;
         y <= WorldChunkSpanGridCells * 2;
         y += 7)
    {
        const double left =
            deriveContinuousSurfaceElevation01(a, {edgeX - 1, y});
        const double edge =
            deriveContinuousSurfaceElevation01(a, {edgeX, y});
        const double right =
            deriveContinuousSurfaceElevation01(a, {edgeX + 1, y});
        assert(std::abs(edge - left) < 0.08);
        assert(std::abs(right - edge) < 0.08);
    }

    // The field must have meaningful large-scale relief; it is not a flat
    // bootstrap plate disguised as a continuous API.
    double minimum = std::numeric_limits<double>::max();
    double maximum = std::numeric_limits<double>::lowest();
    for (int cy = -64; cy <= 64; cy += 4)
    {
        for (int cx = -64; cx <= 64; cx += 4)
        {
            const GridPos p{
                cx * WorldChunkSpanGridCells + WorldChunkSpanGridCells / 2,
                cy * WorldChunkSpanGridCells + WorldChunkSpanGridCells / 2
            };
            const double value = deriveContinuousSurfaceElevation01(a, p);
            minimum = std::min(minimum, value);
            maximum = std::max(maximum, value);
        }
    }
    assert(maximum - minimum > 0.12);

    // Different WorldSeed must change geography somewhere in the same address set.
    const WorldGenesisIdentity other =
        makeWorldGenesisIdentity(seed + 1, 111, ContinuousTerrainGenerationVersion);
    bool foundDifference = false;
    for (const GridPos p : probes)
    {
        if (deriveContinuousSurfaceElevation01(a, p)
            != deriveContinuousSurfaceElevation01(other, p))
        {
            foundDifference = true;
            break;
        }
    }
    assert(foundDifference);

    return 0;
}
