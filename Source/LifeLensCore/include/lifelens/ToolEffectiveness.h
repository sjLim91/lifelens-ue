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
        // Carry-capable vessels are reusable transport capacity, not a cutting/
        // striking edge. Water filling already pays for capacity through
        // emptySimpleContainerCount(), so generic gather-tool wear would
        // double-charge the same vessel and eventually delete it.
        // Vessel-specific damage/repair can own durability separately later.
        case ToolCapability::Carry: return 0.0;
        case ToolCapability::Heat: return 0.80;
        case ToolCapability::None:
        default: return 0.0;
    }
}

inline double gatheringToolFormBonus(
    const ItemStack& tool,
    ToolCapability capability)
{
    // Material properties model what an edge can physically do. A specialized
    // cast head also gains from its geometry: the bronze axe concentrates force
    // for chopping and the bronze pick for striking hard deposits.
    if(tool.kind==ItemKind::BronzeAxe
       && tool.material==MaterialKind::Bronze
       && capability==ToolCapability::Chop){
        return 0.10;
    }
    if(tool.kind==ItemKind::BronzePick
       && tool.material==MaterialKind::Bronze
       && capability==ToolCapability::Strike){
        return 0.08;
    }
    return 0.0;
}

inline GatherToolUseProfile inspectGatherTool(
    const Inventory& inventory,
    MaterialKind material)
{
    GatherToolUseProfile result;
    result.capability=gatheringToolCapability(material);
    if(result.capability==ToolCapability::None) return result;

    GatherToolUseProfile best;
    best.capability=result.capability;
    for(const auto& stack:inventory.stacks()){
        if(stack.quantity<=0 || itemCapability(stack.kind)!=result.capability) continue;
        if(stack.durability<=0.001) continue;

        GatherToolUseProfile candidate;
        candidate.available=true;
        candidate.capability=result.capability;
        candidate.tool=stack;
        candidate.tool.quantity=1;
        candidate.durabilityBefore=std::max(
            0.0,std::min(1.0,stack.durability));
        const double quality=std::max(0.0,std::min(1.0,stack.quality));
        const MaterialProperties properties=materialProperties(stack.material);
        const double materialPerformance=std::max(
            0.0,std::min(
                1.0,
                0.55*properties.hardness
                    +0.45*properties.sharpnessPotential));
        candidate.quantityMultiplier=
            1.10+0.18*quality+0.14*candidate.durabilityBefore
            +0.18*materialPerformance
            +gatheringToolFormBonus(stack,candidate.capability);
        const double materialWearFactor=std::max(
            0.45,
            1.10-0.45*properties.hardness+0.20*properties.brittleness);
        candidate.wear=
            (0.04+0.04*(1.0-quality))
            *gatheringToolWearScale(candidate.capability)
            *materialWearFactor;
        candidate.durabilityAfter=std::max(
            0.0,candidate.durabilityBefore-candidate.wear);
        if(!best.available
           || candidate.quantityMultiplier>best.quantityMultiplier+1e-12
           || (std::abs(
                candidate.quantityMultiplier-best.quantityMultiplier)<=1e-12
               && candidate.durabilityBefore>best.durabilityBefore)){
            best=candidate;
        }
    }
    return best;
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

    // Remove exactly the selected quality/durability stack. A broken or stale
    // sibling stack of the same item/material must never absorb this wear.
    if(!inventory.removeExactOne(profile.tool)){
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
        case ItemKind::DiggingStick: return "DiggingStick";
        case ItemKind::StoneHammer: return "StoneHammer";
        case ItemKind::BronzeAxe: return "BronzeAxe";
        case ItemKind::BronzePick: return "BronzePick";
        case ItemKind::SimpleContainer: return "SimpleContainer";
        case ItemKind::Cordage: return "Cordage";
        case ItemKind::FuelBundle: return "FuelBundle";
        case ItemKind::RawMaterial:
        default: return "RawMaterial";
    }
}

} // namespace lifelens