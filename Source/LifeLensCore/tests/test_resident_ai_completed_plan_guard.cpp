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

    // Model-authored tags and copied firsthand flags are NOT executed
    // actions. Neither false failures nor false successes may consume plans.
    Character narration=actor;
    for(auto& memory:narration.memory.entries)
        memory.what="model says the attempt failed";
    assert(!residentAIPlanAlreadyAttempted(narration,thought,food,510));
    Character counterfeitActor=actor;
    for(auto& memory:counterfeitActor.memory.entries)
        memory.who=999;
    assert(!residentAIPlanAlreadyAttempted(
        counterfeitActor,thought,food,510));
    Character counterfeitLocation=actor;
    for(auto& memory:counterfeitLocation.memory.entries)
        memory.where="model-claimed-an-action";
    assert(!residentAIPlanAlreadyAttempted(
        counterfeitLocation,thought,food,510));
    Character counterfeitCause=actor;
    for(auto& memory:counterfeitCause.memory.entries){
        memory.tags[2]="failure";
        memory.what="실제 문명 행동 Civilization:Gather:PlantFood"
            " 수행을 시도했지만 실패했다 (직접 확인한 원인: model-says-so)";
    }
    assert(!residentAIPlanAlreadyAttempted(
        counterfeitCause,thought,food,510));
    // The legitimate Core writer's real failure and success still block an
    // old plan until a fresh model thought is accepted.
    Character genuineFailure=actor;
    for(auto& memory:genuineFailure.memory.entries){
        memory.tags[2]="failure";
        memory.what="실제 문명 행동 Civilization:Gather:PlantFood"
            " 수행을 시도했지만 실패했다 (직접 확인한 원인: source-exhausted)";
    }
    assert(residentAIPlanAlreadyAttempted(
        genuineFailure,thought,food,510));

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

    // Source-specific plan blocking must also reject model-authored
    // invented source locations and the wrong Core-recipient identity.
    Character fakeSourced=sourced;
    for(auto& memory:fakeSourced.memory.entries)
        if(memory.tags.size()==3){
            memory.where="resource-node:101extra";
        }
    assert(!residentAIPlanAlreadyAttempted(
        fakeSourced,sourceThought,oldSource,504));
    Character wrongSourceActor=sourced;
    for(auto& memory:wrongSourceActor.memory.entries) memory.who=99;
    assert(!residentAIPlanAlreadyAttempted(
        wrongSourceActor,sourceThought,oldSource,504));
    // A genuine old-source action cannot block a different real node.
    assert(!residentAIPlanAlreadyAttempted(
        sourced,sourceThought,replacement,504));

    // An arbitrary location beginning with resource-node: is not valid
    // provenance. Only a real canonical uint64 Core node counts.
    for(const std::string& where:{
         std::string("resource-node:101garbage"),
         std::string("resource-node:000101"),
         std::string("resource-node:18446744073709551616")}){
        Character malformed=sourced;
        for(auto& memory:malformed.memory.entries)
            if(memory.tags.size()==3) memory.where=where;
        assert(!residentAIPlanAlreadyAttempted(
            malformed,sourceThought,oldSource,504));
        CivilizationUtilityDecision unsourced=oldSource;
        unsourced.resourceNode=0;
        assert(!residentAIPlanAlreadyAttempted(
            malformed,sourceThought,unsourced,504));
    }

    // A successful partial ingredient collection is normally consumed.
    // Only the caller that has rechecked the STILL-UNMET real recipe can
    // explicitly request continuation for the same original tool goal.
    Character partial;
    partial.id=25;
    partial.alive=true;
    CivilizationUtilityDecision clay;
    clay.intent=CivilizationIntent::Gather;
    clay.material=MaterialKind::Clay;
    clay.resourceNode=515;
    AcceptedCognitiveDecision toolThought=thought;
    toolThought.actor=partial.id;
    toolThought.intent=CognitiveIntentKind::CraftUsefulTools;
    rememberCivilizationAttemptOutcome(partial,clay,503,true);
    assert(residentAIPlanAlreadyAttempted(
        partial,toolThought,clay,504));
    assert(!residentAIPlanAlreadyAttempted(
        partial,toolThought,clay,504,true));
    rememberCivilizationAttemptOutcome(partial,clay,505,false);
    assert(residentAIPlanAlreadyAttempted(
        partial,toolThought,clay,506,true));

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
