#pragma once

#include <algorithm>
#include <cmath>
#include <optional>

#include "AutonomousExperimentPlanner.h"
#include "AutonomousSocialOutcomeLearning.h"
#include "AutonomousGoalActionLink.h"

namespace lifelens {

// B4b: the normal Core utility system MUST first choose to spend time on
// social life. Only then may a resident's own desired relationship outcome
// alter WHOM to approach/repair. No new Social action, no extra social
// opportunity, no change to survival utility, no movement shortcuts.
// The Core still executes the existing pending context action and determines
// all real outcomes; this is a bounded target/method preference only.
struct AutonomousPreferredSocialGoal {
    SocialUtilityDecision decision{};
    AutonomousGoalCandidate goal{};
    std::uint64_t contextFingerprint=0;
};

// The detailed variant returns the actual validated goal that caused the
// reroute. Never re-compose a different goal at action completion.
inline std::optional<AutonomousPreferredSocialGoal>
preferAutonomousSocialGoalTargetWithGoal(
    const World& world,
    const RelationshipBook& relationships,
    const Character& actor,
    const SocialUtilityDecision& baseline)
{
    if(!actor.alive || actor.id==0 || requiresDirectCare(actor.lifeStage)
       || world.minute<0 ||world.minute%15!=0
       // Avoidance protects the resident; Comfort protects another person.
       // Neither may be displaced by a merely aspirational trust goal.
       || (baseline.intent!=SocialIntent::Approach
           &&baseline.intent!=SocialIntent::Repair)
       ||baseline.target==0
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

    std::optional<AutonomousPreferredSocialGoal> best;
    double bestRank=baseline.utility+0.02;
    for(const auto& goal:goals){
        if(goal.desiredState.size()!=1
           ||goal.desiredState.front().metric
              !=AutonomousGoalMetric::RelationshipTrust
           ||goal.priority<0.40) continue;
        // B5b: compete only methods the Core already supports, with an
        // actor-specific bounded exploration budget. No extra social slot is
        // granted; the normal Simulation pending token still owns execution.
        AutonomousExperimentBudget budget;
        const double riskTolerance=std::clamp(
            actor.personality.riskTolerance,0.0,1.0);
        budget.maxRiskProxy=0.08+0.16*riskTolerance;
        budget.maxEffortUnits=actor.personality.patience>=0.25 ? 2u : 1u;
        const auto options=composeAutonomousExperimentOptions(
            world,relationships,request,goal,budget);
        if(options.status!=AutonomousExperimentStatus::CandidatesOnly)
            continue;
        for(const auto& option:options.alternatives){
            if(option.steps.size()!=1) continue;
            const auto& step=option.steps.front();
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

            // Personally experienced outcomes are bounded tie-breakers,
            // never assumed probabilities of trust gain or fresh actions.
            const double personalExperience=learnedSocialMoodDisposition(
                actor,step.target,step.socialIntent);
            const double progressExperience=learnedAutonomousGoalProgressDisposition(
                actor,step.target,step.socialIntent);
            const double ranked=actualScore+0.16*goal.priority
                +0.06*personalExperience+0.04*progressExperience;
            if(ranked>bestRank){
                bestRank=ranked;
                best=AutonomousPreferredSocialGoal{
                    {step.socialIntent,step.target,actualScore},
                    goal,options.contextFingerprint
                };
            }
        }
    }
    return best;
}

// Compatibility with the existing B4b API and tests. The chosen social
// decision remains identical when no outcome-learning evidence exists.
inline std::optional<SocialUtilityDecision> preferAutonomousSocialGoalTarget(
    const World& world,
    const RelationshipBook& relationships,
    const Character& actor,
    const SocialUtilityDecision& baseline)
{
    const auto selected=preferAutonomousSocialGoalTargetWithGoal(
        world,relationships,actor,baseline);
    if(!selected) return std::nullopt;
    return selected->decision;
}

} // namespace lifelens
