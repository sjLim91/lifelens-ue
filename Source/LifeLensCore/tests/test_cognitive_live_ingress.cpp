#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include "lifelens/CognitiveLiveIngressValidation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

static void expectRejected(
    const Simulation& sim,const LiveCognitiveProposal& offered,
    LiveCognitiveValidationIssue expected)
{
    const auto result=validateLiveCognitiveProposal(sim,offered);
    assert(!result.valid);
    assert(result.issue==expected);
}

int main()
{
    Simulation sim(4242001);
    sim.setupNewGame();
    assert(!sim.world().characters.empty());
    const CharacterId actor=sim.world().characters.front().id;
    LiveCognitiveProposal valid;
    valid.worldSeed=sim.world().seed;
    valid.populationSeed=sim.world().populationSeed;
    valid.generationVersion=sim.world().generationVersion;
    valid.request=sim.observeCognitiveRequest(
        actor,CognitiveTriggerKind::ResourceScarcity);
    assert(valid.request.actor==actor);
    valid.proposal.actor=actor;
    valid.proposal.intent=CognitiveIntentKind::ImproveFoodSecurity;
    valid.proposal.priority=0.73;
    valid.claimedContextFingerprint=
        liveCognitiveRequestFingerprint(sim.world(),valid.request);

    const auto accepted=validateLiveCognitiveProposal(sim,valid);
    assert(accepted.valid);
    assert(accepted.issue==LiveCognitiveValidationIssue::None);
    assert(accepted.currentContextFingerprint==valid.claimedContextFingerprint);

    // Audit authority: merely checking ingress never changes any Core bytes.
    std::vector<std::uint8_t> before,after;
    std::string error;
    assert(encodeSimulationSnapshot(sim.captureSnapshot(),before,&error));
    const auto logs=sim.logs();
    for(int repeat=0;repeat<20;++repeat){
        assert(validateLiveCognitiveProposal(sim,valid).valid);
    }
    assert(encodeSimulationSnapshot(sim.captureSnapshot(),after,&error));
    assert(before==after && sim.logs()==logs);

    auto wrong=valid;
    ++wrong.worldSeed;
    expectRejected(sim,wrong,LiveCognitiveValidationIssue::WrongWorld);
    wrong=valid;
    ++wrong.populationSeed;
    expectRejected(sim,wrong,LiveCognitiveValidationIssue::WrongWorld);
    wrong=valid;
    ++wrong.generationVersion;
    expectRejected(sim,wrong,LiveCognitiveValidationIssue::WrongWorld);
    wrong=valid;
    wrong.request.minute-=1;
    expectRejected(sim,wrong,LiveCognitiveValidationIssue::StaleTime);
    wrong=valid;
    wrong.request.trigger=CognitiveTriggerKind::None;
    expectRejected(sim,wrong,LiveCognitiveValidationIssue::StaleTime);
    wrong=valid;
    wrong.request.actor=0;
    expectRejected(sim,wrong,LiveCognitiveValidationIssue::ActorUnavailable);
    wrong=valid;
    wrong.request.actor=999999999ULL;
    expectRejected(sim,wrong,LiveCognitiveValidationIssue::ActorUnavailable);

    // Critical regression: a caller may alter its own request AND recompute
    // its own matching fingerprint. It still cannot spoof the current Core
    // Need/Memory/Belief or allowedIntents.
    wrong=valid;
    wrong.request.needs.hunger+=0.01;
    wrong.claimedContextFingerprint=
        liveCognitiveRequestFingerprint(sim.world(),wrong.request);
    expectRejected(sim,wrong,LiveCognitiveValidationIssue::ForgedContext);
    wrong=valid;
    wrong.request.allowedIntents.clear();
    wrong.claimedContextFingerprint=
        liveCognitiveRequestFingerprint(sim.world(),wrong.request);
    expectRejected(sim,wrong,LiveCognitiveValidationIssue::ForgedContext);
    wrong=valid;
    CognitiveMemoryEvidence fake;
    fake.what="invented memory of a hidden distant resource";
    wrong.request.memories.push_back(fake);
    wrong.claimedContextFingerprint=
        liveCognitiveRequestFingerprint(sim.world(),wrong.request);
    expectRejected(sim,wrong,LiveCognitiveValidationIssue::ForgedContext);
    wrong=valid;
    CognitiveBeliefEvidence invented;
    invented.proposition="magically knows the future";
    wrong.request.beliefs.push_back(invented);
    wrong.claimedContextFingerprint=
        liveCognitiveRequestFingerprint(sim.world(),wrong.request);
    expectRejected(sim,wrong,LiveCognitiveValidationIssue::ForgedContext);

    wrong=valid;
    wrong.claimedContextFingerprint^=1ULL;
    expectRejected(sim,wrong,LiveCognitiveValidationIssue::ForgedContext);
    wrong=valid;
    wrong.proposal.actor=99999999ULL;
    expectRejected(sim,wrong,LiveCognitiveValidationIssue::InvalidProposal);
    wrong=valid;
    wrong.proposal.intent=CognitiveIntentKind::CooperateWithResident;
    wrong.proposal.targetResident=99999999ULL;
    expectRejected(sim,wrong,LiveCognitiveValidationIssue::InvalidProposal);

    // Hash is deterministic within exactly one authoritative world context.
    Simulation replay(4242001);
    replay.setupNewGame();
    const auto same=replay.observeCognitiveRequest(
        actor,CognitiveTriggerKind::ResourceScarcity);
    assert(liveCognitiveRequestFingerprint(sim.world(),valid.request)
        ==liveCognitiveRequestFingerprint(replay.world(),same));

    Simulation elsewhere(874213954);
    elsewhere.setupNewGame();
    assert(liveCognitiveRequestFingerprint(sim.world(),valid.request)
        !=liveCognitiveRequestFingerprint(elsewhere.world(),valid.request));

    sim.runMinutes(1);
    expectRejected(sim,valid,LiveCognitiveValidationIssue::StaleTime);
    return 0;
}
