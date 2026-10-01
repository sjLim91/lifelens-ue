#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include "Civilization.h"
#include "Facility.h"
#include "PrimitiveSanitation.h"
#include "World.h"

namespace lifelens {

// Stable world-facing identities. These are deliberately independent from the
// legacy TechniqueId ordinal so future techniques can map onto reusable
// capabilities without turning an era label into simulation authority.
inline constexpr std::uint32_t CivilizationCapabilitySchemaVersion=1;
inline constexpr std::uint32_t CivilizationTechnologySchemaVersion=1;

enum class CivilizationCapabilityId : std::uint16_t {
    None=0,
    Cutting=1,
    WoodChopping=2,
    CordageProduction=3,
    WaterTransport=4,
    DesignatedSanitation=5,
    PitSanitation=6,
    SharedStorage=7,
    Digging=8,
    HardMaterialStriking=9,
    ControlledFire=10,
    CopperSmelting=11,
    Cultivation=12,
    TinSmelting=13,
    BronzeAlloying=14,
    BronzeWoodworking=15,
    BronzeMining=16
};

enum class CivilizationTechnologyId : std::uint16_t {
    None=0,
    SharpFlake=1001,
    ChippedStoneTool=1002,
    FireMaking=1003,
    FiberCordage=1004,
    SimpleContainer=1005,
    DesignatedSanitationArea=1006,
    DugSanitationPit=1007,
    PrimitiveStorage=1008,
    DiggingStick=1009,
    StoneHammer=1010,
    CopperSmelting=1011,
    Cultivation=1012,
    TinSmelting=1013,
    BronzeAlloying=1014,
    BronzeAxe=1015,
    BronzePick=1016
};

inline const char* civilizationCapabilityName(CivilizationCapabilityId id)
{
    switch(id){
        case CivilizationCapabilityId::Cutting: return "Cutting";
        case CivilizationCapabilityId::WoodChopping: return "WoodChopping";
        case CivilizationCapabilityId::CordageProduction: return "CordageProduction";
        case CivilizationCapabilityId::WaterTransport: return "WaterTransport";
        case CivilizationCapabilityId::DesignatedSanitation: return "DesignatedSanitation";
        case CivilizationCapabilityId::PitSanitation: return "PitSanitation";
        case CivilizationCapabilityId::SharedStorage: return "SharedStorage";
        case CivilizationCapabilityId::Digging: return "Digging";
        case CivilizationCapabilityId::HardMaterialStriking: return "HardMaterialStriking";
        case CivilizationCapabilityId::ControlledFire: return "ControlledFire";
        case CivilizationCapabilityId::CopperSmelting: return "CopperSmelting";
        case CivilizationCapabilityId::Cultivation: return "Cultivation";
        case CivilizationCapabilityId::TinSmelting: return "TinSmelting";
        case CivilizationCapabilityId::BronzeAlloying: return "BronzeAlloying";
        case CivilizationCapabilityId::BronzeWoodworking: return "BronzeWoodworking";
        case CivilizationCapabilityId::BronzeMining: return "BronzeMining";
        case CivilizationCapabilityId::None:
        default: return "None";
    }
}

inline const char* civilizationTechnologyName(CivilizationTechnologyId id)
{
    switch(id){
        case CivilizationTechnologyId::SharpFlake: return "SharpFlake";
        case CivilizationTechnologyId::ChippedStoneTool: return "ChippedStoneTool";
        case CivilizationTechnologyId::FireMaking: return "FireMaking";
        case CivilizationTechnologyId::FiberCordage: return "FiberCordage";
        case CivilizationTechnologyId::SimpleContainer: return "SimpleContainer";
        case CivilizationTechnologyId::DesignatedSanitationArea: return "DesignatedSanitationArea";
        case CivilizationTechnologyId::DugSanitationPit: return "DugSanitationPit";
        case CivilizationTechnologyId::PrimitiveStorage: return "PrimitiveStorage";
        case CivilizationTechnologyId::DiggingStick: return "DiggingStick";
        case CivilizationTechnologyId::StoneHammer: return "StoneHammer";
        case CivilizationTechnologyId::CopperSmelting: return "CopperSmelting";
        case CivilizationTechnologyId::Cultivation: return "Cultivation";
        case CivilizationTechnologyId::TinSmelting: return "TinSmelting";
        case CivilizationTechnologyId::BronzeAlloying: return "BronzeAlloying";
        case CivilizationTechnologyId::BronzeAxe: return "BronzeAxe";
        case CivilizationTechnologyId::BronzePick: return "BronzePick";
        case CivilizationTechnologyId::None:
        default: return "None";
    }
}

inline CivilizationTechnologyId technologyIdForTechnique(TechniqueId technique)
{
    switch(technique){
        case TechniqueId::SharpFlake: return CivilizationTechnologyId::SharpFlake;
        case TechniqueId::ChippedStoneTool: return CivilizationTechnologyId::ChippedStoneTool;
        case TechniqueId::FireMaking: return CivilizationTechnologyId::FireMaking;
        case TechniqueId::FiberCordage: return CivilizationTechnologyId::FiberCordage;
        case TechniqueId::SimpleContainer: return CivilizationTechnologyId::SimpleContainer;
        case TechniqueId::DesignatedSanitationArea: return CivilizationTechnologyId::DesignatedSanitationArea;
        case TechniqueId::DugSanitationPit: return CivilizationTechnologyId::DugSanitationPit;
        case TechniqueId::PrimitiveStorage: return CivilizationTechnologyId::PrimitiveStorage;
        case TechniqueId::DiggingStick: return CivilizationTechnologyId::DiggingStick;
        case TechniqueId::StoneHammer: return CivilizationTechnologyId::StoneHammer;
        case TechniqueId::CopperSmelting: return CivilizationTechnologyId::CopperSmelting;
        case TechniqueId::Cultivation: return CivilizationTechnologyId::Cultivation;
        case TechniqueId::TinSmelting: return CivilizationTechnologyId::TinSmelting;
        case TechniqueId::BronzeAlloying: return CivilizationTechnologyId::BronzeAlloying;
        case TechniqueId::BronzeAxe: return CivilizationTechnologyId::BronzeAxe;
        case TechniqueId::BronzePick: return CivilizationTechnologyId::BronzePick;
        case TechniqueId::None:
        default: return CivilizationTechnologyId::None;
    }
}

inline CivilizationCapabilityId primaryCapabilityForTechnology(
    CivilizationTechnologyId technology)
{
    switch(technology){
        case CivilizationTechnologyId::SharpFlake:
            return CivilizationCapabilityId::Cutting;
        case CivilizationTechnologyId::ChippedStoneTool:
            return CivilizationCapabilityId::WoodChopping;
        case CivilizationTechnologyId::FireMaking:
            return CivilizationCapabilityId::ControlledFire;
        case CivilizationTechnologyId::FiberCordage:
            return CivilizationCapabilityId::CordageProduction;
        case CivilizationTechnologyId::SimpleContainer:
            return CivilizationCapabilityId::WaterTransport;
        case CivilizationTechnologyId::DesignatedSanitationArea:
            return CivilizationCapabilityId::DesignatedSanitation;
        case CivilizationTechnologyId::DugSanitationPit:
            return CivilizationCapabilityId::PitSanitation;
        case CivilizationTechnologyId::PrimitiveStorage:
            return CivilizationCapabilityId::SharedStorage;
        case CivilizationTechnologyId::DiggingStick:
            return CivilizationCapabilityId::Digging;
        case CivilizationTechnologyId::StoneHammer:
            return CivilizationCapabilityId::HardMaterialStriking;
        case CivilizationTechnologyId::CopperSmelting:
            return CivilizationCapabilityId::CopperSmelting;
        case CivilizationTechnologyId::Cultivation:
            return CivilizationCapabilityId::Cultivation;
        case CivilizationTechnologyId::TinSmelting:
            return CivilizationCapabilityId::TinSmelting;
        case CivilizationTechnologyId::BronzeAlloying:
            return CivilizationCapabilityId::BronzeAlloying;
        case CivilizationTechnologyId::BronzeAxe:
            return CivilizationCapabilityId::BronzeWoodworking;
        case CivilizationTechnologyId::BronzePick:
            return CivilizationCapabilityId::BronzeMining;
        case CivilizationTechnologyId::None:
        default:
            return CivilizationCapabilityId::None;
    }
}

inline int livingResidentCount(const World& world)
{
    int count=0;
    for(const Character& resident:world.characters) if(resident.alive) ++count;
    return count;
}

inline int livingTechniqueKnowerCountForCapability(
    const World& world,
    TechniqueId technique,
    KnowledgeLevel minimum)
{
    int count=0;
    for(const Character& resident:world.characters){
        if(!resident.alive) continue;
        if(resident.civilization.knowledge.knowsAtLeast(technique,minimum)) ++count;
    }
    return count;
}

inline int worldItemUnits(
    const World& world,
    ItemKind item,
    MaterialKind material=MaterialKind::Unknown,
    bool ignoreMaterial=true)
{
    int total=0;
    for(const Character& resident:world.characters){
        if(!resident.alive) continue;
        total+=resident.civilization.inventory.count(item,material,ignoreMaterial);
    }
    for(const StorageSite& storage:world.storageSites){
        total+=storage.inventory.count(item,material,ignoreMaterial);
    }
    return total;
}

inline int worldRawMaterialUnits(const World& world,MaterialKind material)
{
    int total=0;
    for(const ResourceNode& node:world.resourceNodes){
        if(node.material==material) total+=std::max(0,node.quantity);
    }
    for(const Character& resident:world.characters){
        if(!resident.alive) continue;
        total+=resident.civilization.inventory.count(ItemKind::RawMaterial,material);
    }
    for(const StorageSite& storage:world.storageSites){
        total+=storage.inventory.count(ItemKind::RawMaterial,material);
    }
    for(const ConstructedFacility& facility:world.facilities){
        if(facility.kind==FacilityKind::FirePit
           && material==MaterialKind::Charcoal){
            total+=std::max(0,facility.charcoalUnits);
        }
        if(facility.kind==FacilityKind::Furnace){
            if(material==MaterialKind::Charcoal){
                total+=std::max(0,facility.fuelUnits);
            }
            if(facility.furnaceChargeMaterial==material){
                total+=std::max(0,facility.oreUnits);
            }
            if(facility.furnaceOutputMaterial==material){
                total+=std::max(0,facility.metalUnits);
            }
        }
    }
    return total;
}

inline int operationalFacilityCount(const World& world,FacilityKind kind)
{
    int count=0;
    for(const ConstructedFacility& facility:world.facilities){
        if(facility.kind==kind && facilityOperationalAndActive(facility)) ++count;
    }
    return count;
}

inline int activeSanitationCount(
    const World& world,
    PrimitiveSanitationSiteKind kind)
{
    int count=0;
    for(const PrimitiveSanitationSite& site:world.primitiveSanitationSites){
        if(site.active && site.kind==kind) ++count;
    }
    return count;
}

struct CivilizationCapabilityObservation {
    CivilizationCapabilityId capability=CivilizationCapabilityId::None;
    bool operational=false;
    int knowledgeableResidents=0;
    int supportingFacilityCount=0;
    int supportingToolUnits=0;
    int availableInputUnits=0;
};

inline CivilizationCapabilityObservation evaluateCivilizationCapability(
    const World& world,
    CivilizationCapabilityId id)
{
    CivilizationCapabilityObservation out;
    out.capability=id;

    const int firePits=operationalFacilityCount(world,FacilityKind::FirePit);
    const int furnaces=operationalFacilityCount(world,FacilityKind::Furnace);
    switch(id){
        case CivilizationCapabilityId::Cutting:
            out.supportingToolUnits=worldItemUnits(world,ItemKind::SharpFlake);
            out.operational=out.supportingToolUnits>0;
            break;
        case CivilizationCapabilityId::WoodChopping:
            out.supportingToolUnits=
                worldItemUnits(world,ItemKind::StoneCuttingTool)
                +worldItemUnits(world,ItemKind::BronzeAxe);
            out.operational=out.supportingToolUnits>0;
            break;
        case CivilizationCapabilityId::CordageProduction:
            out.knowledgeableResidents=livingTechniqueKnowerCountForCapability(
                world,TechniqueId::FiberCordage,KnowledgeLevel::Reproducible);
            out.availableInputUnits=worldRawMaterialUnits(world,MaterialKind::Fiber);
            out.operational=out.knowledgeableResidents>0 && out.availableInputUnits>=2;
            break;
        case CivilizationCapabilityId::WaterTransport:
            out.supportingToolUnits=worldItemUnits(world,ItemKind::SimpleContainer);
            out.operational=out.supportingToolUnits>0;
            break;
        case CivilizationCapabilityId::DesignatedSanitation:
            out.supportingFacilityCount=activeSanitationCount(
                world,PrimitiveSanitationSiteKind::DesignatedArea)
                +activeSanitationCount(world,PrimitiveSanitationSiteKind::DugPit);
            out.operational=out.supportingFacilityCount>0;
            break;
        case CivilizationCapabilityId::PitSanitation:
            out.supportingFacilityCount=activeSanitationCount(
                world,PrimitiveSanitationSiteKind::DugPit);
            out.operational=out.supportingFacilityCount>0;
            break;
        case CivilizationCapabilityId::SharedStorage:
            out.supportingFacilityCount=operationalFacilityCount(
                world,FacilityKind::PrimitiveStorage);
            out.availableInputUnits=static_cast<int>(world.storageSites.size());
            out.operational=out.supportingFacilityCount>0 && out.availableInputUnits>0;
            break;
        case CivilizationCapabilityId::Digging:
            out.supportingToolUnits=worldItemUnits(world,ItemKind::DiggingStick);
            out.operational=out.supportingToolUnits>0;
            break;
        case CivilizationCapabilityId::HardMaterialStriking:
            out.supportingToolUnits=
                worldItemUnits(world,ItemKind::StoneHammer)
                +worldItemUnits(world,ItemKind::BronzePick);
            out.operational=out.supportingToolUnits>0;
            break;
        case CivilizationCapabilityId::ControlledFire:
            out.knowledgeableResidents=livingTechniqueKnowerCountForCapability(
                world,TechniqueId::FireMaking,KnowledgeLevel::Reproducible);
            out.supportingFacilityCount=firePits;
            out.availableInputUnits=worldRawMaterialUnits(world,MaterialKind::Wood);
            out.operational=out.knowledgeableResidents>0
                && out.supportingFacilityCount>0
                && out.availableInputUnits>0;
            break;
        case CivilizationCapabilityId::CopperSmelting:
            out.knowledgeableResidents=livingTechniqueKnowerCountForCapability(
                world,TechniqueId::CopperSmelting,KnowledgeLevel::Reproducible);
            out.supportingFacilityCount=furnaces;
            out.availableInputUnits=std::min(
                worldRawMaterialUnits(world,MaterialKind::CopperOre),
                worldRawMaterialUnits(world,MaterialKind::Charcoal));
            out.operational=out.knowledgeableResidents>0
                && firePits>0 && furnaces>0
                && out.availableInputUnits>0;
            break;
        case CivilizationCapabilityId::Cultivation: {
            out.knowledgeableResidents=livingTechniqueKnowerCountForCapability(
                world,TechniqueId::Cultivation,KnowledgeLevel::Reproducible);
            out.supportingFacilityCount=operationalFacilityCount(
                world,FacilityKind::CultivatedPlot);
            bool planted=false;
            for(const ConstructedFacility& facility:world.facilities){
                if(facility.kind==FacilityKind::CultivatedPlot
                   && facilityOperationalAndActive(facility)
                   && facility.cropPlanted){
                    planted=true;
                    break;
                }
            }
            const int seeds=worldRawMaterialUnits(world,MaterialKind::PlantFood);
            const int water=worldRawMaterialUnits(world,MaterialKind::Water);
            out.availableInputUnits=std::min(seeds,water);
            out.operational=out.knowledgeableResidents>0
                && out.supportingFacilityCount>0
                && (planted || out.availableInputUnits>0);
            break;
        }
        case CivilizationCapabilityId::TinSmelting:
            out.knowledgeableResidents=livingTechniqueKnowerCountForCapability(
                world,TechniqueId::TinSmelting,KnowledgeLevel::Reproducible);
            out.supportingFacilityCount=furnaces;
            out.availableInputUnits=std::min(
                worldRawMaterialUnits(world,MaterialKind::TinOre),
                worldRawMaterialUnits(world,MaterialKind::Charcoal));
            out.operational=out.knowledgeableResidents>0
                && firePits>0 && furnaces>0
                && out.availableInputUnits>0;
            break;
        case CivilizationCapabilityId::BronzeAlloying: {
            out.knowledgeableResidents=livingTechniqueKnowerCountForCapability(
                world,TechniqueId::BronzeAlloying,KnowledgeLevel::Reproducible);
            out.supportingFacilityCount=furnaces;
            const int copperBatches=
                worldRawMaterialUnits(world,MaterialKind::CopperMetal)/2;
            const int tinBatches=
                worldRawMaterialUnits(world,MaterialKind::TinMetal);
            const int fuelBatches=
                worldRawMaterialUnits(world,MaterialKind::Charcoal);
            out.availableInputUnits=std::min({
                copperBatches,tinBatches,fuelBatches});
            out.operational=out.knowledgeableResidents>0
                && firePits>0 && furnaces>0
                && out.availableInputUnits>0;
            break;
        }
        case CivilizationCapabilityId::BronzeWoodworking:
            out.supportingToolUnits=worldItemUnits(
                world,ItemKind::BronzeAxe,MaterialKind::Bronze,false);
            out.operational=out.supportingToolUnits>0;
            break;
        case CivilizationCapabilityId::BronzeMining:
            out.supportingToolUnits=worldItemUnits(
                world,ItemKind::BronzePick,MaterialKind::Bronze,false);
            out.operational=out.supportingToolUnits>0;
            break;
        case CivilizationCapabilityId::None:
        default:
            break;
    }
    return out;
}

inline std::vector<CivilizationCapabilityObservation>
buildCivilizationCapabilityObservations(const World& world)
{
    constexpr std::array<CivilizationCapabilityId,16> ids={
        CivilizationCapabilityId::Cutting,
        CivilizationCapabilityId::WoodChopping,
        CivilizationCapabilityId::CordageProduction,
        CivilizationCapabilityId::WaterTransport,
        CivilizationCapabilityId::DesignatedSanitation,
        CivilizationCapabilityId::PitSanitation,
        CivilizationCapabilityId::SharedStorage,
        CivilizationCapabilityId::Digging,
        CivilizationCapabilityId::HardMaterialStriking,
        CivilizationCapabilityId::ControlledFire,
        CivilizationCapabilityId::CopperSmelting,
        CivilizationCapabilityId::Cultivation,
        CivilizationCapabilityId::TinSmelting,
        CivilizationCapabilityId::BronzeAlloying,
        CivilizationCapabilityId::BronzeWoodworking,
        CivilizationCapabilityId::BronzeMining
    };

    std::vector<CivilizationCapabilityObservation> result;
    result.reserve(ids.size());
    for(const CivilizationCapabilityId id:ids){
        result.push_back(evaluateCivilizationCapability(world,id));
    }
    return result;
}

inline const CivilizationCapabilityObservation* findCivilizationCapability(
    const std::vector<CivilizationCapabilityObservation>& observations,
    CivilizationCapabilityId id)
{
    for(const auto& observation:observations){
        if(observation.capability==id) return &observation;
    }
    return nullptr;
}

struct CivilizationTechnologyObservation {
    CivilizationTechnologyId technology=CivilizationTechnologyId::None;
    TechniqueId legacyTechnique=TechniqueId::None;
    CivilizationCapabilityId primaryCapability=CivilizationCapabilityId::None;
    int livingKnowerCount=0;
    int reproducibleKnowerCount=0;
    bool known=false;
    bool reproducible=false;
    bool operational=false;
    double diffusion01=0.0;
};

inline std::vector<CivilizationTechnologyObservation>
buildCivilizationTechnologyObservations(
    const World& world,
    const std::vector<CivilizationCapabilityObservation>& capabilities)
{
    constexpr std::array<TechniqueId,16> techniques={
        TechniqueId::SharpFlake,
        TechniqueId::ChippedStoneTool,
        TechniqueId::FireMaking,
        TechniqueId::FiberCordage,
        TechniqueId::SimpleContainer,
        TechniqueId::DesignatedSanitationArea,
        TechniqueId::DugSanitationPit,
        TechniqueId::PrimitiveStorage,
        TechniqueId::DiggingStick,
        TechniqueId::StoneHammer,
        TechniqueId::CopperSmelting,
        TechniqueId::Cultivation,
        TechniqueId::TinSmelting,
        TechniqueId::BronzeAlloying,
        TechniqueId::BronzeAxe,
        TechniqueId::BronzePick
    };
    const int living=std::max(1,livingResidentCount(world));

    std::vector<CivilizationTechnologyObservation> result;
    result.reserve(techniques.size());
    for(const TechniqueId technique:techniques){
        CivilizationTechnologyObservation observed;
        observed.legacyTechnique=technique;
        observed.technology=technologyIdForTechnique(technique);
        observed.primaryCapability=primaryCapabilityForTechnology(
            observed.technology);
        observed.livingKnowerCount=livingTechniqueKnowerCountForCapability(
            world,technique,KnowledgeLevel::Observed);
        observed.reproducibleKnowerCount=livingTechniqueKnowerCountForCapability(
            world,technique,KnowledgeLevel::Reproducible);
        observed.known=observed.livingKnowerCount>0;
        observed.reproducible=observed.reproducibleKnowerCount>0;
        observed.diffusion01=std::max(
            0.0,std::min(
                1.0,
                static_cast<double>(observed.livingKnowerCount)
                    /static_cast<double>(living)));
        const auto* capability=findCivilizationCapability(
            capabilities,observed.primaryCapability);
        observed.operational=observed.reproducible
            && capability!=nullptr && capability->operational;
        result.push_back(observed);
    }
    return result;
}

} // namespace lifelens
