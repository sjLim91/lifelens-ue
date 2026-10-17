#pragma once

#include <optional>

#include "CognitiveDecisionJournal.h"
#include "CivilizationDecision.h"
#include "ResidentCivilizationOutcomeLearning.h"

namespace lifelens {

// A recent, repeatedly FAILED firsthand action should no longer be forced
// as the next AI action every 15 simulated minutes. The Core's authoritative
// completed-attempt beliefs and memory are the only evidence here. Model
// assertions and a pending (unexecuted) action are not learning signals.
// This is a soft fallback: it never disables urgent provision, rewrites Core
// Need thresholds, invents materials, or blocks future successful attempts.
inline bool residentAIRecentRepeatedActionFailure(
    const Character& actor,
    const CivilizationUtilityDecision& candidate,
    int minute)
{
    if(minute<0 ||candidate.intent==CivilizationIntent::None) return false;
    const std::string action=civilizationActionExperienceKey(candidate);
    if(action.empty()) return false;
    const BeliefRecord* belief=actor.beliefs.find(
        0,actionExperienceProposition(action.c_str()));
    if(belief==nullptr ||belief->contradictionCount<2
       ||belief->contradictionWeight<belief->supportWeight+1.5
       ||belief->lastUpdatedMinute>minute
       ||minute-belief->lastUpdatedMinute>120) return false;

    int latestFailure=-1;
    int latestSuccess=-1;
    for(const auto& memory:actor.memory.entries){
        if(memory.source!=MemorySource::DirectWitness
           ||!memory.witnessed
           ||memory.minute<0 ||memory.minute>minute
           ||minute-memory.minute>120
           ||memory.tags.size()!=3
           ||memory.tags[0]!="action_experience"
           ||memory.tags[1]!=action)
            continue;
        if(memory.tags[2]=="failure")
            latestFailure=std::max(latestFailure,memory.minute);
        if(memory.tags[2]=="success")
            latestSuccess=std::max(latestSuccess,memory.minute);
    }
    return latestFailure>=0 &&latestFailure>latestSuccess;
}

// Translate an accepted resident's *strategic preference* into ONE existing
// Core-authorized affordance candidate. This is never an action execution:
// normal Civilization routing, inventory, terrain, path and outcome checks
// still own every physical consequence.
inline std::optional<CivilizationUtilityDecision>
chooseResidentAIExistingCivilizationAffordance(
    const World& world,
    const Character& actor,
    GridPos actorPosition,
    const AcceptedCognitiveDecision& thought,
    const SettlementPopulation* settlementPopulation=nullptr)
{
    if(actor.id==0 ||!actor.alive
       ||actor.civilization.character!=actor.id
       ||thought.actor!=actor.id
       ||thought.minute>world.minute
       ||world.minute-thought.minute>96
       ||thought.priority<0.55
       ||thought.target!=0
       ||!std::isfinite(thought.priority))
        return std::nullopt;

    CivilizationUtilityDecision candidate;
    switch(thought.intent){
        case CognitiveIntentKind::ImproveFoodSecurity:
            candidate=bestGatherDecisionAtPosition(
                world,actor,actorPosition,settlementPopulation,
                MaterialKind::PlantFood);
            break;
        case CognitiveIntentKind::ImproveWaterSecurity:
            candidate=bestGatherDecisionAtPosition(
                world,actor,actorPosition,settlementPopulation,
                MaterialKind::Water);
            break;
        case CognitiveIntentKind::AcquireMaterials:
            candidate=bestGatherDecisionAtPosition(
                world,actor,actorPosition,settlementPopulation);
            break;
        case CognitiveIntentKind::ExploreOpportunity:
            candidate=bestResourceExplorationDecisionAtPosition(
                world,actor,actorPosition);
            break;
        case CognitiveIntentKind::CraftUsefulTools:
            candidate=bestCraftDecisionAtPosition(
                world,actor,actorPosition,settlementPopulation);
            // Toolcraft is different from unrequested settlement projects.
            if(candidate.intent!=CivilizationIntent::Craft
               ||candidate.facilityAction!=FacilityBuildAction::None
               ||candidate.technique==TechniqueId::None)
                return std::nullopt;
            break;
        case CognitiveIntentKind::ExpandCultivation:
            candidate=bestCultivationDecision(
                world,actor,actorPosition,settlementPopulation);
            break;
        default:
            return std::nullopt;
    }
    // After real repeated failures, prefer an EXISTING exploration
    // affordance instead of reissuing the same model-backed attempt.
    // If none is legal, leave the slot to the ordinary Core planner.
    if(residentAIRecentRepeatedActionFailure(actor,candidate,world.minute)){
        const CivilizationUtilityDecision alternative=
            bestResourceExplorationDecisionAtPosition(
                world,actor,actorPosition);
        if(alternative.intent==CivilizationIntent::Explore
           &&alternative.utility>=0.14
           &&alternative.utility<=1.0
           &&std::isfinite(alternative.utility)
           &&!residentAIRecentRepeatedActionFailure(
                actor,alternative,world.minute))
            return alternative;
        return std::nullopt;
    }
    // Never grant a new virtual action. The Core must have produced
    // a positive, currently applicable candidate from actual world truth.
    if(candidate.intent==CivilizationIntent::None
       ||candidate.utility<0.14
       ||candidate.utility>1.0)
        return std::nullopt;
    return candidate;
}

} // namespace lifelens
