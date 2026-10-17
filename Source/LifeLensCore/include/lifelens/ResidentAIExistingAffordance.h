#pragma once

#include <optional>

#include "CognitiveDecisionJournal.h"
#include "CivilizationDecision.h"
#include "ResidentCivilizationOutcomeLearning.h"

namespace lifelens {

// Repeated failure is a resident's OWN firsthand evidence. It does not
// establish that all resources are gone, and a later successful attempt
// immediately cancels this short-lived alternative-seeking preference.
inline bool residentAIHasRecentRepeatedFailure(
    const Character& actor,
    const CivilizationUtilityDecision& candidate,
    int currentMinute)
{
    if(currentMinute<0 ||candidate.intent==CivilizationIntent::None)
        return false;
    const std::string key=civilizationActionExperienceKey(candidate);
    const BeliefRecord* belief=actor.beliefs.find(
        0,actionExperienceProposition(key.c_str()));
    if(belief==nullptr
       ||belief->contradictionCount<3
       ||belief->contradictionWeight<belief->supportWeight+2.0)
        return false;

    int lastFailure=-1;
    int lastSuccess=-1;
    for(const MemoryRecord& memory:actor.memory.entries){
        if(memory.source!=MemorySource::DirectWitness
           ||!memory.witnessed
           ||memory.minute<0 ||memory.minute>currentMinute
           ||memory.tags.size()!=3
           ||memory.tags[0]!="action_experience"
           ||memory.tags[1]!=key)
            continue;
        if(memory.tags[2]=="failure")
            lastFailure=std::max(lastFailure,memory.minute);
        else if(memory.tags[2]=="success")
            lastSuccess=std::max(lastSuccess,memory.minute);
    }
    return lastFailure>=0 &&currentMinute-lastFailure<=1440
        &&lastFailure>lastSuccess;
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
    // Never grant a new virtual action. The Core must have produced
    // a positive, currently applicable candidate from actual world truth.
    if(candidate.intent==CivilizationIntent::None
       ||candidate.utility<0.14
       ||candidate.utility>1.0)
        return std::nullopt;

    // A resident should not let the same optional model suggestion
    // monopolize every future slot after repeated REAL failures. Explore
    // only an existing Core-certified opportunity for that SAME material.
    // If no such route exists, let the ordinary Core planner choose again.
    // Urgent survival is selected before this optional resident-AI lane.
    if(residentAIHasRecentRepeatedFailure(actor,candidate,world.minute)){
        if(candidate.intent==CivilizationIntent::Gather){
            const auto alternative=bestResourceExplorationDecisionAtPosition(
                world,actor,actorPosition,settlementPopulation,
                candidate.material);
            if(alternative.intent==CivilizationIntent::Explore
               &&alternative.material==candidate.material
               &&alternative.utility>=0.14
               &&alternative.utility<=1.0)
                return alternative;
        }
        return std::nullopt;
    }
    return candidate;
}

} // namespace lifelens
