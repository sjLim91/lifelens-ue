#include <cassert>
#include <iostream>
#include "al1_natural_witness.h"
#include "lifelens/ResidentGatherGoalSnapshotCodec.h"

using namespace lifelens;

int main()
{
    Simulation natural(874213954ULL);
    natural.setupNewGame();
    CharacterId actor=0;
    for(int tick=0;tick<1440 &&actor==0;++tick){
        natural.step();
        for(const auto& c:natural.world().characters)
            if(natural.observePendingContextAction(c.id).gatherGoal.active()){
                actor=c.id;
                break;
            }
    }
    assert(actor!=0);
    const auto snapshot=natural.captureSnapshot();
    const auto encoded=al1_test::bytes(snapshot);
    SimulationStateSnapshot decoded;
    std::string error;
    assert(decodeSimulationSnapshot(encoded,decoded,&error));
    assert(al1_test::bytes(decoded)==encoded);
    const auto& before=snapshot.runtime.at(actor).pendingContext;
    const auto& after=decoded.runtime.at(actor).pendingContext;
    assert(validAttachedResidentGatherGoal(actor,after,decoded.world.minute));
    assert(after.gatherGoal.goalId==before.gatherGoal.goalId
        &&after.gatherGoal.targetQuantity==before.gatherGoal.targetQuantity);

    Simulation replay(1);
    assert(replay.restoreSnapshot(decoded,&error));
    for(int tick=0;tick<180;++tick){ natural.step(); replay.step(); }
    assert(al1_test::bytes(natural.captureSnapshot())
        ==al1_test::bytes(replay.captureSnapshot()));

    // Old snapshots have no optional goal tail: restore their original
    // pending command without inventing a retrospective goal or memory.
    auto legacy=snapshot;
    for(auto& entry:legacy.runtime) entry.second.pendingContext.gatherGoal={};
    const auto oldBytes=al1_test::bytes(legacy);
    assert(decodeSimulationSnapshot(oldBytes,decoded,&error));
    assert(!decoded.runtime.at(actor).pendingContext.gatherGoal.active());
    assert(decoded.runtime.at(actor).pendingContext.token==before.token);
    assert(al1_test::bytes(decoded)==oldBytes);

    // The optional extension marker is legal text in old resident memories
    // and logs. It must not make those saves unreadable, or hide a real tail
    // in a new save. These are codec fixtures, not success/learning evidence.
    const std::string marker(ResidentGatherGoalExtensionMagic,
        sizeof(ResidentGatherGoalExtensionMagic));
    for(const bool includeGoal:{false,true}){
        for(const bool inMemory:{false,true}){
            auto textCollision=includeGoal ? snapshot : legacy;
            if(inMemory){
                auto& character=textCollision.world.characters.front();
                MemoryRecord text;
                text.who=character.id;
                text.sourceCharacter=character.id;
                text.minute=textCollision.world.minute;
                text.what="ordinary stored text "+marker+" remains text";
                text.where="snapshot-codec-fixture";
                text.source=MemorySource::Inferred;
                text.witnessed=false;
                character.memory.entries.push_back(text);
            }else{
                textCollision.logs.push_back("ordinary saved log "+marker);
            }
            const auto textBytes=al1_test::bytes(textCollision);
            assert(decodeSimulationSnapshot(textBytes,decoded,&error));
            assert(al1_test::bytes(decoded)==textBytes);
            assert(decoded.runtime.at(actor).pendingContext.gatherGoal.active()
                ==includeGoal);
            assert(decoded.runtime.at(actor).pendingContext.token==before.token);
        }
    }

    auto bad=snapshot;
    ++bad.runtime.at(actor).pendingContext.gatherGoal.token;
    std::vector<std::uint8_t> rejected;
    assert(!encodeSimulationSnapshot(bad,rejected,&error));
    assert(!replay.restoreSnapshot(bad,&error));
    auto truncated=encoded;
    truncated.pop_back();
    const auto unchanged=al1_test::bytes(decoded);
    assert(!decodeSimulationSnapshot(truncated,decoded,&error));
    assert(al1_test::bytes(decoded)==unchanged);
    std::cout<<"AL1_GATHER_GOAL_SAVE replay=equal legacy=accepted"
        <<" marker_in_log_or_memory=accepted invalid=rejected\n";
}
