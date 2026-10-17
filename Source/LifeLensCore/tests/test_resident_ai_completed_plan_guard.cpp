#include <cassert>
#include <string>

#include "lifelens/ResidentAICompletedPlanGuard.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

int main()
{
    Character actor;
    actor.id=8;
    actor.alive=true;
    CivilizationUtilityDecision food;
    food.intent=CivilizationIntent::Gather;
    food.material=MaterialKind::PlantFood;
    CivilizationUtilityDecision wood=food;
    wood.material=MaterialKind::Wood;

    AcceptedCognitiveDecision thought;
    thought.actor=actor.id;
    thought.intent=CognitiveIntentKind::ImproveFoodSecurity;
    thought.minute=480;

    // The accepted model thought is not an action completion.
    assert(!residentAIPlanAlreadyAttempted(actor,thought,food,495));
    rememberCivilizationAttemptOutcome(actor,food,501,true);
    assert(residentAIPlanAlreadyAttempted(actor,thought,food,510));
    assert(!residentAIPlanAlreadyAttempted(actor,thought,wood,510));
    assert(!residentAIPlanAlreadyAttempted(actor,thought,food,500));

    // A new model opinion issued after that experience is not consumed.
    thought.minute=501;
    assert(!residentAIPlanAlreadyAttempted(actor,thought,food,510));
    thought.minute=480;

    Character hearsay=actor;
    for(auto& m:hearsay.memory.entries){
        m.source=MemorySource::ToldByOther;
        m.witnessed=false;
    }
    assert(!residentAIPlanAlreadyAttempted(hearsay,thought,food,510));
    Character forged=actor;
    for(auto& m:forged.memory.entries) m.sourceCharacter=999;
    assert(!residentAIPlanAlreadyAttempted(forged,thought,food,510));

    Character other=actor;
    other.id=9;
    AcceptedCognitiveDecision otherThought=thought;
    otherThought.actor=9;
    assert(!residentAIPlanAlreadyAttempted(other,otherThought,food,510));

    // Real failed actions also invalidate the old plan. This is a personal
    // outcome, not an assertion by the local model.
    Character failed;
    failed.id=12;
    AcceptedCognitiveDecision failedThought=thought;
    failedThought.actor=12;
    rememberCivilizationAttemptOutcome(failed,food,502,false);
    assert(residentAIPlanAlreadyAttempted(failed,failedThought,food,510));

    // Snapshot restore preserves the owner/provenance and matches the exact
    // execution key; no new journal or snapshot extension is necessary.
    Simulation sim(8842);
    sim.setupNewGame();
    const CharacterId first=sim.world().characters[0].id;
    for(auto& c:sim.world().characters)
        if(c.id==first){
            rememberCivilizationAttemptOutcome(c,food,501,true);
            break;
        }
    AcceptedCognitiveDecision snapshotThought=thought;
    snapshotThought.actor=first;
    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(sim.captureSnapshot(),bytes,&error));
    SimulationStateSnapshot restoredSnapshot;
    assert(decodeSimulationSnapshot(bytes,restoredSnapshot,&error));
    Simulation restored(8842);
    assert(restored.restoreSnapshot(restoredSnapshot,&error));
    bool verified=false;
    for(const auto& c:restored.world().characters){
        if(c.id==first){
            verified=residentAIPlanAlreadyAttempted(
                c,snapshotThought,food,510);
            break;
        }
    }
    assert(verified);
    return 0;
}
