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

    // The memory identifies a real source: repeating the SAME exhausted
    // source is blocked, while a separate real node is NOT the old attempt.
    Character sourced;
    sourced.id=24;
    sourced.alive=true;
    CivilizationUtilityDecision oldSource=food;
    oldSource.resourceNode=101;
    CivilizationUtilityDecision replacement=food;
    replacement.resourceNode=202;
    AcceptedCognitiveDecision sourceThought=thought;
    sourceThought.actor=24;
    rememberCivilizationAttemptOutcome(
        sourced,oldSource,503,false,
        ResidentGatherFailureCause::SourceExhausted);
    assert(residentAIPlanAlreadyAttempted(
        sourced,sourceThought,oldSource,504));
    assert(!residentAIPlanAlreadyAttempted(
        sourced,sourceThought,replacement,504));
    // A legacy save with no source identity must remain fail-closed.
    Character legacy=sourced;
    for(auto& m:legacy.memory.entries)
        if(m.tags.size()==3 &&m.tags[1]=="Civilization:Gather:PlantFood")
            m.where="내가 직접 시도한 행동";
    assert(residentAIPlanAlreadyAttempted(
        legacy,sourceThought,replacement,504));

    // Snapshot restore preserves the owner/provenance and matches the exact
    // execution key; no new journal or snapshot extension is necessary.
    Simulation sim(8842);
    sim.setupNewGame();
    const CharacterId first=sim.world().characters[0].id;
    for(auto& c:sim.world().characters)
        if(c.id==first){
            rememberCivilizationAttemptOutcome(c,food,480,true);
            break;
        }
    AcceptedCognitiveDecision snapshotThought=thought;
    snapshotThought.actor=first;
    snapshotThought.minute=479;
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
                c,snapshotThought,food,480);
            break;
        }
    }
    assert(verified);
    return 0;
}
