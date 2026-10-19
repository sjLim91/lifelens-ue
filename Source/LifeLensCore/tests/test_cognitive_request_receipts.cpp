#include <cassert>
#include <vector>
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
using namespace lifelens;

static CognitiveProposal food(CharacterId actor){
    CognitiveProposal p;p.actor=actor;
    p.intent=CognitiveIntentKind::ImproveFoodSecurity;p.priority=0.9;
    return p;
}
int main(){
    Simulation first(42,9,3),other(43,9,3);
    first.setupNewGame();other.setupNewGame();
    // All private context is identical: the WORLD identity must still bind it.
    other.world().characters=first.world().characters;
    other.relationships()=first.relationships();
    other.world().minute=first.world().minute;
    const auto actor=first.world().characters.front().id;
    const auto a=first.observeCognitiveRequest(actor,CognitiveTriggerKind::Reflection);
    const auto b=other.observeCognitiveRequest(actor,CognitiveTriggerKind::Reflection);
    assert(a.worldSeed==42 &&a.populationSeed==9 &&a.generationVersion==3);
    assert(cognitiveRequestFingerprint(a)!=cognitiveRequestFingerprint(b));
    assert(!other.acceptCognitiveProposal(a,food(actor),"local-test","cross-world"));

    auto wrong=a;wrong.populationSeed++;
    assert(!first.acceptCognitiveProposal(wrong,food(actor),"local-test","wrong-pop"));
    wrong=a;wrong.generationVersion++;
    assert(!first.acceptCognitiveProposal(wrong,food(actor),"local-test","wrong-version"));
    const auto fingerprint=cognitiveRequestFingerprint(a);
    // Observing a view alone cannot fabricate an issued model receipt.
    assert(!first.acceptObservedCognitiveProposal(actor,a.minute,fingerprint,
        a.trigger,food(actor),"local-test","unissued"));
    const auto issued=first.issueCognitiveRequest(actor,a.trigger);
    assert(cognitiveRequestFingerprint(issued)==fingerprint);
    first.runMinutes(1);
    assert(first.acceptObservedCognitiveProposal(actor,a.minute,fingerprint,
        a.trigger,food(actor),"local-test","observed-input"));
    const auto accepted=first.acceptedCognitiveDecisions().back();
    assert(accepted.observedMinute==a.minute);
    assert(accepted.observedContextFingerprint==fingerprint);
    assert(accepted.minute==a.minute+1);
    const auto current=first.observeCognitiveRequest(actor,a.trigger);
    assert(accepted.contextFingerprint==cognitiveRequestFingerprint(current));
    first.runMinutes(1);
    assert(!first.acceptObservedCognitiveProposal(actor,a.minute,fingerprint,
        a.trigger,food(actor),"local-test","duplicate"));

    std::vector<std::uint8_t> bytes,again;
    assert(encodeSimulationSnapshot(first.captureSnapshot(),bytes));
    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded));
    assert(decoded.acceptedCognitiveDecisions.back().observedMinute==a.minute);
    assert(decoded.acceptedCognitiveDecisions.back().observedContextFingerprint==fingerprint);
    assert(encodeSimulationSnapshot(decoded,again));
    assert(bytes==again);

    const auto pending=first.issueCognitiveRequest(actor,a.trigger);
    Simulation replay(42,9,3);
    assert(replay.restoreSnapshot(first.captureSnapshot()));
    assert(!replay.acceptObservedCognitiveProposal(
        actor,pending.minute,cognitiveRequestFingerprint(pending),pending.trigger,
        food(actor),"local-test","not-restored-unaccepted-input"));
    const auto expired=first.issueCognitiveRequest(actor,a.trigger);
    first.runMinutes(ObservedCognitiveRequestMaxAgeMinutes+1);
    assert(!first.acceptObservedCognitiveProposal(
        actor,expired.minute,cognitiveRequestFingerprint(expired),expired.trigger,
        food(actor),"local-test","expired"));

    Simulation bounded(4242001);
    bounded.setupNewGame();
    const auto old=bounded.issueCognitiveRequest(actor,a.trigger);
    for(int i=0;i<65;++i){
        ++bounded.world().minute;
        bounded.issueCognitiveRequest(actor,a.trigger);
    }
    assert(bounded.world().minute-old.minute<=ObservedCognitiveRequestMaxAgeMinutes);
    assert(!bounded.acceptObservedCognitiveProposal(actor,old.minute,
        cognitiveRequestFingerprint(old),old.trigger,food(actor),
        "local-test","evicted"));
}
