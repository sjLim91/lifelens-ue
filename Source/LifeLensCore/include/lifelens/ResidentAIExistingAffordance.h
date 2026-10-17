#pragma once

#include <optional>

#include "CognitiveDecisionJournal.h"
#include "CivilizationDecision.h"
#include "ResidentCivilizationOutcomeLearning.h"

namespace lifelens {

// A model's proposed strategy is not evidence. Require at least three
// firsthand failed Core attempts, a worse-than-neutral accumulated belief,
// and the actor's most recent firsthand outcome for the SAME material being
// a failure. Never learn a false inability from hearsay or other residents.
inline bool residentAIHasRecentRepeatedGatherFailure(
    const Character& actor,MaterialKind material,int minute)
{
    if(!actor.alive ||actor.id==0 ||minute<0
       ||material==MaterialKind::Unknown) return false;
    CivilizationUtilityDecision attempted;
    attempted.intent=CivilizationIntent::Gather;
    attempted.material=material;
    const std::string key=civilizationActionExperienceKey(attempted);
    const BeliefRecord* belief=actor.beliefs.find(
        0,actionExperienceProposition(key.c_str()));
    if(belief==nullptr
       ||belief->contradictionCount<3
       ||belief->contradictionWeight<belief->supportWeight+2.0
       ||belief->lastUpdatedMinute>minute) return false;

    int latest=-1;
    bool latestFailed=false;
    for(const MemoryRecord& memory:actor.memory.entries){
        if(memory.source!=MemorySource::DirectWitness
           ||!memory.witnessed
           ||memory.minute<0 ||memory.minute>minute
           ||memory.tags.size()!=3
           ||memory.tags[0]!="action_experience"
           ||memory.tags[1]!=key) continue;
        if(memory.tags[2]!="failure" &&memory.tags[2]!="success")
            continue;
        if(memory.minute>latest){
            latest=memory.minute;
            latestFailed=memory.tags[2]=="failure";
        }else if(memory.minute==latest
                 &&memory.tags[2]=="success"){
            latestFailed=false;
        }
    }
    return latestFailed &&minute-latest<=1440;
}

// A Core recipe can explain the next prerequisite for an AI resident.
// This only chooses an existing legal raw-material Gather affordance;
// it never manufactures ingredients, knowledge, water, or new actions.
inline CivilizationUtilityDecision
chooseResidentAIMissingRecipeMaterial(
    const World& world,
    const Character& resident,
    GridPos position,
    TechniqueId technique,
    const SettlementPopulation* population=nullptr)
{
    CivilizationUtilityDecision none;
    if(technique==TechniqueId::None
       ||!resident.civilization.knowledge.knowsAtLeast(
           technique,KnowledgeLevel::Reproducible))
        return none;
    const TechniqueRecipe recipe=techniqueRecipe(technique);
    if(recipe.technique!=technique) return none;
    for(const Ingredient& ingredient:recipe.inputs){
        if(ingredient.quantity<=0
           ||ingredient.kind!=ItemKind::RawMaterial
           ||ingredient.material==MaterialKind::Unknown
           ||resident.civilization.inventory.count(
                ingredient.kind,ingredient.material,
                ingredient.anyMaterial)>=ingredient.quantity)
            continue;
        const auto gather=bestGatherDecisionAtPosition(
            world,resident,position,population,ingredient.material);
        if(gather.intent==CivilizationIntent::Gather
           &&gather.material==ingredient.material)
            return gather;
    }
    return none;
}

// A resident without reproducible technique knowledge may only attempt
// an existing Core experiment. If its real experiment recipe lacks raw
// ingredients, gathering an actual resource can be the next bounded step.
// This helper neither learns the technique nor performs the experiment.
inline CivilizationUtilityDecision
chooseResidentAIMissingExperimentMaterial(
    const World& world,
    const Character& resident,
    GridPos position,
    ExperimentKind experiment,
    const SettlementPopulation* population=nullptr)
{
    CivilizationUtilityDecision none;
    const TechniqueId technique=experimentTechnique(experiment);
    if(technique==TechniqueId::None
       ||resident.civilization.knowledge.knowsAtLeast(
           technique,KnowledgeLevel::Reproducible)
       ||!technologyKnowledgePrerequisitesSatisfied(
           resident.civilization.knowledge,technologyIdForTechnique(technique)))
        return none;

    ExperimentContext context;
    context.worldSeed=world.seed;
    context.actor=resident.id;
    context.attemptIndex=static_cast<std::uint64_t>(std::max(0,world.minute));
    context.kind=experiment;
    context.material=experimentMaterial(experiment);
    if(!experimentPrerequisitesMet(context,resident.civilization.knowledge))
        return none;

    const TechniqueRecipe recipe=experimentRecipe(context);
    for(const Ingredient& ingredient:recipe.inputs){
        if(ingredient.quantity<=0
           ||ingredient.kind!=ItemKind::RawMaterial
           ||ingredient.material==MaterialKind::Unknown
           ||resident.civilization.inventory.count(
               ingredient.kind,ingredient.material,ingredient.anyMaterial)
                  >=ingredient.quantity)
            continue;
        const auto gather=bestGatherDecisionAtPosition(
            world,resident,position,population,ingredient.material);
        if(gather.intent==CivilizationIntent::Gather
           &&gather.material==ingredient.material)
            return gather;
    }
    return none;
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
            // A desire for water does not create an imaginary water item.
            // If the real blocker is the absence of a transport vessel, the
            // resident may instead craft an actually known, affordable
            // SimpleContainer using the existing Core recipe.
            if(candidate.intent==CivilizationIntent::None
               &&emptySimpleContainerCount(actor.civilization.inventory)<=0
               &&knownNaturalResourceUnits(world,MaterialKind::Water)>0){
                candidate=bestCraftDecisionAtPosition(
                    world,actor,actorPosition,settlementPopulation,
                    TechniqueId::SimpleContainer);
                if(candidate.intent==CivilizationIntent::None){
                    if(actor.civilization.knowledge.knowsAtLeast(
                        TechniqueId::SimpleContainer,KnowledgeLevel::Reproducible)){
                        candidate=chooseResidentAIMissingRecipeMaterial(
                            world,actor,actorPosition,
                            TechniqueId::SimpleContainer,settlementPopulation);
                    }else{
                        // An unknown vessel technique cannot be crafted.
                        // ShapeClay is a real, probabilistic Core experiment;
                        // lack of real clay may first require a real gather.
                        candidate=bestExperimentDecisionAtPosition(
                            world,actor,actorPosition,settlementPopulation,
                            TechniqueId::SimpleContainer);
                        if(candidate.intent==CivilizationIntent::None)
                            candidate=chooseResidentAIMissingExperimentMaterial(
                                world,actor,actorPosition,
                                ExperimentKind::ShapeClay,settlementPopulation);
                    }
                }
            }
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
    // After repeated REAL failure to gather this material, consider
    // exploring for another genuine source instead of prescribing the same
    // failing gather forever. Exploration must be a valid, same-material
    // Core affordance; if none exists, retain the normal decision/fallback.
    MaterialKind retriedMaterial=MaterialKind::Unknown;
    if(candidate.intent==CivilizationIntent::Gather){
        retriedMaterial=candidate.material;
    }else if(thought.intent==CognitiveIntentKind::ImproveFoodSecurity){
        retriedMaterial=MaterialKind::PlantFood;
    }else if(thought.intent==CognitiveIntentKind::ImproveWaterSecurity){
        retriedMaterial=MaterialKind::Water;
    }
    if(retriedMaterial!=MaterialKind::Unknown
       &&residentAIHasRecentRepeatedGatherFailure(
           actor,retriedMaterial,world.minute)){
        const auto alternate=bestResourceExplorationDecisionAtPosition(
            world,actor,actorPosition,settlementPopulation,retriedMaterial);
        if(alternate.intent==CivilizationIntent::Explore
           &&alternate.material==retriedMaterial
           &&std::isfinite(alternate.utility)
           &&alternate.utility>=0.14 &&alternate.utility<=1.0)
            candidate=alternate;
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
