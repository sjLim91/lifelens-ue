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

    // No current item exposes Dig/Strike yet, so these materials intentionally
    // keep baseline output until those tool recipes are discovered later.
    CHECK(!inspectGatherTool(inventory,MaterialKind::Clay).available);
    CHECK(!inspectGatherTool(inventory,MaterialKind::Stone).available);

    std::cout << "tool effectiveness passed\n";
    return 0;
}
