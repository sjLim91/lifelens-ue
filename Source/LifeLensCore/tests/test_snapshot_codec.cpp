#include <iostream>
#include <string>
#include <vector>

#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

int main()
{
    Simulation source(777331);
    source.setupNewGame();
    source.runMinutes(5000);

    // Seed representative family/history state so book codecs are non-empty.
    Character& a=source.world().characters[0];
    Character& b=source.world().characters[2];
    Relationship& ab=source.relationships().getOrCreate(a.id,b.id);
    Relationship& ba=source.relationships().getOrCreate(b.id,a.id);
    ab.affection=0.83; ab.trust=0.72; ab.attraction=0.68; ab.romanticInterest=0.74; ab.commitment=0.61;
    ba.affection=0.79; ba.trust=0.75; ba.attraction=0.70; ba.romanticInterest=0.71; ba.commitment=0.64;
    CHECK(source.romances().startDating(a.id,b.id,a.id,source.world().minute-1000));
    CHECK(source.households().create(901,{a.id,b.id},9,2500.0));
    CHECK(source.genealogy().linkSpouses(a.id,b.id));
    PregnancyState* pregnancy=source.pregnancies().start(b.id,a.id,source.world().minute-100);
    CHECK(pregnancy!=nullptr);
    pregnancy->fatigue=0.31;
    pregnancy->stress=0.22;

    MemoryRecord memory;
    memory.who=b.id;
    memory.what="persistent-memory";
    memory.where="shared-home";
    memory.minute=source.world().minute-25;
    memory.importance=0.91;
    memory.tags={"save","family"};
    a.memory.add(memory);
    a.beliefs.getOrCreate(b.id,"trusted").applyEvidence(true,0.88,source.world().minute);

    const SimulationStateSnapshot snapshot=source.captureSnapshot();
    std::vector<std::uint8_t> bytes;
    std::string error;
    CHECK(encodeSimulationSnapshot(snapshot,bytes,&error));
    CHECK(error.empty());
    CHECK(bytes.size()>64);

    SimulationStateSnapshot decoded;
    CHECK(decodeSimulationSnapshot(bytes,decoded,&error));
    CHECK(error.empty());

    // Canonical codec contract: decode+encode must reproduce identical bytes.
    std::vector<std::uint8_t> reencoded;
    CHECK(encodeSimulationSnapshot(decoded,reencoded,&error));
    CHECK(bytes==reencoded);

    Simulation restored(1);
    CHECK(restored.restoreSnapshot(decoded,&error));
    CHECK(error.empty());

    // Restored Core must serialize identically immediately.
    std::vector<std::uint8_t> restoredBytes;
    CHECK(encodeSimulationSnapshot(restored.captureSnapshot(),restoredBytes,&error));
    CHECK(restoredBytes==bytes);

    // And must continue into the exact same future, proving RNG/runtime state.
    source.runMinutes(10000);
    restored.runMinutes(10000);
    std::vector<std::uint8_t> futureA,futureB;
    CHECK(encodeSimulationSnapshot(source.captureSnapshot(),futureA,&error));
    CHECK(encodeSimulationSnapshot(restored.captureSnapshot(),futureB,&error));
    CHECK(futureA==futureB);

    // Corruption/format mismatches are rejected cleanly.
    std::vector<std::uint8_t> corrupted=bytes;
    corrupted[0]^=0xff;
    SimulationStateSnapshot rejected;
    CHECK(!decodeSimulationSnapshot(corrupted,rejected,&error));
    CHECK(!error.empty());

    std::vector<std::uint8_t> truncated(bytes.begin(),bytes.begin()+bytes.size()/2);
    CHECK(!decodeSimulationSnapshot(truncated,rejected,&error));

    std::vector<std::uint8_t> trailing=bytes;
    trailing.push_back(0x42);
    CHECK(!decodeSimulationSnapshot(trailing,rejected,&error));

    std::cout << "snapshot binary codec roundtrip + continuation passed\n";
    return 0;
}
