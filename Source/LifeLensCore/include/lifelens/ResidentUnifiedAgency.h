#pragma once

#include <cmath>
#include <optional>

#include "ResidentAIExistingAffordance.h"
#include "ResidentAICompletedPlanGuard.h"
#include "AutonomousSocialOutcomeLearning.h"
#include "SocialUtility.h"

namespace lifelens {

// ONE resident decision slot. A real resident cognition chooses a strategic
// preference; Core only establishes the physically/socially executable
// affordance. Survival reflexes and offline deterministic choice remain
// fallbacks, not separate agents racing the same actor.
// No entity, inventory, relationship, RNG or simulation state is mutated.
enum class ResidentAgencyAuthority {
    EmbodiedFallback,
    ResidentCognition,
    PersonalGoal
};

struct ResidentAgencyChoice {
    UnifiedUtilityDecision decision{};
    ResidentAgencyAuthority authority=ResidentAgencyAuthority::EmbodiedFallback;
    CognitiveIntentKind cognitiveIntent=CognitiveIntentKind::None;
    std::uint64_t contextFingerprint=0;
};

inline ResidentAgencyChoice chooseSingleResidentAgencyDecision(
    const World& world,
    const Character& actor,
    GridPos actorPosition,
    const RelationshipBook& relationships,
    const UnifiedUtilityDecision& fallback,
    const AcceptedCognitiveDecision* newestThought,
    const SettlementPopulation* population,
    std::optional<GridPos> socialTargetPosition,
    bool urgentNeed)
{
    ResidentAgencyChoice answer;
    answer.decision=fallback;
    if(!actor.alive ||actor.id==0
       ||requiresDirectCare(actor.lifeStage) ||urgentNeed
       ||newestThought==nullptr) return answer;
    const AcceptedCognitiveDecision& thought=*newestThought;
    // This is the sole Core decision slot: even if a caller bypasses the
    // normal accepted-journal gate, an unbound or malformed model thought
    // cannot obtain physical/social decision authority.
    if(thought.actor!=actor.id || thought.minute<0
       ||thought.minute>world.minute
       ||world.minute-thought.minute>ResidentAgencyGoalHorizonMinutes
       ||thought.contextFingerprint==0
       ||thought.intent==CognitiveIntentKind::None
       ||!std::isfinite(thought.priority)
       ||thought.priority<0.50 ||thought.priority>1.0) return answer;

    const auto select=[&](const UnifiedUtilityDecision& chosen){
        answer.decision=chosen;
        answer.authority=ResidentAgencyAuthority::ResidentCognition;
        answer.cognitiveIntent=thought.intent;
        answer.contextFingerprint=thought.contextFingerprint;
    };

    if(thought.intent==CognitiveIntentKind::CooperateWithResident
       ||thought.intent==CognitiveIntentKind::ResolveConflict){
        // A chosen resident cannot dismiss avoidance or someone else's
        // emergency comfort merely to satisfy an optional private goal.
        if(fallback.kind==UnifiedDecisionKind::Social
           &&(fallback.social.intent==SocialIntent::Avoid
              ||fallback.social.intent==SocialIntent::Comfort))
            return answer;
        if(thought.target==0 ||thought.target==actor.id
           ||!socialTargetPosition) return answer;
        const auto dx=std::abs(
            static_cast<std::int64_t>(actorPosition.x)
            -socialTargetPosition->x);
        const auto dy=std::abs(
            static_cast<std::int64_t>(actorPosition.y)
            -socialTargetPosition->y);
        if(dx+dy>SettlementServiceRadiusGrid) return answer;
        const Character* target=cognitiveCharacter(world,thought.target);
        if(target==nullptr ||!target->alive) return answer;
        const bool cooperate=
            thought.intent==CognitiveIntentKind::CooperateWithResident;
        const SocialIntent intent=cooperate
            ?SocialIntent::Approach:SocialIntent::Repair;
        const double baseScore=cooperate
            ?scoreApproachIntent(world,actor,thought.target,relationships)
            :scoreRepairIntent(actor,thought.target,relationships);
        // The SAME resident's already recorded firsthand self-mood outcome
        // now influences its REAL optional social choice, not just a
        // read-only hypothetical plan. Other minds and proposed words cannot
        // improve that actor's utility. This never grants a blocked action,
        // bypasses care/avoidance, or overrides urgent survival.
        const double personalDisposition=learnedSocialMoodDisposition(
            actor,thought.target,intent);
        const double score=std::clamp(
            baseScore*(1.0+0.25*personalDisposition),0.0,1.0);
        if(score<0.18) return answer;

        UnifiedUtilityDecision next=fallback;
        next.kind=UnifiedDecisionKind::Social;
        next.social={intent,thought.target,score};
        next.utility=score;
        select(next);
        return answer;
    }

    if(!lifeStageProfile(actor.lifeStage).canWork
       ||thought.priority<0.55
       ||(fallback.kind==UnifiedDecisionKind::Civilization
          &&fallback.civilization.intent==CivilizationIntent::Craft
          &&fallback.civilization.facilityAction
              !=FacilityBuildAction::None))
        return answer;

    const auto candidate=chooseResidentAIExistingCivilizationAffordance(
        world,actor,actorPosition,thought,population);
    if(!candidate
       ||residentAIPlanAlreadyAttempted(
           actor,thought,*candidate,world.minute,
           thought.intent==CognitiveIntentKind::CraftUsefulTools
               &&candidate->intent==CivilizationIntent::Gather))
        return answer;
    UnifiedUtilityDecision next=fallback;
    next.kind=UnifiedDecisionKind::Civilization;
    next.civilization=*candidate;
    next.utility=candidate->utility;
    select(next);
    return answer;
}

} // namespace lifelens
