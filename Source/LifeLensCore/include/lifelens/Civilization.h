#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include "Ids.h"

namespace lifelens {

using ResourceNodeId = std::uint64_t;
using StorageId = std::uint64_t;

enum class MaterialKind {
    Unknown,
    Stone,
    Flint,
    Wood,
    Fiber,
    Clay,
    Water,
    PlantFood,
    Bone,
    Hide,
    CopperOre,
    TinOre,
    IronOre,
    Charcoal
};

struct MaterialProperties {
    double hardness=0.0;
    double sharpnessPotential=0.0;
    double brittleness=0.0;
    double flexibility=0.0;
    double combustibility=0.0;
    double heatResistance=0.0;
    double nutrition=0.0;
};

inline MaterialProperties materialProperties(MaterialKind kind)
{
    switch(kind){
        case MaterialKind::Stone: return {0.76,0.48,0.52,0.02,0.00,0.88,0.00};
        case MaterialKind::Flint: return {0.82,0.94,0.86,0.01,0.00,0.90,0.00};
        case MaterialKind::Wood: return {0.34,0.18,0.20,0.42,0.88,0.24,0.00};
        case MaterialKind::Fiber: return {0.08,0.02,0.08,0.92,0.72,0.08,0.00};
        case MaterialKind::Clay: return {0.12,0.01,0.18,0.66,0.00,0.34,0.00};
        case MaterialKind::Water: return {0.00,0.00,0.00,1.00,0.00,0.04,0.00};
        case MaterialKind::PlantFood: return {0.04,0.00,0.08,0.36,0.54,0.06,0.72};
        case MaterialKind::Bone: return {0.58,0.42,0.44,0.08,0.08,0.54,0.00};
        case MaterialKind::Hide: return {0.10,0.01,0.05,0.88,0.30,0.12,0.00};
        case MaterialKind::CopperOre: return {0.72,0.12,0.22,0.02,0.00,0.82,0.00};
        case MaterialKind::TinOre: return {0.62,0.10,0.26,0.02,0.00,0.76,0.00};
        case MaterialKind::IronOre: return {0.86,0.08,0.20,0.01,0.00,0.94,0.00};
        case MaterialKind::Charcoal: return {0.18,0.02,0.64,0.02,0.98,0.52,0.00};
        case MaterialKind::Unknown:
        default: return {};
    }
}

enum class ItemKind {
    RawMaterial,
    SharpFlake,
    StoneCuttingTool,
    Cordage,
    SimpleContainer,
    FuelBundle
};

enum class ToolCapability {
    None,
    Cut,
    Chop,
    Dig,
    Strike,
    Carry,
    Heat
};

inline ToolCapability itemCapability(ItemKind kind)
{
    switch(kind){
        case ItemKind::SharpFlake: return ToolCapability::Cut;
        case ItemKind::StoneCuttingTool: return ToolCapability::Chop;
        case ItemKind::SimpleContainer: return ToolCapability::Carry;
        case ItemKind::FuelBundle: return ToolCapability::Heat;
        default: return ToolCapability::None;
    }
}

struct ItemStack {
    ItemKind kind=ItemKind::RawMaterial;
    MaterialKind material=MaterialKind::Unknown;
    int quantity=0;
    double quality=0.5;
    double durability=1.0;
};

class Inventory {
public:
    const std::vector<ItemStack>& stacks() const { return stacks_; }

    int count(ItemKind kind,MaterialKind material,bool anyMaterial=false) const
    {
        int total=0;
        for(const auto& stack:stacks_){
            if(stack.kind!=kind) continue;
            if(!anyMaterial && stack.material!=material) continue;
            total+=stack.quantity;
        }
        return total;
    }

    void add(ItemStack item)
    {
        if(item.quantity<=0) return;
        item.quality=clamp01(item.quality);
        item.durability=clamp01(item.durability);
        for(auto& stack:stacks_){
            if(stack.kind==item.kind && stack.material==item.material
               && near(stack.quality,item.quality) && near(stack.durability,item.durability)){
                stack.quantity+=item.quantity;
                return;
            }
        }
        stacks_.push_back(item);
    }

    bool remove(ItemKind kind,MaterialKind material,int quantity,bool anyMaterial=false)
    {
        if(quantity<=0) return true;
        if(count(kind,material,anyMaterial)<quantity) return false;
        int remaining=quantity;
        for(auto& stack:stacks_){
            if(remaining<=0) break;
            if(stack.kind!=kind) continue;
            if(!anyMaterial && stack.material!=material) continue;
            const int take=std::min(stack.quantity,remaining);
            stack.quantity-=take;
            remaining-=take;
        }
        stacks_.erase(std::remove_if(stacks_.begin(),stacks_.end(),[](const ItemStack& s){return s.quantity<=0;}),stacks_.end());
        return remaining==0;
    }

    bool transferTo(Inventory& target,ItemKind kind,MaterialKind material,int quantity,bool anyMaterial=false)
    {
        if(quantity<=0) return true;
        if(count(kind,material,anyMaterial)<quantity) return false;

        int remaining=quantity;
        std::vector<ItemStack> moved;
        for(const auto& stack:stacks_){
            if(remaining<=0) break;
            if(stack.kind!=kind) continue;
            if(!anyMaterial && stack.material!=material) continue;
            const int take=std::min(stack.quantity,remaining);
            ItemStack part=stack;
            part.quantity=take;
            moved.push_back(part);
            remaining-=take;
        }
        if(remaining!=0) return false;
        if(!remove(kind,material,quantity,anyMaterial)) return false;
        for(const auto& stack:moved) target.add(stack);
        return true;
    }

private:
    static double clamp01(double value){ return std::max(0.0,std::min(1.0,value)); }
    static bool near(double a,double b){ return a>b ? a-b<1e-9 : b-a<1e-9; }
    std::vector<ItemStack> stacks_;
};

struct ResourceNode {
    ResourceNodeId id=0;
    MaterialKind material=MaterialKind::Unknown;
    int quantity=0;
    int maxQuantity=0;
    bool renewable=false;
    int regenerationPerDay=0;

    int harvest(int requested,Inventory& destination,double efficiency=1.0)
    {
        if(requested<=0 || quantity<=0 || efficiency<=0.0) return 0;
        const int capacity=std::max(1,static_cast<int>(static_cast<double>(requested)*std::max(0.1,efficiency)));
        const int taken=std::min(quantity,capacity);
        quantity-=taken;
        destination.add({ItemKind::RawMaterial,material,taken,0.5,1.0});
        return taken;
    }

    void regenerateDay()
    {
        if(!renewable || regenerationPerDay<=0 || maxQuantity<=quantity) return;
        quantity=std::min(maxQuantity,quantity+regenerationPerDay);
    }
};

struct StorageSite {
    StorageId id=0;
    Inventory inventory;
};

enum class TechniqueId {
    None,
    SharpFlake,
    ChippedStoneTool,
    FireMaking,
    FiberCordage,
    SimpleContainer,
    DesignatedSanitationArea
};

enum class KnowledgeLevel : int {
    Unknown=0,
    Observed=1,
    Hypothesized=2,
    Understood=3,
    Reproducible=4,
    Practiced=5,
    Mastered=6
};

struct TechniqueKnowledge {
    TechniqueId technique=TechniqueId::None;
    KnowledgeLevel level=KnowledgeLevel::Unknown;
    double confidence=0.0;
    int successfulUses=0;
};

class KnowledgeState {
public:
    KnowledgeLevel level(TechniqueId technique) const
    {
        const TechniqueKnowledge* record=find(technique);
        return record ? record->level : KnowledgeLevel::Unknown;
    }

    double confidence(TechniqueId technique) const
    {
        const TechniqueKnowledge* record=find(technique);
        return record ? record->confidence : 0.0;
    }

    bool knowsAtLeast(TechniqueId technique,KnowledgeLevel minimum) const
    {
        return static_cast<int>(level(technique))>=static_cast<int>(minimum);
    }

    void learn(TechniqueId technique,KnowledgeLevel newLevel,double newConfidence)
    {
        if(technique==TechniqueId::None) return;
        TechniqueKnowledge* record=findMutable(technique);
        if(!record){
            records_.push_back({technique,newLevel,clamp01(newConfidence),0});
            return;
        }
        if(static_cast<int>(newLevel)>static_cast<int>(record->level)) record->level=newLevel;
        record->confidence=std::max(record->confidence,clamp01(newConfidence));
    }

    void recordSuccessfulUse(TechniqueId technique)
    {
        TechniqueKnowledge* record=findMutable(technique);
        if(!record) return;
        ++record->successfulUses;
        if(record->successfulUses>=3 && record->level==KnowledgeLevel::Reproducible) record->level=KnowledgeLevel::Practiced;
        if(record->successfulUses>=12 && record->level==KnowledgeLevel::Practiced) record->level=KnowledgeLevel::Mastered;
        record->confidence=clamp01(record->confidence+0.03);
    }

    const std::vector<TechniqueKnowledge>& all() const { return records_; }

private:
    static double clamp01(double value){ return std::max(0.0,std::min(1.0,value)); }
    const TechniqueKnowledge* find(TechniqueId technique) const
    {
        for(const auto& record:records_) if(record.technique==technique) return &record;
        return nullptr;
    }
    TechniqueKnowledge* findMutable(TechniqueId technique)
    {
        for(auto& record:records_) if(record.technique==technique) return &record;
        return nullptr;
    }
    std::vector<TechniqueKnowledge> records_;
};

enum class ExperimentKind {
    StrikeStone,
    HaftSharpFlake,
    FrictionWood,
    TwistFiber,
    ShapeClay,
    DesignateSanitationArea
};

enum class CivilizationEventType {
    Gathered,
    Stored,
    ExperimentFailed,
    Discovered,
    Crafted
};

struct CivilizationEvent {
    CivilizationEventType type=CivilizationEventType::Gathered;
    CharacterId actor=0;
    TechniqueId technique=TechniqueId::None;
    MaterialKind material=MaterialKind::Unknown;
    ItemKind item=ItemKind::RawMaterial;
    int quantity=0;
};

struct ExperimentContext {
    std::uint64_t worldSeed=1;
    CharacterId actor=0;
    std::uint64_t attemptIndex=0;
    ExperimentKind kind=ExperimentKind::StrikeStone;
    MaterialKind material=MaterialKind::Unknown;
    double learningSkill=0.5;
    double curiosity=0.5;
    double patience=0.5;
    bool sanitationProblemRecognized=false;
    bool sanitationSiteAvailable=false;
};

struct ExperimentResult {
    bool attempted=false;
    bool success=false;
    double roll=1.0;
    double successChance=0.0;
    TechniqueId discovered=TechniqueId::None;
    bool producedItem=false;
    ItemStack output{};
    CivilizationEvent event{};
};

struct Ingredient {
    ItemKind kind=ItemKind::RawMaterial;
    MaterialKind material=MaterialKind::Unknown;
    int quantity=0;
    bool anyMaterial=false;
};

struct TechniqueRecipe {
    TechniqueId technique=TechniqueId::None;
    std::vector<Ingredient> inputs;
    bool producesItem=false;
    ItemKind outputKind=ItemKind::RawMaterial;
    MaterialKind outputMaterial=MaterialKind::Unknown;
    int outputQuantity=0;
};

inline TechniqueRecipe techniqueRecipe(TechniqueId technique)
{
    switch(technique){
        case TechniqueId::SharpFlake:
            return {technique,{{ItemKind::RawMaterial,MaterialKind::Flint,2,false}},true,ItemKind::SharpFlake,MaterialKind::Flint,1};
        case TechniqueId::ChippedStoneTool:
            return {technique,{{ItemKind::SharpFlake,MaterialKind::Unknown,1,true},{ItemKind::RawMaterial,MaterialKind::Wood,1,false}},true,ItemKind::StoneCuttingTool,MaterialKind::Flint,1};
        case TechniqueId::FireMaking:
            return {technique,{{ItemKind::RawMaterial,MaterialKind::Wood,2,false}},false,ItemKind::FuelBundle,MaterialKind::Wood,0};
        case TechniqueId::FiberCordage:
            return {technique,{{ItemKind::RawMaterial,MaterialKind::Fiber,2,false}},true,ItemKind::Cordage,MaterialKind::Fiber,1};
        case TechniqueId::SimpleContainer:
            return {technique,{{ItemKind::RawMaterial,MaterialKind::Clay,3,false}},true,ItemKind::SimpleContainer,MaterialKind::Clay,1};
        case TechniqueId::DesignatedSanitationArea:
            return {technique,{},false,ItemKind::RawMaterial,MaterialKind::Unknown,0};
        case TechniqueId::None:
        default:
            return {};
    }
}

inline bool hasIngredients(const Inventory& inventory,const std::vector<Ingredient>& inputs)
{
    for(const auto& input:inputs){
        if(input.quantity<=0) continue;
        if(inventory.count(input.kind,input.material,input.anyMaterial)<input.quantity) return false;
    }
    return true;
}

inline bool consumeIngredients(Inventory& inventory,const std::vector<Ingredient>& inputs)
{
    if(!hasIngredients(inventory,inputs)) return false;
    for(const auto& input:inputs){
        if(input.quantity<=0) continue;
        if(!inventory.remove(input.kind,input.material,input.quantity,input.anyMaterial)) return false;
    }
    return true;
}

inline TechniqueId experimentTechnique(ExperimentKind kind)
{
    switch(kind){
        case ExperimentKind::StrikeStone: return TechniqueId::SharpFlake;
        case ExperimentKind::HaftSharpFlake: return TechniqueId::ChippedStoneTool;
        case ExperimentKind::FrictionWood: return TechniqueId::FireMaking;
        case ExperimentKind::TwistFiber: return TechniqueId::FiberCordage;
        case ExperimentKind::ShapeClay: return TechniqueId::SimpleContainer;
        case ExperimentKind::DesignateSanitationArea: return TechniqueId::DesignatedSanitationArea;
        default: return TechniqueId::None;
    }
}

inline std::uint64_t civilizationMix(std::uint64_t value)
{
    value+=0x9e3779b97f4a7c15ULL;
    value=(value^(value>>30))*0xbf58476d1ce4e5b9ULL;
    value=(value^(value>>27))*0x94d049bb133111ebULL;
    return value^(value>>31);
}

inline double civilizationRoll(const ExperimentContext& context)
{
    std::uint64_t value=context.worldSeed ? context.worldSeed : 1;
    value=civilizationMix(value^context.actor);
    value=civilizationMix(value^(static_cast<std::uint64_t>(context.kind)+1ULL)*0x9e3779b97f4a7c15ULL);
    value=civilizationMix(value^(static_cast<std::uint64_t>(context.material)+1ULL)*0xbf58476d1ce4e5b9ULL);
    value=civilizationMix(value^context.attemptIndex*0x94d049bb133111ebULL);
    const std::uint64_t mantissa=value>>11;
    return static_cast<double>(mantissa)*(1.0/9007199254740992.0);
}

inline double experimentBaseChance(ExperimentKind kind,MaterialKind material)
{
    switch(kind){
        case ExperimentKind::StrikeStone:
            if(material==MaterialKind::Flint) return 0.42;
            if(material==MaterialKind::Stone) return 0.24;
            return 0.0;
        case ExperimentKind::HaftSharpFlake: return material==MaterialKind::Wood ? 0.28 : 0.0;
        case ExperimentKind::FrictionWood: return material==MaterialKind::Wood ? 0.15 : 0.0;
        case ExperimentKind::TwistFiber: return material==MaterialKind::Fiber ? 0.34 : 0.0;
        case ExperimentKind::ShapeClay: return material==MaterialKind::Clay ? 0.30 : 0.0;
        case ExperimentKind::DesignateSanitationArea: return material==MaterialKind::Unknown ? 0.32 : 0.0;
        default: return 0.0;
    }
}

inline double clampCivilization01(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

inline TechniqueRecipe experimentRecipe(const ExperimentContext& context)
{
    TechniqueRecipe recipe=techniqueRecipe(experimentTechnique(context.kind));
    if(context.kind==ExperimentKind::StrikeStone && context.material==MaterialKind::Stone){
        recipe.inputs={{ItemKind::RawMaterial,MaterialKind::Stone,2,false}};
        recipe.outputMaterial=MaterialKind::Stone;
    }
    if(context.kind==ExperimentKind::HaftSharpFlake){
        recipe.inputs={{ItemKind::SharpFlake,MaterialKind::Unknown,1,true},{ItemKind::RawMaterial,MaterialKind::Wood,1,false}};
    }
    return recipe;
}

inline bool experimentPrerequisitesMet(const ExperimentContext& context,const KnowledgeState& knowledge)
{
    if(context.kind==ExperimentKind::HaftSharpFlake){
        return knowledge.knowsAtLeast(TechniqueId::SharpFlake,KnowledgeLevel::Reproducible);
    }
    if(context.kind==ExperimentKind::DesignateSanitationArea){
        return context.sanitationProblemRecognized && context.sanitationSiteAvailable;
    }
    return true;
}

inline ExperimentResult attemptExperiment(
    const ExperimentContext& context,
    Inventory& inventory,
    KnowledgeState& knowledge)
{
    ExperimentResult result;
    result.discovered=experimentTechnique(context.kind);
    result.event.actor=context.actor;
    result.event.technique=result.discovered;
    result.event.material=context.material;

    if(context.actor==0 || result.discovered==TechniqueId::None) return result;
    if(!experimentPrerequisitesMet(context,knowledge)) return result;

    TechniqueRecipe recipe=experimentRecipe(context);
    if(!hasIngredients(inventory,recipe.inputs)) return result;

    const double base=experimentBaseChance(context.kind,context.material);
    if(base<=0.0) return result;

    result.attempted=true;
    result.roll=civilizationRoll(context);
    result.successChance=clampCivilization01(
        base
        +clampCivilization01(context.learningSkill)*0.22
        +clampCivilization01(context.curiosity)*0.12
        +clampCivilization01(context.patience)*0.08);

    consumeIngredients(inventory,recipe.inputs);

    if(result.roll>=result.successChance){
        knowledge.learn(result.discovered,KnowledgeLevel::Hypothesized,0.30);
        result.event.type=CivilizationEventType::ExperimentFailed;
        return result;
    }

    result.success=true;
    knowledge.learn(result.discovered,KnowledgeLevel::Reproducible,0.72+0.20*(1.0-result.roll));
    knowledge.recordSuccessfulUse(result.discovered);
    result.event.type=CivilizationEventType::Discovered;

    if(recipe.producesItem && recipe.outputQuantity>0){
        result.producedItem=true;
        result.output={recipe.outputKind,recipe.outputMaterial,recipe.outputQuantity,0.45+0.35*context.learningSkill,1.0};
        inventory.add(result.output);
        result.event.item=result.output.kind;
        result.event.quantity=result.output.quantity;
    }
    return result;
}

struct CraftResult {
    bool success=false;
    ItemStack output{};
    CivilizationEvent event{};
};

inline CraftResult reproduceTechnique(
    CharacterId actor,
    TechniqueId technique,
    Inventory& inventory,
    KnowledgeState& knowledge,
    double skill=0.5)
{
    CraftResult result;
    result.event.actor=actor;
    result.event.technique=technique;
    if(actor==0 || !knowledge.knowsAtLeast(technique,KnowledgeLevel::Reproducible)) return result;

    TechniqueRecipe recipe=techniqueRecipe(technique);
    if(recipe.technique==TechniqueId::None || !hasIngredients(inventory,recipe.inputs)) return result;
    if(!consumeIngredients(inventory,recipe.inputs)) return result;

    result.success=true;
    result.event.type=CivilizationEventType::Crafted;
    knowledge.recordSuccessfulUse(technique);
    if(recipe.producesItem && recipe.outputQuantity>0){
        result.output={recipe.outputKind,recipe.outputMaterial,recipe.outputQuantity,0.50+0.35*clampCivilization01(skill),1.0};
        inventory.add(result.output);
        result.event.item=result.output.kind;
        result.event.material=result.output.material;
        result.event.quantity=result.output.quantity;
    }
    return result;
}

struct IndividualCivilizationState {
    CharacterId character=0;
    Inventory inventory;
    KnowledgeState knowledge;
    double gatheringSkill=0.5;
    double craftingSkill=0.5;
    double learningSkill=0.5;
};

inline CivilizationEvent gatherResource(
    IndividualCivilizationState& individual,
    ResourceNode& node,
    int requested)
{
    CivilizationEvent event;
    event.type=CivilizationEventType::Gathered;
    event.actor=individual.character;
    event.material=node.material;
    event.item=ItemKind::RawMaterial;
    const double efficiency=0.75+0.5*clampCivilization01(individual.gatheringSkill);
    event.quantity=node.harvest(requested,individual.inventory,efficiency);
    return event;
}

inline CivilizationEvent storeItems(
    IndividualCivilizationState& individual,
    StorageSite& storage,
    ItemKind kind,
    MaterialKind material,
    int quantity,
    bool anyMaterial=false)
{
    CivilizationEvent event;
    event.type=CivilizationEventType::Stored;
    event.actor=individual.character;
    event.item=kind;
    event.material=material;
    if(individual.inventory.transferTo(storage.inventory,kind,material,quantity,anyMaterial)) event.quantity=quantity;
    return event;
}

} // namespace lifelens
