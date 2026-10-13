#pragma once

#include <algorithm>
#include <cmath>
#include <optional>

#include "AutonomousAffordancePlanner.h"

namespace lifelens {

// B4b: the normal Core utility system MUST first choose to spend time on
// social life. Only then may a resident's own desired relationship outcome
// alter WHOM to approach/repair. No new Social action, no extra social
// opportunity, no change to survival utility, no movement shortcuts.
// The Core still executes the existing pending context action and determines
// all real outcomes; this is a bounded target/method preference only.
inline std::optional<SocialUtilityDecision>
preferAutonomousSocialGoalTarget(
    const World& world,
    const RelationshipBook& relationships,
    const Character& actor,
    const SocialUtilityDecision& baseline)
{
    if(!actor.alive || actor.id==0 || requiresDirectCare(actor.lifeStage)
       || world.minute<0 ||world.minute%15!=0
       || baseline.intent==SocialIntent::None ||baseline.target==0
       ||!std::isfinite(baseline.utility)
       ||baseline.utility<0.18 ||baseline.utility>1.0)
        return std::nullopt;

    const double urgent=DefaultSimulationRuleset.utilityAI.urgentThreshold;
    if(actor.needs.hunger>=urgent || actor.needs.thirst>=urgent
       || actor.needs.sleep>=urgent || actor.needs.bladder>=urgent
       || actor.needs.hygiene>=urgent)
        return std::nullopt;

    const CognitiveRequest request=buildCognitiveRequest(
        actor,relationships,world.minute,CognitiveTriggerKind::Reflection,
        cognitiveStrategicIntentCatalog(),
        DefaultCognitiveMemoryContextLimit,
        DefaultCognitiveBeliefContextLimit,
        DefaultCognitiveRelationshipContextLimit);
    const auto goals=composeAutonomousGoalCandidates(world,request,6);

    std::optional<SocialUtilityDecision> best;
    double bestRank=baseline.utility+0.02;
    for(const auto& goal:goals){
        if(goal.desiredState.size()!=1
           ||goal.desiredState.front().metric
              !=AutonomousGoalMetric::RelationshipTrust
           ||goal.priority<0.40) continue;
        const auto candidate=draftAutonomousAffordancePlan(
            world,relationships,request,goal);
        if(candidate.status!=AutonomousAffordancePlanStatus::CandidateOnly
           ||candidate.proposedSteps.size()!=1) continue;
        const auto& step=candidate.proposedSteps.front();
        if(step.kind!=AutonomousAffordanceKind::ExistingSocialIntent
           ||step.target==0 ||step.target==baseline.target) continue;

        const Character* target=cognitiveCharacter(world,step.target);
        if(target==nullptr ||!target->alive) continue;
        const double actualScore=step.socialIntent==SocialIntent::Repair
            ?scoreRepairIntent(actor,step.target,relationships)
            :scoreApproachIntent(world,actor,step.target,relationships);
        if(!std::isfinite(actualScore)
           ||actualScore<0.18
           ||actualScore<baseline.utility-0.12) continue;

        // Goal priority may only break CLOSE social alternatives. It can
        // never create a positive social score or force social over survival,
        // work, care, trade or an urgent physical need.
        const double ranked=actualScore+0.16*goal.priority;
        if(ranked>bestRank){
            bestRank=ranked;
            best=SocialUtilityDecision{
                step.socialIntent,step.target,actualScore
            };
        }
    }
    return best;
}

} // namespace lifelens
