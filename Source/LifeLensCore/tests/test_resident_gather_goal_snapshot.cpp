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

    // v2's suspended record is still the original promise, not an active
    // physical action. Round-trip and same-world restore preserve it exactly.
    auto suspended=snapshot;
    auto& runtime=suspended.runtime.at(actor);
    runtime.suspendedGather.goal=before.gatherGoal;
    runtime.suspendedGather.source=before.civilization.resourceNode;
    runtime.suspendedGather.requestedQuantity=before.civilization.quantity;
    runtime.pendingContext.clear();
    const auto suspendedBytes=al1_test::bytes(suspended);
    assert(decodeSimulationSnapshot(suspendedBytes,decoded,&error));
    assert(decoded.runtime.at(actor).suspendedGather.active());
    assert(!decoded.runtime.at(actor).pendingContext.active());
    assert(decoded.runtime.at(actor).suspendedGather.goal.goalId
        ==before.gatherGoal.goalId);
    assert(decoded.runtime.at(actor).suspendedGather.source
        ==before.civilization.resourceNode);
    assert(al1_test::bytes(decoded)==suspendedBytes);
    assert(replay.restoreSnapshot(decoded,&error));
    assert(al1_test::bytes(replay.captureSnapshot())==suspendedBytes);

    auto malformedSuspension=suspended;
    malformedSuspension.runtime.at(actor).suspendedGather.source=0;
    std::vector<std::uint8_t> rejected;
    assert(!encodeSimulationSnapshot(malformedSuspension,rejected,&error));
    assert(!replay.restoreSnapshot(malformedSuspension,&error));

    auto corruptedSuspended=suspendedBytes;
    corruptedSuspended.back()^=0x55;
    const auto unchangedSuspended=al1_test::bytes(decoded);
    assert(!decodeSimulationSnapshot(corruptedSuspended,decoded,&error));
    assert(al1_test::bytes(decoded)==unchangedSuspended);

    // Pre-upgrade extension v1 has the same active goal rows/checksum, but no
    // suspended count. Reconstruct this exact old tail from v2 with zero
    // suspended entries and verify backward decoding.
    auto v1=encoded;
    const auto magicStart=std::find_end(v1.begin(),v1.end(),
        ResidentGatherGoalExtensionMagic,
        ResidentGatherGoalExtensionMagic+sizeof(ResidentGatherGoalExtensionMagic));
    assert(magicStart!=v1.end());
    const auto offset=static_cast<std::size_t>(magicStart-v1.begin());
    assert(v1[offset+8]==2 &&v1[offset+9]==0);
    v1[offset+8]=1;
    const auto emptyCount=v1.size()-8-4;
    for(int i=0;i<4;++i) assert(v1[emptyCount+i]==0);
    v1.erase(v1.begin()+static_cast<std::ptrdiff_t>(emptyCount),
        v1.begin()+static_cast<std::ptrdiff_t>(emptyCount+4));
    // A v1 checksum covers only active rows; v2 also protects its empty
    // suspended-count field. Recreate the historical checksum exactly.
    std::vector<CharacterId> v1Actors;
    for(const auto& entry:snapshot.runtime)
        if(entry.second.pendingContext.gatherGoal.active())
            v1Actors.push_back(entry.first);
    std::sort(v1Actors.begin(),v1Actors.end());
    std::uint64_t v1Checksum=14695981039346656037ULL;
    hashGatherGoalInteger(v1Checksum,v1Actors.size());
    for(CharacterId id:v1Actors)
        hashGatherGoal(v1Checksum,snapshot.runtime.at(id).pendingContext.gatherGoal);
    for(int i=0;i<8;++i)
        v1[v1.size()-8+i]=static_cast<std::uint8_t>((v1Checksum>>(8*i))&0xffu);
    assert(decodeSimulationSnapshot(v1,decoded,&error));
    assert(decoded.runtime.at(actor).pendingContext.gatherGoal.active());
    assert(!decoded.runtime.at(actor).suspendedGather.active());

    auto bad=snapshot;
    ++bad.runtime.at(actor).pendingContext.gatherGoal.token;
    rejected.clear();
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
