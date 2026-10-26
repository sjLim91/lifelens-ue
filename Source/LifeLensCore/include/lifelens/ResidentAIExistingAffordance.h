#pragma once

#include <initializer_list>
#include <optional>

#include "CognitiveDecisionJournal.h"
#include "ResidentCognitivePreference.h"
#include "CivilizationDecision.h"
#include "ResidentCivilizationOutcomeLearning.h"
#include "ResidentResourceOutcomePreference.h"
#include "ResidentAICompletedPlanGuard.h"

namespace lifelens {

// One personally chosen strategy may outlive a short inference interval.
// Subsequent Core steps must still satisfy the live world, prerequisites,
// personal outcome evidence and normal bodily survival at every tick.

// Validate the entire Core-authored Gather outcome, not a self-declared
// DirectWitness flag or tags. An unverified future "success" must not erase
// a genuine failure, and forged failures must not poison resident planning.
inline std::optional<bool> residentAIVerifiedGatherOutcome(
    const Character& actor,const MemoryRecord& memory,
    const std::string& key,int minute)
{
    if(memory.source!=MemorySource::DirectWitness
       ||!memory.witnessed ||memory.sourceCharacter!=actor.id
       ||memory.who!=actor.id ||memory.minute<0
       ||memory.minute>minute ||memory.tags.size()!=3
       ||memory.tags[0]!="action_experience"
       ||memory.tags[1]!=key) return std::nullopt;
    const std::string prefix="실제 문명 행동 "+key;
    if(memory.tags[2]=="success"
       &&memory.what==prefix+" 수행에 성공했다") return true;
    if(memory.tags[2]!="failure") return std::nullopt;
    const std::string failure=prefix+" 수행을 시도했지만 실패했다";
    if(memory.what==failure) return false;
    const std::string diagnosed=failure+" (직접 확인한 원인: ";
    for(const char* cause:{"source-removed","source-exhausted",
                           "container-missing","access-unavailable"})
        if(memory.what==diagnosed+cause+")") return false;
    return std::nullopt;
}

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
        const auto succeeded=residentAIVerifiedGatherOutcome(
            actor,memory,key,minute);
        if(!succeeded.has_value()) continue;
        if(memory.minute>latest){
            latest=memory.minute;
            latestFailed=!*succeeded;
        }else if(memory.minute==latest &&*succeeded){
            latestFailed=false;
        }
    }
    return latestFailed &&minute-latest<=1440;
}

// A verified *single* firsthand failure can already prove one known
// resource source was lost. Unlike generic gathering difficulty, exhaustion
// or removal does not require three failed attempts before exploring.
// Compare Core-written action memory text exactly: hearsay, inferred claims,
// arbitrary model rationale, and water container/access failures cannot
// masquerade as a lost source. The latest firsthand SUCCESS overrides it.
inline bool residentAIHasConfirmedLostGatherSource(
    const Character& actor,MaterialKind material,int minute)
{
    if(!actor.alive ||actor.id==0 ||minute<0
       ||material==MaterialKind::Unknown) return false;
    CivilizationUtilityDecision decision;
    decision.intent=CivilizationIntent::Gather;
    decision.material=material;
    const std::string key=civilizationActionExperienceKey(decision);
    const BeliefRecord* belief=actor.beliefs.find(
        0,actionExperienceProposition(key.c_str()));
    if(belief==nullptr ||belief->contradictionCount==0
       ||belief->lastUpdatedMinute>minute) return false;

    // A verified exhausted/removed source remains unavailable even after
    // this resident succeeds at a DIFFERENT node of the same material.
    // Reuse the same actor/witness/Core-text contract as per-node replanning,
    // so a fabricated success cannot erase an authentic loss.
    const auto evidence=buildResidentResourceOutcomeIndex(actor,minute);
    for(const auto& entry:evidence.entries){
        if(entry.first.first!=key) continue;
        const auto& result=entry.second;
        if(result.hasEvidence &&!result.succeeded
           &&result.reason==ResidentResourceOutcomeReason::SourceUnavailable
           &&result.minute>=0
           &&static_cast<std::int64_t>(minute)-result.minute<=1440)
            return true;
    }
    return false;
}

// A witnessed failure at source A remains part of the resident's personal
// learning across fresh model turns for one day. It is different from a
// failure to gather material M anywhere. The Core already records actual node provenance on
// executed gather outcomes. Never infer a failure for an unseen node, and
// never interpret a resident's proposed words as real source evidence.
inline bool residentAIRecentGatherSourceFailed(
    const Character& actor,MaterialKind material,ResourceNodeId node,
    int thoughtMinute,int nowMinute)
{
    if(!actor.alive ||actor.id==0 ||node==0
       ||thoughtMinute<0 ||nowMinute<thoughtMinute
       ||material==MaterialKind::Unknown) return false;
    CivilizationUtilityDecision gather;
    gather.intent=CivilizationIntent::Gather;
    gather.material=material;
    const std::string key=civilizationActionExperienceKey(gather);
    // Both "any failed source" and "this failed source" must use the SAME
    // Core-authored exact-message / actor / time / source evidence contract.
    // Tags or a purported firsthand flag alone are not executed actions.
    const auto evidence=buildResidentResourceOutcomeIndex(actor,nowMinute)
        .find(node,key);
    return evidence.hasEvidence &&!evidence.succeeded
        &&evidence.minute>=0
        &&static_cast<std::int64_t>(nowMinute)-evidence.minute
            <=ResidentAgencyGoalHorizonMinutes;
}

inline bool residentAIHasRecentSourcedGatherFailure(
    const Character& actor,MaterialKind material,
    int thoughtMinute,int nowMinute)
{
    if(!actor.alive ||actor.id==0
       ||thoughtMinute<0 ||nowMinute<thoughtMinute
       ||material==MaterialKind::Unknown) return false;
    CivilizationUtilityDecision gather;
    gather.intent=CivilizationIntent::Gather;
    gather.material=material;
    const std::string key=civilizationActionExperienceKey(gather);
    // A historical failure is not an ONGOING failed source once this same
    // resident has actually succeeded there. The Core-derived per-source
    // index already resolves the latest authentic attempt (same-minute
    // success wins), rejects rumors, and never guesses a failed source from
    // the absence of a node in a bounded perception scan.
    const auto evidence=buildResidentResourceOutcomeIndex(actor,nowMinute);
    for(const auto& entry:evidence.entries)
        if(entry.first.first==key
           &&entry.second.hasEvidence
           &&!entry.second.succeeded
           &&entry.second.minute>=0
           &&nowMinute-entry.second.minute<=ResidentAgencyGoalHorizonMinutes)
            return true;
    return false;
}

// Keep the SAME resident tool/water goal after a verified failure at one
// ingredient source. Core may offer another real node of the exact material.
// Never retry a personally failed node by merely choosing the closest raw
// source, and never conjure a missing ingredient or bypass Core execution.
inline CivilizationUtilityDecision chooseResidentAIFreshIngredientGather(
    const World& world,const Character& resident,GridPos position,
    MaterialKind material,const SettlementPopulation* population=nullptr)
{
    CivilizationUtilityDecision gather;
    if(material==MaterialKind::Unknown) return gather;
    const auto index=buildResidentResourceOutcomeIndex(resident,world.minute);
    CivilizationUtilityDecision keyDecision;
    keyDecision.intent=CivilizationIntent::Gather;
    keyDecision.material=material;
    const std::string key=civilizationActionExperienceKey(keyDecision);
    std::vector<ResourceNodeId> rejected;
    // Action memory is bounded. Exclude at most four real source failures
    // rather than walking unbounded node lists or retrying failed sources.
    for(std::size_t tries=0;tries<=4;++tries){
        gather=bestGatherDecisionAtPosition(
            world,resident,position,population,material,&rejected);
        if(gather.intent!=CivilizationIntent::Gather
           ||gather.material!=material ||gather.resourceNode==0)
            return {};
        const auto outcome=index.find(gather.resourceNode,key);
        if(!outcome.hasEvidence ||outcome.succeeded)
            return gather;
        // A real Core failure at A makes B eligible under the existing
        // physical affordance rules. Source access remains checked by Core.
        rejected.push_back(gather.resourceNode);
    }
    return {};
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
        const auto gather=chooseResidentAIFreshIngredientGather(
            world,resident,position,ingredient.material,population);
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
        const auto gather=chooseResidentAIFreshIngredientGather(
            world,resident,position,ingredient.material,population);
        if(gather.intent==CivilizationIntent::Gather
           &&gather.material==ingredient.material)
            return gather;
    }
    return none;
}

// Only real, unmet TOOL output recipes qualify for this personal goal.
// Ordinary Core craft candidates can also mean sanitation work, facilities
// or practicing already-satisfied outputs. Those remain normal civilization
// options, not evidence of fulfilling the resident's chosen tool objective.
inline bool residentAIToolOutputNeeded(
    const Character& resident,TechniqueId technique)
{
    const TechniqueRecipe recipe=techniqueRecipe(technique);
    if(recipe.technique!=technique ||!recipe.producesItem
       ||recipe.outputQuantity<=0
       ||desiredTechniqueOutputStock(technique)<=0
       ||(itemCapability(recipe.outputKind)==ToolCapability::None
           &&recipe.outputKind!=ItemKind::Cordage))
        return false;
    return resident.civilization.inventory.count(
        recipe.outputKind,recipe.outputMaterial,
        recipe.outputMaterial==MaterialKind::Unknown)
            <desiredTechniqueOutputStock(technique);
}

// Resolve an observed tool's missing knowledge through real Core prerequisite
// edges. Each invocation proposes ONE ordinary Core action; no recipe, item,
// outcome, or learned knowledge is generated by the planner.
inline CivilizationUtilityDecision chooseResidentAIKnowledgePrerequisiteStep(
    const World& world,const Character& resident,GridPos position,
    TechnologyId desired,const SettlementPopulation* population=nullptr,
    const AcceptedCognitiveDecision* thought=nullptr,
    std::size_t depth=0)
{
    if(desired==TechnologyId::None ||depth>=TechnologyRegistry.size())
        return {};
    const auto& knowledge=resident.civilization.knowledge;
    for(const auto& edge:TechnologyPrerequisiteRegistry){
        if(edge.technology!=desired) continue;
        const auto prerequisite=techniqueForTechnology(edge.prerequisite);
        if(prerequisite==TechniqueId::None
           ||knowledge.knowsAtLeast(prerequisite,edge.minimumKnowledge))
            continue;
        const auto preceding=chooseResidentAIKnowledgePrerequisiteStep(
            world,resident,position,edge.prerequisite,population,thought,depth+1);
        if(preceding.intent!=CivilizationIntent::None) return preceding;
    }
    // Metal experiments require a physically built and operational furnace.
    // Under the SAME observed tool goal, planning/delivering/working on the
    // actual project is a genuine executable prerequisite, not free smelting.
    if(desired==TechnologyId::CopperSmelting
       ||desired==TechnologyId::TinSmelting
       ||desired==TechnologyId::BronzeAlloying){
        if(!hasOperationalFurnaceNear(world,position)){
            const auto furnace=bestPrimitiveFurnaceDecision(
                world,resident,position);
            if(furnace.intent==CivilizationIntent::Craft
               &&furnace.facilityKind==FacilityKind::Furnace
               &&furnace.facilityAction!=FacilityBuildAction::None
               &&(thought==nullptr ||!residentAIPlanAlreadyAttempted(
                   resident,*thought,furnace,world.minute)))
                return furnace;
            const auto* project=primitiveFurnaceProjectNear(world,position);
            if(project!=nullptr &&project->state!=FacilityState::Operational){
                for(const auto& requirement:project->requirements){
                    if(requirement.required<=requirement.delivered
                       ||resident.civilization.inventory.count(
                            ItemKind::RawMaterial,requirement.material)>0)
                        continue;
                    const auto ingredient=chooseResidentAIFreshIngredientGather(
                        world,resident,position,requirement.material,population);
                    if(ingredient.intent==CivilizationIntent::Gather
                       &&(thought==nullptr ||!residentAIPlanAlreadyAttempted(
                           resident,*thought,ingredient,world.minute,true)))
                        return ingredient;
                }
            }
            return {};
        }
    }
    if(!technologyKnowledgePrerequisitesSatisfied(knowledge,desired))
        return {};
    const auto technique=techniqueForTechnology(desired);
    if(technique==TechniqueId::None
       ||knowledge.knowsAtLeast(technique,KnowledgeLevel::Reproducible))
        return {};
    const auto experiment=bestExperimentDecisionAtPosition(
        world,resident,position,population,technique);
    if(experiment.intent==CivilizationIntent::Experiment
       &&(thought==nullptr ||!residentAIPlanAlreadyAttempted(
           resident,*thought,experiment,world.minute)))
        return experiment;
    for(const auto kind:CivilizationExperimentCatalog){
        if(experimentTechnique(kind)!=technique) continue;
        const auto gather=chooseResidentAIMissingExperimentMaterial(
            world,resident,position,kind,population);
        if(gather.intent==CivilizationIntent::Gather
           &&(thought==nullptr ||!residentAIPlanAlreadyAttempted(
               resident,*thought,gather,world.minute,true)))
            return gather;
        ExperimentContext ctx;
        ctx.worldSeed=world.seed;
        ctx.actor=resident.id;
        ctx.attemptIndex=static_cast<std::uint64_t>(std::max(0,world.minute));
        ctx.kind=kind;
        ctx.material=experimentMaterial(kind);
        if(kind==ExperimentKind::SmeltCopperOre
           ||kind==ExperimentKind::SmeltTinOre
           ||kind==ExperimentKind::AlloyBronze)
            // This is planning a prerequisite BEFORE its ore arrives;
            // a real operating furnace suffices for the hypothesis. Actual
            // experiment execution will still require charcoal and ore.
            ctx.smeltingOpportunityAvailable=hasOperationalFurnace(world);
        if(!experimentPrerequisitesMet(ctx,knowledge)) continue;
        for(const auto& ingredient:experimentRecipe(ctx).inputs){
            if(ingredient.kind!=ItemKind::RawMaterial
               ||ingredient.material==MaterialKind::Unknown
               ||ingredient.quantity<=0
               ||resident.civilization.inventory.count(
                  ingredient.kind,ingredient.material,ingredient.anyMaterial)
                     >=ingredient.quantity)
                continue;
            // The ordinary experiment chooser cannot start before the
            // ingredient is present. That is a reason to procure a REAL
            // source, not to abandon the resident's original technology goal.
            const auto liveGather=chooseResidentAIFreshIngredientGather(
                world,resident,position,ingredient.material,population);
            if(liveGather.intent==CivilizationIntent::Gather
               &&(thought==nullptr ||!residentAIPlanAlreadyAttempted(
                   resident,*thought,liveGather,world.minute,true)))
                return liveGather;
            // Only an actual Core exploration opportunity can seek remote ore.
            const auto explore=bestResourceExplorationDecisionAtPosition(
                world,resident,position,population,ingredient.material);
            if(explore.intent==CivilizationIntent::Explore
               &&explore.material==ingredient.material
               &&(thought==nullptr ||!residentAIPlanAlreadyAttempted(
                   resident,*thought,explore,world.minute)))
                return explore;
        }
    }
    return {};
}

// Tool-making is a real multi-step personal goal. If the desired item is
// not yet craftable, derive the next legal prerequisite from the SAME Core
// technology/recipe registry instead of abandoning this resident's goal.
// This is only an observation/choice: no free inventory, discoveries or XP.
inline CivilizationUtilityDecision chooseResidentAIToolPrerequisite(
    const World& world,
    const Character& resident,
    GridPos position,
    const SettlementPopulation* population=nullptr,
    const AcceptedCognitiveDecision* thought=nullptr)
{
    CivilizationUtilityDecision best;
    for(const TechnologyDefinition& technology:TechnologyRegistry){
        const TechniqueId technique=technology.legacyTechnique;
        if(!residentAIToolOutputNeeded(resident,technique)) continue;

        CivilizationUtilityDecision next;
        if(resident.civilization.knowledge.knowsAtLeast(
            technique,KnowledgeLevel::Reproducible)){
            // Known tool recipe but missing a real raw ingredient. Other
            // intermediate crafts (cordage, flakes) are separately eligible
            // tool goals in this same registry, with their own real steps.
            next=chooseResidentAIMissingRecipeMaterial(
                world,resident,position,technique,population);
        }else{
            // An unknown tool must actually be experimented with. Its
            // probabilistic success is owned by Core, never guaranteed here.
            next=bestExperimentDecisionAtPosition(
                world,resident,position,population,technique);
            if(next.intent!=CivilizationIntent::Experiment
               ||next.technique!=technique){
                // Lack of ingredients makes a Core experiment unavailable.
                // The next step of the SAME goal can gather the real missing
                // raw input, but only for a technique in Core's own experiment
                // catalog and only if its knowledge prerequisites hold.
                next=CivilizationUtilityDecision{};
                for(const ExperimentKind experiment:CivilizationExperimentCatalog){
                    if(experimentTechnique(experiment)!=technique) continue;
                    const auto gather=chooseResidentAIMissingExperimentMaterial(
                        world,resident,position,experiment,population);
                    if(gather.intent==CivilizationIntent::Gather
                       &&gather.material!=MaterialKind::Unknown)
                        considerCivilizationDecision(next,gather);
                }
                // No direct experiment is available: a genuinely observed
                // advanced tool can seek missing smelting/alloying knowledge.
                if(next.intent==CivilizationIntent::None
                   &&resident.civilization.knowledge.knowsAtLeast(
                       technique,KnowledgeLevel::Observed))
                    next=chooseResidentAIKnowledgePrerequisiteStep(
                        world,resident,position,technology.id,population,thought);
            }
        }
        if((next.intent==CivilizationIntent::Gather
             ||next.intent==CivilizationIntent::Experiment
             ||next.intent==CivilizationIntent::Explore
             ||(next.intent==CivilizationIntent::Craft
                &&next.facilityKind==FacilityKind::Furnace
                &&next.facilityAction!=FacilityBuildAction::None))
           &&std::isfinite(next.utility)
           &&next.utility>=0.14 &&next.utility<=1.0
           // A goal's next action must not be the already-attempted failed
           // experiment/source. Search the SAME legal Core catalog for a
           // fresh possible step, without fabricating success or a recipe.
           &&(thought==nullptr ||!residentAIPlanAlreadyAttempted(
               resident,*thought,next,world.minute,
               next.intent==CivilizationIntent::Gather)))
            considerCivilizationDecision(best,next);
    }
    return best;
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
       ||world.minute-thought.minute>ResidentAgencyGoalHorizonMinutes
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
            // The generic chooser can also return an infrastructure action;
            // it is not authorization for a model's personal tool goal.
            if(candidate.intent!=CivilizationIntent::Craft
               ||candidate.facilityAction!=FacilityBuildAction::None
               ||!residentAIToolOutputNeeded(actor,candidate.technique)
               ||residentAIPlanAlreadyAttempted(
                   actor,thought,candidate,world.minute))
                candidate=chooseResidentAIToolPrerequisite(
                    world,actor,actorPosition,settlementPopulation,&thought);
            // A missing tool may require collecting a REAL ingredient or a
            // Core experiment, not a magical craft on this same tick.
            if(candidate.intent!=CivilizationIntent::Craft
               &&candidate.intent!=CivilizationIntent::Gather
               &&candidate.intent!=CivilizationIntent::Experiment
               &&candidate.intent!=CivilizationIntent::Explore)
                return std::nullopt;
            if(candidate.intent==CivilizationIntent::Craft
               &&candidate.facilityAction!=FacilityBuildAction::None
               &&candidate.facilityKind!=FacilityKind::Furnace)
                return std::nullopt;
            if(candidate.intent==CivilizationIntent::Craft
               &&candidate.technique==TechniqueId::None)
                return std::nullopt;
            break;
        case CognitiveIntentKind::ExpandCultivation:
            candidate=bestCultivationDecision(
                world,actor,actorPosition,settlementPopulation);
            break;
        default:
            return std::nullopt;
    }
    // A failed source is not proof that every other source of the same
    // material has failed. Look for another live Core resource without
    // mutating the World, preserving the resident's chosen original goal.
    if(candidate.intent==CivilizationIntent::Gather
       &&candidate.resourceNode!=0){
        std::vector<ResourceNodeId> rejected;
        // The personal memory ring keeps at most four actual attempts for
        // one material. Bound alternative checks even in huge worlds.
        for(std::size_t attempt=0;attempt<4;++attempt){
            if(!residentAIRecentGatherSourceFailed(
                actor,candidate.material,candidate.resourceNode,
                thought.minute,world.minute)) break;
            rejected.push_back(candidate.resourceNode);
            const auto other=bestGatherDecisionAtPosition(
                world,actor,actorPosition,settlementPopulation,
                candidate.material,&rejected);
            if(other.intent!=CivilizationIntent::Gather
               ||other.material!=candidate.material)
                break;
            candidate=other;
        }
    }

    // Only roam after actual repeated material failure when no fresh known
    // node can satisfy the same material goal. Source-less legacy memories
    // retain the conservative previous behavior.
    MaterialKind retriedMaterial=MaterialKind::Unknown;
    if(candidate.intent==CivilizationIntent::Gather){
        retriedMaterial=candidate.material;
    }else if(thought.intent==CognitiveIntentKind::ImproveFoodSecurity){
        retriedMaterial=MaterialKind::PlantFood;
    }else if(thought.intent==CognitiveIntentKind::ImproveWaterSecurity){
        retriedMaterial=MaterialKind::Water;
    }
    const bool freshRealSource=candidate.intent==CivilizationIntent::Gather
        &&candidate.resourceNode!=0
        &&residentAIHasRecentSourcedGatherFailure(
            actor,retriedMaterial,thought.minute,world.minute)
        &&!residentAIRecentGatherSourceFailed(
            actor,retriedMaterial,candidate.resourceNode,
            thought.minute,world.minute);
    if(retriedMaterial!=MaterialKind::Unknown &&!freshRealSource
       &&(residentAIHasRecentRepeatedGatherFailure(
              actor,retriedMaterial,world.minute)
          ||(candidate.intent!=CivilizationIntent::Gather
             &&residentAIHasConfirmedLostGatherSource(
                 actor,retriedMaterial,world.minute)))){
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
