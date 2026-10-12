#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include "lifelens/CognitiveAcceptedJournal.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

int main()
{
    Simulation sim(77221);
    sim.setupNewGame();
    assert(sim.world().characters.size()>=2);
    Character& actor=sim.world().characters[0];
    const CognitiveRequest request=sim.observeCognitiveRequest(
        actor.id,CognitiveTriggerKind::ResourceScarcity);
    assert(!request.allowedIntents.empty());
    const std::string context=cognitiveRequestFingerprint(request);
    assert(context==cognitiveRequestFingerprint(request));
    CognitiveRequest changed=request;
    changed.personality.curiosity+=0.01;
    assert(context!=cognitiveRequestFingerprint(changed));
    CognitiveProposal proposal;
    proposal.actor=actor.id;
    proposal.intent=CognitiveIntentKind::ImproveWaterSecurity;
    proposal.priority=0.82345123456789;
    proposal.rationale="Prefer a sustainable water supply.";

    assert(recordAcceptedCognitiveProposal(
        sim.world(),actor,request,proposal,
        context,"free-local-provider","sha256-rationale")
        ==CognitiveJournalWriteResult::Recorded);
    assert(recordAcceptedCognitiveProposal(
        sim.world(),actor,request,proposal,
        context,"free-local-provider","different-rationale")
        ==CognitiveJournalWriteResult::AlreadyRecorded);

    auto journal=observeAcceptedCognitionJournal(actor);
    assert(journal.size()==1);
    assert(journal.front().actor==actor.id);
    assert(journal.front().intent==CognitiveIntentKind::ImproveWaterSecurity);
    assert(journal.front().backendIdentity=="free-local-provider");
    assert(cognitivePriorityBits(journal.front().priority)
        ==cognitivePriorityBits(proposal.priority));

    // Invalid target, stale context and missing provenance must fail closed.
    CognitiveProposal invalid=proposal;
    invalid.intent=CognitiveIntentKind::CooperateWithResident;
    invalid.targetResident=99999;
    assert(recordAcceptedCognitiveProposal(
        sim.world(),actor,request,invalid,"context-2","local","hash")
        ==CognitiveJournalWriteResult::Rejected);
    assert(recordAcceptedCognitiveProposal(
        sim.world(),actor,request,proposal,"","local","hash")
        ==CognitiveJournalWriteResult::Rejected);
    CognitiveRequest stale=request;
    --stale.minute;
    assert(recordAcceptedCognitiveProposal(
        sim.world(),actor,stale,proposal,"context-3","local","hash")
        ==CognitiveJournalWriteResult::Rejected);

    // Journal is saved as existing Core MemoryState, no snapshot schema bump.
    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(sim.captureSnapshot(),bytes,&error));
    SimulationStateSnapshot restored;
    assert(decodeSimulationSnapshot(bytes,restored,&error));
    const Character* recovered=nullptr;
    for(const Character& c:restored.world.characters)
        if(c.id==actor.id) recovered=&c;
    assert(recovered!=nullptr);
    journal=observeAcceptedCognitionJournal(*recovered);
    assert(journal.size()==1);
    assert(journal[0].contextFingerprint==context);
    assert(cognitivePriorityBits(journal[0].priority)
        ==cognitivePriorityBits(proposal.priority));
    // An accepted event remains present and replayable after Core save/load.
    World world=restored.world;
    Character& replay=world.characters[0];
    assert(recordAcceptedCognitiveProposal(
        world,replay,request,proposal,
        context,"free-local-provider","sha256-rationale")
        ==CognitiveJournalWriteResult::AlreadyRecorded);

    // One resident's journal does not appear in another resident's memory.
    assert(observeAcceptedCognitionJournal(world.characters[1]).empty());

    // Storage is bounded even if external cognition is requested repeatedly.
    for(int i=0;i<100;++i){
        ++world.minute;
        CognitiveRequest next=request;
        next.minute=world.minute;
        assert(recordAcceptedCognitiveProposal(
            world,replay,next,proposal,
            cognitiveRequestFingerprint(next),"local","digest")
            ==CognitiveJournalWriteResult::Recorded);
    }
    assert(observeAcceptedCognitionJournal(replay).size()
           ==MaxAcceptedCognitionJournalPerResident);
    return 0;
}
