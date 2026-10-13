#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "AutonomousGoalComposer.h"
#include "CognitiveDecisionJournal.h"
#include "SocialUtility.h"
#include "UtilityAI.h"

namespace lifelens {

// B4a: a goal is not an action. This read-only, fail-closed bridge identifies
// existing Core physical/social intents that MIGHT help with a validated
// personal goal. Nothing here alters simulation truth or authorizes a commit.
inline constexpr std::size_t MaxAutonomousAffordanceSteps=6;

enum class AutonomousAffordancePlanStatus {
    RejectedContext,
    RejectedGoal,
    AlreadySatisfied,
    CandidateOnly,
    Blocked,
    Unsupported
};

enum class AutonomousAffordanceReason {
    None,
    StaleOrForgedContext,
    InvalidGoal,
    MissingProvision,
    UrgentSurvival,
    UnsupportedMetric,
    UnsupportedDirection,
    SocialApproachUnavailable
};

enum class AutonomousAffordanceKind {
    ExistingPhysicalGoal,
    ExistingSocialIntent
};

struct AutonomousAffordanceStep {
    AutonomousAffordanceKind kind=AutonomousAffordanceKind::ExistingPhysicalGoal;
    Goal physicalGoal=Goal::Idle;
    SocialIntent socialIntent=SocialIntent::None;
    CharacterId target=0;
    std::size_t goalPredicateIndex=0;
};

struct AutonomousAffordancePlan {
    CharacterId actor=0;
    int minute=-1;
    std::uint64_t contextFingerprint=0;
    AutonomousAffordancePlanStatus status=
        AutonomousAffordancePlanStatus::RejectedContext;
    AutonomousAffordanceReason reason=
        AutonomousAffordanceReason::StaleOrForgedContext;
    std::vector<AutonomousAffordanceStep> proposedSteps;
};

inline double autonomousNeedValue(
    const CognitiveRequest& request,AutonomousNeedKind need)
{
    switch(need){
        case AutonomousNeedKind::Hunger: return request.needs.hunger;
        case AutonomousNeedKind::Thirst: return request.needs.thirst;
        case AutonomousNeedKind::Sleep: return request.needs.sleep;
        case AutonomousNeedKind::Bladder: return request.needs.bladder;
        case AutonomousNeedKind::Hygiene: return request.needs.hygiene;
        default: return -1.0;
    }
}

inline Goal existingPhysicalGoalForAutonomousNeed(AutonomousNeedKind need)
{
    switch(need){
        case AutonomousNeedKind::Hunger: return Goal::Eat;
        case AutonomousNeedKind::Thirst: return Goal::Drink;
        case AutonomousNeedKind::Sleep: return Goal::Sleep;
        case AutonomousNeedKind::Bladder: return Goal::UseToilet;
        case AutonomousNeedKind::Hygiene: return Goal::Wash;
        default: return Goal::Idle;
    }
}

// This is deliberately NOT a planner with magic "new technology" effects.
// Each step names an existing Core intent; no movement destination, success,
// resource creation or relation change is invented. Execution needs a
// separate per-step Core validator, physical engine and observed outcome.
inline AutonomousAffordancePlan draftAutonomousAffordancePlan(
    const World& world,
    const RelationshipBook& relationships,
    const CognitiveRequest& request,
    const AutonomousGoalCandidate& goal)
{
    AutonomousAffordancePlan plan;
    plan.actor=request.actor;
    plan.minute=world.minute;
    const auto fail=[&](
        AutonomousAffordancePlanStatus status,
        AutonomousAffordanceReason reason)
    {
        plan.status=status;
        plan.reason=reason;
        plan.proposedSteps.clear(); // no partial plans
        return plan;
    };

    const Character* actor=cognitiveCharacter(world,request.actor);
    if(actor==nullptr ||!actor->alive
       ||requiresDirectCare(actor->lifeStage)
       ||request.minute!=world.minute
       ||request.trigger==CognitiveTriggerKind::None)
        return fail(AutonomousAffordancePlanStatus::RejectedContext,
                    AutonomousAffordanceReason::StaleOrForgedContext);

    // Reconstruct the actor's bounded PERSONAL context, including provenance,
    // from actual Core truth. A model may only offer a goal, not overwrite
    // needs, memories, relationships, intent catalog or resident identity.
    const CognitiveRequest authoritative=buildCognitiveRequest(
        *actor,relationships,world.minute,request.trigger,
        cognitiveStrategicIntentCatalog(),
        DefaultCognitiveMemoryContextLimit,
        DefaultCognitiveBeliefContextLimit,
        DefaultCognitiveRelationshipContextLimit);
    plan.contextFingerprint=cognitiveRequestFingerprint(authoritative);
    if(cognitiveRequestFingerprint(request)!=plan.contextFingerprint)
        return fail(AutonomousAffordancePlanStatus::RejectedContext,
                    AutonomousAffordanceReason::StaleOrForgedContext);

    if(!validateAutonomousGoalCandidate(world,authoritative,goal).accepted)
        return fail(AutonomousAffordancePlanStatus::RejectedGoal,
                    AutonomousAffordanceReason::InvalidGoal);

    const double urgent=DefaultSimulationRuleset.utilityAI.urgentThreshold;
    const bool survivalUrgent=
        actor->needs.hunger>=urgent
        ||actor->needs.thirst>=urgent
        ||actor->needs.sleep>=urgent
        ||actor->needs.bladder>=urgent;

    for(std::size_t i=0;i<goal.desiredState.size();++i){
        const auto& condition=goal.desiredState[i];
        if(plan.proposedSteps.size()>=MaxAutonomousAffordanceSteps)
            return fail(AutonomousAffordancePlanStatus::RejectedGoal,
                        AutonomousAffordanceReason::InvalidGoal);
        switch(condition.metric){
            case AutonomousGoalMetric::ActorNeed: {
                if(condition.comparison!=AutonomousGoalCompare::AtMost)
                    return fail(AutonomousAffordancePlanStatus::Unsupported,
                                AutonomousAffordanceReason::UnsupportedDirection);
                const double observed=autonomousNeedValue(authoritative,condition.need);
                if(!std::isfinite(observed)||observed<0.0)
                    return fail(AutonomousAffordancePlanStatus::RejectedContext,
                                AutonomousAffordanceReason::StaleOrForgedContext);
                if(observed<=condition.threshold) break;
                const Goal physical=existingPhysicalGoalForAutonomousNeed(
                    condition.need);
                if(physical==Goal::Idle ||!actionAvailableFor(world,*actor,physical))
                    return fail(AutonomousAffordancePlanStatus::Blocked,
                                AutonomousAffordanceReason::MissingProvision);
                plan.proposedSteps.push_back({
                    AutonomousAffordanceKind::ExistingPhysicalGoal,
                    physical,SocialIntent::None,0,i
                });
                break;
            }
            case AutonomousGoalMetric::RelationshipTrust: {
                if(condition.comparison!=AutonomousGoalCompare::AtLeast)
                    return fail(AutonomousAffordancePlanStatus::Unsupported,
                                AutonomousAffordanceReason::UnsupportedDirection);
                const auto it=std::find_if(
                    authoritative.relationships.begin(),
                    authoritative.relationships.end(),
                    [&](const CognitiveRelationshipEvidence& rel){
                        return rel.target==condition.targetResident;
                    });
                if(it==authoritative.relationships.end())
                    return fail(AutonomousAffordancePlanStatus::RejectedContext,
                                AutonomousAffordanceReason::StaleOrForgedContext);
                if(it->trust>=condition.threshold) break;
                if(survivalUrgent)
                    return fail(AutonomousAffordancePlanStatus::Blocked,
                                AutonomousAffordanceReason::UrgentSurvival);
                const Character* target=
                    cognitiveCharacter(world,condition.targetResident);
                if(target==nullptr ||!target->alive)
                    return fail(AutonomousAffordancePlanStatus::Blocked,
                                AutonomousAffordanceReason::SocialApproachUnavailable);
                const double repair=scoreRepairIntent(
                    *actor,condition.targetResident,relationships);
                const double approach=scoreApproachIntent(
                    world,*actor,condition.targetResident,relationships);
                if(!(std::isfinite(repair) && std::isfinite(approach))
                   ||(repair<=0.0 && approach<=0.0))
                    return fail(AutonomousAffordancePlanStatus::Blocked,
                                AutonomousAffordanceReason::SocialApproachUnavailable);
                const SocialIntent route=repair>approach
                    ? SocialIntent::Repair:SocialIntent::Approach;
                plan.proposedSteps.push_back({
                    AutonomousAffordanceKind::ExistingSocialIntent,
                    Goal::Idle,route,condition.targetResident,i
                });
                break;
            }
            // B4a does not pretend to know a resource location merely because
            // an inventory goal mentions a material; or that a resident's
            // continued survival can be commanded as an executable action.
            case AutonomousGoalMetric::ActorRawMaterialCount:
            case AutonomousGoalMetric::KnownResidentAlive:
            default:
                return fail(AutonomousAffordancePlanStatus::Unsupported,
                            AutonomousAffordanceReason::UnsupportedMetric);
        }
    }
    plan.status=plan.proposedSteps.empty()
        ? AutonomousAffordancePlanStatus::AlreadySatisfied
        : AutonomousAffordancePlanStatus::CandidateOnly;
    plan.reason=AutonomousAffordanceReason::None;
    return plan;
}

} // namespace lifelens
