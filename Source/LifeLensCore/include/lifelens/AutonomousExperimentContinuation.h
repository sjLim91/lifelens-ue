#pragma once

#include <cmath>
#include <optional>

#include "AutonomousExperimentPlanner.h"

namespace lifelens {

// A read-only replan review for the SAME resident's existing experiment.
// Fresh Core context is required at every review; an earlier successful
// proposal is NEVER a Core action receipt or proof of completed work.
enum class AutonomousExperimentContinuationStatus {
    InvalidPreviousProposal,
    InvalidCurrentContext,
    DeadlineExpired,
    GoalAlreadySatisfied,
    NoSafeAlternative,
    PreviousAlternativeStillPreferred,
    ReplanToFreshAlternative
};

struct AutonomousExperimentContinuation {
    AutonomousExperimentContinuationStatus status=
        AutonomousExperimentContinuationStatus::InvalidCurrentContext;
    std::optional<AutonomousExperimentAlternative> currentBest;
    int reviewedMinute=-1;
    // No execute/commit authority: the native Core decision, action token,
    // inventory and physical validator remain authoritative.
};

inline bool sameAutonomousExperimentAlternative(
    const AutonomousExperimentAlternative& a,
    const AutonomousExperimentAlternative& b)
{
    return identicalAutonomousExperimentSteps(a.steps,b.steps)
        &&a.effortUnits==b.effortUnits
        &&std::isfinite(a.riskProxy)
        &&std::isfinite(a.preferenceScore)
        &&a.riskProxy==b.riskProxy
        &&a.preferenceScore==b.preferenceScore;
}

inline AutonomousExperimentContinuation reviewAutonomousExperimentContinuation(
    const World& world,const RelationshipBook& relationships,
    const CognitiveRequest& freshRequest,
    const AutonomousGoalCandidate& goal,
    const AutonomousExperimentOptions& priorOptions,
    const AutonomousExperimentAlternative& selectedBefore,
    const AutonomousExperimentBudget& budget={})
{
    AutonomousExperimentContinuation out;
    out.reviewedMinute=world.minute;
    if(!validAutonomousExperimentBudget(budget)) return out;
    // The prior selected plan must have actually been in the trusted
    // *candidate* receipt. Neither an arbitrary injected step nor a forged
    // improvement in risk/utility may be carried forward.
    if(priorOptions.status!=AutonomousExperimentStatus::CandidatesOnly
       ||priorOptions.actor==0 ||priorOptions.actor!=goal.actor
       ||priorOptions.contextFingerprint==0
       ||priorOptions.minute<0
       ||priorOptions.minute>world.minute
       ||goal.createdMinute>priorOptions.minute
       ||goal.expiresMinute<priorOptions.minute
       ||std::none_of(priorOptions.alternatives.begin(),
            priorOptions.alternatives.end(),
            [&](const AutonomousExperimentAlternative& alternative){
                return sameAutonomousExperimentAlternative(
                    alternative,selectedBefore);
            })){
        out.status=AutonomousExperimentContinuationStatus::InvalidPreviousProposal;
        return out;
    }
    if(freshRequest.actor!=priorOptions.actor
       ||freshRequest.minute!=world.minute
       ||freshRequest.trigger==CognitiveTriggerKind::None){
        out.status=AutonomousExperimentContinuationStatus::InvalidCurrentContext;
        return out;
    }
    if(world.minute>goal.expiresMinute){
        out.status=AutonomousExperimentContinuationStatus::DeadlineExpired;
        return out;
    }
    const auto fresh=composeAutonomousExperimentOptions(
        world,relationships,freshRequest,goal,budget);
    if(fresh.status==AutonomousExperimentStatus::RejectedContext
       ||fresh.status==AutonomousExperimentStatus::RejectedGoal
       ||fresh.status==AutonomousExperimentStatus::InvalidBudget){
        out.status=AutonomousExperimentContinuationStatus::InvalidCurrentContext;
        return out;
    }
    if(fresh.status==AutonomousExperimentStatus::AlreadySatisfied){
        // It might have been satisfied by some OTHER action; this is not
        // evidence that the previously selected action succeeded.
        out.status=AutonomousExperimentContinuationStatus::GoalAlreadySatisfied;
        return out;
    }
    if(fresh.status!=AutonomousExperimentStatus::CandidatesOnly
       ||fresh.alternatives.empty()){
        out.status=AutonomousExperimentContinuationStatus::NoSafeAlternative;
        return out;
    }

    // Always re-evaluate from current Core truth; no old risk/effort score,
    // stale resource, target availability or personal belief is trusted.
    out.currentBest=fresh.alternatives.front();
    const bool stillTheBest=identicalAutonomousExperimentSteps(
        selectedBefore.steps,out.currentBest->steps);
    out.status=stillTheBest
        ?AutonomousExperimentContinuationStatus::PreviousAlternativeStillPreferred
        :AutonomousExperimentContinuationStatus::ReplanToFreshAlternative;
    return out;
}

} // namespace lifelens
