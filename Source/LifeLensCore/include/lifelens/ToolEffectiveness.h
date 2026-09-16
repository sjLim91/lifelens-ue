#pragma once

#include <algorithm>
#include <cmath>

#include "Civilization.h"

namespace lifelens {

struct GatherToolUseProfile {
    bool available=false;
    bool used=false;
    bool broken=false;
    ToolCapability capability=ToolCapability::None;
    ItemStack tool{};
    double quantityMultiplier=1.0;
    double wear=0.0;
    double durabilityBefore=0.0;
    double durabilityAfter=0.0;
};

inline ToolCapability gatheringToolCapability(MaterialKind material)
{
    switch(material){
        case MaterialKind::Wood:
            return ToolCapability::Chop;
        case MaterialKind::Fiber:
        case MaterialKind::Hide:
            return ToolCapability::Cut;
        case MaterialKind::Water:
        case MaterialKind::PlantFood:
            return ToolCapability::Carry;
        case MaterialKind::Clay:
            return ToolCapability::Dig;
        case MaterialKind::Stone:
        case MaterialKind::Flint:
        case MaterialKind::Bone:
        case MaterialKind::CopperOre:
        case MaterialKind::TinOre:
        case MaterialKind::IronOre:
            return ToolCapability::Strike;
        case MaterialKind::Charcoal:
        case MaterialKind::Unknown:
        default:
            return ToolCapability::None;
    }
}

inline double gatheringToolWearScale(ToolCapability capability)
{
    switch(capability){
        case ToolCapability::Chop: return 1.20;
        case ToolCapability::Dig: return 1.20;
        case ToolCapability::Strike: return 1.15;
        case ToolCapability::Cut: return 1.00;
        case ToolCapability::Carry: return 0.60;
        case ToolCapability::Heat: return 0.80;
        case ToolCapability::None:
        default: return 0.0;
    }
}

inline GatherToolUseProfile inspectGatherTool(
    const Inventory& inventory,
    MaterialKind material)
{
    GatherToolUseProfile result;
    result.capability=gatheringToolCapability(material);
    if(result.capability==ToolCapability::None) return result;

    for(const auto& stack:inventory.stacks()){
        if(stack.quantity<=0 || itemCapability(stack.kind)!=result.capability) continue;
        if(stack.durability<=0.001) continue;

        result.available=true;
        result.tool=stack;
        result.tool.quantity=1;
        result.durabilityBefore=std::max(0.0,std::min(1.0,stack.durability));
        const double quality=std::max(0.0,std::min(1.0,stack.quality));
        result.quantityMultiplier=
            1.10+0.18*quality+0.14*result.durabilityBefore;
        result.wear=(0.04+0.04*(1.0-quality))*gatheringToolWearScale(result.capability);
        result.durabilityAfter=std::max(0.0,result.durabilityBefore-result.wear);
        return result;
    }
    return result;
}

inline int gatheringQuantityWithTool(
    const Inventory& inventory,
    MaterialKind material,
    int baseQuantity)
{
    const int base=std::max(1,baseQuantity);
    const GatherToolUseProfile profile=inspectGatherTool(inventory,material);
    if(!profile.available) return base;
    return std::max(base+1,
        static_cast<int>(std::floor(
            static_cast<double>(base)*profile.quantityMultiplier+0.5)));
}

inline bool consumeGatherToolUse(
    Inventory& inventory,
    MaterialKind material,
    GatherToolUseProfile* outProfile=nullptr)
{
    GatherToolUseProfile profile=inspectGatherTool(inventory,material);
    if(!profile.available){
        if(outProfile) *outProfile=profile;
        return false;
    }

    // Inventory keeps equal-quality/equal-durability tools stacked. Selecting
    // the first usable matching capability therefore lets public remove/add
    // split exactly one tool instance into its newly worn durability state.
    if(!inventory.remove(profile.tool.kind,profile.tool.material,1)){
        if(outProfile) *outProfile=GatherToolUseProfile{};
        return false;
    }

    profile.used=true;
    profile.broken=profile.durabilityAfter<=0.02;
    if(!profile.broken){
        ItemStack worn=profile.tool;
        worn.quantity=1;
        worn.durability=profile.durabilityAfter;
        inventory.add(worn);
    }

    if(outProfile) *outProfile=profile;
    return true;
}

inline const char* toolItemName(ItemKind item)
{
    switch(item){
        case ItemKind::SharpFlake: return "SharpFlake";
        case ItemKind::StoneCuttingTool: return "StoneCuttingTool";
        case ItemKind::SimpleContainer: return "SimpleContainer";
        case ItemKind::Cordage: return "Cordage";
        case ItemKind::FuelBundle: return "FuelBundle";
        case ItemKind::RawMaterial:
        default: return "RawMaterial";
    }
}

} // namespace lifelens
