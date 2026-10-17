#pragma once

#include <optional>

#include "CognitiveDecisionJournal.h"
#include "CivilizationDecision.h"

namespace lifelens {

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
       ||thought.target!=0)
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
    return candidate;
}

} // namespace lifelens
