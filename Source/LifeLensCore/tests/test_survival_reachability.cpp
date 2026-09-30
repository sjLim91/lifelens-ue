#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/CoreNavigation.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

static std::vector<std::uint8_t> encode(const Simulation& simulation)
{
    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(simulation.captureSnapshot(),bytes,&error));
    return bytes;
}

int main()
{
    // Production seed 42 used to strand all four residents by day 10. The
    // nearest food access is dry but lies across an impassable water barrier.
    Simulation simulation(42);
    simulation.setupNewGame();
    auto snapshot=simulation.captureSnapshot();
    const CharacterId id=snapshot.world.characters.front().id;
    const GridPos stranded{-189,593};
    const GridPos inaccessible{-190,587};
    snapshot.runtime.at(id).pos=stranded;
    auto& resident=snapshot.world.characters.front();
    resident.needs={0.95,0.20,0.20,0.20,0.20};
    resident.civilization.inventory=Inventory{};
    std::string error;
    assert(simulation.restoreSnapshot(snapshot,&error));

    std::vector<GridPos> route;
    assert(coreGroundTraversable(simulation.world(),inaccessible));
    assert(!buildCoreGroundRoute(
        simulation.world(),stranded,inaccessible,1,route));

    const auto decision=urgentSurvivalProvisionDecisionAtPosition(
        simulation.world(),simulation.world().characters.front(),stranded);
    assert(decision.intent==CivilizationIntent::Gather);
    GridPos access{};
    assert(resolveCivilizationResourceAccessGridPosition(
        simulation.world(),decision.resourceNode,access));
    assert(buildCoreGroundRoute(
        simulation.world(),stranded,access,1,route));

    // A closer disconnected store must not displace an accessible provision.
    StorageSite blocked;
    blocked.id=99001;
    blocked.pos=inaccessible;
    blocked.inventory.add({ItemKind::RawMaterial,MaterialKind::PlantFood,4,0.5,1.0});
    simulation.world().storageSites.push_back(blocked);
    const auto withoutAccessibleStorage=urgentSurvivalProvisionDecisionAtPosition(
        simulation.world(),simulation.world().characters.front(),stranded);
    assert(withoutAccessibleStorage.intent==CivilizationIntent::Gather);

    StorageSite reachable=blocked;
    reachable.id=99002;
    reachable.pos={-176,590};
    assert(buildCoreGroundRoute(
        simulation.world(),stranded,reachable.pos,1,route));
    simulation.world().storageSites.push_back(reachable);
    const auto retrieval=urgentSurvivalProvisionDecisionAtPosition(
        simulation.world(),simulation.world().characters.front(),stranded);
    assert(retrieval.intent==CivilizationIntent::Retrieve);
    assert(retrieval.storage==reachable.id);
    simulation.world().storageSites.clear();

    // Exercise real movement, gathering and consumption; no relocation or
    // fabricated provisions may be used to recover the stranded resident.
    const auto encoded=encode(simulation);
    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(encoded,decoded,&error));
    Simulation resumed(1);
    assert(resumed.restoreSnapshot(decoded,&error));
    GridPos previous=stranded;
    bool ate=false;
    for(int minute=0;minute<720;++minute){
        simulation.step();
        resumed.step();
        GridPos current{};
        assert(simulation.runtimePosition(id,current));
        assert(manhattan(previous,current)<=1);
        previous=current;
        if(simulation.world().characters.front().needs.hunger<0.70) ate=true;
    }
    assert(ate);
    assert(encode(simulation)==encode(resumed));
    std::cout << "LifeLens reachable survival provisions: PASS\n";
}
