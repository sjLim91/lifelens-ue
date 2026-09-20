#include "lifelens/MacroWorldGenesis.h"
#include "lifelens/Simulation.h"

#include <cassert>
#include <cmath>
#include <vector>

using namespace lifelens;

namespace {

bool sameMacroFacts(const MacroRegionFacts& a,const MacroRegionFacts& b)
{
    return a.coord==b.coord
        && a.elevation==b.elevation
        && a.moisture==b.moisture
        && a.temperature==b.temperature
        && a.waterPotential==b.waterPotential
        && a.fertilityPotential==b.fertilityPotential
        && a.woodPotential==b.woodPotential
        && a.stonePotential==b.stonePotential
        && a.foodPotential==b.foodPotential
        && a.traversalEase==b.traversalEase
        && a.hazardPotential==b.hazardPotential
        && a.biome==b.biome;
}

void assertUnitRange(double value)
{
    assert(value>=0.0);
    assert(value<=1.0);
}

} // namespace

int main()
{
    const WorldSeed seed=874213954ULL;
    const WorldGenesisIdentity peopleA=makeWorldGenesisIdentity(seed,111,1);
    const WorldGenesisIdentity peopleB=makeWorldGenesisIdentity(seed,222,1);

    // Population randomization must not move climate/geography/resource potential.
    const std::vector<ChunkCoord> probes={
        {0,0},{1,0},{-1,0},{6,7},{-9,4},{12,-12},{-12,12}
    };
    for(const ChunkCoord coord:probes){
        const MacroRegionFacts a=deriveMacroRegionFacts(peopleA,coord);
        const MacroRegionFacts b=deriveMacroRegionFacts(peopleB,coord);
        assert(sameMacroFacts(a,b));
        assertUnitRange(a.elevation);
        assertUnitRange(a.moisture);
        assertUnitRange(a.temperature);
        assertUnitRange(a.waterPotential);
        assertUnitRange(a.fertilityPotential);
        assertUnitRange(a.woodPotential);
        assertUnitRange(a.stonePotential);
        assertUnitRange(a.foodPotential);
        assertUnitRange(a.traversalEase);
        assertUnitRange(a.hazardPotential);
        assertUnitRange(scoreInitialStartRegion(a));
    }

    // Coarse value-noise fields are spatially coherent rather than independent
    // per-chunk dice rolls. Immediate neighbours cannot jump across the full range.
    const MacroRegionFacts center=deriveMacroRegionFacts(peopleA,{3,-4});
    const MacroRegionFacts east=deriveMacroRegionFacts(peopleA,{4,-4});
    assert(std::abs(center.elevation-east.elevation)<0.35);
    assert(std::abs(center.moisture-east.moisture)<0.35);
    assert(std::abs(center.temperature-east.temperature)<0.35);

    const InitialStartRegionSelection startA=selectInitialStartRegion(peopleA);
    const InitialStartRegionSelection startB=selectInitialStartRegion(peopleB);
    assert(startA.region.coord==startB.region.coord);
    assert(sameMacroFacts(startA.region,startB.region));
    assert(startA.viability==startB.viability);
    assert(startA.evaluatedCandidates==(MacroStartSearchRadiusChunks*2+1)*(MacroStartSearchRadiusChunks*2+1));
    assert((startA.region.coord!=ChunkCoord{0,0}));

    // The selected location must actually beat the arbitrary origin for this
    // deterministic replay seed and satisfy basic early-survival potential.
    const MacroRegionFacts origin=deriveMacroRegionFacts(peopleA,{0,0});
    assert(startA.viability>=scoreInitialStartRegion(origin));
    assert(startA.region.waterPotential>=0.35);
    assert(startA.region.foodPotential>=0.30);
    assert(startA.region.woodPotential>=0.22);
    assert(startA.region.stonePotential>=0.18);

    const WorldGenesisIdentity anotherWorld=makeWorldGenesisIdentity(seed+1,111,1);
    const MacroRegionFacts different=deriveMacroRegionFacts(anotherWorld,{3,-4});
    assert(!sameMacroFacts(center,different));

    const WorldGenesisIdentity currentPeopleA =
        makeWorldGenesisIdentity(seed,111,CurrentWorldGenerationVersion);
    const WorldGenesisIdentity currentPeopleB =
        makeWorldGenesisIdentity(seed,222,CurrentWorldGenerationVersion);
    const InitialStartRegionSelection currentStartA =
        selectInitialFreshwaterAdjacentRegion(currentPeopleA);
    const InitialStartRegionSelection currentStartB =
        selectInitialFreshwaterAdjacentRegion(currentPeopleB);
    assert(currentStartA.region.coord == currentStartB.region.coord);
    assert(deriveHydrologyFacts(
        currentPeopleA,currentStartA.region.coord).surfaceKind == SurfaceWaterKind::None);

    ChunkCoord currentFreshwaterA{};
    ChunkCoord currentFreshwaterB{};
    assert(findNearestFreshSurfaceWaterChunk(
        currentPeopleA,
        currentStartA.region.coord,
        currentFreshwaterA,
        FreshSurfaceNeighbourRadiusChunks));
    assert(findNearestFreshSurfaceWaterChunk(
        currentPeopleB,
        currentStartB.region.coord,
        currentFreshwaterB,
        FreshSurfaceNeighbourRadiusChunks));
    assert(currentFreshwaterA == currentFreshwaterB);
    assert(isFreshSurfaceWater(
        deriveHydrologyFacts(currentPeopleA,currentFreshwaterA)));

    Simulation currentSimulation(
        seed,111,CurrentWorldGenerationVersion);
    currentSimulation.setupNewGame();
    assert(currentSimulation.world().initialStartRegion().region.coord
        == currentStartA.region.coord);
    assert(currentSimulation.world().findGeneratedNaturalChunk(
        currentStartA.region.coord) != nullptr);
    assert(currentSimulation.world().findGeneratedNaturalChunk(
        currentFreshwaterA) != nullptr);

    const GridPos currentWaterOrigin =
        chunkOriginGrid(currentFreshwaterA);
    const GridPos currentWaterCenter = {
        currentWaterOrigin.x + WorldChunkSpanGridCells / 2,
        currentWaterOrigin.y + WorldChunkSpanGridCells / 2
    };
    int alignedFreshWaterNodeCount = 0;
    for(const ResourceNode& node : currentSimulation.world().resourceNodes){
        if(node.material != MaterialKind::Water){
            continue;
        }
        assert(chunkCoordForGrid(node.pos) == currentFreshwaterA);
        assert(node.pos.x == currentWaterCenter.x);
        assert(node.pos.y == currentWaterCenter.y);
        ++alignedFreshWaterNodeCount;
    }
    assert(alignedFreshWaterNodeCount == 1);

    World worldA(seed,111,1);
    World worldB(seed,222,1);
    assert(worldA.initialStartRegion().region.coord==worldB.initialStartRegion().region.coord);
    assert(sameMacroFacts(worldA.macroRegionFacts({6,7}),worldB.macroRegionFacts({6,7})));

    // Production NEW GAME still starts with nature and founders only. Milestone A
    // materializes the WG-2-selected natural chunk and places founder Core positions
    // inside it without manufacturing civilization infrastructure.
    Simulation simulation(seed,111,1);
    simulation.setupNewGame();
    assert(simulation.world().characters.size()==4);
    assert(simulation.world().objects.empty());
    assert(simulation.world().primitiveSanitationSites.empty());
    assert(simulation.world().storageSites.empty());
    assert(simulation.world().initialStartRegion().region.coord==startA.region.coord);
    assert(simulation.world().generatedNaturalChunks.size()==1);
    for(const Character& founder:simulation.world().characters){
        GridPos pos{};
        assert(simulation.runtimePosition(founder.id,pos));
        assert(chunkCoordForGrid(pos)==startA.region.coord);
    }

    return 0;
}
