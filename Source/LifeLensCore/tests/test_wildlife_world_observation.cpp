#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/NaturalEcologyValidation.h"
#include "lifelens/WildlifeObservation.h"
#include <cassert>
#include <iostream>
using namespace lifelens;
static std::vector<std::uint8_t> bytes(const Simulation& sim){
    std::vector<std::uint8_t> result; std::string error;
    assert(encodeSimulationSnapshot(sim.captureSnapshot(),result,&error)); return result;
}
int main(){
    Simulation sim{77,0,4}; sim.setupNewGame();
    assert(sim.world().wildlifeEnabled && !sim.world().wildlifePopulations.empty());
    assert(validNaturalEcology(sim.world()));
    const auto rng=sim.world().rng;
    const auto start=sim.world().initialStartRegionCoord;
    const auto count=sim.world().wildlifePopulations.size();
    sim.world().materializeNaturalChunk(start);
    assert(sim.world().wildlifePopulations.size()==count && sim.world().rng==rng);
    std::ostringstream observation; appendWildlifeObservation(observation,sim.world(),start,2);
    assert(observation.str().find("\"available\":true")!=std::string::npos);
    assert(observation.str().find("\"id\":")!=std::string::npos);
    assert(sim.world().rng==rng);
    assert(observation.str().find("behavior")!=std::string::npos);
    sim.runMinutes(1000); assert(validNaturalEcology(sim.world()));
    for(const auto& p:sim.world().wildlifePopulations) assert(p.dayIndex==sim.world().minute/(24*60));
    SimulationStateSnapshot restored; std::string error;
    auto encoded=bytes(sim); assert(decodeSimulationSnapshot(encoded,restored,&error));
    Simulation clone{77,0,4}; assert(clone.restoreSnapshot(restored,&error));
    assert(bytes(clone)==encoded);
    sim.runMinutes(3000); clone.runMinutes(3000); assert(bytes(sim)==bytes(clone));
    auto bad=sim.captureSnapshot(); bad.world.wildlifePopulations[0].worldSeed++;
    std::vector<std::uint8_t> rejected; assert(!encodeSimulationSnapshot(bad,rejected,&error));
    auto corrupt=encoded; corrupt.back()^=1; assert(!decodeSimulationSnapshot(corrupt,restored,&error));
    // Legacy v1 ecology has no wildlife. Restore and stepping must never invent it.
    auto legacy=sim.captureSnapshot(); legacy.world.wildlifeEnabled=false; legacy.world.wildlifePopulations.clear();
    assert(encodeSimulationSnapshot(legacy,encoded,&error));
    assert(decodeSimulationSnapshot(encoded,restored,&error));
    assert(!restored.world.wildlifeEnabled && restored.world.wildlifePopulations.empty());
    assert(clone.restoreSnapshot(restored,&error)); clone.runMinutes(1500);
    assert(!clone.world().wildlifeEnabled && clone.world().wildlifePopulations.empty());
    std::cout << "PASS Core wildlife lifecycle, read-only observation, save/replay, corruption and legacy restore\n";
}
