#include <cassert>
#include <cmath>
#include <string>
#include <vector>
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

int main()
{
    Simulation sim(99117);
    sim.setupNewGame();
    const auto before=sim.captureSnapshot();
    const CharacterId resident=sim.world().characters.front().id;
    const CognitiveRequest request=sim.observeCognitiveRequest(
        resident,CognitiveTriggerKind::Reflection);
    assert(request.actor==resident);
    assert(request.minute==sim.world().minute);

    // Identical memory text from a different provenance must invalidate a
    // previous model-context fingerprint, even when all numbers are the same.
    CognitiveRequest evidenceRequest=request;
    CognitiveMemoryEvidence evidence;
    evidence.what="a resident claimed there is clean water here";
    evidence.where="grid:1,1";
    evidence.source=MemorySource::DirectWitness;
    evidence.witnessed=true;
    evidenceRequest.memories.push_back(evidence);
    const auto directHash=cognitiveRequestFingerprint(evidenceRequest);
    evidenceRequest.memories.back().source=MemorySource::ToldByOther;
    const auto reportedHash=cognitiveRequestFingerprint(evidenceRequest);
    assert(reportedHash!=directHash);
    evidenceRequest.memories.back().witnessed=false;
    assert(cognitiveRequestFingerprint(evidenceRequest)!=reportedHash);

    CognitiveProposal proposal;
    proposal.actor=resident;
    proposal.intent=CognitiveIntentKind::ImproveWaterSecurity;
    proposal.priority=0.73;
    assert(sim.acceptCognitiveProposal(request,proposal,"local-model-v1","digest001"));
    assert(sim.acceptedCognitiveDecisions().size()==1);
    const auto& first=sim.acceptedCognitiveDecisions().front();
    assert(first.contextFingerprint==cognitiveRequestFingerprint(request));
    assert(first.intent==proposal.intent);
    assert(first.priority==0.73);
    assert(!sim.acceptCognitiveProposal(request,proposal,"local-model-v1","digest001"));

    // The model may not fabricate a safer Need/Belief context.
    CognitiveRequest invented=request;
    invented.needs.thirst=1.0;
    assert(!sim.acceptCognitiveProposal(invented,proposal,"model","digest"));

    // Save/restore accepted input as typed Core truth; the model is not
    // called again just because a snapshot was loaded.
    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(sim.captureSnapshot(),bytes,&error));
    SimulationStateSnapshot loaded;
    assert(decodeSimulationSnapshot(bytes,loaded,&error));
    assert(loaded.acceptedCognitiveDecisions.size()==1);
    assert(loaded.acceptedCognitiveDecisions.front().backendIdentity=="local-model-v1");
    Simulation replay(99117);
    assert(replay.restoreSnapshot(loaded,&error));
    assert(replay.acceptedCognitiveDecisions().size()==1);
    assert(!replay.acceptCognitiveProposal(request,proposal,"local-model-v1","digest001"));

    // Old snapshots with no accepted thought still decode exactly and have
    // no mandatory new marker or journal side effect.
    assert(before.acceptedCognitiveDecisions.empty());
    std::vector<std::uint8_t> emptyBytes;
    assert(encodeSimulationSnapshot(before,emptyBytes,&error));
    SimulationStateSnapshot emptyLoaded;
    assert(decodeSimulationSnapshot(emptyBytes,emptyLoaded,&error));
    assert(emptyLoaded.acceptedCognitiveDecisions.empty());
    assert(bytes.size()>emptyBytes.size());

    // The old request is stale once simulation time has moved forward.
    sim.step();
    assert(!sim.acceptCognitiveProposal(request,proposal,"model","digest"));
    // A bad/imaginary target can never become an accepted thought.
    const auto fresh=sim.observeCognitiveRequest(
        resident,CognitiveTriggerKind::Reflection);
    proposal.intent=CognitiveIntentKind::CooperateWithResident;
    proposal.targetResident=999999;
    assert(!sim.acceptCognitiveProposal(fresh,proposal,"model","digest"));

    // On-disk tampering of accepted provenance must fail closed.
    loaded.acceptedCognitiveDecisions.front().actor=0;
    std::vector<std::uint8_t> corrupted;
    assert(!encodeSimulationSnapshot(loaded,corrupted,&error));
    loaded.acceptedCognitiveDecisions.front().actor=resident;
    loaded.acceptedCognitiveDecisions.front().intent=
        static_cast<CognitiveIntentKind>(9999);
    assert(!encodeSimulationSnapshot(loaded,corrupted,&error));
    loaded.acceptedCognitiveDecisions.front().intent=
        CognitiveIntentKind::ImproveWaterSecurity;
    loaded.acceptedCognitiveDecisions.front().trigger=
        static_cast<CognitiveTriggerKind>(9999);
    assert(!encodeSimulationSnapshot(loaded,corrupted,&error));
    return 0;
}
