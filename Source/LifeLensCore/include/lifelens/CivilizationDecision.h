#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>

#include "Civilization.h"
#include "Character.h"
#include "World.h"

namespace lifelens {

enum class CivilizationIntent {
    None,
    Gather,
    Store,
    Experiment,
    Craft
};

inline const char* civilizationIntentName(CivilizationIntent intent)
{
    switch(intent){
        case CivilizationIntent::Gather: return "Gather";
        case CivilizationIntent::Store: return "Store";
        case CivilizationIntent::Experiment: return "Experiment";
        case CivilizationIntent::Craft: return "Craft";
        default: return "None";
    }
}

inline const char* materialName(MaterialKind material)
{
    switch(material){
        case MaterialKind::Stone: return "Stone";
        case MaterialKind::Flint: return "Flint";
        case MaterialKind::Wood: return "Wood";
        case MaterialKind::Fiber: return "Fiber";
        case MaterialKind::Clay: return "Clay";
        case MaterialKind::Water: return "Water";
        case MaterialKind::PlantFood: return "PlantFood";
        case MaterialKind::Bone: return "Bone";
        case MaterialKind::Hide: return "Hide";
        case MaterialKind::CopperOre: return "CopperOre";
        case MaterialKind::TinOre: return "TinOre";
        case MaterialKind::IronOre: return "IronOre";
        case MaterialKind::Charcoal: return "Charcoal";
        default: return "Unknown";
    }
}

inline const char* techniqueName(TechniqueId technique)
{
    switch(technique){
        case TechniqueId::SharpFlake: return "SharpFlake";
        case TechniqueId::ChippedStoneTool: return "ChippedStoneTool";
        case TechniqueId::FireMaking: return "FireMaking";
        case TechniqueId::FiberCordage: return "FiberCordage";
        case TechniqueId::SimpleContainer: return "SimpleContainer";
        default: return "None";
    }
}

struct CivilizationUtilityDecision {
    CivilizationIntent intent=CivilizationIntent::None;
    double utility=0.0;
    ResourceNodeId resourceNode=0;
    StorageId storage=0;
    ExperimentKind experiment=ExperimentKind::StrikeStone;
    MaterialKind material=MaterialKind::Unknown;
    TechniqueId technique=TechniqueId::None;
    ItemKind item=ItemKind::RawMaterial;
    int quantity=0;
};

struct CivilizationExecutionResult {
    bool executed=false;
    bool success=false;
    CivilizationEvent event{};
    ExperimentResult experiment{};
    CraftResult craft{};
};

inline double civilizationPreference(std::uint64_t worldSeed,CharacterId actor,std::uint64_t salt)
{
    std::uint64_t value=worldSeed ? worldSeed : 1;
    value=civilizationMix(value^actor);
    value=civilizationMix(value^(salt+1ULL)*0x9e3779b97f4a7c15ULL);
    const std::uint64_t mantissa=value>>11;
    return static_cast<double>(mantissa)*(1.0/9007199254740992.0);
}

inline int inventoryUnitCount(const Inventory& inventory)
{
    int total=0;
    for(const auto& stack:inventory.stacks()) total+=std::max(0,stack.quantity);
    return total;
}

inline int storageCountForMaterial(const World& world,MaterialKind material)
{
    int total=0;
    for(const auto& storage:world.storageSites){
        total+=storage.inventory.count(ItemKind::RawMaterial,material);
    }
    return total;
}

inline int worldItemCount(const World& world,const Character& self,ItemKind kind,MaterialKind material,bool anyMaterial=false)
{
    int total=self.civilization.inventory.count(kind,material,anyMaterial);
    for(const auto& storage:world.storageSites) total+=storage.inventory.count(kind,material,anyMaterial);
    return total;
}

inline const TechniqueKnowledge* civilizationKnowledgeRecord(const KnowledgeState& knowledge,TechniqueId technique)
{
    for(const auto& record:knowledge.all()) if(record.technique==technique) return &record;
    return nullptr;
}

inline MaterialKind experimentMaterial(ExperimentKind kind)
{
    switch(kind){
        case ExperimentKind::StrikeStone: return MaterialKind::Flint;
        case ExperimentKind::HaftSharpFlake: return MaterialKind::Wood;
        case ExperimentKind::FrictionWood: return MaterialKind::Wood;
        case ExperimentKind::TwistFiber: return MaterialKind::Fiber;
        case ExperimentKind::ShapeClay: return MaterialKind::Clay;
        default: return MaterialKind::Unknown;
    }
}

inline double materialProgressDemand(const Character& self,MaterialKind material)
{
    const KnowledgeState& knowledge=self.civilization.knowledge;
    switch(material){
        case MaterialKind::Flint:
            return knowledge.knowsAtLeast(TechniqueId::SharpFlake,KnowledgeLevel::Reproducible) ? 0.55 : 1.0;
        case MaterialKind::Wood:
            if(!knowledge.knowsAtLeast(TechniqueId::SharpFlake,KnowledgeLevel::Reproducible)) return 0.30;
            if(!knowledge.knowsAtLeast(TechniqueId::ChippedStoneTool,KnowledgeLevel::Reproducible)) return 0.92;
            if(!knowledge.knowsAtLeast(TechniqueId::FireMaking,KnowledgeLevel::Reproducible)) return 0.82;
            return 0.45;
        case MaterialKind::Fiber:
            return knowledge.knowsAtLeast(TechniqueId::FiberCordage,KnowledgeLevel::Reproducible) ? 0.38 : 0.84;
        case MaterialKind::Clay:
            return knowledge.knowsAtLeast(TechniqueId::SimpleContainer,KnowledgeLevel::Reproducible) ? 0.35 : 0.80;
        case MaterialKind::PlantFood:
            return 0.25+0.55*clampCivilization01(self.needs.hunger);
        case MaterialKind::Water:
            return 0.25+0.55*clampCivilization01(self.needs.thirst);
        case MaterialKind::Stone:
            return 0.24;
        default:
            return 0.12;
    }
}

inline void considerCivilizationDecision(CivilizationUtilityDecision& best,const CivilizationUtilityDecision& candidate)
{
    if(candidate.intent==CivilizationIntent::None || candidate.utility<=0.0) return;
    if(candidate.utility>best.utility+1e-12) best=candidate;
}

inline CivilizationUtilityDecision bestGatherDecision(const World& world,const Character& self)
{
    CivilizationUtilityDecision best;
    for(const auto& node:world.resourceNodes){
        if(node.id==0 || node.quantity<=0 || node.material==MaterialKind::Unknown) continue;
        const int held=self.civilization.inventory.count(ItemKind::RawMaterial,node.material);
        const int stored=storageCountForMaterial(world,node.material);
        const int target=(node.material==MaterialKind::Water || node.material==MaterialKind::PlantFood) ? 4 : 5;
        const double gap=clampCivilization01(static_cast<double>(std::max(0,target-held-std::min(stored,2)))/static_cast<double>(target));
        const double demand=materialProgressDemand(self,node.material);
        const double preference=civilizationPreference(world.seed,self.id,100ULL+static_cast<std::uint64_t>(node.material));
        const double score=clampCivilization01(
            0.07+0.12*self.personality.curiosity+0.05*self.personality.adaptability+
            0.08*self.civilization.gatheringSkill+0.16*demand+0.13*gap+0.07*preference);
        CivilizationUtilityDecision candidate;
        candidate.intent=CivilizationIntent::Gather;
        candidate.utility=score;
        candidate.resourceNode=node.id;
        candidate.material=node.material;
        candidate.item=ItemKind::RawMaterial;
        candidate.quantity=2+static_cast<int>(2.0*clampCivilization01(self.civilization.gatheringSkill));
        considerCivilizationDecision(best,candidate);
    }
    return best;
}

inline CivilizationUtilityDecision bestExperimentDecision(const World& world,const Character& self)
{
    CivilizationUtilityDecision best;
    const std::array<ExperimentKind,5> experiments={
        ExperimentKind::StrikeStone,ExperimentKind::HaftSharpFlake,ExperimentKind::FrictionWood,
        ExperimentKind::TwistFiber,ExperimentKind::ShapeClay};

    for(const ExperimentKind kind:experiments){
        const TechniqueId technique=experimentTechnique(kind);
        if(technique==TechniqueId::None) continue;
        if(self.civilization.knowledge.knowsAtLeast(technique,KnowledgeLevel::Reproducible)) continue;

        ExperimentContext context;
        context.worldSeed=world.seed;
        context.actor=self.id;
        context.attemptIndex=static_cast<std::uint64_t>(std::max(0,world.minute));
        context.kind=kind;
        context.material=experimentMaterial(kind);
        context.learningSkill=self.civilization.learningSkill;
        context.curiosity=self.personality.curiosity;
        context.patience=self.personality.patience;

        if(!experimentPrerequisitesMet(context,self.civilization.knowledge)) continue;
        const TechniqueRecipe recipe=experimentRecipe(context);
        if(!hasIngredients(self.civilization.inventory,recipe.inputs)) continue;
        if(experimentBaseChance(kind,context.material)<=0.0) continue;

        const KnowledgeLevel level=self.civilization.knowledge.level(technique);
        const double hypothesisBoost=level==KnowledgeLevel::Hypothesized ? 0.08 : (level==KnowledgeLevel::Understood ? 0.05 : 0.0);
        const double preference=civilizationPreference(world.seed,self.id,200ULL+static_cast<std::uint64_t>(kind));
        const double score=clampCivilization01(
            0.11+0.22*self.personality.curiosity+0.10*self.personality.openness+
            0.07*self.personality.patience+0.12*self.civilization.learningSkill+0.08*preference+hypothesisBoost);

        CivilizationUtilityDecision candidate;
        candidate.intent=CivilizationIntent::Experiment;
        candidate.utility=score;
        candidate.experiment=kind;
        candidate.material=context.material;
        candidate.technique=technique;
        considerCivilizationDecision(best,candidate);
    }
    return best;
}

inline int desiredTechniqueOutputStock(TechniqueId technique)
{
    switch(technique){
        case TechniqueId::SharpFlake: return 2;
        case TechniqueId::ChippedStoneTool: return 1;
        case TechniqueId::FiberCordage: return 2;
        case TechniqueId::SimpleContainer: return 1;
        case TechniqueId::FireMaking: return 0;
        default: return 0;
    }
}

inline CivilizationUtilityDecision bestCraftDecision(const World& world,const Character& self)
{
    CivilizationUtilityDecision best;
    const std::array<TechniqueId,5> techniques={TechniqueId::SharpFlake,TechniqueId::ChippedStoneTool,TechniqueId::FireMaking,TechniqueId::FiberCordage,TechniqueId::SimpleContainer};

    for(const TechniqueId technique:techniques){
        if(!self.civilization.knowledge.knowsAtLeast(technique,KnowledgeLevel::Reproducible)) continue;
        const TechniqueRecipe recipe=techniqueRecipe(technique);
        if(recipe.technique==TechniqueId::None || !hasIngredients(self.civilization.inventory,recipe.inputs)) continue;

        const TechniqueKnowledge* record=civilizationKnowledgeRecord(self.civilization.knowledge,technique);
        const int successfulUses=record ? record->successfulUses : 0;
        double stockNeed=0.0;
        if(recipe.producesItem && recipe.outputQuantity>0){
            const int desired=desiredTechniqueOutputStock(technique);
            const int available=worldItemCount(world,self,recipe.outputKind,recipe.outputMaterial,recipe.outputMaterial==MaterialKind::Unknown);
            stockNeed=desired>0 ? clampCivilization01(static_cast<double>(std::max(0,desired-available))/static_cast<double>(desired)) : 0.0;
        }else{
            stockNeed=successfulUses<3 ? 1.0 : 0.0;
        }
        const double practiceNeed=successfulUses<3 ? 1.0 : (successfulUses<12 ? 0.35 : 0.0);
        if(stockNeed<=0.0 && practiceNeed<=0.0) continue;

        const double preference=civilizationPreference(world.seed,self.id,300ULL+static_cast<std::uint64_t>(technique));
        const double score=clampCivilization01(
            0.08+0.11*self.civilization.craftingSkill+0.08*self.personality.conscientiousness+
            0.05*self.personality.curiosity+0.05*preference+0.15*stockNeed+0.07*practiceNeed);

        CivilizationUtilityDecision candidate;
        candidate.intent=CivilizationIntent::Craft;
        candidate.utility=score;
        candidate.technique=technique;
        candidate.item=recipe.outputKind;
        candidate.material=recipe.outputMaterial;
        candidate.quantity=recipe.outputQuantity;
        considerCivilizationDecision(best,candidate);
    }
    return best;
}

inline CivilizationUtilityDecision bestStoreDecision(const World& world,const Character& self)
{
    CivilizationUtilityDecision best;
    if(world.storageSites.empty()) return best;
    const int total=inventoryUnitCount(self.civilization.inventory);
    if(total<7) return best;

    const StorageId storage=world.storageSites.front().id;
    for(const auto& stack:self.civilization.inventory.stacks()){
        const int keep=stack.kind==ItemKind::RawMaterial ? 4 : 1;
        const int surplus=stack.quantity-keep;
        if(surplus<=0) continue;
        const double fullness=clampCivilization01(static_cast<double>(std::max(0,total-6))/10.0);
        const double preference=civilizationPreference(world.seed,self.id,400ULL+static_cast<std::uint64_t>(stack.kind)*32ULL+static_cast<std::uint64_t>(stack.material));
        const double score=clampCivilization01(
            0.07+0.15*self.personality.orderliness+0.09*self.personality.conscientiousness+
            0.13*fullness+0.04*preference);

        CivilizationUtilityDecision candidate;
        candidate.intent=CivilizationIntent::Store;
        candidate.utility=score;
        candidate.storage=storage;
        candidate.item=stack.kind;
        candidate.material=stack.material;
        candidate.quantity=std::min(surplus,3);
        considerCivilizationDecision(best,candidate);
    }
    return best;
}

inline CivilizationUtilityDecision chooseCivilizationUtilityDecision(const World& world,const Character& self)
{
    CivilizationUtilityDecision best;
    if(self.id==0 || self.civilization.character!=self.id) return best;
    considerCivilizationDecision(best,bestExperimentDecision(world,self));
    considerCivilizationDecision(best,bestCraftDecision(world,self));
    considerCivilizationDecision(best,bestStoreDecision(world,self));
    considerCivilizationDecision(best,bestGatherDecision(world,self));
    return best;
}

inline ResourceNode* findCivilizationResource(World& world,ResourceNodeId id)
{
    for(auto& node:world.resourceNodes) if(node.id==id) return &node;
    return nullptr;
}

inline StorageSite* findCivilizationStorage(World& world,StorageId id)
{
    for(auto& storage:world.storageSites) if(storage.id==id) return &storage;
    return nullptr;
}

inline CivilizationExecutionResult executeCivilizationDecision(World& world,Character& self,const CivilizationUtilityDecision& decision)
{
    CivilizationExecutionResult result;
    switch(decision.intent){
        case CivilizationIntent::Gather: {
            ResourceNode* node=findCivilizationResource(world,decision.resourceNode);
            if(!node) return result;
            result.event=gatherResource(self.civilization,*node,std::max(1,decision.quantity));
            result.executed=result.event.quantity>0;
            result.success=result.executed;
            if(result.executed) self.civilization.gatheringSkill=clampCivilization01(self.civilization.gatheringSkill+0.0015*static_cast<double>(result.event.quantity));
            return result;
        }
        case CivilizationIntent::Store: {
            StorageSite* storage=findCivilizationStorage(world,decision.storage);
            if(!storage) return result;
            result.event=storeItems(self.civilization,*storage,decision.item,decision.material,std::max(1,decision.quantity));
            result.executed=result.event.quantity>0;
            result.success=result.executed;
            return result;
        }
        case CivilizationIntent::Experiment: {
            ExperimentContext context;
            context.worldSeed=world.seed;
            context.actor=self.id;
            context.attemptIndex=static_cast<std::uint64_t>(std::max(0,world.minute));
            context.kind=decision.experiment;
            context.material=decision.material;
            context.learningSkill=self.civilization.learningSkill;
            context.curiosity=self.personality.curiosity;
            context.patience=self.personality.patience;
            result.experiment=attemptExperiment(context,self.civilization.inventory,self.civilization.knowledge);
            result.executed=result.experiment.attempted;
            result.success=result.experiment.success;
            result.event=result.experiment.event;
            if(result.executed) self.civilization.learningSkill=clampCivilization01(self.civilization.learningSkill+(result.success ? 0.006 : 0.0025));
            return result;
        }
        case CivilizationIntent::Craft: {
            result.craft=reproduceTechnique(self.id,decision.technique,self.civilization.inventory,self.civilization.knowledge,self.civilization.craftingSkill);
            result.executed=result.craft.success;
            result.success=result.craft.success;
            result.event=result.craft.event;
            if(result.success) self.civilization.craftingSkill=clampCivilization01(self.civilization.craftingSkill+0.004);
            return result;
        }
        case CivilizationIntent::None:
        default:
            return result;
    }
}

inline void regenerateCivilizationEnvironment(World& world)
{
    for(auto& node:world.resourceNodes) node.regenerateDay();
}

} // namespace lifelens
