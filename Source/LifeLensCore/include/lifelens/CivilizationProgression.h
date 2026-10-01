#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "Civilization.h"
#include "PrimitiveFireProgression.h"
#include "PrimitiveSanitation.h"
#include "PrimitiveSmeltingProgression.h"
#include "World.h"

namespace lifelens {

// C3 stable identities are explicit and never depend on presentation labels or
// simulation time. New values append so future contracts retain identity when
// implementation details evolve.
enum class CapabilityId : std::uint16_t {
    None=0,
    Cut=1,
    Chop=2,
    ControlFire=3,
    Bind=4,
    CarryLiquid=5,
    Sanitation=6,
    StoreGoods=7,
    Dig=8,
    Strike=9,
    SmeltMetal=10,
    CultivateFood=11,
    AlloyMetal=12
};

enum class TechnologyId : std::uint16_t {
    None=0,
    SharpFlake=1,
    ChippedStoneTool=2,
    FireMaking=3,
    FiberCordage=4,
    SimpleContainer=5,
    DesignatedSanitationArea=6,
    DugSanitationPit=7,
    PrimitiveStorage=8,
    DiggingStick=9,
    StoneHammer=10,
    CopperSmelting=11,
    Cultivation=12,
    TinSmelting=13,
    BronzeAlloying=14,
    BronzeAxe=15,
    BronzePick=16
};

struct TechnologyDefinition {
    TechnologyId id=TechnologyId::None;
    TechniqueId legacyTechnique=TechniqueId::None;
    CapabilityId primaryCapability=CapabilityId::None;
};

inline constexpr std::array<TechnologyDefinition,16> TechnologyRegistry={{
    {TechnologyId::SharpFlake,TechniqueId::SharpFlake,CapabilityId::Cut},
    {TechnologyId::ChippedStoneTool,TechniqueId::ChippedStoneTool,CapabilityId::Chop},
    {TechnologyId::FireMaking,TechniqueId::FireMaking,CapabilityId::ControlFire},
    {TechnologyId::FiberCordage,TechniqueId::FiberCordage,CapabilityId::Bind},
    {TechnologyId::SimpleContainer,TechniqueId::SimpleContainer,CapabilityId::CarryLiquid},
    {TechnologyId::DesignatedSanitationArea,TechniqueId::DesignatedSanitationArea,CapabilityId::Sanitation},
    {TechnologyId::DugSanitationPit,TechniqueId::DugSanitationPit,CapabilityId::Sanitation},
    {TechnologyId::PrimitiveStorage,TechniqueId::PrimitiveStorage,CapabilityId::StoreGoods},
    {TechnologyId::DiggingStick,TechniqueId::DiggingStick,CapabilityId::Dig},
    {TechnologyId::StoneHammer,TechniqueId::StoneHammer,CapabilityId::Strike},
    {TechnologyId::CopperSmelting,TechniqueId::CopperSmelting,CapabilityId::SmeltMetal},
    {TechnologyId::Cultivation,TechniqueId::Cultivation,CapabilityId::CultivateFood},
    {TechnologyId::TinSmelting,TechniqueId::TinSmelting,CapabilityId::SmeltMetal},
    {TechnologyId::BronzeAlloying,TechniqueId::BronzeAlloying,CapabilityId::AlloyMetal},
    {TechnologyId::BronzeAxe,TechniqueId::BronzeAxe,CapabilityId::Chop},
    {TechnologyId::BronzePick,TechniqueId::BronzePick,CapabilityId::Strike}
}};

inline const char* capabilityIdName(CapabilityId id)
{
    switch(id){
        case CapabilityId::Cut: return "Cut";
        case CapabilityId::Chop: return "Chop";
        case CapabilityId::ControlFire: return "ControlFire";
        case CapabilityId::Bind: return "Bind";
        case CapabilityId::CarryLiquid: return "CarryLiquid";
        case CapabilityId::Sanitation: return "Sanitation";
        case CapabilityId::StoreGoods: return "StoreGoods";
        case CapabilityId::Dig: return "Dig";
        case CapabilityId::Strike: return "Strike";
        case CapabilityId::SmeltMetal: return "SmeltMetal";
        case CapabilityId::CultivateFood: return "CultivateFood";
        case CapabilityId::AlloyMetal: return "AlloyMetal";
        case CapabilityId::None:
        default: return "None";
    }
}

inline const char* technologyIdName(TechnologyId id)
{
    switch(id){
        case TechnologyId::SharpFlake: return "SharpFlake";
        case TechnologyId::ChippedStoneTool: return "ChippedStoneTool";
        case TechnologyId::FireMaking: return "FireMaking";
        case TechnologyId::FiberCordage: return "FiberCordage";
        case TechnologyId::SimpleContainer: return "SimpleContainer";
        case TechnologyId::DesignatedSanitationArea: return "DesignatedSanitationArea";
        case TechnologyId::DugSanitationPit: return "DugSanitationPit";
        case TechnologyId::PrimitiveStorage: return "PrimitiveStorage";
        case TechnologyId::DiggingStick: return "DiggingStick";
        case TechnologyId::StoneHammer: return "StoneHammer";
        case TechnologyId::CopperSmelting: return "CopperSmelting";
        case TechnologyId::Cultivation: return "Cultivation";
        case TechnologyId::TinSmelting: return "TinSmelting";
        case TechnologyId::BronzeAlloying: return "BronzeAlloying";
        case TechnologyId::BronzeAxe: return "BronzeAxe";
        case TechnologyId::BronzePick: return "BronzePick";
        case TechnologyId::None:
        default: return "None";
    }
}

inline TechnologyId technologyIdForTechnique(TechniqueId technique)
{
    for(const auto& definition:TechnologyRegistry){
        if(definition.legacyTechnique==technique) return definition.id;
    }
    return TechnologyId::None;
}

inline TechniqueId techniqueForTechnology(TechnologyId technology)
{
    for(const auto& definition:TechnologyRegistry){
        if(definition.id==technology) return definition.legacyTechnique;
    }
    return TechniqueId::None;
}

inline const TechnologyDefinition* technologyDefinition(TechnologyId technology)
{
    for(const auto& definition:TechnologyRegistry){
        if(definition.id==technology) return &definition;
    }
    return nullptr;
}

inline bool inventoryHasToolCapability(
    const Inventory& inventory,
    ToolCapability capability)
{
    if(capability==ToolCapability::None) return false;
    for(const ItemStack& stack:inventory.stacks()){
        if(stack.quantity>0
           && stack.durability>0.001
           && itemCapability(stack.kind)==capability) return true;
    }
    return false;
}

inline bool operationalCultivatedPlotExists(const World& world)
{
    for(const ConstructedFacility& facility:world.facilities){
        if(facility.kind==FacilityKind::CultivatedPlot
           && facility.state==FacilityState::Operational
           && facility.active) return true;
    }
    return false;
}

inline bool activeSanitationExists(
    const World& world,
    PrimitiveSanitationSiteKind minimumKind)
{
    for(const PrimitiveSanitationSite& site:world.primitiveSanitationSites){
        if(!site.active) continue;
        if(minimumKind==PrimitiveSanitationSiteKind::DesignatedArea) return true;
        if(site.kind==PrimitiveSanitationSiteKind::DugPit) return true;
    }
    return false;
}

inline bool technologyOperational(
    const World& world,
    const Character& resident,
    TechnologyId technology)
{
    const TechnologyDefinition* definition=technologyDefinition(technology);
    if(definition==nullptr) return false;
    const TechniqueId technique=definition->legacyTechnique;
    if(!resident.civilization.knowledge.knowsAtLeast(
        technique,KnowledgeLevel::Reproducible)) return false;

    const Inventory& inventory=resident.civilization.inventory;
    switch(technology){
        case TechnologyId::FireMaking:
            return hasOperationalFirePit(world)
                || hasIngredients(inventory,techniqueRecipe(technique).inputs);
        case TechnologyId::DesignatedSanitationArea:
            return activeSanitationExists(
                world,PrimitiveSanitationSiteKind::DesignatedArea);
        case TechnologyId::DugSanitationPit:
            return activeSanitationExists(
                world,PrimitiveSanitationSiteKind::DugPit);
        case TechnologyId::PrimitiveStorage:
            return !world.storageSites.empty();
        case TechnologyId::CopperSmelting:
            return smeltingOpportunityAvailable(
                world,resident,TechniqueId::CopperSmelting);
        case TechnologyId::TinSmelting:
            return smeltingOpportunityAvailable(
                world,resident,TechniqueId::TinSmelting);
        case TechnologyId::BronzeAlloying:
            return smeltingOpportunityAvailable(
                world,resident,TechniqueId::BronzeAlloying);
        case TechnologyId::Cultivation:
            return operationalCultivatedPlotExists(world);
        case TechnologyId::None:
            return false;
        default:
            break;
    }

    const TechniqueRecipe recipe=techniqueRecipe(technique);
    if(recipe.technique==TechniqueId::None) return false;
    if(recipe.producesItem && recipe.outputQuantity>0){
        const bool anyMaterial=recipe.outputMaterial==MaterialKind::Unknown;
        if(inventory.count(
            recipe.outputKind,recipe.outputMaterial,anyMaterial)>0) return true;
    }
    return hasIngredients(inventory,recipe.inputs);
}

inline bool technologyAdopted(
    const Character& resident,
    TechnologyId technology)
{
    const TechniqueId technique=techniqueForTechnology(technology);
    if(technique==TechniqueId::None) return false;
    for(const TechniqueKnowledge& knowledge:resident.civilization.knowledge.all()){
        if(knowledge.technique!=technique) continue;
        // Legacy discovery records the first successful use. Repeated real use
        // or a practiced knowledge level is therefore the first adoption signal.
        return knowledge.successfulUses>=2
            || static_cast<int>(knowledge.level)
                >=static_cast<int>(KnowledgeLevel::Practiced);
    }
    return false;
}

inline bool capabilityAvailable(
    const World& world,
    const Character& resident,
    CapabilityId capability)
{
    const Inventory& inventory=resident.civilization.inventory;
    switch(capability){
        case CapabilityId::Cut:
            return inventoryHasToolCapability(inventory,ToolCapability::Cut);
        case CapabilityId::Chop:
            return inventoryHasToolCapability(inventory,ToolCapability::Chop);
        case CapabilityId::Dig:
            return inventoryHasToolCapability(inventory,ToolCapability::Dig);
        case CapabilityId::Strike:
            return inventoryHasToolCapability(inventory,ToolCapability::Strike);
        case CapabilityId::CarryLiquid:
            return simpleContainerCount(inventory)>0;
        case CapabilityId::ControlFire:
            return technologyOperational(world,resident,TechnologyId::FireMaking);
        case CapabilityId::Bind:
            return inventory.count(
                ItemKind::Cordage,MaterialKind::Fiber)>0
                || technologyOperational(
                    world,resident,TechnologyId::FiberCordage);
        case CapabilityId::Sanitation:
            return technologyOperational(
                    world,resident,TechnologyId::DesignatedSanitationArea)
                || technologyOperational(
                    world,resident,TechnologyId::DugSanitationPit);
        case CapabilityId::StoreGoods:
            return technologyOperational(
                world,resident,TechnologyId::PrimitiveStorage);
        case CapabilityId::SmeltMetal:
            return technologyOperational(
                    world,resident,TechnologyId::CopperSmelting)
                || technologyOperational(
                    world,resident,TechnologyId::TinSmelting);
        case CapabilityId::CultivateFood:
            return technologyOperational(
                world,resident,TechnologyId::Cultivation);
        case CapabilityId::AlloyMetal:
            return technologyOperational(
                world,resident,TechnologyId::BronzeAlloying);
        case CapabilityId::None:
        default:
            return false;
    }
}

struct CivilizationCapabilityStatus {
    CapabilityId capability=CapabilityId::None;
    bool available=false;
    int knownSupportingTechnologies=0;
    int operationalSupportingTechnologies=0;
};

struct CivilizationTechnologyStatus {
    TechnologyId technology=TechnologyId::None;
    TechniqueId legacyTechnique=TechniqueId::None;
    CapabilityId primaryCapability=CapabilityId::None;
    KnowledgeLevel knowledgeLevel=KnowledgeLevel::Unknown;
    bool discovered=false;
    bool reproducible=false;
    bool operational=false;
    bool adopted=false;
    int successfulUses=0;
};

inline std::array<CapabilityId,12> allCapabilityIds()
{
    return {
        CapabilityId::Cut,CapabilityId::Chop,CapabilityId::ControlFire,
        CapabilityId::Bind,CapabilityId::CarryLiquid,CapabilityId::Sanitation,
        CapabilityId::StoreGoods,CapabilityId::Dig,CapabilityId::Strike,
        CapabilityId::SmeltMetal,CapabilityId::CultivateFood,
        CapabilityId::AlloyMetal
    };
}

inline CivilizationCapabilityStatus observeCapabilityStatus(
    const World& world,
    const Character& resident,
    CapabilityId capability)
{
    CivilizationCapabilityStatus status;
    status.capability=capability;
    status.available=capabilityAvailable(world,resident,capability);
    for(const TechnologyDefinition& definition:TechnologyRegistry){
        if(definition.primaryCapability!=capability) continue;
        const KnowledgeLevel level=
            resident.civilization.knowledge.level(definition.legacyTechnique);
        if(level!=KnowledgeLevel::Unknown) ++status.knownSupportingTechnologies;
        if(technologyOperational(world,resident,definition.id)){
            ++status.operationalSupportingTechnologies;
        }
    }
    return status;
}

inline CivilizationTechnologyStatus observeTechnologyStatus(
    const World& world,
    const Character& resident,
    TechnologyId technology)
{
    CivilizationTechnologyStatus status;
    const TechnologyDefinition* definition=technologyDefinition(technology);
    if(definition==nullptr) return status;

    status.technology=technology;
    status.legacyTechnique=definition->legacyTechnique;
    status.primaryCapability=definition->primaryCapability;
    const TechniqueKnowledge* record=nullptr;
    for(const TechniqueKnowledge& knowledge:resident.civilization.knowledge.all()){
        if(knowledge.technique==definition->legacyTechnique){
            record=&knowledge;
            break;
        }
    }
    if(record==nullptr) return status;
    status.knowledgeLevel=record->level;
    status.discovered=record->level!=KnowledgeLevel::Unknown;
    status.reproducible=
        static_cast<int>(record->level)
        >=static_cast<int>(KnowledgeLevel::Reproducible);
    status.successfulUses=record->successfulUses;
    status.operational=technologyOperational(world,resident,technology);
    status.adopted=technologyAdopted(resident,technology);
    return status;
}

} // namespace lifelens
