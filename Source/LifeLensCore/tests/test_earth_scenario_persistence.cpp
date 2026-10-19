#include <algorithm>
#include <cassert>
#include <limits>
#include <vector>
#include "lifelens/EarthNaturalLandMask.h"
#include "lifelens/EarthScenarioSnapshotCodec.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
using namespace lifelens;
int main()
{
    Simulation sim(4242001);
    sim.setupNewGame();
    const auto ordinary=sim.captureSnapshot();
    std::vector<std::uint8_t> oldBytes;
    assert(encodeSimulationSnapshot(ordinary,oldBytes));
    auto bound=ordinary;
    auto& state=bound.world.earthScenario;
    state.available=true;
    state.worldSeed=bound.world.seed;
    state.generationVersion=bound.world.generationVersion;
    state.geographicOrigin={37.0,127.0};
    state.localGridOrigin=bound.world.initialStartRegionCenterGrid();
    state.metersPerGridCell=25.0;
    state.earthDataVersion=EarthLandMaskVersion;
    state.geographySource="natural-earth";
    std::vector<std::uint8_t> bytes;
    assert(encodeSimulationSnapshot(bound,bytes));
    assert(bytes!=oldBytes);
    SimulationStateSnapshot restored;
    assert(decodeSimulationSnapshot(bytes,restored));
    assert(restored.world.earthScenario.available);
    assert(restored.world.earthScenario.worldSeed==bound.world.seed);
    assert(restored.world.earthScenario.geographicOrigin.latitudeDeg==37.0);
    Simulation replay(4242001);
    assert(replay.restoreSnapshot(restored));
    std::vector<std::uint8_t> again;
    assert(encodeSimulationSnapshot(replay.captureSnapshot(),again));
    assert(again==bytes);
    assert(decodeSimulationSnapshot(oldBytes,restored));
    assert(!restored.world.earthScenario.available);
    std::vector<std::uint8_t> legacyAgain;
    assert(encodeSimulationSnapshot(restored,legacyAgain));
    assert(legacyAgain==oldBytes);

    // Corrupt binding is rejected atomically before any old world is replaced.
    auto corrupt=bytes;
    const auto marker=std::search(corrupt.begin(),corrupt.end(),
        EarthScenarioSnapshotMagic,
        EarthScenarioSnapshotMagic+sizeof(EarthScenarioSnapshotMagic));
    assert(marker!=corrupt.end());
    const auto offset=static_cast<std::size_t>(marker-corrupt.begin())+12;
    corrupt[offset]^=1;
    assert(!decodeSimulationSnapshot(corrupt,restored));
    assert(!restored.world.earthScenario.available);
    std::vector<std::uint8_t> retained;
    assert(encodeSimulationSnapshot(restored,retained));
    assert(retained==oldBytes);
    for(int invalid=0;invalid<4;++invalid){
        auto bad=bound;
        auto& geographic=bad.world.earthScenario;
        if(invalid==0)geographic.worldSeed++;
        if(invalid==1)geographic.geographicOrigin.latitudeDeg=91;
        if(invalid==2)geographic.metersPerGridCell=0;
        if(invalid==3)geographic.earthDataVersion="../not-a-pack";
        assert(!encodeSimulationSnapshot(bad,again));
        assert(!replay.restoreSnapshot(bad));
    }
}
