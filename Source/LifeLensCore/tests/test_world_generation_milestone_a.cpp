#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/WorldGenerationSnapshotCodec.h"

#include <cassert>
#include <string>
#include <vector>

using namespace lifelens;

namespace {

bool sameResourceNodes(const World& a, const World& b)
{
    if(a.resourceNodes.size()!=b.resourceNodes.size()) return false;
    for(std::size_t i=0;i<a.resourceNodes.size();++i){
        const auto& x=a.resourceNodes[i];
        const auto& y=b.resourceNodes[i];
        if(x.id!=y.id || x.material!=y.material || x.quantity!=y.quantity
           || x.maxQuantity!=y.maxQuantity || x.renewable!=y.renewable
           || x.regenerationPerDay!=y.regenerationPerDay) return false;
    }
    return true;
}

} // namespace

int main()
{
    const WorldSeed seed=874213954ULL;
    const PopulationSeed peopleA=111ULL;
    const PopulationSeed peopleB=222ULL;

    // Natural detail is independent of population randomness.
    const WorldGenesisIdentity identityA=makeWorldGenesisIdentity(seed,peopleA,1);
    const WorldGenesisIdentity identityB=makeWorldGenesisIdentity(seed,peopleB,1);
    const std::vector<ChunkCoord> probes={{0,0},{-1,2},{6,-7},{12,12}};
    for(const ChunkCoord coord:probes){
        const GeneratedNaturalChunk a=deriveGeneratedNaturalChunk(identityA,coord,480);
        const GeneratedNaturalChunk b=deriveGeneratedNaturalChunk(identityB,coord,480);
        assert(sameGeneratedNaturalChunkBaseline(a,b));
        assert(validGeneratedNaturalChunk(a));
    }

    Simulation simulation(seed,peopleA,1);
    simulation.setupNewGame();
    World& world=simulation.world();
    const InitialStartRegionSelection selected=selectInitialStartRegion(identityA);

    assert(world.hasInitialStartRegionSelection);
    assert(world.initialStartRegionCoord==selected.region.coord);
    assert(world.initialStartRegionViability==selected.viability);
    assert(world.generatedNaturalChunks.size()==1);
    assert(world.storageSites.empty());
    assert(world.objects.empty());
    assert(world.primitiveSanitationSites.empty());

    const GeneratedNaturalChunk* startChunk=world.findGeneratedNaturalChunk(selected.region.coord);
    assert(startChunk!=nullptr);
    assert(!startChunk->resourcePatches.empty());
    assert(world.resourceNodes.size()==startChunk->resourcePatches.size());
    assert(validateWorldGenerationSnapshotState(world));

    // Founders now occupy authoritative positions inside the selected global
    // start chunk instead of all defaulting to {0,0}.
    for(const Character& founder:world.characters){
        GridPos pos{};
        assert(simulation.runtimePosition(founder.id,pos));
        assert(chunkCoordForGrid(pos)==selected.region.coord);
    }

    // Materialization order cannot change untouched detail or global resource ordering.
    World orderA(seed,peopleA,1);
    World orderB(seed,peopleB,1);
    orderA.resourceNodes.clear(); orderA.generatedNaturalChunks.clear();
    orderB.resourceNodes.clear(); orderB.generatedNaturalChunks.clear();
    orderA.materializeNaturalChunk({3,-2});
    orderA.materializeNaturalChunk({-4,5});
    orderB.materializeNaturalChunk({-4,5});
    orderB.materializeNaturalChunk({3,-2});
    assert(sameGeneratedNaturalChunkBaseline(
        *orderA.findGeneratedNaturalChunk({3,-2}),
        *orderB.findGeneratedNaturalChunk({3,-2})));
    assert(sameGeneratedNaturalChunkBaseline(
        *orderA.findGeneratedNaturalChunk({-4,5}),
        *orderB.findGeneratedNaturalChunk({-4,5})));
    assert(sameResourceNodes(orderA,orderB));

    // Depletion is history, not generator input. Save/load must restore the exact
    // generated registry and current resource quantity without rerolling patches.
    assert(!world.resourceNodes.empty());
    const ResourceNodeId depletedId=world.resourceNodes.front().id;
    world.resourceNodes.front().quantity=std::max(0,world.resourceNodes.front().quantity-7);
    const int depletedQuantity=world.resourceNodes.front().quantity;
    const GeneratedNaturalChunk baselineBefore=*startChunk;

    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(simulation.captureSnapshot(),bytes,&error));
    assert(error.empty());
    assert(bytes.size()>64);

    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    assert(error.empty());
    assert(decoded.world.populationSeed==peopleA);
    assert(decoded.world.generationVersion==1);
    assert(decoded.world.hasInitialStartRegionSelection);
    assert(decoded.world.initialStartRegionCoord==selected.region.coord);
    const GeneratedNaturalChunk* decodedChunk=decoded.world.findGeneratedNaturalChunk(selected.region.coord);
    assert(decodedChunk!=nullptr);
    assert(sameGeneratedNaturalChunkBaseline(baselineBefore,*decodedChunk));
    const ResourceNode* decodedResource=findWorldGenerationResourceNode(decoded.world,depletedId);
    assert(decodedResource!=nullptr);
    assert(decodedResource->quantity==depletedQuantity);

    Simulation restored(1);
    assert(restored.restoreSnapshot(decoded,&error));
    GridPos restoredPos{};
    assert(restored.runtimePosition(restored.world().characters.front().id,restoredPos));
    assert(chunkCoordForGrid(restoredPos)==selected.region.coord);

    std::vector<std::uint8_t> reencoded;
    assert(encodeSimulationSnapshot(restored.captureSnapshot(),reencoded,&error));
    assert(bytes==reencoded);

    // Same world, different people: terrain/resources stay exact while founder
    // randomization remains free to differ.
    Simulation otherPeople(seed,peopleB,1);
    otherPeople.setupNewGame();
    const GeneratedNaturalChunk* otherChunk=otherPeople.world().findGeneratedNaturalChunk(selected.region.coord);
    assert(otherChunk!=nullptr);
    assert(sameGeneratedNaturalChunkBaseline(baselineBefore,*otherChunk));

    return 0;
}
