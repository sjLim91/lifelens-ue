#include <iostream>

#include "lifelens/ToolEffectiveness.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

int main()
{
    Inventory inventory;

    CHECK(gatheringToolCapability(MaterialKind::Wood)==ToolCapability::Chop);
    CHECK(gatheringToolCapability(MaterialKind::Fiber)==ToolCapability::Cut);
    CHECK(gatheringToolCapability(MaterialKind::Water)==ToolCapability::Carry);
    CHECK(gatheringToolCapability(MaterialKind::Clay)==ToolCapability::Dig);
    CHECK(gatheringToolCapability(MaterialKind::Stone)==ToolCapability::Strike);
    CHECK(itemCapability(ItemKind::DiggingStick)==ToolCapability::Dig);
    CHECK(itemCapability(ItemKind::StoneHammer)==ToolCapability::Strike);
    CHECK(gatheringQuantityWithTool(inventory,MaterialKind::Wood,3)==3);

    inventory.add({ItemKind::StoneCuttingTool,MaterialKind::Flint,1,0.80,1.0});
    const GatherToolUseProfile fresh=inspectGatherTool(inventory,MaterialKind::Wood);
    CHECK(fresh.available);
    CHECK(fresh.capability==ToolCapability::Chop);
    CHECK(fresh.tool.kind==ItemKind::StoneCuttingTool);
    CHECK(fresh.quantityMultiplier>1.0);
    CHECK(fresh.wear>0.0);
    CHECK(gatheringQuantityWithTool(inventory,MaterialKind::Wood,3)>=4);

    GatherToolUseProfile used;
    CHECK(consumeGatherToolUse(inventory,MaterialKind::Wood,&used));
    CHECK(used.used);
    CHECK(!used.broken);
    CHECK(used.durabilityAfter<used.durabilityBefore);
    CHECK(inventory.count(ItemKind::StoneCuttingTool,MaterialKind::Flint)==1);

    const GatherToolUseProfile worn=inspectGatherTool(inventory,MaterialKind::Wood);
    CHECK(worn.available);
    CHECK(worn.durabilityBefore<fresh.durabilityBefore);
    CHECK(worn.quantityMultiplier<fresh.quantityMultiplier);

    int uses=1;
    while(inventory.count(ItemKind::StoneCuttingTool,MaterialKind::Flint)>0 && uses<64){
        CHECK(consumeGatherToolUse(inventory,MaterialKind::Wood,&used));
        ++uses;
    }
    CHECK(uses>1);
    CHECK(uses<64);
    CHECK(inventory.count(ItemKind::StoneCuttingTool,MaterialKind::Flint)==0);
    CHECK(used.broken);

    inventory.add({ItemKind::SharpFlake,MaterialKind::Flint,1,0.70,1.0});
    CHECK(inspectGatherTool(inventory,MaterialKind::Fiber).available);
    CHECK(gatheringQuantityWithTool(inventory,MaterialKind::Fiber,2)>=3);

    inventory.add({ItemKind::SimpleContainer,MaterialKind::Clay,1,0.75,1.0});
    CHECK(inspectGatherTool(inventory,MaterialKind::Water).available);
    CHECK(gatheringQuantityWithTool(inventory,MaterialKind::Water,2)>=3);

    inventory.add({ItemKind::DiggingStick,MaterialKind::Wood,1,0.72,1.0});
    const GatherToolUseProfile dig=inspectGatherTool(inventory,MaterialKind::Clay);
    CHECK(dig.available);
    CHECK(dig.capability==ToolCapability::Dig);
    CHECK(dig.tool.kind==ItemKind::DiggingStick);
    CHECK(gatheringQuantityWithTool(inventory,MaterialKind::Clay,2)>=3);

    inventory.add({ItemKind::StoneHammer,MaterialKind::Stone,1,0.76,1.0});
    const GatherToolUseProfile strike=inspectGatherTool(inventory,MaterialKind::Stone);
    CHECK(strike.available);
    CHECK(strike.capability==ToolCapability::Strike);
    CHECK(strike.tool.kind==ItemKind::StoneHammer);
    CHECK(gatheringQuantityWithTool(inventory,MaterialKind::Stone,2)>=3);
    CHECK(inspectGatherTool(inventory,MaterialKind::IronOre).available);

    // A dead sibling stack must not absorb wear meant for the selected usable tool.
    Inventory exactInventory;
    exactInventory.add({ItemKind::DiggingStick,MaterialKind::Wood,1,0.10,0.0});
    exactInventory.add({ItemKind::DiggingStick,MaterialKind::Wood,1,0.90,1.0});
    GatherToolUseProfile exactUse;
    CHECK(consumeGatherToolUse(exactInventory,MaterialKind::Clay,&exactUse));
    CHECK(exactUse.tool.quality==0.90);
    bool foundDead=false;
    bool foundWornLive=false;
    for(const ItemStack& stack:exactInventory.stacks()){
        if(stack.kind!=ItemKind::DiggingStick) continue;
        if(stack.quality==0.10 && stack.durability==0.0) foundDead=true;
        if(stack.quality==0.90 && stack.durability<1.0 && stack.durability>0.0) foundWornLive=true;
    }
    CHECK(foundDead);
    CHECK(foundWornLive);

    // Primitive technology progression is gated by prior knowledge, not granted for free.
    KnowledgeState knowledge;
    ExperimentContext digContext;
    digContext.actor=1;
    digContext.kind=ExperimentKind::ShapeDiggingStick;
    digContext.material=MaterialKind::Wood;
    CHECK(!experimentPrerequisitesMet(digContext,knowledge));
    knowledge.learn(TechniqueId::SharpFlake,KnowledgeLevel::Reproducible,0.8);
    CHECK(experimentPrerequisitesMet(digContext,knowledge));

    ExperimentContext hammerContext;
    hammerContext.actor=1;
    hammerContext.kind=ExperimentKind::HaftStoneHammer;
    hammerContext.material=MaterialKind::Stone;
    CHECK(!experimentPrerequisitesMet(hammerContext,knowledge));
    knowledge.learn(TechniqueId::ChippedStoneTool,KnowledgeLevel::Reproducible,0.8);
    CHECK(!experimentPrerequisitesMet(hammerContext,knowledge));
    knowledge.learn(TechniqueId::FiberCordage,KnowledgeLevel::Reproducible,0.8);
    CHECK(experimentPrerequisitesMet(hammerContext,knowledge));

    const TechniqueRecipe digRecipe=techniqueRecipe(TechniqueId::DiggingStick);
    CHECK(digRecipe.producesItem);
    CHECK(digRecipe.outputKind==ItemKind::DiggingStick);
    CHECK(digRecipe.outputMaterial==MaterialKind::Wood);

    const TechniqueRecipe hammerRecipe=techniqueRecipe(TechniqueId::StoneHammer);
    CHECK(hammerRecipe.producesItem);
    CHECK(hammerRecipe.outputKind==ItemKind::StoneHammer);
    CHECK(hammerRecipe.outputMaterial==MaterialKind::Stone);

    Inventory craftInventory;
    craftInventory.add({ItemKind::RawMaterial,MaterialKind::Wood,2,0.5,1.0});
    knowledge.learn(TechniqueId::DiggingStick,KnowledgeLevel::Reproducible,0.8);
    CraftResult craftedDig=reproduceTechnique(1,TechniqueId::DiggingStick,craftInventory,knowledge,0.7);
    CHECK(craftedDig.success);
    CHECK(craftInventory.count(ItemKind::DiggingStick,MaterialKind::Wood)==1);

    craftInventory.add({ItemKind::RawMaterial,MaterialKind::Stone,2,0.5,1.0});
    craftInventory.add({ItemKind::RawMaterial,MaterialKind::Wood,1,0.5,1.0});
    craftInventory.add({ItemKind::Cordage,MaterialKind::Fiber,1,0.7,1.0});
    knowledge.learn(TechniqueId::StoneHammer,KnowledgeLevel::Reproducible,0.8);
    CraftResult craftedHammer=reproduceTechnique(1,TechniqueId::StoneHammer,craftInventory,knowledge,0.7);
    CHECK(craftedHammer.success);
    CHECK(craftInventory.count(ItemKind::StoneHammer,MaterialKind::Stone)==1);

    std::cout << "tool effectiveness passed\n";
    return 0;
}