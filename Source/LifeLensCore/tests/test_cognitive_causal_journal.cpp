#include <cassert>
#include <iostream>
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/WebClientBridge.h"
using namespace lifelens;
int main(){
    Simulation sim(874213954);sim.setupNewGame();
    const auto actor=sim.world().characters.front().id;
    const auto request=sim.issueCognitiveRequest(actor,CognitiveTriggerKind::Reflection);
    CognitiveProposal bad;bad.actor=actor;bad.intent=CognitiveIntentKind::CooperateWithResident;bad.targetResident=actor;bad.priority=.9;
    assert(!sim.acceptObservedCognitiveProposal(actor,request.minute,cognitiveRequestFingerprint(request),request.trigger,bad,"test","self-target"));
    assert(sim.cognitiveCausalJournal().size()==1);
    assert(!sim.cognitiveCausalJournal().back().accepted &&sim.cognitiveCausalJournal().back().steps.empty());
    CognitiveProposal good;good.actor=actor;good.intent=CognitiveIntentKind::ImproveWaterSecurity;good.priority=.73;
    assert(sim.acceptCognitiveProposal(request,good,"test","valid"));
    assert(!sim.acceptCognitiveProposal(request,good,"test","valid"));
    assert(sim.cognitiveCausalJournal().size()==3);
    assert(sim.cognitiveCausalJournal().back().rejection==CognitiveTraceRejection::Duplicate);
    auto snapshot=sim.captureSnapshot();
    assert(validCognitiveCausalJournal(snapshot.cognitiveCausalJournal,snapshot.world,snapshot.acceptedCognitiveDecisions));
    std::vector<std::uint8_t> bytes,again;std::string error;
    assert(encodeSimulationSnapshot(snapshot,bytes,&error));
    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    assert(encodeSimulationSnapshot(decoded,again,&error) &&bytes==again);
    assert(decoded.cognitiveCausalJournal.size()==3);
    auto corrupted=snapshot;corrupted.cognitiveCausalJournal[1].origin.contextFingerprint^=1;
    assert(!encodeSimulationSnapshot(corrupted,again,&error));
    corrupted=snapshot;corrupted.cognitiveCausalJournal[1].id=corrupted.cognitiveCausalJournal[0].id;
    assert(!encodeSimulationSnapshot(corrupted,again,&error));
    corrupted=snapshot;corrupted.cognitiveCausalJournal[0].steps.push_back(CognitiveCausalStep{});
    assert(!encodeSimulationSnapshot(corrupted,again,&error));
    corrupted=snapshot;corrupted.cognitiveCausalJournal[1].origin.minute=snapshot.world.minute+1;
    assert(!encodeSimulationSnapshot(corrupted,again,&error));
    // Legacy cognition v1/v2 remains readable with no invented historical step.
    auto legacy=snapshot;legacy.cognitiveCausalJournal.clear();
    assert(encodeSimulationSnapshot(legacy,bytes,&error));assert(decodeSimulationSnapshot(bytes,decoded,&error));
    assert(decoded.cognitiveCausalJournal.empty());
    Simulation expired(874213954);expired.setupNewGame();
    const auto old=expired.issueCognitiveRequest(actor,CognitiveTriggerKind::Reflection);
    expired.world().minute+=ObservedCognitiveRequestMaxAgeMinutes+1;
    assert(!expired.acceptObservedCognitiveProposal(actor,old.minute,cognitiveRequestFingerprint(old),old.trigger,good,"test","expired"));
    assert(expired.cognitiveCausalJournal().size()==1 &&expired.cognitiveCausalJournal().back().rejection==CognitiveTraceRejection::Expired);
    WebClientBridge bridge;assert(bridge.newGame("874213954","",CurrentWorldGenerationVersion));
    const auto before=bridge.saveSnapshotBase64();
    assert(bridge.cognitiveCausalJournalJson("1").find("\"records\":[]")!=std::string::npos);
    assert(bridge.cognitiveCausalJournalJson("999999")=="null");
    assert(bridge.cognitiveCausalJournalJson("1e0")=="null");
    assert(before==bridge.saveSnapshotBase64());
    std::cout<<"Core causal journal: exact approval identity, rejected/duplicate separation, codec replay, corrupted ownership/future guards, read-only bridge PASS\n";
}
