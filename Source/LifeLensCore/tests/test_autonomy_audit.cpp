#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/AutonomyAuditReadModel.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

int main()
{
    // Synthetic contract: read-only, sorted by actor id, duplicate/stale
    // checkpoints rejected atomically, evidence deltas not mere stock counts.
    SimulationStateSnapshot state;
    state.world.minute=600;
    Character first;
    first.id=13;
    first.name="Later";
    Character second;
    second.id=7;
    second.name="Earlier";
    state.world.characters={first,second};
    state.runtime[7].goal=Goal::Eat;
    state.runtime[13].goal=Goal::Sleep;

    AutonomyAuditSeries audit;
    assert(audit.observe(state));
    const auto initial=observeAutonomyCheckpoint(state);
    assert(initial.residents.size()==2);
    assert(initial.residents[0].id==7 && initial.residents[1].id==13);
    assert(initial.physicalGoals[0]==1 && initial.physicalGoals[2]==1);
    assert(audit.checkpoints==1 && audit.residentSamples==2);
    const auto unchanged=audit.json();
    assert(!audit.observe(state));
    assert(audit.json()==unchanged);

    auto wrong=state;
    wrong.world.seed=999;
    wrong.world.minute=601;
    assert(!audit.observe(wrong));
    wrong=state;
    wrong.world.minute=601;
    wrong.world.generationVersion+=1;
    assert(!audit.observe(wrong));
    wrong=state;
    wrong.world.minute=601;
    wrong.world.populationSeed+=1;
    assert(!audit.observe(wrong));
    wrong=state;
    wrong.version+=1;
    wrong.world.minute=601;
    assert(!audit.observe(wrong));
    assert(audit.json()==unchanged);

    state.world.minute=610;
    state.runtime[7].goal=Goal::Drink;
    state.runtime[13].pendingContext.token=42;
    state.runtime[13].pendingContext.kind=ContextActionKind::Civilization;
    rememberActionOutcome(state.world.characters[1],"Drink",610,false);
    rememberActionOutcome(state.world.characters[0],"Sleep",610,true);
    // Genuine typed Core-accepted input journal, never a simulated action.
    AcceptedCognitiveDecision accepted;
    accepted.actor=7;
    accepted.minute=610;
    accepted.trigger=CognitiveTriggerKind::Reflection;
    accepted.contextFingerprint=42;
    accepted.backendIdentity="test-local";
    accepted.intent=CognitiveIntentKind::ImproveWaterSecurity;
    accepted.priority=0.75;
    accepted.rationaleDigest="test-digest";
    state.acceptedCognitiveDecisions.push_back(accepted);
    assert(audit.observe(state));
    assert(audit.checkpoints==2);
    assert(audit.sampledCivilizationContext==1);
    assert(audit.actors.at(7).observedGoalChanges==1);
    assert(audit.actors.at(13).observedGoalChanges==0);
    assert(audit.actors.at(7).observedFailureEvidenceDelta==1);
    assert(audit.actors.at(13).observedSuccessEvidenceDelta==1);
    assert(audit.observedAcceptedCognitiveDecisions==1);
    assert(audit.actors.at(7).observedAcceptedCognitiveDecisions==1);
    assert(audit.actors.at(13).observedAcceptedCognitiveDecisions==0);
    assert(audit.lastAcceptedCognitiveDecisionsStored==1);
    // Actual belief evidence attributed to each observed action, never
    // mislabeled as a count of completed physical actions.
    assert(audit.actors.at(7).observedFailureEvidenceByActionDelta[1]==1); // Drink
    assert(audit.actors.at(13).observedSuccessEvidenceByActionDelta[2]==1); // Sleep
    assert(audit.observedFailureEvidenceByActionDelta[1]==1);
    assert(audit.observedSuccessEvidenceByActionDelta[2]==1);
    assert(audit.observedFailureEvidenceByActionDelta[2]==0);
    assert(audit.observedSuccessEvidenceByActionDelta[1]==0);
    assert(audit.json().find("\"failureEvidenceDeltaByAction\":{\"Eat\":0,\"Drink\":1")
           !=std::string::npos);
    assert(audit.json().find("\"evidenceCaveat\"")!=std::string::npos);
    assert(audit.json().find("\"notInstrumented\"")!=std::string::npos);
    assert(audit.json().find("\"novelGoalCreation\"")!=std::string::npos);
    assert(audit.json().find("\"worldSeed\":\"1\"")!=std::string::npos);

    // A repeat without new accepted time is not a second experience.
    assert(!audit.observe(state));
    assert(audit.actors.at(7).observedFailureEvidenceDelta==1);
    assert(audit.observedFailureEvidenceByActionDelta[1]==1);

    // Unrecognized Core experience labels are grouped into "Other" without
    // injecting free-form action strings into the JSON diagnostic.
    state.world.minute=620;
    rememberActionOutcome(state.world.characters[1],"UnlistedWorldMethod",620,false);
    AcceptedCognitiveDecision acceptedOther=accepted;
    acceptedOther.actor=13;
    acceptedOther.minute=620;
    acceptedOther.contextFingerprint=77;
    state.acceptedCognitiveDecisions.push_back(acceptedOther);
    assert(audit.observe(state));
    assert(audit.observedFailureEvidenceByActionDelta[6]==1); // Other
    assert(audit.actors.at(7).observedFailureEvidenceByActionDelta[6]==1);
    assert(audit.observedAcceptedCognitiveDecisions==2);
    assert(audit.actors.at(13).observedAcceptedCognitiveDecisions==1);
    assert(audit.lastAcceptedCognitiveDecisionsStored==2);
    assert(audit.json().find("\"observedAcceptedCognitiveDecisions\":2")
           !=std::string::npos);
    assert(audit.json().find("acceptedCognitionExecutionEffects")
           !=std::string::npos);
    assert(audit.json().find("acceptedCognitionEvents")==std::string::npos);
    assert(audit.json().find("UnlistedWorldMethod")==std::string::npos);
    const auto afterOther=audit.json();
    assert(!audit.observe(state));
    assert(audit.json()==afterOther);

    // Restored audit observation starts from a baseline; retained history
    // must never be miscounted as new decisions. New minute is counted once.
    AutonomyAuditSeries resumed;
    assert(resumed.observe(state));
    assert(resumed.lastAcceptedCognitiveDecisionsStored==2);
    assert(resumed.observedAcceptedCognitiveDecisions==0);
    state.world.minute=630;
    AcceptedCognitiveDecision later=accepted;
    later.minute=630;
    later.contextFingerprint=88;
    state.acceptedCognitiveDecisions.push_back(later);
    assert(resumed.observe(state));
    assert(resumed.observedAcceptedCognitiveDecisions==1);
    assert(audit.observe(state));
    assert(audit.observedAcceptedCognitiveDecisions==3);
    assert(audit.actors.at(7).observedAcceptedCognitiveDecisions==2);

    // Observe the actual Core simulation without changing its state or RNG.
    // B keeps running independently without audit snapshots. The end-state
    // binary must be exactly the same as audited A, not merely similar.
    for(const std::uint64_t seed:{874213954ULL,4242001ULL}){
        Simulation observed(seed);
        Simulation control(seed);
        observed.setupNewGame();
        control.setupNewGame();
        AutonomyAuditSeries timeline;
        assert(timeline.observe(observed.captureSnapshot()));
        for(int hour=0;hour<24;++hour){
            observed.runMinutes(60);
            control.runMinutes(60);
            assert(timeline.observe(observed.captureSnapshot()));
        }
        assert(timeline.checkpoints==25);
        assert(timeline.lastMinute-timeline.firstMinute==1440);
        const auto observedSnapshot=observed.captureSnapshot();
        const auto controlSnapshot=control.captureSnapshot();
        std::vector<std::uint8_t> observedBytes,controlBytes;
        std::string error;
        assert(encodeSimulationSnapshot(observedSnapshot,observedBytes,&error));
        assert(encodeSimulationSnapshot(controlSnapshot,controlBytes,&error));
        assert(observedBytes==controlBytes);
        assert(observed.logs()==control.logs());
        const auto json=timeline.json();
        assert(json.find("\"source\":\"CoreSnapshotReadOnly\"")
            !=std::string::npos);
        assert(json.find("\"residentSamples\":0")==std::string::npos);
        assert(json.find("\"checkpointCount\":25")!=std::string::npos);
        std::cout<<json<<"\n";
    }
    return 0;
}
