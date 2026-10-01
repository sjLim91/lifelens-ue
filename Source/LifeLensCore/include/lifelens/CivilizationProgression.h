#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

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

enum class TechnologyPopulationState : std::uint8_t {
    Unknown=0,
    Observed,
    Reproducible,
    Operational,
    Diffusing,
    Common,
    Declining,
    Lost
};

enum class CivilizationTransformationId : std::uint16_t {
    None=0,
    ResourceBuffering=1,
    ManagedFoodProduction=2,
    MetallurgicalProduction=3,
    AdvancedTooling=4,
    KnowledgeDiffusion=5
};

enum class InnovationPressureDriver : std::uint8_t {
    None=0,
    Survival,
    Exposure,
    Logistics,
    FoodSecurity,
    Sanitation,
    Production,
    ResourceScarcity
};

struct InnovationPressureObservation {
    TechnologyId technology=TechnologyId::None;
    InnovationPressureDriver dominantDriver=InnovationPressureDriver::None;
    double pressure01=0.0;
    double survival01=0.0;
    double exposure01=0.0;
    double logistics01=0.0;
    double foodSecurity01=0.0;
    double sanitation01=0.0;
    double production01=0.0;
    double resourceScarcity01=0.0;
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

inline const char* innovationPressureDriverName(InnovationPressureDriver driver)
{
    switch(driver){
        case InnovationPressureDriver::Survival: return "Survival";
        case InnovationPressureDriver::Exposure: return "Exposure";
        case InnovationPressureDriver::Logistics: return "Logistics";
        case InnovationPressureDriver::FoodSecurity: return "FoodSecurity";
        case InnovationPressureDriver::Sanitation: return "Sanitation";
        case InnovationPressureDriver::Production: return "Production";
        case InnovationPressureDriver::ResourceScarcity: return "ResourceScarcity";
        case InnovationPressureDriver::None:
        default: return "None";
    }
}

inline const char* technologyPopulationStateName(TechnologyPopulationState state)
{
    switch(state){
        case TechnologyPopulationState::Observed: return "Observed";
        case TechnologyPopulationState::Reproducible: return "Reproducible";
        case TechnologyPopulationState::Operational: return "Operational";
        case TechnologyPopulationState::Diffusing: return "Diffusing";
        case TechnologyPopulationState::Common: return "Common";
        case TechnologyPopulationState::Declining: return "Declining";
        case TechnologyPopulationState::Lost: return "Lost";
        case TechnologyPopulationState::Unknown:
        default: return "Unknown";
    }
}

inline const char* civilizationTransformationName(CivilizationTransformationId id)
{
    switch(id){
        case CivilizationTransformationId::ResourceBuffering: return "ResourceBuffering";
        case CivilizationTransformationId::ManagedFoodProduction: return "ManagedFoodProduction";
        case CivilizationTransformationId::MetallurgicalProduction: return "MetallurgicalProduction";
        case CivilizationTransformationId::AdvancedTooling: return "AdvancedTooling";
        case CivilizationTransformationId::KnowledgeDiffusion: return "KnowledgeDiffusion";
        case CivilizationTransformationId::None:
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

struct CivilizationTechnologyPopulationStatus {
    TechnologyId technology=TechnologyId::None;
    TechnologyPopulationState state=TechnologyPopulationState::Unknown;
    int livingKnowerCount=0;
    int reproducibleKnowerCount=0;
    int operationalResidentCount=0;
    int adoptedResidentCount=0;
    int successfulUseCount=0;
    double diffusion01=0.0;
    bool historicallyKnown=false;
    int historicalFactCount=0;
    int firstEvidenceMinute=-1;
    int latestEvidenceMinute=-1;
};

inline int civilizationLivingResidentCount(const World& world)
{
    int count=0;
    for(const Character& resident:world.characters){
        if(resident.alive) ++count;
    }
    return count;
}

inline CivilizationTechnologyPopulationStatus observeTechnologyPopulationStatus(
    const World& world,
    TechnologyId technology,
    bool historicallyKnown=false)
{
    CivilizationTechnologyPopulationStatus status;
    status.technology=technology;
    status.historicallyKnown=historicallyKnown;

    const int living=std::max(1,civilizationLivingResidentCount(world));
    for(const Character& resident:world.characters){
        if(!resident.alive) continue;
        const CivilizationTechnologyStatus residentStatus=
            observeTechnologyStatus(world,resident,technology);
        if(residentStatus.discovered) ++status.livingKnowerCount;
        if(residentStatus.reproducible) ++status.reproducibleKnowerCount;
        if(residentStatus.operational) ++status.operationalResidentCount;
        if(residentStatus.adopted) ++status.adoptedResidentCount;
        status.successfulUseCount+=std::max(0,residentStatus.successfulUses);
    }

    status.diffusion01=std::max(
        0.0,
        std::min(
            1.0,
            static_cast<double>(status.adoptedResidentCount)
                /static_cast<double>(living)));

    if(status.livingKnowerCount==0){
        status.state=historicallyKnown
            ? TechnologyPopulationState::Lost
            : TechnologyPopulationState::Unknown;
    }else if(status.reproducibleKnowerCount==0){
        status.state=TechnologyPopulationState::Observed;
    }else if(status.operationalResidentCount==0){
        status.state=
            status.adoptedResidentCount>0 || status.successfulUseCount>=2
                ? TechnologyPopulationState::Declining
                : TechnologyPopulationState::Reproducible;
    }else if(status.adoptedResidentCount==0){
        status.state=TechnologyPopulationState::Operational;
    }else if(status.adoptedResidentCount*2>=living){
        status.state=TechnologyPopulationState::Common;
    }else{
        status.state=TechnologyPopulationState::Diffusing;
    }
    return status;
}

inline const CivilizationTechnologyPopulationStatus* findTechnologyPopulationStatus(
    const std::vector<CivilizationTechnologyPopulationStatus>& statuses,
    TechnologyId technology)
{
    for(const auto& status:statuses){
        if(status.technology==technology) return &status;
    }
    return nullptr;
}

inline int civilizationWorldItemUnits(
    const World& world,
    ItemKind item,
    MaterialKind material=MaterialKind::Unknown,
    bool anyMaterial=false)
{
    int total=0;
    for(const Character& resident:world.characters){
        if(!resident.alive) continue;
        total+=resident.civilization.inventory.count(item,material,anyMaterial);
    }
    for(const StorageSite& storage:world.storageSites){
        total+=storage.inventory.count(item,material,anyMaterial);
    }
    return total;
}

inline int civilizationOperationalFacilityCount(
    const World& world,
    FacilityKind kind)
{
    int count=0;
    for(const ConstructedFacility& facility:world.facilities){
        if(facility.kind==kind && facilityOperationalAndActive(facility)) ++count;
    }
    return count;
}

inline double civilizationProgressionClamp01(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

struct CivilizationTransformationStatus {
    CivilizationTransformationId transformation=CivilizationTransformationId::None;
    bool active=false;
    double magnitude01=0.0;
    int evidenceCount=0;
    int supportingTechnologyCount=0;
};

inline int civilizationSupportingTechnologyCount(
    const std::vector<CivilizationTechnologyPopulationStatus>& technologies,
    std::initializer_list<TechnologyId> ids)
{
    int count=0;
    for(const TechnologyId id:ids){
        const auto* status=findTechnologyPopulationStatus(technologies,id);
        if(status==nullptr) continue;
        if(status->state!=TechnologyPopulationState::Unknown
           && status->state!=TechnologyPopulationState::Lost){
            ++count;
        }
    }
    return count;
}

inline std::vector<CivilizationTransformationStatus>
buildCivilizationTransformationStatuses(
    const World& world,
    const std::vector<CivilizationTechnologyPopulationStatus>& technologies)
{
    std::vector<CivilizationTransformationStatus> result;
    result.reserve(5);
    const int living=std::max(1,civilizationLivingResidentCount(world));

    int storedUnits=0;
    for(const StorageSite& storage:world.storageSites){
        for(const ItemStack& stack:storage.inventory.stacks()){
            storedUnits+=std::max(0,stack.quantity);
        }
    }
    CivilizationTransformationStatus buffering;
    buffering.transformation=CivilizationTransformationId::ResourceBuffering;
    buffering.evidenceCount=storedUnits;
    buffering.supportingTechnologyCount=civilizationSupportingTechnologyCount(
        technologies,{TechnologyId::PrimitiveStorage});
    buffering.magnitude01=civilizationProgressionClamp01(
        static_cast<double>(storedUnits)
            /static_cast<double>(living*8));
    buffering.active=
        civilizationOperationalFacilityCount(
            world,FacilityKind::PrimitiveStorage)>0
        && storedUnits>0;
    result.push_back(buffering);

    int operationalPlots=0;
    int plantedPlots=0;
    int harvestUnits=0;
    for(const ConstructedFacility& facility:world.facilities){
        if(facility.kind!=FacilityKind::CultivatedPlot
           || !facilityOperationalAndActive(facility)) continue;
        ++operationalPlots;
        if(facility.cropPlanted) ++plantedPlots;
        harvestUnits+=std::max(0,facility.cropHarvestUnits);
    }
    CivilizationTransformationStatus food;
    food.transformation=CivilizationTransformationId::ManagedFoodProduction;
    food.evidenceCount=plantedPlots+harvestUnits;
    food.supportingTechnologyCount=civilizationSupportingTechnologyCount(
        technologies,{TechnologyId::Cultivation});
    food.magnitude01=civilizationProgressionClamp01(
        0.45*static_cast<double>(operationalPlots)
            /static_cast<double>(living)
        +0.35*static_cast<double>(plantedPlots)
            /static_cast<double>(living)
        +0.20*static_cast<double>(harvestUnits)
            /static_cast<double>(living*2));
    food.active=operationalPlots>0 && (plantedPlots>0 || harvestUnits>0);
    result.push_back(food);

    int metalUnits=
        civilizationWorldItemUnits(
            world,ItemKind::RawMaterial,MaterialKind::CopperMetal)
        +civilizationWorldItemUnits(
            world,ItemKind::RawMaterial,MaterialKind::TinMetal)
        +civilizationWorldItemUnits(
            world,ItemKind::RawMaterial,MaterialKind::Bronze);
    for(const ConstructedFacility& facility:world.facilities){
        if(facility.kind==FacilityKind::Furnace
           && facilityOperationalAndActive(facility)){
            metalUnits+=std::max(0,facility.metalUnits);
        }
    }
    const int furnaces=civilizationOperationalFacilityCount(
        world,FacilityKind::Furnace);
    CivilizationTransformationStatus metallurgy;
    metallurgy.transformation=
        CivilizationTransformationId::MetallurgicalProduction;
    metallurgy.evidenceCount=metalUnits;
    metallurgy.supportingTechnologyCount=civilizationSupportingTechnologyCount(
        technologies,{
            TechnologyId::CopperSmelting,
            TechnologyId::TinSmelting,
            TechnologyId::BronzeAlloying});
    metallurgy.magnitude01=civilizationProgressionClamp01(
        (furnaces>0 ? 0.30 : 0.0)
        +0.70*static_cast<double>(metalUnits)
            /static_cast<double>(living*3));
    metallurgy.active=furnaces>0 && metalUnits>0;
    result.push_back(metallurgy);

    const int advancedTools=
        civilizationWorldItemUnits(
            world,ItemKind::BronzeAxe,MaterialKind::Bronze)
        +civilizationWorldItemUnits(
            world,ItemKind::BronzePick,MaterialKind::Bronze);
    CivilizationTransformationStatus tooling;
    tooling.transformation=CivilizationTransformationId::AdvancedTooling;
    tooling.evidenceCount=advancedTools;
    tooling.supportingTechnologyCount=civilizationSupportingTechnologyCount(
        technologies,{TechnologyId::BronzeAxe,TechnologyId::BronzePick});
    tooling.magnitude01=civilizationProgressionClamp01(
        static_cast<double>(advancedTools)/static_cast<double>(living));
    tooling.active=advancedTools>0;
    result.push_back(tooling);

    double knowledgeCoverageSum=0.0;
    int knowledgeTechnologyCount=0;
    int multiResidentKnowledgeCount=0;
    for(const auto& technology:technologies){
        if(!technology.historicallyKnown && technology.livingKnowerCount<=0) continue;
        knowledgeCoverageSum+=civilizationProgressionClamp01(
            static_cast<double>(technology.livingKnowerCount)
                /static_cast<double>(living));
        ++knowledgeTechnologyCount;
        if(technology.livingKnowerCount>1) ++multiResidentKnowledgeCount;
    }
    CivilizationTransformationStatus diffusion;
    diffusion.transformation=CivilizationTransformationId::KnowledgeDiffusion;
    diffusion.evidenceCount=multiResidentKnowledgeCount;
    diffusion.supportingTechnologyCount=knowledgeTechnologyCount;
    diffusion.magnitude01=
        knowledgeTechnologyCount>0
            ? civilizationProgressionClamp01(
                knowledgeCoverageSum
                /static_cast<double>(knowledgeTechnologyCount))
            : 0.0;
    diffusion.active=multiResidentKnowledgeCount>0;
    result.push_back(diffusion);

    return result;
}

} // namespace lifelens
