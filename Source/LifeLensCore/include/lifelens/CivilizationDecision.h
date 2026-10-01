#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <utility>

#include "Civilization.h"
#include "Character.h"
#include "CivilizationSpatial.h"
#include "CultivationProgression.h"
#include "Facility.h"
#include "PrimitiveFireProgression.h"
#include "PrimitiveSanitation.h"
#include "PrimitiveSmeltingProgression.h"
#include "PrimitiveStorageProgression.h"
#include "ProvisionPreference.h"
#include "ResourceExploration.h"
#include "SettlementProgression.h"
#include "World.h"

namespace lifelens {

enum class CivilizationIntent {
    None,
    Gather,
    Store,
    Experiment,
    Craft,
    // Appended so existing enum ordinals remain stable.
    Retrieve,
    // Core-authored frontier search. Keep appended for persisted enum stability.
    Explore
};

enum class FacilityBuildAction {
    None,
    Plan,
    DeliverMaterial,
    Work,
    Repair,
    Fuel,
    Ignite,
    CollectCharcoal,
    LoadSmeltCharge,
    CollectMetal,
    Plant,
    Water,
    Tend,
    Harvest
};

inline const char* civilizationIntentName(CivilizationIntent intent)
{
    switch(intent){
        case CivilizationIntent::Gather: return "Gather";
        case CivilizationIntent::Store: return "Store";
        case CivilizationIntent::Experiment: return "Experiment";
        case CivilizationIntent::Craft: return "Craft";
        case CivilizationIntent::Retrieve: return "Retrieve";
        case CivilizationIntent::Explore: return "Explore";
        default: return "None";
    }
}

inline const char* facilityBuildActionName(FacilityBuildAction action)
{
    switch(action){
        case FacilityBuildAction::Plan: return "Plan";
        case FacilityBuildAction::DeliverMaterial: return "DeliverMaterial";
        case FacilityBuildAction::Work: return "Work";
        case FacilityBuildAction::Repair: return "Repair";
        case FacilityBuildAction::Fuel: return "Fuel";
        case FacilityBuildAction::Ignite: return "Ignite";
        case FacilityBuildAction::CollectCharcoal: return "CollectCharcoal";
        case FacilityBuildAction::LoadSmeltCharge: return "LoadSmeltCharge";
        case FacilityBuildAction::CollectMetal: return "CollectMetal";
        case FacilityBuildAction::Plant: return "Plant";
        case FacilityBuildAction::Water: return "Water";
        case FacilityBuildAction::Tend: return "Tend";
        case FacilityBuildAction::Harvest: return "Harvest";
        case FacilityBuildAction::None:
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
        case MaterialKind::CopperMetal: return "CopperMetal";
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
        case TechniqueId::DesignatedSanitationArea: return "DesignatedSanitationArea";
        case TechniqueId::DugSanitationPit: return "DugSanitationPit";
        case TechniqueId::PrimitiveStorage: return "PrimitiveStorage";
        case TechniqueId::DiggingStick: return "DiggingStick";
        case TechniqueId::StoneHammer: return "StoneHammer";
        case TechniqueId::CopperSmelting: return "CopperSmelting";
        case TechniqueId::Cultivation: return "Cultivation";
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

    FacilityBuildAction facilityAction=FacilityBuildAction::None;
    FacilityId facility=0;
    FacilityKind facilityKind=FacilityKind::PrimitiveStorage;
    bool hasFacilityTarget=false;
    GridPos facilityTargetPos{};
    double facilityWork=0.0;
};

struct CivilizationExecutionResult {
    bool executed=false;
    bool success=false;
    CivilizationEvent event{};
    ExperimentResult experiment{};
    CraftResult craft{};
    SanitationSiteId sanitationSiteId=0;
    GridPos sanitationSitePos{};
    bool sanitationImprovementCompleted=false;
    double sanitationWorkBefore=0.0;
    double sanitationWorkAfter=0.0;

    FacilityId facilityId=0;
    FacilityKind facilityKind=FacilityKind::PrimitiveStorage;
    FacilityBuildAction facilityAction=FacilityBuildAction::None;
    GridPos facilityPos{};
    bool facilityCompleted=false;
    StorageId activatedStorage=0;
    double facilityWorkBefore=0.0;
    double facilityWorkAfter=0.0;
    double facilityDurabilityBefore=0.0;
    double facilityDurabilityAfter=0.0;
    int facilityFuelUnits=0;
    int facilityCharcoalUnits=0;
    int facilityOreUnits=0;
    int facilityMetalUnits=0;
    double facilityHeatLevel=0.0;
    bool facilityLit=false;
};

inline double civilizationPreference(std::uint64_t worldSeed,CharacterId actor,std::uint64_t salt)
{
    std::uint64_t value=worldSeed ? worldSeed : 1;
    value=civilizationMix(value^actor);
    value=civilizationMix(value^(salt+1ULL)*0x9e3779b97f4a7c15ULL);
    const std::uint64_t mantissa=value>>11;
    return static_cast<double>(mantissa)*(1.0/9007199254740992.0);
}

inline GridPos civilizationSanitationReferencePosition(const World& world)
{
    return world.hasInitialStartRegionSelection
        ? world.initialStartRegionCenterGrid()
        : GridPos{};
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
        total+=material==MaterialKind::Water
            ? portableWaterCount(storage.inventory)
            : storage.inventory.count(ItemKind::RawMaterial,material);
    }
    return total;
}


inline bool settlementStorageServesPosition(
    const StorageSite& storage,
    GridPos authoritativePosition,
    int maxDistance=SettlementServiceRadiusGrid)
{
    return storage.id!=0
        && manhattan(storage.pos,authoritativePosition)<=std::max(0,maxDistance);
}

inline int storageCountForMaterialNear(
    const World& world,
    MaterialKind material,
    GridPos authoritativePosition,
    int maxDistance=SettlementServiceRadiusGrid)
{
    int total=0;
    for(const auto& storage:world.storageSites){
        if(!settlementStorageServesPosition(
            storage,authoritativePosition,maxDistance)) continue;
        total+=material==MaterialKind::Water
            ? portableWaterCount(storage.inventory)
            : storage.inventory.count(ItemKind::RawMaterial,material);
    }
    return total;
}

inline const StorageSite* nearestSettlementStorage(
    const World& world,
    GridPos authoritativePosition,
    int maxDistance=SettlementServiceRadiusGrid)
{
    const StorageSite* best=nullptr;
    int bestDistance=std::max(0,maxDistance)+1;
    for(const auto& storage:world.storageSites){
        if(!settlementStorageServesPosition(
            storage,authoritativePosition,maxDistance)) continue;
        const int distance=manhattan(storage.pos,authoritativePosition);
        if(best==nullptr || distance<bestDistance
           || (distance==bestDistance && storage.id<best->id)){
            best=&storage;
            bestDistance=distance;
        }
    }
    return best;
}

inline int constructionMaterialDemandForKindNear(
    const World& world,
    FacilityKind kind,
    MaterialKind material,
    GridPos authoritativePosition,
    int maxDistance=SettlementServiceRadiusGrid)
{
    if(material==MaterialKind::Unknown) return 0;
    int demand=0;
    for(const auto& facility:world.facilities){
        if(facility.kind!=kind
           || facility.state==FacilityState::Operational
           || facility.state==FacilityState::Ruined
           || manhattan(
               facility.pos,authoritativePosition)>std::max(0,maxDistance)){
            continue;
        }
        demand+=std::max(0,facilityMissingMaterial(facility,material));
    }
    return demand;
}

inline int settlementRepairMaterialDemandNear(
    const World& world,
    MaterialKind material,
    GridPos authoritativePosition,
    int maxDistance=SettlementServiceRadiusGrid)
{
    if(material==MaterialKind::Unknown) return 0;
    int demand=0;
    for(const auto& facility:world.facilities){
        if(manhattan(
            facility.pos,authoritativePosition)>std::max(0,maxDistance)){
            continue;
        }
        if(settlementFacilityNeedsMaintenance(facility)){
            if(facilityRepairMaterial(facility.kind)==material) ++demand;
            continue;
        }
        if(settlementFacilityCanRestore(facility)){
            demand+=facilityRestorationMaterialRequirement(
                facility.kind,material);
        }
    }
    return demand;
}

inline int residentCommittedMaterialDemandAtPosition(
    const World& world,
    const Character& resident,
    MaterialKind material,
    GridPos authoritativePosition)
{
    int demand=0;
    for(const FacilityKind kind:{
        FacilityKind::WorkSurface,
        FacilityKind::SleepingPlace,
        FacilityKind::Shelter
    }){
        demand+=constructionMaterialDemandForKindNear(
            world,kind,material,authoritativePosition);
    }
    demand+=settlementRepairMaterialDemandNear(
        world,material,authoritativePosition);

    if(resident.civilization.knowledge.knowsAtLeast(
        TechniqueId::PrimitiveStorage,KnowledgeLevel::Reproducible)){
        demand+=constructionMaterialDemandForKindNear(
            world,FacilityKind::PrimitiveStorage,
            material,authoritativePosition);
    }
    if(resident.civilization.knowledge.knowsAtLeast(
        TechniqueId::FireMaking,KnowledgeLevel::Reproducible)){
        demand+=constructionMaterialDemandForKindNear(
            world,FacilityKind::FirePit,
            material,authoritativePosition);
    }
    if(primitiveFurnaceKnowledgeReady(resident)){
        demand+=constructionMaterialDemandForKindNear(
            world,FacilityKind::Furnace,
            material,authoritativePosition);
    }
    if(resident.civilization.knowledge.knowsAtLeast(
        TechniqueId::Cultivation,KnowledgeLevel::Reproducible)){
        demand+=constructionMaterialDemandForKindNear(
            world,FacilityKind::CultivatedPlot,
            material,authoritativePosition);
    }
    return demand;
}

inline int residentUncoveredCommittedMaterialDemandAtPosition(
    const World& world,
    const Character& resident,
    MaterialKind material,
    GridPos authoritativePosition)
{
    const int held=material==MaterialKind::Water
        ? portableWaterCount(resident.civilization.inventory)
        : resident.civilization.inventory.count(
            ItemKind::RawMaterial,material);
    return std::max(
        0,
        residentCommittedMaterialDemandAtPosition(
            world,resident,material,authoritativePosition)
            -held
            -storageCountForMaterialNear(
                world,material,authoritativePosition));
}

inline int worldItemCount(const World& world,const Character& self,ItemKind kind,MaterialKind material,bool anyMaterial=false)
{
    int total=self.civilization.inventory.count(kind,material,anyMaterial);
    for(const auto& storage:world.storageSites) total+=storage.inventory.count(kind,material,anyMaterial);
    return total;
}

// Settlement logistics is shared infrastructure: committed projects and
// restoration work may be supplied by a common stockpile rather than requiring
// the same resident who gathered a material to remain its permanent carrier.
inline int settlementCarriedMaterialCount(
    const World& world,
    MaterialKind material)
{
    int total=0;
    for(const auto& resident:world.characters){
        if(!resident.alive) continue;
        total+=material==MaterialKind::Water
            ? portableWaterCount(resident.civilization.inventory)
            : resident.civilization.inventory.count(
                ItemKind::RawMaterial,material);
    }
    return total;
}

inline int settlementConstructionMaterialDemand(
    const World& world,
    MaterialKind material)
{
    return settlementConstructionMissingMaterial(world,material)
        +primitiveStorageMissingMaterial(world,material)
        +primitiveFirePitMissingMaterial(world,material)
        +primitiveFurnaceMissingMaterial(world,material)
        +cultivationConstructionMissingMaterial(world,material);
}

inline int settlementCommittedMaterialDemand(
    const World& world,
    MaterialKind material)
{
    return settlementConstructionMaterialDemand(world,material)
        +settlementRepairMaterialDemand(world,material);
}

inline int settlementUncoveredMaterialDemand(
    const World& world,
    MaterialKind material)
{
    return std::max(
        0,
        settlementCommittedMaterialDemand(world,material)
            -settlementCarriedMaterialCount(world,material)
            -storageCountForMaterial(world,material));
}

inline int residentCommittedMaterialDemand(
    const World& world,
    const Character& resident,
    MaterialKind material)
{
    int demand=settlementConstructionMissingMaterial(world,material)
        +settlementRepairMaterialDemand(world,material);

    if(resident.civilization.knowledge.knowsAtLeast(
        TechniqueId::PrimitiveStorage,KnowledgeLevel::Reproducible)){
        demand+=primitiveStorageMissingMaterial(world,material);
    }
    if(resident.civilization.knowledge.knowsAtLeast(
        TechniqueId::FireMaking,KnowledgeLevel::Reproducible)){
        demand+=primitiveFirePitMissingMaterial(world,material);
    }
    if(primitiveFurnaceKnowledgeReady(resident)){
        demand+=primitiveFurnaceMissingMaterial(world,material);
    }
    if(resident.civilization.knowledge.knowsAtLeast(
        TechniqueId::Cultivation,KnowledgeLevel::Reproducible)){
        demand+=cultivationConstructionMissingMaterial(world,material);
    }
    return demand;
}

inline int residentUncoveredCommittedMaterialDemand(
    const World& world,
    const Character& resident,
    MaterialKind material)
{
    const int held=material==MaterialKind::Water
        ? portableWaterCount(resident.civilization.inventory)
        : resident.civilization.inventory.count(
            ItemKind::RawMaterial,material);
    return std::max(
        0,
        residentCommittedMaterialDemand(world,resident,material)
            -held
            -storageCountForMaterial(world,material));
}

inline const TechniqueKnowledge* civilizationKnowledgeRecord(const KnowledgeState& knowledge,TechniqueId technique)
{
    for(const auto& record:knowledge.all()) if(record.technique==technique) return &record;
    return nullptr;
}

inline double civilizationTechniqueExperience01(
    const Character& self,
    TechniqueId technique)
{
    const TechniqueKnowledge* record=
        civilizationKnowledgeRecord(self.civilization.knowledge,technique);
    if(record==nullptr) return 0.0;
    return clampCivilization01(
        static_cast<double>(std::max(0,record->successfulUses))/8.0);
}

inline MaterialKind experimentMaterial(ExperimentKind kind)
{
    switch(kind){
        case ExperimentKind::StrikeStone: return MaterialKind::Flint;
        case ExperimentKind::HaftSharpFlake: return MaterialKind::Wood;
        case ExperimentKind::FrictionWood: return MaterialKind::Wood;
        case ExperimentKind::TwistFiber: return MaterialKind::Fiber;
        case ExperimentKind::ShapeClay: return MaterialKind::Clay;
        case ExperimentKind::ShapeDiggingStick: return MaterialKind::Wood;
        case ExperimentKind::HaftStoneHammer: return MaterialKind::Stone;
        case ExperimentKind::SmeltCopperOre: return MaterialKind::CopperOre;
        case ExperimentKind::CultivatePlantFood: return MaterialKind::PlantFood;
        case ExperimentKind::DesignateSanitationArea:
        case ExperimentKind::DigSanitationPit:
        case ExperimentKind::OrganizeStockpile:
            return MaterialKind::Unknown;
        default: return MaterialKind::Unknown;
    }
}

inline double provisionNeedForMaterial(
    const Character& self,
    MaterialKind material)
{
    switch(material){
        case MaterialKind::Water:
            // Water serves both thirst and washing. A resident who is very
            // dirty but not thirsty should still seek/retrieve Water instead
            // of waiting until thirst independently creates demand.
            return clampCivilization01(std::max(
                self.needs.thirst,
                self.needs.hygiene));
        case MaterialKind::PlantFood:
            return clampCivilization01(self.needs.hunger);
        default:
            return 0.0;
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
            if(!knowledge.knowsAtLeast(TechniqueId::DiggingStick,KnowledgeLevel::Reproducible)) return 0.88;
            if(!knowledge.knowsAtLeast(TechniqueId::FireMaking,KnowledgeLevel::Reproducible)) return 0.82;
            return 0.52;
        case MaterialKind::Fiber:
            if(!knowledge.knowsAtLeast(TechniqueId::FiberCordage,KnowledgeLevel::Reproducible)) return 0.84;
            if(!knowledge.knowsAtLeast(TechniqueId::StoneHammer,KnowledgeLevel::Reproducible)) return 0.70;
            return 0.38;
        case MaterialKind::Clay:
            return knowledge.knowsAtLeast(TechniqueId::SimpleContainer,KnowledgeLevel::Reproducible) ? 0.42 : 0.80;
        case MaterialKind::PlantFood:
        case MaterialKind::Water:
            return 0.25+0.55*provisionNeedForMaterial(self,material);
        case MaterialKind::Stone:
            return knowledge.knowsAtLeast(TechniqueId::StoneHammer,KnowledgeLevel::Reproducible) ? 0.40 : 0.72;
        case MaterialKind::CopperOre:
            if(!knowledge.knowsAtLeast(TechniqueId::StoneHammer,KnowledgeLevel::Reproducible)) return 0.18;
            return knowledge.knowsAtLeast(TechniqueId::CopperSmelting,KnowledgeLevel::Reproducible) ? 0.62 : 0.52;
        case MaterialKind::TinOre:
        case MaterialKind::IronOre:
            return knowledge.knowsAtLeast(TechniqueId::StoneHammer,KnowledgeLevel::Reproducible) ? 0.34 : 0.18;
        case MaterialKind::Charcoal:
            return knowledge.knowsAtLeast(TechniqueId::CopperSmelting,KnowledgeLevel::Reproducible) ? 0.62 : 0.28;
        default:
            return 0.12;
    }
}

inline void considerCivilizationDecision(CivilizationUtilityDecision& best,const CivilizationUtilityDecision& candidate)
{
    if(candidate.intent==CivilizationIntent::None || candidate.utility<=0.0) return;
    if(candidate.utility>best.utility+1e-12) best=candidate;
}

inline GridPos civilizationDecisionResourcePosition(
    const World& world,
    const ResourceNode& node)
{
    for(const auto& chunk:world.generatedNaturalChunks){
        for(const auto& patch:chunk.resourcePatches){
            if(patch.nodeId==node.id) return patch.pos;
        }
    }
    return node.pos;
}

inline constexpr int WaterTransportComfortDistanceGrid=8;
inline constexpr int WaterTransportSevereDistanceGrid=WorldChunkSpanGridCells;
inline constexpr int PortableProvisionCarryTarget=2;
inline constexpr int SettlementWaterReserveTarget=8;
inline constexpr int SimpleContainerPersonalStockTarget=
    PortableProvisionCarryTarget;

inline double waterTransportInnovationPressure(
    const World& world,
    const Character& self,
    GridPos authoritativePosition)
{
    // Once the resident owns a reusable vessel, ordinary Gather/Drink owns the
    // refill loop. Likewise, a real nearby stored portable-water reserve already
    // solves the transport problem without inventing another technology.
    if(simpleContainerCount(self.civilization.inventory)>0
       || storageCountForMaterialNear(
            world,MaterialKind::Water,authoritativePosition)>0){
        return 0.0;
    }

    int nearestWaterDistance=std::numeric_limits<int>::max();
    for(const ResourceNode& node:world.resourceNodes){
        if(node.id==0
           || node.material!=MaterialKind::Water
           || node.quantity<=0){
            continue;
        }
        const GridPos waterPos=civilizationDecisionResourcePosition(world,node);
        nearestWaterDistance=std::min(
            nearestWaterDistance,
            manhattan(authoritativePosition,waterPos));
    }
    if(nearestWaterDistance==std::numeric_limits<int>::max()) return 0.0;

    const double distancePressure=clampCivilization01(
        static_cast<double>(
            std::max(0,nearestWaterDistance-WaterTransportComfortDistanceGrid))
        /static_cast<double>(
            std::max(
                1,
                WaterTransportSevereDistanceGrid
                    -WaterTransportComfortDistanceGrid)));
    const double needPressure=provisionNeedForMaterial(
        self,MaterialKind::Water);

    // Repeated long water walks become evidence for a transport problem.
    // This does not grant the solution: residents must still gather Clay,
    // choose ShapeClay over competing work, and pass the normal experiment roll.
    return clampCivilization01(
        0.72*distancePressure
        +0.28*needPressure);
}

inline int settlementContainerCapitalAtPosition(
    const World& world,
    const Character& self,
    GridPos authoritativePosition,
    const SettlementPopulation* population=nullptr)
{
    int total=0;
    if(population!=nullptr){
        for(const auto& entry:*population){
            if(manhattan(entry.second,authoritativePosition)
               >SettlementServiceRadiusGrid) continue;
            for(const Character& resident:world.characters){
                if(resident.id!=entry.first || !resident.alive) continue;
                total+=simpleContainerCount(
                    resident.civilization.inventory);
                break;
            }
        }
    }else{
        total+=simpleContainerCount(self.civilization.inventory);
    }

    for(const StorageSite& storage:world.storageSites){
        if(!settlementStorageServesPosition(
            storage,authoritativePosition)) continue;
        total+=simpleContainerCount(storage.inventory);
    }
    return total;
}

inline int settlementResidentCountAtPosition(
    const World& world,
    const Character& self,
    GridPos authoritativePosition,
    const SettlementPopulation* population=nullptr)
{
    if(population==nullptr) return self.alive ? 1 : 0;

    int count=0;
    for(const auto& entry:*population){
        if(manhattan(entry.second,authoritativePosition)
           >SettlementServiceRadiusGrid) continue;
        for(const Character& resident:world.characters){
            if(resident.id==entry.first && resident.alive){
                ++count;
                break;
            }
        }
    }
    return std::max(1,count);
}

inline double simpleContainerLogisticsStockPressure(
    const World& world,
    const Character& self,
    GridPos authoritativePosition,
    const SettlementPopulation* population=nullptr)
{
    if(!self.civilization.knowledge.knowsAtLeast(
        TechniqueId::SimpleContainer,KnowledgeLevel::Reproducible)){
        return 0.0;
    }

    const int residents=settlementResidentCountAtPosition(
        world,self,authoritativePosition,population);
    const bool sharedStorageAvailable=
        nearestSettlementStorage(world,authoritativePosition)!=nullptr;
    const int desired=
        residents*SimpleContainerPersonalStockTarget
        +(sharedStorageAvailable ? SettlementWaterReserveTarget : 0);
    const int available=settlementContainerCapitalAtPosition(
        world,self,authoritativePosition,population);
    return desired>0
        ? clampCivilization01(
            static_cast<double>(std::max(0,desired-available))
            /static_cast<double>(desired))
        : 0.0;
}

inline int knownNaturalResourceUnits(
    const World& world,
    MaterialKind material)
{
    int total=0;
    for(const auto& node:world.resourceNodes){
        if(node.material==material && node.quantity>0) total+=node.quantity;
    }
    return total;
}

inline int localNaturalResourceUnits(
    const World& world,
    MaterialKind material,
    GridPos authoritativePosition)
{
    constexpr int LocalRadiusChunks=3;
    const int radius=WorldChunkSpanGridCells*LocalRadiusChunks;
    int total=0;
    for(const auto& node:world.resourceNodes){
        if(node.material!=material || node.quantity<=0) continue;
        const GridPos nodePos=civilizationDecisionResourcePosition(world,node);
        const int distance=std::max(
            std::abs(nodePos.x-authoritativePosition.x),
            std::abs(nodePos.y-authoritativePosition.y));
        if(distance<=radius) total+=node.quantity;
    }
    return total;
}

inline double civilizationResourceExplorationPressure(
    const World& world,
    const Character& self,
    MaterialKind material,
    GridPos authoritativePosition)
{
    if(!validNaturalResourceMaterial(material)) return 0.0;

    const int held=material==MaterialKind::Water
        ? portableWaterCount(self.civilization.inventory)
        : self.civilization.inventory.count(ItemKind::RawMaterial,material);
    const int stored=storageCountForMaterialNear(
        world,material,authoritativePosition);
    const int repairMissing=
        settlementRepairMaterialDemandNear(
            world,material,authoritativePosition);
    const int uncoveredCommitted=
        residentUncoveredCommittedMaterialDemandAtPosition(
            world,self,material,authoritativePosition);

    const bool provision=
        material==MaterialKind::Water
        || material==MaterialKind::PlantFood;
    const double progressDemand=materialProgressDemand(self,material);

    // For critical provisions, a known supply is always safer than a blind
    // frontier search. Exploration becomes relevant only after all currently
    // known Water/PlantFood nodes are exhausted.
    if(provision && knownNaturalResourceUnits(world,material)>0) return 0.0;

    // Do not roam for advanced ores merely because the map can contain them.
    // Search needs either a current survival/provision role, a concrete build/
    // repair demand, or enough learned/progressive relevance to justify it.
    if(!provision
       && uncoveredCommitted<=0
       && progressDemand<0.40){
        return 0.0;
    }

    const int reserveTarget=provision ? 8 : 0;
    const int reserveGap=std::max(0,reserveTarget-stored);
    const int fireFuelReserve=
        (hasOperationalFirePit(world) && material==MaterialKind::Wood) ? 3 : 0;
    const int target=
        (provision ? 4 : 5)
        +std::min(4,uncoveredCommitted)
        +fireFuelReserve
        +(provision && !world.storageSites.empty()
            ? std::min(4,reserveGap)
            : 0);
    const int storedCredit=provision
        ? std::min(stored,reserveTarget)
        : std::min(stored,target);
    const int stockGap=std::max(0,target-held-storedCredit);
    if(stockGap<=0 && uncoveredCommitted<=0) return 0.0;

    const int localUnits=localNaturalResourceUnits(
        world,material,authoritativePosition);
    const int comfortableLocalUnits=provision ? 12 : 8;
    const double localScarcity=1.0-clampCivilization01(
        static_cast<double>(localUnits)
        /static_cast<double>(comfortableLocalUnits));
    if(localScarcity<0.35) return 0.0;

    const double gap=clampCivilization01(
        static_cast<double>(stockGap)
        /static_cast<double>(std::max(1,target)));
    const double constructionDemand=uncoveredCommitted>0
        ? clampCivilization01(
            0.45+0.12*static_cast<double>(uncoveredCommitted))
        : 0.0;
    const double maintenanceDemand=
        repairMissing>0 && uncoveredCommitted>0
            ? clampCivilization01(
                0.42+0.18*static_cast<double>(
                    std::min(repairMissing,uncoveredCommitted)))
            : 0.0;
    const double survivalPressure=provisionNeedForMaterial(self,material);

    return clampCivilization01(
        0.22*progressDemand
        +0.30*gap
        +0.26*localScarcity
        +0.18*constructionDemand
        +0.14*maintenanceDemand
        +0.12*survivalPressure);
}

inline CivilizationUtilityDecision bestResourceExplorationDecisionAtPosition(
    const World& world,
    const Character& self,
    GridPos authoritativePosition)
{
    CivilizationUtilityDecision best;
    const std::array<MaterialKind,9> naturalMaterials={
        MaterialKind::Water,
        MaterialKind::Wood,
        MaterialKind::Stone,
        MaterialKind::Flint,
        MaterialKind::Fiber,
        MaterialKind::Clay,
        MaterialKind::PlantFood,
        MaterialKind::CopperOre,
        MaterialKind::TinOre
    };

    for(const MaterialKind material:naturalMaterials){
        const double pressure=civilizationResourceExplorationPressure(
            world,self,material,authoritativePosition);
        if(pressure<=0.0) continue;

        const ResourceExplorationOpportunity opportunity=
            chooseResourceExplorationOpportunity(
                world,self.id,material,authoritativePosition);
        if(!opportunity.available) continue;

        const double preference=civilizationPreference(
            world.seed,self.id,
            900ULL+static_cast<std::uint64_t>(material));
        CivilizationUtilityDecision candidate;
        candidate.intent=CivilizationIntent::Explore;
        candidate.utility=clampCivilization01(
            0.18
            +0.62*pressure
            +0.08*self.personality.curiosity
            +0.05*self.personality.adaptability
            +0.04*opportunity.suitability
            +0.03*preference);
        candidate.material=material;
        candidate.item=ItemKind::RawMaterial;
        considerCivilizationDecision(best,candidate);
    }
    return best;
}

inline CivilizationUtilityDecision bestGatherDecisionAtPosition(
    const World& world,
    const Character& self,
    GridPos authoritativePosition,
    const SettlementPopulation* population=nullptr)
{
    CivilizationUtilityDecision best;
    for(const auto& node:world.resourceNodes){
        if(node.id==0 || node.quantity<=0 || node.material==MaterialKind::Unknown) continue;
        const int held=node.material==MaterialKind::Water
            ? portableWaterCount(self.civilization.inventory)
            : self.civilization.inventory.count(ItemKind::RawMaterial,node.material);
        if(node.material==MaterialKind::Water
           && emptySimpleContainerCount(self.civilization.inventory)<=0){
            continue;
        }
        const int stored=storageCountForMaterialNear(
            world,node.material,authoritativePosition);
        const int repairMissing=
            settlementRepairMaterialDemandNear(
                world,node.material,authoritativePosition);
        const int materialDemand=
            residentUncoveredCommittedMaterialDemandAtPosition(
                world,self,node.material,authoritativePosition);
        const bool provision=
            node.material==MaterialKind::Water
            || node.material==MaterialKind::PlantFood;
        const int baseTarget=provision ? 4 : 5;
        const int settlementReserveTarget=
            node.material==MaterialKind::Water ? 8
            : (node.material==MaterialKind::PlantFood ? 8 : 0);
        const int reserveGap=std::max(0,settlementReserveTarget-stored);
        const int fireFuelReserve=
            (hasOperationalFirePit(world) && node.material==MaterialKind::Wood)
                ? 3 : 0;
        const int target=
            baseTarget
            +std::min(4,materialDemand)
            +fireFuelReserve
            +(provision && !world.storageSites.empty()
                ? std::min(4,reserveGap)
                : 0);
        const int storedCredit=provision
            ? std::min(stored,settlementReserveTarget)
            : std::min(stored,target);
        const double gap=clampCivilization01(
            static_cast<double>(std::max(0,target-held-storedCredit))
            /static_cast<double>(std::max(1,target)));
        const double demand=materialProgressDemand(self,node.material);
        const double constructionDemand=materialDemand>0
            ? clampCivilization01(
                0.45+0.12*static_cast<double>(materialDemand))
            : 0.0;
        const double maintenanceDemand=
            repairMissing>0 && materialDemand>0
                ? clampCivilization01(
                    0.42+0.18*static_cast<double>(
                        std::min(repairMissing,materialDemand)))
                : 0.0;
        const double preference=civilizationPreference(world.seed,self.id,100ULL+static_cast<std::uint64_t>(node.material));
        const GridPos nodePos=civilizationDecisionResourcePosition(world,node);
        const int distance=std::max(
            std::abs(nodePos.x-authoritativePosition.x),
            std::abs(nodePos.y-authoritativePosition.y));
        const double distance01=clampCivilization01(
            static_cast<double>(distance)
            /static_cast<double>(WorldChunkSpanGridCells*6));
        const double localBonus=
            distance<=WorldChunkSpanGridCells*3 ? 0.04 : 0.0;
        const double distancePenalty=provision ? 0.0 : 0.20*distance01;
        const bool containerInputsReady=
            node.material==MaterialKind::Clay
            && hasIngredients(
                self.civilization.inventory,
                techniqueRecipe(TechniqueId::SimpleContainer).inputs);
        const double waterTransportBoost=
            node.material==MaterialKind::Clay && !containerInputsReady
                ? 0.22*std::max(
                    waterTransportInnovationPressure(
                        world,self,authoritativePosition),
                    simpleContainerLogisticsStockPressure(
                        world,self,authoritativePosition,population))
                : 0.0;
        const double score=clampCivilization01(
            0.07+0.12*self.personality.curiosity+0.05*self.personality.adaptability+
            0.08*self.civilization.gatheringSkill+0.16*demand+0.13*gap+
            0.30*constructionDemand+0.24*maintenanceDemand+0.07*preference+
            localBonus+waterTransportBoost-distancePenalty+
            (provision && !world.storageSites.empty()
                ? 0.12*clampCivilization01(
                    static_cast<double>(reserveGap)
                    /static_cast<double>(std::max(1,settlementReserveTarget)))
                : 0.0));
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

inline CivilizationUtilityDecision bestGatherDecision(
    const World& world,
    const Character& self)
{
    return bestGatherDecisionAtPosition(
        world,self,civilizationSanitationReferencePosition(world));
}

inline CivilizationUtilityDecision bestExperimentDecisionAtPosition(
    const World& world,
    const Character& self,
    GridPos authoritativePosition,
    const SettlementPopulation* population=nullptr)
{
    CivilizationUtilityDecision best;
    const GridPos sanitationReference=authoritativePosition;
    const PrimitiveSanitationOpportunity sanitationOpportunity=
        evaluatePrimitiveSanitationOpportunity(
            world.seed,self,world.environmentalResidues,world.minute,sanitationReference);
    const DugSanitationPitOpportunity pitOpportunity=
        evaluateDugSanitationPitOpportunity(
            self,world.environmentalResidues,world.primitiveSanitationSites);
    const PrimitiveStorageNeedObservation storageNeed=
        observePrimitiveStorageNeed(
            world,self,authoritativePosition,population);
    const bool smeltingOpportunity=copperSmeltingOpportunityAvailable(world,self);
    const bool cultivationOpportunity=cultivationExperimentOpportunityAvailable(
        world,self,authoritativePosition,population);
    const std::array<ExperimentKind,12> experiments={
        ExperimentKind::StrikeStone,ExperimentKind::HaftSharpFlake,ExperimentKind::FrictionWood,
        ExperimentKind::TwistFiber,ExperimentKind::ShapeClay,
        ExperimentKind::ShapeDiggingStick,ExperimentKind::HaftStoneHammer,
        ExperimentKind::DesignateSanitationArea,ExperimentKind::DigSanitationPit,
        ExperimentKind::OrganizeStockpile,ExperimentKind::SmeltCopperOre,
        ExperimentKind::CultivatePlantFood};

    for(const ExperimentKind kind:experiments){
        const TechniqueId technique=experimentTechnique(kind);
        if(technique==TechniqueId::None) continue;
        if(self.civilization.knowledge.knowsAtLeast(technique,KnowledgeLevel::Reproducible)) continue;

        const bool designatedExperiment=kind==ExperimentKind::DesignateSanitationArea;
        const bool pitExperiment=kind==ExperimentKind::DigSanitationPit;
        const bool storageExperiment=kind==ExperimentKind::OrganizeStockpile;
        const bool smeltingExperiment=kind==ExperimentKind::SmeltCopperOre;
        const bool cultivationExperiment=kind==ExperimentKind::CultivatePlantFood;
        if(designatedExperiment &&
           (!sanitationOpportunity.problemRecognized || !sanitationOpportunity.siteAvailable)) continue;
        if(pitExperiment && !pitOpportunity.candidateAvailable) continue;
        if(storageExperiment && !storageNeed.recognized) continue;
        if(smeltingExperiment && !smeltingOpportunity) continue;
        if(cultivationExperiment && !cultivationOpportunity) continue;

        ExperimentContext context;
        context.worldSeed=world.seed;
        context.actor=self.id;
        context.attemptIndex=static_cast<std::uint64_t>(std::max(0,world.minute));
        context.kind=kind;
        context.material=experimentMaterial(kind);
        context.learningSkill=self.civilization.learningSkill;
        context.curiosity=self.personality.curiosity;
        context.patience=self.personality.patience;
        context.sanitationProblemRecognized=sanitationOpportunity.problemRecognized;
        context.sanitationSiteAvailable=sanitationOpportunity.siteAvailable;
        context.sanitationPitCandidateAvailable=pitOpportunity.candidateAvailable;
        context.storageProblemRecognized=storageNeed.recognized;
        context.smeltingOpportunityAvailable=smeltingOpportunity;
        context.cultivationOpportunityAvailable=cultivationOpportunity;

        if(!experimentPrerequisitesMet(context,self.civilization.knowledge)) continue;
        const TechniqueRecipe recipe=experimentRecipe(context);
        if(!hasIngredients(self.civilization.inventory,recipe.inputs)) continue;
        if(experimentBaseChance(kind,context.material)<=0.0) continue;

        const KnowledgeLevel level=self.civilization.knowledge.level(technique);
        const double hypothesisBoost=level==KnowledgeLevel::Hypothesized ? 0.08 : (level==KnowledgeLevel::Understood ? 0.05 : 0.0);
        const double preference=civilizationPreference(world.seed,self.id,200ULL+static_cast<std::uint64_t>(kind));
        double sanitationBoost=0.0;
        double storageBoost=0.0;
        double smeltingBoost=0.0;
        double cultivationBoost=0.0;
        double waterTransportBoost=0.0;
        if(designatedExperiment){
            sanitationBoost=0.18+0.16*sanitationOpportunity.problemConfidence+
                0.10*clampCivilization01(self.needs.hygiene);
        }else if(pitExperiment){
            sanitationBoost=0.18
                +0.10*pitOpportunity.problemConfidence
                +0.05*clampCivilization01(static_cast<double>(pitOpportunity.useCount)/4.0)
                +0.08*clampCivilization01(pitOpportunity.siteExposure);
        }else if(storageExperiment){
            storageBoost=0.20+0.22*storageNeed.pressure+
                0.08*self.personality.orderliness+
                0.06*self.personality.conscientiousness;
        }else if(smeltingExperiment){
            smeltingBoost=0.24+0.12*self.personality.curiosity+
                0.08*self.civilization.craftingSkill;
        }else if(cultivationExperiment){
            const CultivationDemandObservation demand=
                observeCultivationDemand(world,authoritativePosition,population);
            cultivationBoost=0.20+0.30*demand.pressure
                +0.08*self.personality.patience
                +0.06*self.personality.conscientiousness;
        }else if(kind==ExperimentKind::ShapeClay){
            waterTransportBoost=
                0.28*waterTransportInnovationPressure(
                    world,self,authoritativePosition);
        }
        const double score=clampCivilization01(
            0.11+0.22*self.personality.curiosity+0.10*self.personality.openness+
            0.07*self.personality.patience+0.12*self.civilization.learningSkill+
            0.08*preference+hypothesisBoost+sanitationBoost+storageBoost
            +smeltingBoost+cultivationBoost+waterTransportBoost);

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

inline CivilizationUtilityDecision bestExperimentDecision(
    const World& world,
    const Character& self)
{
    return bestExperimentDecisionAtPosition(
        world,self,civilizationSanitationReferencePosition(world),nullptr);
}

inline int desiredTechniqueOutputStock(TechniqueId technique)
{
    switch(technique){
        case TechniqueId::SharpFlake: return 2;
        case TechniqueId::ChippedStoneTool: return 1;
        case TechniqueId::FiberCordage: return 2;
        case TechniqueId::SimpleContainer:
            return SimpleContainerPersonalStockTarget;
        case TechniqueId::DiggingStick: return 1;
        case TechniqueId::StoneHammer: return 1;
        case TechniqueId::FireMaking:
        case TechniqueId::CopperSmelting:
        case TechniqueId::Cultivation:
        case TechniqueId::DesignatedSanitationArea:
        case TechniqueId::DugSanitationPit:
        case TechniqueId::PrimitiveStorage:
        default: return 0;
    }
}

inline double settlementCraftDemandPressure(const Character& self)
{
    int successfulUses=0;
    int reproducibleTechniques=0;
    for(const auto& record:self.civilization.knowledge.all()){
        successfulUses+=std::max(0,record.successfulUses);
        if(self.civilization.knowledge.knowsAtLeast(
            record.technique,KnowledgeLevel::Reproducible)) ++reproducibleTechniques;
    }
    return clampCivilization01(
        0.05
        +0.055*static_cast<double>(std::min(successfulUses,10))
        +0.045*static_cast<double>(std::min(reproducibleTechniques,5))
        +0.18*clampCivilization01(self.civilization.craftingSkill));
}

inline double settlementFacilityNeedPressure(
    const World& world,
    const Character& self,
    GridPos authoritativePosition,
    FacilityKind kind,
    const SettlementPopulation* population=nullptr)
{
    if(!isSettlementFoundationFacility(kind)) return 0.0;
    const auto demand=observeSettlementFacilityDemand(
        world,self.id,kind,authoritativePosition,population);
    if(!demand.unmet()) return 0.0;

    if(kind==FacilityKind::SleepingPlace){
        return clampCivilization01(
            (clampCivilization01(std::max(self.needs.sleep,demand.peakSleepNeed))-0.22)/0.48);
    }

    if(kind==FacilityKind::Shelter){
        const DynamicEnvironmentObservation environment=deriveDynamicEnvironment(
            world.genesisIdentity(),chunkCoordForGrid(authoritativePosition),world.minute);
        const EnvironmentalConsequenceProfile consequence=
            deriveEnvironmentalConsequences(environment);
        const double weatherPressure=std::max({
            consequence.heatStress01,
            consequence.coldStress01,
            0.90*environment.precipitationIntensity01,
            0.72*environment.surfaceWetness01,
            0.58*environment.windIntensity01,
            consequence.outdoorWorkFriction01
        });

        // A single harsh minute is not enough to invent a settlement need.
        // Environmental consequences already accumulate into resident Needs;
        // combining local weather with that persistent burden makes Shelter
        // recognition emerge after sustained exposure while keeping the
        // resident's authoritative location causal.
        const double accumulatedBurden=std::max({
            clampCivilization01(self.needs.thirst),
            clampCivilization01(self.needs.sleep),
            clampCivilization01(self.needs.hygiene),
            clampCivilization01(demand.peakExposureBurden)
        });
        return clampCivilization01(
            weatherPressure*(0.08+0.92*accumulatedBurden));
    }

    if(kind==FacilityKind::WorkSurface){
        return settlementCraftDemandPressure(self);
    }

    return 0.0;
}

inline CivilizationUtilityDecision bestSettlementFoundationDecision(
    const World& world,
    const Character& self,
    GridPos authoritativePosition,
    const SettlementPopulation* population=nullptr)
{
    CivilizationUtilityDecision best;
    constexpr std::array<FacilityKind,3> kinds={
        FacilityKind::SleepingPlace,
        FacilityKind::Shelter,
        FacilityKind::WorkSurface
    };

    for(const FacilityKind kind:kinds){
        const ConstructedFacility* project=settlementConstructionProjectNear(
            world,kind,authoritativePosition);
        const double preference=civilizationPreference(
            world.seed,self.id,610ULL+static_cast<std::uint64_t>(kind));
        const SettlementFacilityDemand demand=observeSettlementFacilityDemand(
            world,self.id,kind,authoritativePosition,population);
        const bool restoreNeeded=demand.unmet();
        bool hasRestorableRuin=false;

        for(const auto& facility:world.facilities){
            if(facility.kind!=kind
               || manhattan(
                   facility.pos,
                   authoritativePosition)>SettlementServiceRadiusGrid){
                continue;
            }

            if(settlementFacilityCanRestore(facility) && restoreNeeded){
                hasRestorableRuin=true;
                const auto restorationRequirements=
                    facilityRestorationRequirements(kind);
                bool canRestore=!restorationRequirements.empty();
                int restorationUnits=0;
                for(const auto& requirement:restorationRequirements){
                    restorationUnits+=requirement.required;
                    if(self.civilization.inventory.count(
                        ItemKind::RawMaterial,
                        requirement.material)<requirement.required){
                        canRestore=false;
                    }
                }
                if(!canRestore) continue;

                CivilizationUtilityDecision restore;
                restore.intent=CivilizationIntent::Craft;
                restore.facilityKind=kind;
                restore.facilityAction=FacilityBuildAction::Repair;
                restore.facility=facility.id;
                restore.hasFacilityTarget=true;
                restore.facilityTargetPos=facility.pos;
                restore.material=restorationRequirements.front().material;
                restore.item=ItemKind::RawMaterial;
                restore.quantity=restorationUnits;
                // Reusing an existing frame is deliberately more attractive
                // than consuming a full new construction package.
                restore.utility=clampCivilization01(
                    0.78
                    +0.08*self.personality.conscientiousness
                    +0.06*self.personality.orderliness
                    +0.06*self.civilization.craftingSkill
                    +0.04*preference);
                considerCivilizationDecision(best,restore);
                continue;
            }

            if(!settlementFacilityNeedsMaintenance(facility)) continue;

            const MaterialKind repairMaterial=facilityRepairMaterial(kind);
            const int held=self.civilization.inventory.count(
                ItemKind::RawMaterial,repairMaterial);
            if(held<=0) continue;

            CivilizationUtilityDecision repair;
            repair.intent=CivilizationIntent::Craft;
            repair.facilityKind=kind;
            repair.facilityAction=FacilityBuildAction::Repair;
            repair.facility=facility.id;
            repair.hasFacilityTarget=true;
            repair.facilityTargetPos=facility.pos;
            repair.material=repairMaterial;
            repair.item=ItemKind::RawMaterial;
            repair.quantity=1;

            const double damage=clampCivilization01(1.0-facility.durability);
            repair.utility=clampCivilization01(
                0.46+0.34*damage
                +0.09*self.personality.conscientiousness
                +0.07*self.personality.orderliness
                +0.07*self.civilization.craftingSkill
                +0.04*preference);
            considerCivilizationDecision(best,repair);
        }

        double pressure=settlementFacilityNeedPressure(
            world,self,authoritativePosition,kind,population);
        if(project==nullptr && pressure<0.24 && !hasRestorableRuin) continue;
        if(project!=nullptr) pressure=std::max(pressure,0.46);

        CivilizationUtilityDecision candidate;
        candidate.intent=CivilizationIntent::Craft;
        candidate.facilityKind=kind;
        candidate.item=ItemKind::RawMaterial;
        candidate.technique=TechniqueId::None;

        if(project==nullptr){
            // A recoverable local structure owns the demand until residents
            // either restore it or discover/gather its cheaper repair package.
            // Do not create a replacement beside reusable ruins.
            if(hasRestorableRuin) continue;

            const SettlementFacilitySiteOpportunity site=
                chooseSettlementFacilitySite(
                    world,self.id,kind,authoritativePosition,population);
            if(!site.available) continue;
            candidate.facilityAction=FacilityBuildAction::Plan;
            candidate.hasFacilityTarget=true;
            candidate.facilityTargetPos=site.pos;
            candidate.utility=clampCivilization01(
                0.24+0.56*pressure
                +0.07*self.personality.conscientiousness
                +0.05*self.personality.adaptability
                +0.04*preference);
            considerCivilizationDecision(best,candidate);
            continue;
        }

        candidate.facility=project->id;
        candidate.hasFacilityTarget=true;
        candidate.facilityTargetPos=project->pos;

        bool canDeliver=false;
        for(const auto& requirement:project->requirements){
            const int missing=std::max(0,requirement.required-requirement.delivered);
            const int held=self.civilization.inventory.count(
                ItemKind::RawMaterial,requirement.material);
            if(missing<=0 || held<=0) continue;
            candidate.facilityAction=FacilityBuildAction::DeliverMaterial;
            candidate.material=requirement.material;
            candidate.quantity=std::min({missing,held,2});
            candidate.utility=clampCivilization01(
                0.47+0.32*pressure
                +0.08*self.personality.conscientiousness
                +0.06*self.civilization.gatheringSkill
                +0.04*preference);
            canDeliver=true;
            break;
        }
        if(canDeliver){
            considerCivilizationDecision(best,candidate);
            continue;
        }

        if(facilityMaterialsComplete(*project) && !facilityWorkComplete(*project)){
            candidate.facilityAction=FacilityBuildAction::Work;
            candidate.facilityWork=1.25+1.75*clampCivilization01(
                self.civilization.craftingSkill);
            const double progress=clampCivilization01(
                project->constructionWork/std::max(0.1,project->requiredWork));
            candidate.utility=clampCivilization01(
                0.49+0.28*pressure
                +0.08*self.personality.conscientiousness
                +0.07*self.personality.patience
                +0.07*self.civilization.craftingSkill
                +0.05*progress+0.03*preference);
            considerCivilizationDecision(best,candidate);
        }
    }

    return best;
}

inline CivilizationUtilityDecision bestPrimitiveStorageConstructionDecision(
    const World& world,
    const Character& self,
    GridPos authoritativePosition)
{
    CivilizationUtilityDecision candidate;
    if(!self.civilization.knowledge.knowsAtLeast(
        TechniqueId::PrimitiveStorage,KnowledgeLevel::Reproducible)) return candidate;
    if(hasOperationalPrimitiveStorage(world)) return candidate;

    const ConstructedFacility* project=primitiveStorageProject(world);
    const double preference=civilizationPreference(
        world.seed,self.id,470ULL+static_cast<std::uint64_t>(TechniqueId::PrimitiveStorage));

    candidate.intent=CivilizationIntent::Craft;
    candidate.technique=TechniqueId::PrimitiveStorage;
    candidate.facilityKind=FacilityKind::PrimitiveStorage;
    candidate.item=ItemKind::RawMaterial;

    if(project==nullptr){
        const PrimitiveStorageSiteOpportunity site=
            choosePrimitiveStorageSite(world,self.id,authoritativePosition);
        if(!site.available) return CivilizationUtilityDecision{};
        candidate.facilityAction=FacilityBuildAction::Plan;
        candidate.hasFacilityTarget=true;
        candidate.facilityTargetPos=site.pos;
        candidate.utility=clampCivilization01(
            0.36+0.13*self.personality.orderliness+
            0.10*self.personality.conscientiousness+
            0.07*self.personality.adaptability+
            0.06*preference);
        return candidate;
    }

    candidate.facility=project->id;
    candidate.hasFacilityTarget=true;
    candidate.facilityTargetPos=project->pos;

    for(const auto& requirement:project->requirements){
        const int missing=std::max(0,requirement.required-requirement.delivered);
        const int held=self.civilization.inventory.count(ItemKind::RawMaterial,requirement.material);
        if(missing<=0 || held<=0) continue;
        candidate.facilityAction=FacilityBuildAction::DeliverMaterial;
        candidate.material=requirement.material;
        candidate.quantity=std::min({missing,held,2});
        candidate.utility=clampCivilization01(
            0.44+0.12*self.personality.conscientiousness+
            0.10*self.personality.orderliness+
            0.08*self.civilization.gatheringSkill+
            0.05*preference);
        return candidate;
    }

    if(facilityMaterialsComplete(*project) && !facilityWorkComplete(*project)){
        candidate.facilityAction=FacilityBuildAction::Work;
        candidate.facilityWork=1.35+1.65*clampCivilization01(self.civilization.craftingSkill);
        const double progress=clampCivilization01(
            project->constructionWork/std::max(0.1,project->requiredWork));
        candidate.utility=clampCivilization01(
            0.43+0.14*self.personality.conscientiousness+
            0.10*self.personality.patience+
            0.10*self.civilization.craftingSkill+
            0.07*progress+0.05*preference);
        return candidate;
    }

    return CivilizationUtilityDecision{};
}

inline CivilizationUtilityDecision bestPrimitiveFirePitDecision(
    const World& world,
    const Character& self,
    GridPos authoritativePosition)
{
    CivilizationUtilityDecision candidate;
    if(!self.civilization.knowledge.knowsAtLeast(
        TechniqueId::FireMaking,KnowledgeLevel::Reproducible)) return candidate;

    const ConstructedFacility* project=primitiveFirePitProject(world);
    const double preference=civilizationPreference(
        world.seed,self.id,490ULL+static_cast<std::uint64_t>(TechniqueId::FireMaking));

    candidate.intent=CivilizationIntent::Craft;
    candidate.technique=TechniqueId::FireMaking;
    candidate.facilityKind=FacilityKind::FirePit;
    candidate.item=ItemKind::RawMaterial;

    if(project==nullptr){
        const PrimitiveFirePitSiteOpportunity site=
            choosePrimitiveFirePitSite(world,self.id,authoritativePosition);
        if(!site.available) return CivilizationUtilityDecision{};
        candidate.facilityAction=FacilityBuildAction::Plan;
        candidate.hasFacilityTarget=true;
        candidate.facilityTargetPos=site.pos;
        candidate.utility=clampCivilization01(
            0.38+0.13*self.personality.conscientiousness+
            0.10*self.personality.adaptability+
            0.08*self.personality.curiosity+
            0.05*preference);
        return candidate;
    }

    candidate.facility=project->id;
    candidate.hasFacilityTarget=true;
    candidate.facilityTargetPos=project->pos;

    if(project->state!=FacilityState::Operational){
        for(const auto& requirement:project->requirements){
            const int missing=std::max(0,requirement.required-requirement.delivered);
            const int held=self.civilization.inventory.count(ItemKind::RawMaterial,requirement.material);
            if(missing<=0 || held<=0) continue;
            candidate.facilityAction=FacilityBuildAction::DeliverMaterial;
            candidate.material=requirement.material;
            candidate.quantity=std::min({missing,held,2});
            candidate.utility=clampCivilization01(
                0.46+0.12*self.personality.conscientiousness+
                0.08*self.civilization.gatheringSkill+
                0.06*self.personality.adaptability+
                0.05*preference);
            return candidate;
        }

        if(facilityMaterialsComplete(*project) && !facilityWorkComplete(*project)){
            candidate.facilityAction=FacilityBuildAction::Work;
            candidate.facilityWork=1.30+1.70*clampCivilization01(self.civilization.craftingSkill);
            const double progress=clampCivilization01(
                project->constructionWork/std::max(0.1,project->requiredWork));
            candidate.utility=clampCivilization01(
                0.45+0.13*self.personality.conscientiousness+
                0.10*self.personality.patience+
                0.10*self.civilization.craftingSkill+
                0.07*progress+0.05*preference);
            return candidate;
        }
        return CivilizationUtilityDecision{};
    }

    if(!project->lit && project->charcoalUnits>0){
        candidate.facilityAction=FacilityBuildAction::CollectCharcoal;
        candidate.material=MaterialKind::Charcoal;
        candidate.quantity=std::min(2,project->charcoalUnits);
        candidate.utility=clampCivilization01(
            0.48+0.10*self.personality.conscientiousness+
            0.08*self.personality.curiosity+0.05*preference);
        return candidate;
    }

    const int heldWood=self.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Wood);
    if(project->fuelUnits<2 && heldWood>0){
        candidate.facilityAction=FacilityBuildAction::Fuel;
        candidate.material=MaterialKind::Wood;
        candidate.quantity=std::min({2-project->fuelUnits,heldWood,2});
        candidate.utility=clampCivilization01(
            0.43+0.11*self.personality.conscientiousness+
            0.08*self.personality.adaptability+0.05*preference);
        return candidate;
    }

    if(!project->lit && project->fuelUnits>0){
        candidate.facilityAction=FacilityBuildAction::Ignite;
        candidate.utility=clampCivilization01(
            0.54+0.11*self.personality.curiosity+
            0.08*self.civilization.craftingSkill+0.05*preference);
        return candidate;
    }

    return CivilizationUtilityDecision{};
}

inline CivilizationUtilityDecision bestPrimitiveFurnaceDecision(
    const World& world,
    const Character& self,
    GridPos authoritativePosition)
{
    CivilizationUtilityDecision candidate;
    if(!primitiveFurnaceKnowledgeReady(self) || !hasOperationalFirePitForSmelting(world)) return candidate;

    const ConstructedFacility* project=primitiveFurnaceProject(world);
    const double preference=civilizationPreference(world.seed,self.id,530ULL);
    candidate.intent=CivilizationIntent::Craft;
    candidate.technique=self.civilization.knowledge.knowsAtLeast(
        TechniqueId::CopperSmelting,KnowledgeLevel::Reproducible)
        ? TechniqueId::CopperSmelting : TechniqueId::FireMaking;
    candidate.facilityKind=FacilityKind::Furnace;
    candidate.item=ItemKind::RawMaterial;

    if(project==nullptr){
        const PrimitiveFurnaceSiteOpportunity site=
            choosePrimitiveFurnaceSite(world,self.id,authoritativePosition);
        if(!site.available) return CivilizationUtilityDecision{};
        candidate.facilityAction=FacilityBuildAction::Plan;
        candidate.hasFacilityTarget=true;
        candidate.facilityTargetPos=site.pos;
        candidate.utility=clampCivilization01(
            0.34+0.12*self.personality.curiosity+
            0.12*self.personality.conscientiousness+
            0.10*self.civilization.craftingSkill+0.05*preference);
        return candidate;
    }

    candidate.facility=project->id;
    candidate.hasFacilityTarget=true;
    candidate.facilityTargetPos=project->pos;
    if(project->state!=FacilityState::Operational){
        for(const auto& requirement:project->requirements){
            const int missing=std::max(0,requirement.required-requirement.delivered);
            const int held=self.civilization.inventory.count(ItemKind::RawMaterial,requirement.material);
            if(missing<=0 || held<=0) continue;
            candidate.facilityAction=FacilityBuildAction::DeliverMaterial;
            candidate.material=requirement.material;
            candidate.quantity=std::min({missing,held,2});
            candidate.utility=clampCivilization01(
                0.47+0.12*self.personality.conscientiousness+
                0.10*self.civilization.gatheringSkill+
                0.07*self.civilization.craftingSkill+0.05*preference);
            return candidate;
        }
        if(facilityMaterialsComplete(*project) && !facilityWorkComplete(*project)){
            candidate.facilityAction=FacilityBuildAction::Work;
            candidate.facilityWork=1.20+1.80*clampCivilization01(self.civilization.craftingSkill);
            const double progress=clampCivilization01(
                project->constructionWork/std::max(0.1,project->requiredWork));
            candidate.utility=clampCivilization01(
                0.48+0.13*self.personality.conscientiousness+
                0.11*self.personality.patience+
                0.11*self.civilization.craftingSkill+
                0.08*progress+0.05*preference);
            return candidate;
        }
        return CivilizationUtilityDecision{};
    }

    if(!self.civilization.knowledge.knowsAtLeast(
        TechniqueId::CopperSmelting,KnowledgeLevel::Reproducible)) return CivilizationUtilityDecision{};

    if(!project->lit && project->metalUnits>0){
        candidate.facilityAction=FacilityBuildAction::CollectMetal;
        candidate.material=MaterialKind::CopperMetal;
        candidate.quantity=std::min(2,project->metalUnits);
        candidate.utility=clampCivilization01(
            0.56+0.10*self.personality.conscientiousness+
            0.07*self.civilization.craftingSkill+0.04*preference);
        return candidate;
    }

    const int heldOre=self.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::CopperOre);
    const int heldCharcoal=self.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Charcoal);
    if(!project->lit && project->oreUnits<2 && heldOre>0 && heldCharcoal>0){
        candidate.facilityAction=FacilityBuildAction::LoadSmeltCharge;
        candidate.material=MaterialKind::CopperOre;
        candidate.quantity=std::min({2-project->oreUnits,heldOre,heldCharcoal,2});
        candidate.utility=clampCivilization01(
            0.51+0.11*self.personality.conscientiousness+
            0.10*self.civilization.craftingSkill+0.05*preference);
        return candidate;
    }

    if(!project->lit && project->oreUnits>0 && project->fuelUnits>0){
        candidate.facilityAction=FacilityBuildAction::Ignite;
        candidate.utility=clampCivilization01(
            0.58+0.10*self.personality.curiosity+
            0.10*self.civilization.craftingSkill+0.05*preference);
        return candidate;
    }
    return CivilizationUtilityDecision{};
}

inline double cultivationSeedCommitmentClaim01(
    const Character& self,
    const CultivationDemandObservation& demand,
    double stewardship,
    double experience,
    double preference)
{
    const double hunger=clampCivilization01(self.needs.hunger);
    return clampCivilization01(
        0.38*stewardship
        +0.24*demand.foodPressure
        +0.16*experience
        +0.12*preference
        +0.10*(1.0-hunger));
}

inline CivilizationUtilityDecision bestCultivationDecision(
    const World& world,
    const Character& self,
    GridPos authoritativePosition,
    const SettlementPopulation* population=nullptr)
{
    CivilizationUtilityDecision best;
    if(!self.civilization.knowledge.knowsAtLeast(
        TechniqueId::Cultivation,KnowledgeLevel::Reproducible)) return best;

    const CultivationDemandObservation demand=
        observeCultivationDemand(world,authoritativePosition,population);
    const bool hasDiggingStick=
        self.civilization.inventory.count(
            ItemKind::DiggingStick,MaterialKind::Unknown,true)>0;
    const int seedUnits=self.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood);
    const int waterUnits=portableWaterCount(self.civilization.inventory);
    const double preference=civilizationPreference(
        world.seed,self.id,690ULL+static_cast<std::uint64_t>(TechniqueId::Cultivation));
    const double experience=civilizationTechniqueExperience01(
        self,TechniqueId::Cultivation);
    const double stewardship=clampCivilization01(
        0.30*self.personality.patience
        +0.26*self.personality.conscientiousness
        +0.16*self.personality.orderliness
        +0.14*self.personality.adaptability
        +0.14*experience);

    for(const auto& facility:world.facilities){
        if(facility.kind!=FacilityKind::CultivatedPlot
           || !facilityOperationalAndActive(facility)
           || manhattan(facility.pos,authoritativePosition)>CultivationServiceRadiusGrid){
            continue;
        }

        CivilizationUtilityDecision candidate;
        candidate.intent=CivilizationIntent::Craft;
        candidate.technique=TechniqueId::Cultivation;
        candidate.facilityKind=FacilityKind::CultivatedPlot;
        candidate.facility=facility.id;
        candidate.hasFacilityTarget=true;
        candidate.facilityTargetPos=facility.pos;
        candidate.item=ItemKind::RawMaterial;

        // Harvesting ready food is broadly valuable, but repeated growers still
        // react a little faster because lived experience is part of the choice.
        if(facility.cropHarvestUnits>0){
            candidate.facilityAction=FacilityBuildAction::Harvest;
            candidate.material=MaterialKind::PlantFood;
            candidate.quantity=facility.cropHarvestUnits;
            candidate.utility=clampCivilization01(
                0.58+0.22*demand.foodPressure
                +0.08*self.personality.conscientiousness
                +0.06*experience
                +0.06*preference);
            considerCivilizationDecision(best,candidate);
            continue;
        }

        // Planting is an optional development choice, not an automatic reaction
        // to "seed + tool + plot". Food pressure matters, but personal patience,
        // stewardship, experience and deterministic preference must be able to
        // make two residents choose differently under identical affordances.
        if(!facility.cropPlanted){
            if(!hasDiggingStick || seedUnits<=0) continue;

            // PlantFood is both edible provision and current seed authority.
            // Spending the final locally accessible unit is therefore an
            // allocation decision, not an automatic "plot + seed => Plant".
            // Compare the resident's near-term retention claim with their
            // future cultivation commitment. A tie keeps the reversible option
            // (food) rather than irreversibly consuming it as seed.
            const int accessibleFoodReserve=
                seedUnits+storageCountForMaterialNear(
                    world,MaterialKind::PlantFood,authoritativePosition);
            if(accessibleFoodReserve<=1){
                const double retentionClaim=plantFoodRetentionClaim01(self);
                const double seedCommitment=cultivationSeedCommitmentClaim01(
                    self,demand,stewardship,experience,preference);
                if(retentionClaim>=seedCommitment) continue;
            }

            candidate.facilityAction=FacilityBuildAction::Plant;
            candidate.material=MaterialKind::PlantFood;
            candidate.quantity=1;
            candidate.utility=clampCivilization01(
                0.20+0.18*demand.foodPressure
                +0.40*stewardship
                +0.12*experience
                +0.10*preference);
            considerCivilizationDecision(best,candidate);
            continue;
        }

        if(facility.cropMoisture01<0.46 && waterUnits>0){
            candidate.facilityAction=FacilityBuildAction::Water;
            candidate.material=MaterialKind::Water;
            candidate.quantity=1;
            const double dryness=clampCivilization01(
                (0.52-facility.cropMoisture01)/0.52);
            candidate.utility=clampCivilization01(
                0.34+0.32*dryness+0.08*demand.foodPressure
                +0.12*stewardship
                +0.08*experience
                +0.06*preference);
            considerCivilizationDecision(best,candidate);
        }

        if(hasDiggingStick && facility.cropCare01<0.58){
            candidate.facilityAction=FacilityBuildAction::Tend;
            candidate.material=MaterialKind::Unknown;
            candidate.quantity=0;
            const double careGap=clampCivilization01(1.0-facility.cropCare01);
            candidate.utility=clampCivilization01(
                0.18+0.18*careGap+0.08*demand.foodPressure
                +0.34*stewardship
                +0.12*experience
                +0.10*preference);
            considerCivilizationDecision(best,candidate);
        }
    }

    const ConstructedFacility* project=
        cultivatedPlotProjectNear(world,authoritativePosition);
    if(project!=nullptr){
        CivilizationUtilityDecision candidate;
        candidate.intent=CivilizationIntent::Craft;
        candidate.technique=TechniqueId::Cultivation;
        candidate.facilityKind=FacilityKind::CultivatedPlot;
        candidate.facility=project->id;
        candidate.hasFacilityTarget=true;
        candidate.facilityTargetPos=project->pos;
        candidate.item=ItemKind::RawMaterial;

        // Once a project is physically committed, logistics/work remain fairly
        // strong so personality diversity cannot strand half-built facilities.
        for(const auto& requirement:project->requirements){
            const int missing=std::max(
                0,requirement.required-requirement.delivered);
            const int held=self.civilization.inventory.count(
                ItemKind::RawMaterial,requirement.material);
            if(missing<=0 || held<=0) continue;
            candidate.facilityAction=FacilityBuildAction::DeliverMaterial;
            candidate.material=requirement.material;
            candidate.quantity=std::min({missing,held,2});
            candidate.utility=clampCivilization01(
                0.46+0.24*demand.pressure
                +0.10*self.personality.conscientiousness
                +0.06*self.civilization.gatheringSkill
                +0.06*stewardship
                +0.04*preference);
            considerCivilizationDecision(best,candidate);
            return best;
        }

        if(facilityMaterialsComplete(*project)
           && !facilityWorkComplete(*project)
           && hasDiggingStick){
            candidate.facilityAction=FacilityBuildAction::Work;
            candidate.facilityWork=
                1.15+1.45*clampCivilization01(self.civilization.craftingSkill);
            candidate.utility=clampCivilization01(
                0.45+0.22*demand.pressure
                +0.10*stewardship
                +0.08*self.personality.patience
                +0.06*self.personality.conscientiousness
                +0.05*self.civilization.craftingSkill
                +0.04*experience);
            considerCivilizationDecision(best,candidate);
        }
        return best;
    }

    if(!demand.unmet() || demand.committedPlots>=demand.desiredPlots
       || !hasDiggingStick) return best;

    const CultivatedPlotSiteOpportunity site=
        chooseCultivatedPlotSite(world,self.id,authoritativePosition);
    if(!site.available) return best;

    CivilizationUtilityDecision plan;
    plan.intent=CivilizationIntent::Craft;
    plan.technique=TechniqueId::Cultivation;
    plan.facilityKind=FacilityKind::CultivatedPlot;
    plan.facilityAction=FacilityBuildAction::Plan;
    plan.hasFacilityTarget=true;
    plan.facilityTargetPos=site.pos;
    plan.utility=clampCivilization01(
        0.14+0.26*demand.pressure
        +0.10*site.environment.fertility01
        +0.06*site.environment.naturalMoisture01
        +0.28*stewardship
        +0.10*preference
        +0.06*experience);
    considerCivilizationDecision(best,plan);
    return best;
}

inline CivilizationUtilityDecision bestCraftDecisionAtPosition(
    const World& world,
    const Character& self,
    GridPos authoritativePosition,
    const SettlementPopulation* population=nullptr)
{
    CivilizationUtilityDecision best;
    const GridPos sanitationReference=authoritativePosition;

    considerCivilizationDecision(
        best,bestSettlementFoundationDecision(world,self,authoritativePosition,population));
    considerCivilizationDecision(
        best,bestPrimitiveStorageConstructionDecision(
            world,self,authoritativePosition));
    considerCivilizationDecision(
        best,bestPrimitiveFirePitDecision(
            world,self,authoritativePosition));
    considerCivilizationDecision(
        best,bestPrimitiveFurnaceDecision(
            world,self,authoritativePosition));
    considerCivilizationDecision(
        best,bestCultivationDecision(
            world,self,authoritativePosition,population));

    if(self.civilization.knowledge.knowsAtLeast(
        TechniqueId::DesignatedSanitationArea,KnowledgeLevel::Reproducible)
       && canEstablishDesignatedSanitationArea(
           world.seed,self,world.environmentalResidues,
           world.primitiveSanitationSites,world.minute,sanitationReference)){
        CivilizationUtilityDecision sanitation;
        sanitation.intent=CivilizationIntent::Craft;
        sanitation.technique=TechniqueId::DesignatedSanitationArea;
        sanitation.material=MaterialKind::Unknown;
        sanitation.item=ItemKind::RawMaterial;
        sanitation.quantity=0;
        const double preference=civilizationPreference(
            world.seed,self.id,399ULL+static_cast<std::uint64_t>(TechniqueId::DesignatedSanitationArea));
        sanitation.utility=clampCivilization01(
            0.30+0.14*self.civilization.craftingSkill+
            0.12*self.personality.conscientiousness+
            0.08*self.personality.orderliness+
            0.06*preference);
        considerCivilizationDecision(best,sanitation);
    }

    if(canWorkOnDugSanitationPit(self,world.primitiveSanitationSites)){
        const PrimitiveSanitationSite* site=activePrimitiveSanitationSite(
            world.primitiveSanitationSites);
        if(site!=nullptr && site->kind==PrimitiveSanitationSiteKind::DesignatedArea){
            CivilizationUtilityDecision pit;
            pit.intent=CivilizationIntent::Craft;
            pit.technique=TechniqueId::DugSanitationPit;
            pit.material=MaterialKind::Unknown;
            pit.item=ItemKind::RawMaterial;
            pit.quantity=0;
            const double progress=clampCivilization01(
                site->improvementWork/DugSanitationPitWorkRequired);
            const double preference=civilizationPreference(
                world.seed,self.id,399ULL+static_cast<std::uint64_t>(TechniqueId::DugSanitationPit));
            pit.utility=clampCivilization01(
                0.31+0.15*self.civilization.craftingSkill+
                0.11*self.personality.conscientiousness+
                0.10*self.personality.patience+
                0.06*preference+0.10*progress);
            considerCivilizationDecision(best,pit);
        }
    }

    // FireMaking and CopperSmelting are facility-driven once reproducible. They
    // are intentionally omitted here so residents cannot bypass world heat.
    const std::array<TechniqueId,6> techniques={
        TechniqueId::SharpFlake,TechniqueId::ChippedStoneTool,
        TechniqueId::FiberCordage,TechniqueId::SimpleContainer,
        TechniqueId::DiggingStick,TechniqueId::StoneHammer};

    for(const TechniqueId technique:techniques){
        if(!self.civilization.knowledge.knowsAtLeast(technique,KnowledgeLevel::Reproducible)) continue;
        const TechniqueRecipe recipe=techniqueRecipe(technique);
        if(recipe.technique==TechniqueId::None || !hasIngredients(self.civilization.inventory,recipe.inputs)) continue;

        const TechniqueKnowledge* record=civilizationKnowledgeRecord(self.civilization.knowledge,technique);
        const int successfulUses=record ? record->successfulUses : 0;
        double stockNeed=0.0;
        if(technique==TechniqueId::SimpleContainer){
            stockNeed=simpleContainerLogisticsStockPressure(
                world,self,authoritativePosition,population);
        }else if(recipe.producesItem && recipe.outputQuantity>0){
            const int desired=desiredTechniqueOutputStock(technique);
            const int available=worldItemCount(world,self,recipe.outputKind,recipe.outputMaterial,recipe.outputMaterial==MaterialKind::Unknown);
            stockNeed=desired>0 ? clampCivilization01(static_cast<double>(std::max(0,desired-available))/static_cast<double>(desired)) : 0.0;
        }else{
            stockNeed=successfulUses<3 ? 1.0 : 0.0;
        }
        // Durable logistics infrastructure is demand-driven. Once enough
        // reusable containers exist, do not manufacture extras merely to train
        // crafting skill; that turns practice into unbounded settlement clutter.
        const double practiceNeed=
            technique==TechniqueId::SimpleContainer
                ? (stockNeed>0.0
                    ? (successfulUses<3 ? 1.0 : (successfulUses<12 ? 0.35 : 0.0))
                    : 0.0)
                : (successfulUses<3 ? 1.0 : (successfulUses<12 ? 0.35 : 0.0));
        if(stockNeed<=0.0 && practiceNeed<=0.0) continue;

        const double preference=civilizationPreference(world.seed,self.id,300ULL+static_cast<std::uint64_t>(technique));
        const double logisticsBoost=
            technique==TechniqueId::SimpleContainer
                ? 0.30*stockNeed
                : 0.0;
        const double score=clampCivilization01(
            0.08+0.11*self.civilization.craftingSkill+0.08*self.personality.conscientiousness+
            0.05*self.personality.curiosity+0.05*preference+0.15*stockNeed+
            0.07*practiceNeed+logisticsBoost);

        CivilizationUtilityDecision candidate;
        candidate.intent=CivilizationIntent::Craft;
        candidate.utility=score;
        candidate.technique=technique;
        candidate.item=recipe.outputKind;
        candidate.material=recipe.outputMaterial;
        candidate.quantity=recipe.outputQuantity;

        const ConstructedFacility* workSurface=
            operationalSettlementFacilityNear(
                world,FacilityKind::WorkSurface,authoritativePosition,SettlementServiceRadiusGrid);
        if(workSurface!=nullptr){
            candidate.facilityKind=FacilityKind::WorkSurface;
            candidate.facility=workSurface->id;
            candidate.hasFacilityTarget=true;
            candidate.facilityTargetPos=workSurface->pos;
            candidate.utility=clampCivilization01(
                candidate.utility+0.05*facilityEffectiveness01(*workSurface));
        }

        considerCivilizationDecision(best,candidate);
    }
    return best;
}

inline CivilizationUtilityDecision bestCraftDecision(
    const World& world,
    const Character& self)
{
    return bestCraftDecisionAtPosition(
        world,self,civilizationSanitationReferencePosition(world));
}

inline CivilizationUtilityDecision bestRetrieveDecisionAtPosition(
    const World& world,
    const Character& self,
    GridPos authoritativePosition)
{
    CivilizationUtilityDecision best;

    // Provisions remain direct Need-driven logistics.
    const std::array<std::pair<MaterialKind,double>,2> provisions={{
        {MaterialKind::Water,provisionNeedForMaterial(
            self,MaterialKind::Water)},
        {MaterialKind::PlantFood,provisionNeedForMaterial(
            self,MaterialKind::PlantFood)}
    }};

    for(const StorageSite& storage:world.storageSites){
        if(!settlementStorageServesPosition(
            storage,authoritativePosition)) continue;

        for(const auto& provision:provisions){
            const MaterialKind material=provision.first;
            const double need=provision.second;
            const bool cultivationNeed=
                self.civilization.knowledge.knowsAtLeast(
                    TechniqueId::Cultivation,KnowledgeLevel::Reproducible)
                && cultivationInputNeededNear(
                    world,authoritativePosition,material);
            if(need<0.35 && !cultivationNeed) continue;

            const int held=material==MaterialKind::Water
                ? portableWaterCount(self.civilization.inventory)
                : self.civilization.inventory.count(
                    ItemKind::RawMaterial,material);
            if(held>=PortableProvisionCarryTarget) continue;

            const int stored=material==MaterialKind::Water
                ? portableWaterCount(storage.inventory)
                : storage.inventory.count(
                    ItemKind::RawMaterial,material);
            if(stored<=0) continue;

            const int requested=std::min(
                stored,
                std::max(1,PortableProvisionCarryTarget-held));
            const double carryGap=clampCivilization01(
                static_cast<double>(PortableProvisionCarryTarget-held)
                /static_cast<double>(PortableProvisionCarryTarget));
            const double preference=civilizationPreference(
                world.seed,self.id,
                450ULL+static_cast<std::uint64_t>(material));

            CivilizationUtilityDecision candidate;
            candidate.intent=CivilizationIntent::Retrieve;
            candidate.utility=clampCivilization01(
                0.20
                +0.55*need
                +0.12*carryGap
                +0.05*self.personality.orderliness
                +0.03*preference
                +(cultivationNeed ? 0.34 : 0.0));
            candidate.storage=storage.id;
            candidate.item=ItemKind::RawMaterial;
            candidate.material=material;
            candidate.quantity=requested;
            considerCivilizationDecision(best,candidate);
        }

        // Shared construction stock is useful only when this resident can act
        // on the committed project/repair. Count material already carried by
        // the settlement so several workers do not all withdraw the same job's
        // full requirement before anyone reaches the site.
        for(const auto& stack:storage.inventory.stacks()){
            if(stack.kind!=ItemKind::RawMaterial
               || stack.material==MaterialKind::Unknown
               || stack.material==MaterialKind::Water
               || stack.material==MaterialKind::PlantFood
               || stack.quantity<=0){
                continue;
            }

            const MaterialKind material=stack.material;
            const int residentDemand=
                residentCommittedMaterialDemandAtPosition(
                    world,self,material,authoritativePosition);
            if(residentDemand<=0) continue;

            const int held=self.civilization.inventory.count(
                ItemKind::RawMaterial,material);
            // Another resident's inventory is not shared authority. Until that
            // resident physically delivers or stores the material, only this
            // worker's carried stock and the real storage inventory are usable.
            const int neededFromStorage=std::max(
                0,residentDemand-held);
            if(neededFromStorage<=0) continue;

            const int requested=std::min({
                stack.quantity,
                neededFromStorage,
                std::max(1,3-held)
            });
            if(requested<=0) continue;

            const int constructionDemand=
                residentCommittedMaterialDemandAtPosition(
                    world,self,material,authoritativePosition);
            const int repairDemand=
                settlementRepairMaterialDemandNear(
                    world,material,authoritativePosition);
            const double demandPressure=clampCivilization01(
                0.35
                +0.10*static_cast<double>(
                    std::min(5,neededFromStorage)));
            const double preference=civilizationPreference(
                world.seed,self.id,
                760ULL+static_cast<std::uint64_t>(material));

            CivilizationUtilityDecision candidate;
            candidate.intent=CivilizationIntent::Retrieve;
            candidate.utility=clampCivilization01(
                0.48
                +0.30*demandPressure
                +0.08*self.personality.orderliness
                +0.07*self.personality.conscientiousness
                +0.04*self.civilization.gatheringSkill
                +0.03*preference
                +(repairDemand>0 ? 0.08 : 0.0)
                +(constructionDemand>0 ? 0.06 : 0.0));
            candidate.storage=storage.id;
            candidate.item=ItemKind::RawMaterial;
            candidate.material=material;
            candidate.quantity=requested;
            considerCivilizationDecision(best,candidate);
        }
    }
    return best;
}

inline CivilizationUtilityDecision bestRetrieveDecision(
    const World& world,
    const Character& self)
{
    return bestRetrieveDecisionAtPosition(
        world,self,civilizationSanitationReferencePosition(world));
}

inline CivilizationUtilityDecision bestStoreDecisionAtPosition(
    const World& world,
    const Character& self,
    GridPos authoritativePosition)
{
    CivilizationUtilityDecision best;
    const StorageSite* targetStoragePtr=
        nearestSettlementStorage(world,authoritativePosition);
    if(targetStoragePtr==nullptr) return best;

    // Storage is settlement-local infrastructure. Residents choose the nearest
    // stockpile in their lived area instead of teleporting economy policy back
    // to whichever storage happened to be created first.
    const StorageSite& targetStorage=*targetStoragePtr;
    const StorageId storage=targetStorage.id;
    const int total=inventoryUnitCount(self.civilization.inventory);

    for(const auto& stack:self.civilization.inventory.stacks()){
        // Empty vessels are transport capacity, not stockpile cargo. Filled
        // vessels enter storage only through the Water transfer path, which
        // moves the matching container and Water together.
        if(stack.kind==ItemKind::SimpleContainer) continue;

        const bool provision=stack.kind==ItemKind::RawMaterial
            && (stack.material==MaterialKind::Water
                || stack.material==MaterialKind::PlantFood);
        const int effectiveQuantity=
            stack.kind==ItemKind::RawMaterial
            && stack.material==MaterialKind::Water
                ? portableWaterCount(self.civilization.inventory)
                : stack.quantity;
        const int committedCarryNeed=
            stack.kind==ItemKind::RawMaterial
                ? residentCommittedMaterialDemand(
                    world,self,stack.material)
                : 0;
        const int keep=provision
            ? PortableProvisionCarryTarget
            : (stack.kind==ItemKind::RawMaterial
                ? std::max(4,std::min(effectiveQuantity,committedCarryNeed))
                : 1);
        const int surplus=effectiveQuantity-keep;
        if(surplus<=0) continue;

        // Ordinary material stockpiling still waits for meaningful carrying
        // pressure. Provisions are different: once storage exists, a resident
        // may bank surplus Water/Food even with a light total inventory.
        if(total<7 && !provision) continue;

        const int stored=
            stack.kind==ItemKind::RawMaterial
            && stack.material==MaterialKind::Water
                ? portableWaterCount(targetStorage.inventory)
                : targetStorage.inventory.count(stack.kind,stack.material);
        const int reserveTarget=stack.material==MaterialKind::Water
            ? SettlementWaterReserveTarget
            : (stack.material==MaterialKind::PlantFood ? 8 : 0);
        const double reserveNeed=reserveTarget>0
            ? clampCivilization01(
                static_cast<double>(std::max(0,reserveTarget-stored))
                / static_cast<double>(reserveTarget))
            : 0.0;
        const double fullness=clampCivilization01(
            static_cast<double>(std::max(0,total-6))/10.0);
        const double preference=civilizationPreference(
            world.seed,self.id,
            400ULL+static_cast<std::uint64_t>(stack.kind)*32ULL
                +static_cast<std::uint64_t>(stack.material));
        const double score=clampCivilization01(
            0.07
            +0.15*self.personality.orderliness
            +0.09*self.personality.conscientiousness
            +0.13*fullness
            +0.04*preference
            +(provision ? 0.30*reserveNeed : 0.0));

        CivilizationUtilityDecision candidate;
        candidate.intent=CivilizationIntent::Store;
        candidate.utility=score;
        candidate.storage=storage;
        candidate.item=stack.kind;
        candidate.material=stack.material;
        candidate.quantity=std::min(surplus,provision ? 4 : 3);
        considerCivilizationDecision(best,candidate);
    }
    return best;
}

inline CivilizationUtilityDecision bestStoreDecision(
    const World& world,
    const Character& self)
{
    if(world.storageSites.empty()) return CivilizationUtilityDecision{};
    // Compatibility wrapper: legacy callers had no authoritative runtime
    // position and historically targeted the first stockpile. Production
    // autonomous decisions use bestStoreDecisionAtPosition() instead.
    return bestStoreDecisionAtPosition(
        world,self,world.storageSites.front().pos);
}

inline CivilizationUtilityDecision chooseCivilizationUtilityDecisionAtPosition(
    const World& world,
    const Character& self,
    GridPos authoritativePosition,
    const SettlementPopulation* population=nullptr)
{
    CivilizationUtilityDecision best;
    if(self.id==0 || self.civilization.character!=self.id) return best;
    considerCivilizationDecision(
        best,bestExperimentDecisionAtPosition(
            world,self,authoritativePosition,population));
    considerCivilizationDecision(
        best,bestCraftDecisionAtPosition(world,self,authoritativePosition,population));
    considerCivilizationDecision(
        best,bestRetrieveDecisionAtPosition(
            world,self,authoritativePosition));
    considerCivilizationDecision(
        best,bestStoreDecisionAtPosition(
            world,self,authoritativePosition));
    considerCivilizationDecision(
        best,bestResourceExplorationDecisionAtPosition(
            world,self,authoritativePosition));
    considerCivilizationDecision(
        best,bestGatherDecisionAtPosition(
            world,self,authoritativePosition,population));
    return best;
}

inline CivilizationUtilityDecision chooseCivilizationUtilityDecision(
    const World& world,
    const Character& self)
{
    return chooseCivilizationUtilityDecisionAtPosition(
        world,self,civilizationSanitationReferencePosition(world));
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

inline ConstructedFacility* findCivilizationFacility(World& world,FacilityId id)
{
    for(auto& facility:world.facilities) if(facility.id==id) return &facility;
    return nullptr;
}

inline bool civilizationExecutionNearTarget(
    GridPos authoritativePosition,
    GridPos target,
    int maxGridDelta=1)
{
    const int radius=std::max(0,maxGridDelta);
    return authoritativePosition.x>=target.x-radius
        && authoritativePosition.x<=target.x+radius
        && authoritativePosition.y>=target.y-radius
        && authoritativePosition.y<=target.y+radius;
}

inline CivilizationExecutionResult executeCivilizationDecisionAtPosition(
    World& world,
    Character& self,
    const CivilizationUtilityDecision& decision,
    GridPos authoritativePosition,
    const SettlementPopulation* population=nullptr)
{
    CivilizationExecutionResult result;
    const GridPos sanitationReference=authoritativePosition;

    // Spatial civilization work must be paid for with actual travel. The
    // autonomous runtime already walks to these targets before completion, but
    // this execution boundary is also public to external clients and tests.
    // Guard it here as well so a caller can never gather, move stock, or work
    // on a facility remotely by supplying an unrelated authoritative position.
    if(decision.intent==CivilizationIntent::Craft
       && decision.hasFacilityTarget
       && !civilizationExecutionNearTarget(
           authoritativePosition,decision.facilityTargetPos)){
        return result;
    }

    switch(decision.intent){
        case CivilizationIntent::Explore: {
            if(!validNaturalResourceMaterial(decision.material)) return result;
            const ChunkCoord targetChunk=chunkCoordForGrid(authoritativePosition);
            if(world.findGeneratedNaturalChunk(targetChunk)!=nullptr) return result;
            const MacroSurfaceFacts surface=
                deriveMacroSurfaceFacts(world.genesisIdentity(),targetChunk);
            if(surface.surfaceClass==MacroSurfaceClass::Ocean) return result;
            const HydrologyFacts hydrology=
                deriveHydrologyFacts(world.genesisIdentity(),targetChunk);
            if(surfaceWaterGroundContainsGrid(hydrology,authoritativePosition))
                return result;

            world.materializeNaturalChunk(targetChunk);
            result.event.actor=self.id;
            result.event.type=CivilizationEventType::Explored;
            result.event.material=decision.material;
            result.event.item=ItemKind::RawMaterial;
            result.event.quantity=0;
            result.executed=true;
            result.success=true;
            return result;
        }
        case CivilizationIntent::Gather: {
            ResourceNode* node=findCivilizationResource(world,decision.resourceNode);
            GridPos gatherTarget{};
            if(!node
               || !resolveCivilizationResourceAccessGridPosition(
                   world,decision.resourceNode,gatherTarget)
               || !civilizationExecutionNearTarget(
                   authoritativePosition,gatherTarget)) return result;
            result.event=gatherResource(self.civilization,*node,std::max(1,decision.quantity));
            result.executed=result.event.quantity>0;
            result.success=result.executed;
            if(result.executed) self.civilization.gatheringSkill=clampCivilization01(self.civilization.gatheringSkill+0.0015*static_cast<double>(result.event.quantity));
            return result;
        }
        case CivilizationIntent::Store: {
            StorageSite* storage=findCivilizationStorage(world,decision.storage);
            GridPos storageTarget{};
            if(!storage
               || !resolveCivilizationStorageGridPosition(
                   world,decision.storage,storageTarget)
               || !civilizationExecutionNearTarget(
                   authoritativePosition,storageTarget)) return result;
            result.event=storeItems(self.civilization,*storage,decision.item,decision.material,std::max(1,decision.quantity));
            result.executed=result.event.quantity>0;
            result.success=result.executed;
            if(result.executed){
                for(auto& facility:world.facilities){
                    if(facility.linkedStorage==storage->id
                       && facilityOperationalAndActive(facility)){
                        recordFacilityUse(facility,self.id,world.minute);
                        break;
                    }
                }
            }
            return result;
        }
        case CivilizationIntent::Retrieve: {
            StorageSite* storage=findCivilizationStorage(world,decision.storage);
            GridPos storageTarget{};
            if(!storage
               || !resolveCivilizationStorageGridPosition(
                   world,decision.storage,storageTarget)
               || !civilizationExecutionNearTarget(
                   authoritativePosition,storageTarget)) return result;
            result.event=retrieveItems(
                self.civilization,*storage,
                decision.item,decision.material,
                std::max(1,decision.quantity));
            result.executed=result.event.quantity>0;
            result.success=result.executed;
            if(result.executed){
                for(auto& facility:world.facilities){
                    if(facility.linkedStorage==storage->id
                       && facilityOperationalAndActive(facility)){
                        recordFacilityUse(facility,self.id,world.minute);
                        break;
                    }
                }
            }
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
            if(decision.experiment==ExperimentKind::DesignateSanitationArea){
                const PrimitiveSanitationOpportunity opportunity=
                    evaluatePrimitiveSanitationOpportunity(
                        world.seed,self,world.environmentalResidues,world.minute,
                        sanitationReference);
                context.sanitationProblemRecognized=opportunity.problemRecognized;
                context.sanitationSiteAvailable=opportunity.siteAvailable;
            }else if(decision.experiment==ExperimentKind::DigSanitationPit){
                const DugSanitationPitOpportunity opportunity=
                    evaluateDugSanitationPitOpportunity(
                        self,world.environmentalResidues,world.primitiveSanitationSites);
                context.sanitationPitCandidateAvailable=opportunity.candidateAvailable;
            }else if(decision.experiment==ExperimentKind::OrganizeStockpile){
                context.storageProblemRecognized=
                    observePrimitiveStorageNeed(
                        world,self,authoritativePosition,population).recognized;
            }else if(decision.experiment==ExperimentKind::SmeltCopperOre){
                context.smeltingOpportunityAvailable=copperSmeltingOpportunityAvailable(world,self);
            }else if(decision.experiment==ExperimentKind::CultivatePlantFood){
                context.cultivationOpportunityAvailable=
                    cultivationExperimentOpportunityAvailable(
                        world,self,authoritativePosition,population);
            }
            result.experiment=attemptExperiment(context,self.civilization.inventory,self.civilization.knowledge);
            result.executed=result.experiment.attempted;
            result.success=result.experiment.success;
            result.event=result.experiment.event;
            if(result.executed) self.civilization.learningSkill=clampCivilization01(self.civilization.learningSkill+(result.success ? 0.006 : 0.0025));
            return result;
        }
        case CivilizationIntent::Craft: {
            if(isSettlementFoundationFacility(decision.facilityKind)
               && decision.facilityAction!=FacilityBuildAction::None){
                result.facilityKind=decision.facilityKind;
                result.facilityAction=decision.facilityAction;
                result.craft.event.actor=self.id;
                result.craft.event.type=CivilizationEventType::Crafted;
                result.craft.event.technique=TechniqueId::None;

                if(decision.facilityAction==FacilityBuildAction::Plan){
                    if(!decision.hasFacilityTarget) return result;
                    ConstructedFacility* created=establishSettlementFacilityProject(
                        world,self.id,decision.facilityKind,decision.facilityTargetPos,population);
                    if(created==nullptr) return result;
                    result.executed=true;
                    result.success=true;
                    result.facilityId=created->id;
                    result.facilityPos=created->pos;
                    result.craft.success=true;
                    result.event=result.craft.event;
                    return result;
                }

                ConstructedFacility* facility=findCivilizationFacility(
                    world,decision.facility);
                if(facility==nullptr
                   || facility->kind!=decision.facilityKind
                   || !isSettlementFoundationFacility(facility->kind)
                   || !decision.hasFacilityTarget
                   || facility->pos.x!=decision.facilityTargetPos.x
                   || facility->pos.y!=decision.facilityTargetPos.y) return result;

                result.facilityId=facility->id;
                result.facilityPos=facility->pos;

                if(decision.facilityAction==FacilityBuildAction::Repair){
                    if(!facilityOperationalAndActive(*facility)
                       && !settlementFacilityCanRestore(*facility)) return result;
                    const FacilityRepairResult repair=
                        repairSettlementFacility(world,self,facility->id);
                    if(!repair.repaired) return result;
                    result.executed=true;
                    result.success=true;
                    result.craft.success=true;
                    result.event=result.craft.event;
                    result.facilityDurabilityBefore=repair.durabilityBefore;
                    result.facilityDurabilityAfter=repair.durabilityAfter;
                    self.civilization.craftingSkill=clampCivilization01(
                        self.civilization.craftingSkill+0.0035);
                    return result;
                }

                if(facility->state==FacilityState::Operational
                   || facility->state==FacilityState::Ruined) return result;

                if(decision.facilityAction==FacilityBuildAction::DeliverMaterial){
                    const int delivered=deliverFacilityMaterial(
                        *facility,self.civilization.inventory,decision.material,
                        std::max(1,decision.quantity));
                    if(delivered<=0) return result;
                    result.executed=true;
                    result.success=true;
                    result.craft.success=true;
                    result.craft.event.material=decision.material;
                    result.craft.event.quantity=delivered;
                    result.event=result.craft.event;
                    return result;
                }

                if(decision.facilityAction==FacilityBuildAction::Work){
                    const SettlementFacilityWorkResult work=workOnSettlementFacility(
                        world,self,facility->id,std::max(0.1,decision.facilityWork));
                    if(!work.worked || work.facilityId!=facility->id) return result;
                    result.executed=true;
                    result.success=true;
                    result.facilityCompleted=work.completed;
                    result.facilityWorkBefore=work.workBefore;
                    result.facilityWorkAfter=work.workAfter;
                    result.craft.success=true;
                    result.event=result.craft.event;
                    self.civilization.craftingSkill=clampCivilization01(
                        self.civilization.craftingSkill+0.004);
                    return result;
                }

                return result;
            }

            if(decision.technique==TechniqueId::Cultivation
               && decision.facilityKind==FacilityKind::CultivatedPlot){
                result.facilityKind=FacilityKind::CultivatedPlot;
                result.facilityAction=decision.facilityAction;
                result.craft.event.actor=self.id;
                result.craft.event.type=CivilizationEventType::Crafted;
                result.craft.event.technique=TechniqueId::Cultivation;

                if(decision.facilityAction==FacilityBuildAction::Plan){
                    if(!decision.hasFacilityTarget) return result;
                    ConstructedFacility* created=establishCultivatedPlotProject(
                        world,self.id,decision.facilityTargetPos);
                    if(created==nullptr) return result;
                    result.executed=true;
                    result.success=true;
                    result.facilityId=created->id;
                    result.facilityPos=created->pos;
                    result.craft.success=true;
                    result.event=result.craft.event;
                    return result;
                }

                ConstructedFacility* facility=
                    findCivilizationFacility(world,decision.facility);
                if(facility==nullptr
                   || facility->kind!=FacilityKind::CultivatedPlot
                   || !decision.hasFacilityTarget
                   || facility->pos.x!=decision.facilityTargetPos.x
                   || facility->pos.y!=decision.facilityTargetPos.y){
                    return result;
                }

                result.facilityId=facility->id;
                result.facilityPos=facility->pos;

                if(decision.facilityAction==FacilityBuildAction::DeliverMaterial){
                    if(facility->state==FacilityState::Operational) return result;
                    const int delivered=deliverFacilityMaterial(
                        *facility,self.civilization.inventory,
                        decision.material,std::max(1,decision.quantity));
                    if(delivered<=0) return result;
                    result.executed=true;
                    result.success=true;
                    result.craft.success=true;
                    result.craft.event.material=decision.material;
                    result.craft.event.quantity=delivered;
                    result.event=result.craft.event;
                    return result;
                }

                if(decision.facilityAction==FacilityBuildAction::Work){
                    if(facility->state==FacilityState::Operational) return result;
                    const CultivatedPlotWorkResult work=workOnCultivatedPlot(
                        world,self,facility->id,
                        std::max(0.1,decision.facilityWork));
                    if(!work.worked || work.facilityId!=facility->id) return result;
                    result.executed=true;
                    result.success=true;
                    result.facilityCompleted=work.completed;
                    result.facilityWorkBefore=work.workBefore;
                    result.facilityWorkAfter=work.workAfter;
                    result.craft.success=true;
                    result.event=result.craft.event;
                    self.civilization.craftingSkill=clampCivilization01(
                        self.civilization.craftingSkill+0.003);
                    return result;
                }

                if(!facilityOperationalAndActive(*facility)) return result;

                if(decision.facilityAction==FacilityBuildAction::Plant){
                    if(!plantCultivatedPlot(world,self,*facility)) return result;
                    result.executed=true;
                    result.success=true;
                    result.craft.success=true;
                    result.craft.event.material=MaterialKind::PlantFood;
                    result.craft.event.quantity=1;
                    result.event=result.craft.event;
                    self.civilization.knowledge.recordSuccessfulUse(
                        TechniqueId::Cultivation);
                    return result;
                }

                if(decision.facilityAction==FacilityBuildAction::Water){
                    if(!waterCultivatedPlot(world,self,*facility)) return result;
                    result.executed=true;
                    result.success=true;
                    result.craft.success=true;
                    result.craft.event.material=MaterialKind::Water;
                    result.craft.event.quantity=1;
                    result.event=result.craft.event;
                    self.civilization.knowledge.recordSuccessfulUse(
                        TechniqueId::Cultivation);
                    return result;
                }

                if(decision.facilityAction==FacilityBuildAction::Tend){
                    if(!tendCultivatedPlot(world,self,*facility)) return result;
                    result.executed=true;
                    result.success=true;
                    result.craft.success=true;
                    result.event=result.craft.event;
                    self.civilization.knowledge.recordSuccessfulUse(
                        TechniqueId::Cultivation);
                    return result;
                }

                if(decision.facilityAction==FacilityBuildAction::Harvest){
                    const int harvested=harvestCultivatedPlot(
                        world,self,*facility);
                    if(harvested<=0) return result;
                    result.executed=true;
                    result.success=true;
                    result.craft.success=true;
                    result.craft.event.material=MaterialKind::PlantFood;
                    result.craft.event.quantity=harvested;
                    result.event=result.craft.event;
                    self.civilization.knowledge.recordSuccessfulUse(
                        TechniqueId::Cultivation);
                    self.civilization.gatheringSkill=clampCivilization01(
                        self.civilization.gatheringSkill+0.006);
                    return result;
                }
                return result;
            }

            if(decision.technique==TechniqueId::PrimitiveStorage){
                result.facilityKind=FacilityKind::PrimitiveStorage;
                result.facilityAction=decision.facilityAction;
                result.craft.event.actor=self.id;
                result.craft.event.type=CivilizationEventType::Crafted;
                result.craft.event.technique=TechniqueId::PrimitiveStorage;

                if(decision.facilityAction==FacilityBuildAction::Plan){
                    if(!decision.hasFacilityTarget) return result;
                    ConstructedFacility* created=establishPrimitiveStorageProject(
                        world,self.id,decision.facilityTargetPos);
                    if(created==nullptr) return result;
                    result.executed=true;
                    result.success=true;
                    result.facilityId=created->id;
                    result.facilityPos=created->pos;
                    result.craft.success=true;
                    result.event=result.craft.event;
                    return result;
                }

                ConstructedFacility* facility=findCivilizationFacility(world,decision.facility);
                if(facility==nullptr || facility->kind!=FacilityKind::PrimitiveStorage
                   || facility->state==FacilityState::Operational
                   || !decision.hasFacilityTarget
                   || facility->pos.x!=decision.facilityTargetPos.x
                   || facility->pos.y!=decision.facilityTargetPos.y) return result;

                result.facilityId=facility->id;
                result.facilityPos=facility->pos;
                if(decision.facilityAction==FacilityBuildAction::DeliverMaterial){
                    const int delivered=deliverFacilityMaterial(
                        *facility,self.civilization.inventory,decision.material,
                        std::max(1,decision.quantity));
                    if(delivered<=0) return result;
                    result.executed=true;
                    result.success=true;
                    result.craft.success=true;
                    result.craft.event.material=decision.material;
                    result.craft.event.quantity=delivered;
                    result.event=result.craft.event;
                    return result;
                }

                if(decision.facilityAction==FacilityBuildAction::Work){
                    const PrimitiveStorageWorkResult work=workOnPrimitiveStorage(
                        world,self,std::max(0.1,decision.facilityWork));
                    if(!work.worked || work.facilityId!=facility->id) return result;
                    result.executed=true;
                    result.success=true;
                    result.facilityCompleted=work.completed;
                    result.activatedStorage=work.storageId;
                    result.facilityWorkBefore=work.workBefore;
                    result.facilityWorkAfter=work.workAfter;
                    result.craft.success=true;
                    result.event=result.craft.event;
                    if(work.completed){
                        self.civilization.knowledge.recordSuccessfulUse(TechniqueId::PrimitiveStorage);
                    }
                    self.civilization.craftingSkill=clampCivilization01(
                        self.civilization.craftingSkill+0.004);
                    return result;
                }
                return result;
            }

            if(decision.technique==TechniqueId::FireMaking
               && decision.facilityKind==FacilityKind::FirePit){
                result.facilityKind=FacilityKind::FirePit;
                result.facilityAction=decision.facilityAction;
                result.craft.event.actor=self.id;
                result.craft.event.type=CivilizationEventType::Crafted;
                result.craft.event.technique=TechniqueId::FireMaking;

                if(decision.facilityAction==FacilityBuildAction::Plan){
                    if(!decision.hasFacilityTarget) return result;
                    ConstructedFacility* created=establishPrimitiveFirePitProject(
                        world,self,decision.facilityTargetPos);
                    if(created==nullptr) return result;
                    result.executed=true;
                    result.success=true;
                    result.facilityId=created->id;
                    result.facilityPos=created->pos;
                    result.craft.success=true;
                    result.event=result.craft.event;
                    return result;
                }

                ConstructedFacility* facility=findCivilizationFacility(world,decision.facility);
                if(facility==nullptr || facility->kind!=FacilityKind::FirePit
                   || !decision.hasFacilityTarget
                   || facility->pos.x!=decision.facilityTargetPos.x
                   || facility->pos.y!=decision.facilityTargetPos.y) return result;

                result.facilityId=facility->id;
                result.facilityPos=facility->pos;
                if(decision.facilityAction==FacilityBuildAction::DeliverMaterial){
                    if(facility->state==FacilityState::Operational) return result;
                    const int delivered=deliverFacilityMaterial(
                        *facility,self.civilization.inventory,decision.material,
                        std::max(1,decision.quantity));
                    if(delivered<=0) return result;
                    result.executed=true;
                    result.success=true;
                    result.craft.success=true;
                    result.craft.event.material=decision.material;
                    result.craft.event.quantity=delivered;
                    result.event=result.craft.event;
                    return result;
                }

                if(decision.facilityAction==FacilityBuildAction::Work){
                    if(facility->state==FacilityState::Operational) return result;
                    const PrimitiveFirePitWorkResult work=workOnPrimitiveFirePit(
                        world,self,std::max(0.1,decision.facilityWork));
                    if(!work.worked || work.facilityId!=facility->id) return result;
                    result.executed=true;
                    result.success=true;
                    result.facilityCompleted=work.completed;
                    result.facilityWorkBefore=work.workBefore;
                    result.facilityWorkAfter=work.workAfter;
                    result.craft.success=true;
                    result.event=result.craft.event;
                    if(work.completed){
                        self.civilization.knowledge.recordSuccessfulUse(TechniqueId::FireMaking);
                    }
                    self.civilization.craftingSkill=clampCivilization01(
                        self.civilization.craftingSkill+0.004);
                    return result;
                }

                if(facility->state!=FacilityState::Operational || !facility->active) return result;
                if(decision.facilityAction==FacilityBuildAction::Fuel){
                    const int added=fuelPrimitiveFirePit(
                        world,self,facility->id,std::max(1,decision.quantity));
                    if(added<=0) return result;
                    result.executed=true;
                    result.success=true;
                    result.craft.success=true;
                    result.craft.event.material=MaterialKind::Wood;
                    result.craft.event.quantity=added;
                    result.event=result.craft.event;
                }else if(decision.facilityAction==FacilityBuildAction::Ignite){
                    if(!ignitePrimitiveFirePit(world,self,facility->id)) return result;
                    result.executed=true;
                    result.success=true;
                    result.craft.success=true;
                    result.event=result.craft.event;
                    self.civilization.knowledge.recordSuccessfulUse(TechniqueId::FireMaking);
                }else if(decision.facilityAction==FacilityBuildAction::CollectCharcoal){
                    const int collected=collectPrimitiveFirePitCharcoal(
                        world,self,facility->id,std::max(1,decision.quantity));
                    if(collected<=0) return result;
                    result.executed=true;
                    result.success=true;
                    result.craft.success=true;
                    result.craft.event.material=MaterialKind::Charcoal;
                    result.craft.event.item=ItemKind::RawMaterial;
                    result.craft.event.quantity=collected;
                    result.event=result.craft.event;
                }else{
                    return result;
                }

                result.facilityFuelUnits=facility->fuelUnits;
                result.facilityCharcoalUnits=facility->charcoalUnits;
                result.facilityHeatLevel=facility->heatLevel;
                result.facilityLit=facility->lit;
                if(result.success){
                    self.civilization.craftingSkill=clampCivilization01(
                        self.civilization.craftingSkill+0.0025);
                }
                return result;
            }

            if(decision.facilityKind==FacilityKind::Furnace
               && (decision.technique==TechniqueId::FireMaking
                   || decision.technique==TechniqueId::CopperSmelting)){
                result.facilityKind=FacilityKind::Furnace;
                result.facilityAction=decision.facilityAction;
                result.craft.event.actor=self.id;
                result.craft.event.type=CivilizationEventType::Crafted;
                result.craft.event.technique=decision.technique;

                if(decision.facilityAction==FacilityBuildAction::Plan){
                    if(!decision.hasFacilityTarget) return result;
                    ConstructedFacility* created=establishPrimitiveFurnaceProject(
                        world,self,decision.facilityTargetPos);
                    if(created==nullptr) return result;
                    result.executed=true;
                    result.success=true;
                    result.facilityId=created->id;
                    result.facilityPos=created->pos;
                    result.craft.success=true;
                    result.event=result.craft.event;
                    return result;
                }

                ConstructedFacility* facility=findCivilizationFacility(world,decision.facility);
                if(facility==nullptr || facility->kind!=FacilityKind::Furnace
                   || !decision.hasFacilityTarget
                   || facility->pos.x!=decision.facilityTargetPos.x
                   || facility->pos.y!=decision.facilityTargetPos.y) return result;
                result.facilityId=facility->id;
                result.facilityPos=facility->pos;

                if(decision.facilityAction==FacilityBuildAction::DeliverMaterial){
                    if(facility->state==FacilityState::Operational) return result;
                    const int delivered=deliverFacilityMaterial(
                        *facility,self.civilization.inventory,decision.material,
                        std::max(1,decision.quantity));
                    if(delivered<=0) return result;
                    result.executed=true;
                    result.success=true;
                    result.craft.success=true;
                    result.craft.event.material=decision.material;
                    result.craft.event.quantity=delivered;
                    result.event=result.craft.event;
                    return result;
                }

                if(decision.facilityAction==FacilityBuildAction::Work){
                    if(facility->state==FacilityState::Operational) return result;
                    const PrimitiveFurnaceWorkResult work=workOnPrimitiveFurnace(
                        world,self,std::max(0.1,decision.facilityWork));
                    if(!work.worked || work.facilityId!=facility->id) return result;
                    result.executed=true;
                    result.success=true;
                    result.facilityCompleted=work.completed;
                    result.facilityWorkBefore=work.workBefore;
                    result.facilityWorkAfter=work.workAfter;
                    result.craft.success=true;
                    result.event=result.craft.event;
                    self.civilization.craftingSkill=clampCivilization01(
                        self.civilization.craftingSkill+0.004);
                    return result;
                }

                if(facility->state!=FacilityState::Operational || !facility->active
                   || !self.civilization.knowledge.knowsAtLeast(
                       TechniqueId::CopperSmelting,KnowledgeLevel::Reproducible)) return result;

                if(decision.facilityAction==FacilityBuildAction::LoadSmeltCharge){
                    const int loaded=loadPrimitiveFurnaceCopperCharge(
                        world,self,facility->id,std::max(1,decision.quantity));
                    if(loaded<=0) return result;
                    result.executed=true;
                    result.success=true;
                    result.craft.success=true;
                    result.craft.event.material=MaterialKind::CopperOre;
                    result.craft.event.quantity=loaded;
                    result.event=result.craft.event;
                }else if(decision.facilityAction==FacilityBuildAction::Ignite){
                    if(!ignitePrimitiveFurnace(world,self,facility->id)) return result;
                    result.executed=true;
                    result.success=true;
                    result.craft.success=true;
                    result.event=result.craft.event;
                    self.civilization.knowledge.recordSuccessfulUse(TechniqueId::CopperSmelting);
                }else if(decision.facilityAction==FacilityBuildAction::CollectMetal){
                    const int collected=collectPrimitiveFurnaceCopper(
                        world,self,facility->id,std::max(1,decision.quantity));
                    if(collected<=0) return result;
                    result.executed=true;
                    result.success=true;
                    result.craft.success=true;
                    result.craft.event.material=MaterialKind::CopperMetal;
                    result.craft.event.item=ItemKind::RawMaterial;
                    result.craft.event.quantity=collected;
                    result.event=result.craft.event;
                }else{
                    return result;
                }

                result.facilityFuelUnits=facility->fuelUnits;
                result.facilityOreUnits=facility->oreUnits;
                result.facilityMetalUnits=facility->metalUnits;
                result.facilityHeatLevel=facility->heatLevel;
                result.facilityLit=facility->lit;
                if(result.success){
                    self.civilization.craftingSkill=clampCivilization01(
                        self.civilization.craftingSkill+0.003);
                }
                return result;
            }

            if(decision.technique==TechniqueId::DesignatedSanitationArea){
                const PrimitiveSanitationSiteCreationResult site=establishDesignatedSanitationArea(
                    world.seed,self,world.environmentalResidues,world.primitiveSanitationSites,
                    world.minute,sanitationReference);
                if(!site.established) return result;
                result.executed=true;
                result.success=true;
                result.sanitationSiteId=site.siteId;
                result.sanitationSitePos=site.pos;
                result.craft.success=true;
                result.craft.event.actor=self.id;
                result.craft.event.type=CivilizationEventType::Crafted;
                result.craft.event.technique=TechniqueId::DesignatedSanitationArea;
                result.event=result.craft.event;
                self.civilization.knowledge.recordSuccessfulUse(TechniqueId::DesignatedSanitationArea);
                self.civilization.craftingSkill=clampCivilization01(self.civilization.craftingSkill+0.004);
                return result;
            }
            if(decision.technique==TechniqueId::DugSanitationPit){
                const DugSanitationPitWorkResult work=workOnDugSanitationPit(
                    self,world.environmentalResidues,world.primitiveSanitationSites,world.minute);
                if(!work.worked) return result;
                result.executed=true;
                result.success=true;
                result.sanitationSiteId=work.siteId;
                result.sanitationSitePos=work.pos;
                result.sanitationImprovementCompleted=work.completed;
                result.sanitationWorkBefore=work.workBefore;
                result.sanitationWorkAfter=work.workAfter;
                result.craft.success=true;
                result.craft.event.actor=self.id;
                result.craft.event.type=CivilizationEventType::Crafted;
                result.craft.event.technique=TechniqueId::DugSanitationPit;
                result.event=result.craft.event;
                self.civilization.knowledge.recordSuccessfulUse(TechniqueId::DugSanitationPit);
                self.civilization.craftingSkill=clampCivilization01(self.civilization.craftingSkill+0.004);
                return result;
            }
            ConstructedFacility* workSurface=nullptr;
            double effectiveCraftingSkill=self.civilization.craftingSkill;
            if(decision.facilityKind==FacilityKind::WorkSurface
               && decision.facility!=0
               && decision.hasFacilityTarget){
                workSurface=findCivilizationFacility(world,decision.facility);
                if(workSurface==nullptr
                   || workSurface->kind!=FacilityKind::WorkSurface
                   || !facilityOperationalAndActive(*workSurface)
                   || workSurface->pos.x!=decision.facilityTargetPos.x
                   || workSurface->pos.y!=decision.facilityTargetPos.y) return result;
                effectiveCraftingSkill=clampCivilization01(
                    effectiveCraftingSkill+
                    settlementWorkSurfaceSkillBonus(*workSurface));
            }

            result.craft=reproduceTechnique(
                self.id,
                decision.technique,
                self.civilization.inventory,
                self.civilization.knowledge,
                effectiveCraftingSkill);
            result.executed=result.craft.success;
            result.success=result.craft.success;
            result.event=result.craft.event;
            if(result.success){
                if(workSurface!=nullptr){
                    result.facilityId=workSurface->id;
                    result.facilityKind=workSurface->kind;
                    result.facilityPos=workSurface->pos;
                    result.facilityDurabilityBefore=workSurface->durability;
                    recordFacilityUse(*workSurface,self.id,world.minute);
                    applyFacilityWear(
                        *workSurface,
                        facilityWearPerUse(workSurface->kind));
                    result.facilityDurabilityAfter=workSurface->durability;
                }
                self.civilization.craftingSkill=clampCivilization01(
                    self.civilization.craftingSkill+0.004);
            }
            return result;
        }
        case CivilizationIntent::None:
        default:
            return result;
    }
}

inline CivilizationExecutionResult executeCivilizationDecision(
    World& world,
    Character& self,
    const CivilizationUtilityDecision& decision)
{
    // Preserve the non-spatial convenience helper for focused unit tests, but
    // resolve the same authoritative target that the autonomous context runtime
    // would walk to before invoking the position-aware execution boundary.
    GridPos resolvedPosition=civilizationSanitationReferencePosition(world);
    if(decision.intent==CivilizationIntent::Gather){
        GridPos target{};
        if(resolveCivilizationResourceAccessGridPosition(
                world,decision.resourceNode,target)){
            resolvedPosition=target;
        }
    }else if(decision.intent==CivilizationIntent::Store
             || decision.intent==CivilizationIntent::Retrieve){
        GridPos target{};
        if(resolveCivilizationStorageGridPosition(
                world,decision.storage,target)){
            resolvedPosition=target;
        }
    }else if(decision.intent==CivilizationIntent::Craft
             && decision.hasFacilityTarget){
        resolvedPosition=decision.facilityTargetPos;
    }

    return executeCivilizationDecisionAtPosition(
        world,self,decision,resolvedPosition);
}

inline void regenerateCivilizationEnvironment(World& world)
{
    for(auto& node:world.resourceNodes) node.regenerateDay();
}

} // namespace lifelens
