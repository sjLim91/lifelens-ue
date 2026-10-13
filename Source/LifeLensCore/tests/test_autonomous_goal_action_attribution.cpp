#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "lifelens/AutonomousGoalActionLinkSnapshotCodec.h"
#include "lifelens/AutonomousSocialGoalRouting.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

#define CHECK(cond) do { if(!(cond)) { \
    std::cerr<<"failed: "<<#cond<<" at "<<__LINE__<<"\n"; \
    return 1; \
} } while(false)

static Character* findResident(Simulation& sim,CharacterId id)
{
    for(auto& character:sim.world().characters){
        if(character.id==id) return &character;
    }
    return nullptr;
}

static bool hasGoalFeedback(
    const Character& actor,
    CharacterId target,
    const char* outcome)
{
    for(const auto& memory:actor.memory.entries){
        if(memory.who==target
           &&memory.tags.size()==3
           &&memory.tags[0]=="goal_action_feedback"
           &&memory.what==outcome)
            return true;
    }
    return false;
}

int main()
{
    // The same action can execute without completing its original goal.
    AutonomousGoalActionLink link;
    link.token=5001;
    link.actor=11;
    link.target=22;
    link.intent=SocialIntent::Approach;
    link.issuedMinute=100;
    link.contextFingerprint=123456;
    link.trustAtIssue=0.20;
    link.trustThreshold=0.40;
    CHECK(validAutonomousGoalActionLink(link));
    using Verdict=AutonomousGoalActionOutcome;
    CHECK(classifyAutonomousGoalActionOutcome(link,0.20,0.20)
          ==Verdict::NotAchievedNoProgress);
    CHECK(classifyAutonomousGoalActionOutcome(link,0.20,0.25)
          ==Verdict::ProgressButNotAchieved);
    CHECK(classifyAutonomousGoalActionOutcome(link,0.20,0.40)
          ==Verdict::AchievedDuringExecution);
    CHECK(classifyAutonomousGoalActionOutcome(link,0.42,0.43)
          ==Verdict::AlreadyReachedBeforeExecution);
    CHECK(classifyAutonomousGoalActionOutcome(
        link,std::nullopt,0.44)==Verdict::Unverifiable);
    CHECK(classifyAutonomousGoalActionOutcome(
        link,0.24,std::nullopt)==Verdict::Unverifiable);

    Character learner;
    learner.id=11;
    const double neutral=learnedAutonomousGoalProgressDisposition(
        learner,22,SocialIntent::Approach);
    CHECK(neutral==0.0);
    CHECK(!rememberAutonomousGoalActionOutcome(
        learner,link,5002,110,Verdict::NotAchievedNoProgress));
    CHECK(learner.memory.entries.empty());
    CHECK(rememberAutonomousGoalActionOutcome(
        learner,link,5001,110,Verdict::NotAchievedNoProgress));
    CHECK(learnedAutonomousGoalProgressDisposition(
        learner,22,SocialIntent::Approach)<0.0);
    CHECK(!rememberAutonomousGoalActionOutcome(
        learner,link,5001,110,Verdict::NotAchievedNoProgress));
    CHECK(learner.memory.entries.size()==1);
    const auto* learnerBelief=learner.beliefs.find(
        22,autonomousGoalProgressProposition(SocialIntent::Approach));
    CHECK(learnerBelief &&learnerBelief->contradictionCount==1);
    CHECK(rememberAutonomousGoalActionOutcome(
        learner,link,5001,111,Verdict::ProgressButNotAchieved));
    CHECK(learner.memory.entries.size()==1);
    CHECK(learnerBelief->supportCount==1);
    CHECK(rememberAutonomousGoalActionOutcome(
        learner,link,5001,112,Verdict::AchievedDuringExecution));
    CHECK(learner.memory.entries.size()==1);
    CHECK(hasGoalFeedback(learner,22,"achieved-during-action"));
    Character independent;
    independent.id=33;
    CHECK(learnedAutonomousGoalProgressDisposition(
        independent,22,SocialIntent::Approach)==0.0);
    const auto successes=learnerBelief->supportCount;
    CHECK(rememberAutonomousGoalActionOutcome(
        learner,link,5001,113,Verdict::AlreadyReachedBeforeExecution));
    CHECK(learnerBelief->supportCount==successes);
    CHECK(!rememberAutonomousGoalActionOutcome(
        learner,link,5001,114,Verdict::Unverifiable));

    // Legacy v8 snapshots have NO invented token-to-goal provenance.
    Simulation sim(777);
    sim.setupSocialDemo();
    sim.world().resourceNodes.clear();
    sim.world().storageSites.clear();
    for(auto& character:sim.world().characters){
        character.needs={0.05,0.05,0.05,0.05,0.05};
    }
    sim.setExternalPhysicalExecution(true);
    PendingContextActionObservation pending;
    for(int minute=0;minute<10 &&!pending.active;++minute){
        sim.step();
        for(const auto& resident:sim.world().characters){
            const auto candidate=sim.observePendingContextAction(resident.id);
            if(candidate.active &&candidate.kind==ContextActionKind::Social){
                pending=candidate;
                break;
            }
        }
    }
    CHECK(pending.active &&pending.token!=0);
    CHECK(pending.targetResident!=0);
    auto initial=sim.captureSnapshot();
    CHECK(!initial.runtime.at(pending.actor)
        .pendingContext.autonomousGoal.active());

    std::vector<std::uint8_t> legacyBytes;
    std::string error;
    CHECK(encodeSimulationSnapshot(initial,legacyBytes,&error));
    SimulationStateSnapshot oldDecoded;
    CHECK(decodeSimulationSnapshot(legacyBytes,oldDecoded,&error));
    CHECK(!oldDecoded.runtime.at(pending.actor)
        .pendingContext.autonomousGoal.active());

    // Artificially attach a VALIDATED Core-authorized goal to a pending
    // action fixture. There is still no outcome or belief before execution.
    auto& action=initial.runtime[pending.actor].pendingContext;
    action.social.intent=SocialIntent::Approach;
    auto& ownRelation=initial.relationships.getOrCreate(
        pending.actor,pending.targetResident);
    ownRelation.trust=0.10;
    AutonomousGoalActionLink tied;
    tied.token=action.token;
    tied.actor=pending.actor;
    tied.target=pending.targetResident;
    tied.intent=SocialIntent::Approach;
    tied.issuedMinute=action.issuedMinute;
    tied.contextFingerprint=0x019f11;
    tied.trustAtIssue=0.10;
    tied.trustThreshold=0.25;
    CHECK(validAutonomousGoalActionLink(tied));
    action.autonomousGoal=tied;
    CHECK(validAttachedAutonomousGoalLink(
        pending.actor,action,initial.world.minute));

    std::vector<std::uint8_t> inFlightBytes;
    CHECK(encodeSimulationSnapshot(initial,inFlightBytes,&error));
    CHECK(inFlightBytes.size()>legacyBytes.size());
    SimulationStateSnapshot resumed;
    CHECK(decodeSimulationSnapshot(inFlightBytes,resumed,&error));
    const auto& recovered=resumed.runtime.at(pending.actor)
        .pendingContext.autonomousGoal;
    CHECK(recovered.token==tied.token);
    CHECK(recovered.target==tied.target);
    CHECK(recovered.contextFingerprint==tied.contextFingerprint);
    CHECK(std::abs(recovered.trustThreshold-0.25)<1e-12);
    CHECK(std::abs(recovered.trustAtIssue-0.10)<1e-12);
    CHECK(resumed.world.minute==initial.world.minute);

    // Reject stale/mismatched token, not silently rewriting the intention.
    auto forged=initial;
    forged.runtime[pending.actor].pendingContext
        .autonomousGoal.token++;
    CHECK(!encodeSimulationSnapshot(forged,inFlightBytes,&error));
    CHECK(!error.empty());
    CHECK(encodeSimulationSnapshot(initial,inFlightBytes,&error));

    // Reject a truncated optional extension atomically.
    auto truncated=inFlightBytes;
    truncated.pop_back();
    SimulationStateSnapshot untouched;
    untouched.world.seed=99199;
    CHECK(!decodeSimulationSnapshot(truncated,untouched,&error));
    CHECK(untouched.world.seed==99199);

    Simulation restored(777);
    CHECK(restored.restoreSnapshot(resumed,&error));
    CHECK(error.empty());
    restored.setExternalPhysicalExecution(true);
    Character* actor=findResident(restored,pending.actor);
    Character* target=findResident(restored,pending.targetResident);
    CHECK(actor &&target);
    const std::size_t beforeMem=actor->memory.entries.size();
    const double selfTrustBefore=
        restored.relationships().find(actor->id,target->id)->trust;

    GridPos destination{};
    CHECK(restored.runtimePosition(target->id,destination));
    CHECK(!restored.completeExternalContextAction(
        actor->id,pending.token+1,destination));
    CHECK(actor->memory.entries.size()==beforeMem);
    CHECK(restored.observePendingContextAction(actor->id).active);

    CHECK(restored.completeExternalContextAction(
        actor->id,pending.token,destination));
    CHECK(!restored.observePendingContextAction(actor->id).active);
    // Core changes recipient's perception; the actor's OWN trust goal
    // remains unmet. Success of the interaction is not goal success.
    CHECK(std::abs(
        restored.relationships().find(actor->id,target->id)->trust
        -selfTrustBefore)<1e-12);
    CHECK(hasGoalFeedback(
        *actor,target->id,"not-achieved-no-progress"));
    const BeliefRecord* learned=actor->beliefs.find(
        target->id,autonomousGoalProgressProposition(SocialIntent::Approach));
    CHECK(learned &&learned->contradictionCount>=1);
    const std::size_t afterMem=actor->memory.entries.size();
    CHECK(!restored.completeExternalContextAction(
        actor->id,pending.token,destination));
    CHECK(actor->memory.entries.size()==afterMem);
    bool sawOutcomeLog=false;
    for(const auto& line:restored.logs()){
        if(line.find("COG_GOAL_OUTCOME actor=")!=std::string::npos
           &&line.find("not-achieved-no-progress")!=std::string::npos)
            sawOutcomeLog=true;
    }
    CHECK(sawOutcomeLog);

    const auto finalSnapshot=restored.captureSnapshot();
    std::vector<std::uint8_t> finalBytes;
    CHECK(encodeSimulationSnapshot(finalSnapshot,finalBytes,&error));
    SimulationStateSnapshot finalRead;
    CHECK(decodeSimulationSnapshot(finalBytes,finalRead,&error));
    CHECK(!finalRead.runtime.at(actor->id)
        .pendingContext.autonomousGoal.active());
    Simulation replayed(777);
    CHECK(replayed.restoreSnapshot(finalRead,&error));
    CHECK(hasGoalFeedback(
        *findResident(replayed,actor->id),target->id,
        "not-achieved-no-progress"));

    // Cancelling/preempting a pending action drops its link without
    // manufacturing a failure or a successful interaction.
    auto cancelled=initial;
    cancelled.runtime[pending.actor].pendingContext.clear();
    CHECK(!cancelled.runtime[pending.actor]
        .pendingContext.autonomousGoal.active());
    std::vector<std::uint8_t> cancelledBytes;
    CHECK(encodeSimulationSnapshot(cancelled,cancelledBytes,&error));
    SimulationStateSnapshot cancelledRead;
    CHECK(decodeSimulationSnapshot(cancelledBytes,cancelledRead,&error));
    CHECK(!cancelledRead.runtime.at(pending.actor)
        .pendingContext.autonomousGoal.active());

    std::cout<<"autonomous goal/action attribution and save/replay passed\n";
    return 0;
}
