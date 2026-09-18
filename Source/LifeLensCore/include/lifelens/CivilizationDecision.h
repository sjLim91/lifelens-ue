#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>

#include "Civilization.h"
#include "Character.h"
#include "Facility.h"
#include "PrimitiveFireProgression.h"
#include "PrimitiveSanitation.h"
#include "PrimitiveSmeltingProgression.h"
#include "PrimitiveStorageProgression.h"
#include "SettlementProgression.h"
#include "World.h"

namespace lifelens {

enum class CivilizationIntent {
    None,
    Gather,
    Store,
    Experiment,
    Craft
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
    CollectMetal
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
        case ExperimentKind::ShapeDiggingStick: return MaterialKind::Wood;
        case ExperimentKind::HaftStoneHammer: return MaterialKind::Stone;
        case ExperimentKind::SmeltCopperOre: return MaterialKind::CopperOre;
        case ExperimentKind::DesignateSanitationArea:
        case ExperimentKind::DigSanitationPit:
        case ExperimentKind::OrganizeStockpile:
            return MaterialKind::Unknown;
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
            return 0.25+0.55*clampCivilization01(self.needs.hunger);
        case MaterialKind::Water:
            return 0.25+0.55*clampCivilization01(self.needs.thirst);
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

inline CivilizationUtilityDecision bestGatherDecision(const World& world,const Character& self)
{
    CivilizationUtilityDecision best;
    for(const auto& node:world.resourceNodes){
        if(node.id==0 || node.quantity<=0 || node.material==MaterialKind::Unknown) continue;
        const int held=self.civilization.inventory.count(ItemKind::RawMaterial,node.material);
        const int stored=storageCountForMaterial(world,node.material);
        const int storageMissing=primitiveStorageMissingMaterial(world,node.material);
        const int fireMissing=primitiveFirePitMissingMaterial(world,node.material);
        const int furnaceMissing=primitiveFurnaceMissingMaterial(world,node.material);
        const int settlementMissing=settlementConstructionMissingMaterial(world,node.material);
        const int repairMissing=settlementRepairMaterialDemand(world,node.material);
        const int constructionMissing=std::max(
            settlementMissing,
            std::max(storageMissing,std::max(fireMissing,furnaceMissing)));
        const int materialDemand=constructionMissing+repairMissing;
        const int baseTarget=(node.material==MaterialKind::Water || node.material==MaterialKind::PlantFood) ? 4 : 5;
        const int fireFuelReserve=(hasOperationalFirePit(world) && node.material==MaterialKind::Wood) ? 3 : 0;
        const int target=baseTarget+std::min(4,materialDemand)+fireFuelReserve;
        const double gap=clampCivilization01(static_cast<double>(std::max(0,target-held-std::min(stored,2)))/static_cast<double>(std::max(1,target)));
        const double demand=materialProgressDemand(self,node.material);
        const double constructionDemand=constructionMissing>0
            ? clampCivilization01(0.45+0.12*static_cast<double>(constructionMissing))
            : 0.0;
        const double maintenanceDemand=repairMissing>0
            ? clampCivilization01(0.42+0.18*static_cast<double>(repairMissing))
            : 0.0;
        const double preference=civilizationPreference(world.seed,self.id,100ULL+static_cast<std::uint64_t>(node.material));
        const double score=clampCivilization01(
            0.07+0.12*self.personality.curiosity+0.05*self.personality.adaptability+
            0.08*self.civilization.gatheringSkill+0.16*demand+0.13*gap+
            0.30*constructionDemand+0.24*maintenanceDemand+0.07*preference);
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
    const GridPos sanitationReference=civilizationSanitationReferencePosition(world);
    const PrimitiveSanitationOpportunity sanitationOpportunity=
        evaluatePrimitiveSanitationOpportunity(
            world.seed,self,world.environmentalResidues,world.minute,sanitationReference);
    const DugSanitationPitOpportunity pitOpportunity=
        evaluateDugSanitationPitOpportunity(
            self,world.environmentalResidues,world.primitiveSanitationSites);
    const PrimitiveStorageNeedObservation storageNeed=observePrimitiveStorageNeed(world,self);
    const bool smeltingOpportunity=copperSmeltingOpportunityAvailable(world,self);
    const std::array<ExperimentKind,11> experiments={
        ExperimentKind::StrikeStone,ExperimentKind::HaftSharpFlake,ExperimentKind::FrictionWood,
        ExperimentKind::TwistFiber,ExperimentKind::ShapeClay,
        ExperimentKind::ShapeDiggingStick,ExperimentKind::HaftStoneHammer,
        ExperimentKind::DesignateSanitationArea,ExperimentKind::DigSanitationPit,
        ExperimentKind::OrganizeStockpile,ExperimentKind::SmeltCopperOre};

    for(const ExperimentKind kind:experiments){
        const TechniqueId technique=experimentTechnique(kind);
        if(technique==TechniqueId::None) continue;
        if(self.civilization.knowledge.knowsAtLeast(technique,KnowledgeLevel::Reproducible)) continue;

        const bool designatedExperiment=kind==ExperimentKind::DesignateSanitationArea;
        const bool pitExperiment=kind==ExperimentKind::DigSanitationPit;
        const bool storageExperiment=kind==ExperimentKind::OrganizeStockpile;
        const bool smeltingExperiment=kind==ExperimentKind::SmeltCopperOre;
        if(designatedExperiment &&
           (!sanitationOpportunity.problemRecognized || !sanitationOpportunity.siteAvailable)) continue;
        if(pitExperiment && !pitOpportunity.candidateAvailable) continue;
        if(storageExperiment && !storageNeed.recognized) continue;
        if(smeltingExperiment && !smeltingOpportunity) continue;

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
        }
        const double score=clampCivilization01(
            0.11+0.22*self.personality.curiosity+0.10*self.personality.openness+
            0.07*self.personality.patience+0.12*self.civilization.learningSkill+
            0.08*preference+hypothesisBoost+sanitationBoost+storageBoost+smeltingBoost);

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
        case TechniqueId::DiggingStick: return 1;
        case TechniqueId::StoneHammer: return 1;
        case TechniqueId::FireMaking:
        case TechniqueId::CopperSmelting:
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
    FacilityKind kind)
{
    if(!isSettlementFoundationFacility(kind)) return 0.0;

    if(kind==FacilityKind::SleepingPlace){
        if(hasOperationalSettlementFacility(world,FacilityKind::SleepingPlace)
           || hasOperationalSettlementFacility(world,FacilityKind::Shelter)) return 0.0;
        return clampCivilization01(
            (clampCivilization01(self.needs.sleep)-0.22)/0.48);
    }

    if(kind==FacilityKind::Shelter){
        if(hasOperationalSettlementFacility(world,FacilityKind::Shelter)) return 0.0;
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
            clampCivilization01(self.needs.hygiene)
        });
        return clampCivilization01(
            weatherPressure*(0.08+0.92*accumulatedBurden));
    }

    if(kind==FacilityKind::WorkSurface){
        if(hasOperationalSettlementFacility(world,FacilityKind::WorkSurface)) return 0.0;
        return settlementCraftDemandPressure(self);
    }

    return 0.0;
}

inline CivilizationUtilityDecision bestSettlementFoundationDecision(
    const World& world,
    const Character& self,
    GridPos authoritativePosition)
{
    CivilizationUtilityDecision best;
    constexpr std::array<FacilityKind,3> kinds={
        FacilityKind::SleepingPlace,
        FacilityKind::Shelter,
        FacilityKind::WorkSurface
    };

    for(const FacilityKind kind:kinds){
        const ConstructedFacility* project=settlementFacilityProject(world,kind);
        const double preference=civilizationPreference(
            world.seed,self.id,610ULL+static_cast<std::uint64_t>(kind));

        if(project!=nullptr && facilityOperationalAndActive(*project)){
            if(!settlementFacilityNeedsMaintenance(*project)) continue;

            const MaterialKind repairMaterial=facilityRepairMaterial(kind);
            const int held=self.civilization.inventory.count(
                ItemKind::RawMaterial,repairMaterial);
            if(held<=0) continue;

            CivilizationUtilityDecision repair;
            repair.intent=CivilizationIntent::Craft;
            repair.facilityKind=kind;
            repair.facilityAction=FacilityBuildAction::Repair;
            repair.facility=project->id;
            repair.hasFacilityTarget=true;
            repair.facilityTargetPos=project->pos;
            repair.material=repairMaterial;
            repair.item=ItemKind::RawMaterial;
            repair.quantity=1;

            const double damage=clampCivilization01(1.0-project->durability);
            repair.utility=clampCivilization01(
                0.46+0.34*damage
                +0.09*self.personality.conscientiousness
                +0.07*self.personality.orderliness
                +0.07*self.civilization.craftingSkill
                +0.04*preference);
            considerCivilizationDecision(best,repair);
            continue;
        }

        if(hasOperationalSettlementFacility(world,kind)) continue;

        double pressure=settlementFacilityNeedPressure(
            world,self,authoritativePosition,kind);
        if(project==nullptr && pressure<0.24) continue;
        if(project!=nullptr) pressure=std::max(pressure,0.46);

        CivilizationUtilityDecision candidate;
        candidate.intent=CivilizationIntent::Craft;
        candidate.facilityKind=kind;
        candidate.item=ItemKind::RawMaterial;
        candidate.technique=TechniqueId::None;

        if(project==nullptr){
            const SettlementFacilitySiteOpportunity site=
                chooseSettlementFacilitySite(world,self.id,kind);
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
    const Character& self)
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
        const PrimitiveStorageSiteOpportunity site=choosePrimitiveStorageSite(world,self.id);
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
    const Character& self)
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
        const PrimitiveFirePitSiteOpportunity site=choosePrimitiveFirePitSite(world,self.id);
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
    const Character& self)
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
        const PrimitiveFurnaceSiteOpportunity site=choosePrimitiveFurnaceSite(world,self.id);
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

inline CivilizationUtilityDecision bestCraftDecisionAtPosition(
    const World& world,
    const Character& self,
    GridPos authoritativePosition)
{
    CivilizationUtilityDecision best;
    const GridPos sanitationReference=civilizationSanitationReferencePosition(world);

    considerCivilizationDecision(
        best,bestSettlementFoundationDecision(world,self,authoritativePosition));
    considerCivilizationDecision(best,bestPrimitiveStorageConstructionDecision(world,self));
    considerCivilizationDecision(best,bestPrimitiveFirePitDecision(world,self));
    considerCivilizationDecision(best,bestPrimitiveFurnaceDecision(world,self));

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

        const ConstructedFacility* workSurface=
            operationalSettlementFacility(world,FacilityKind::WorkSurface);
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

inline CivilizationUtilityDecision chooseCivilizationUtilityDecisionAtPosition(
    const World& world,
    const Character& self,
    GridPos authoritativePosition)
{
    CivilizationUtilityDecision best;
    if(self.id==0 || self.civilization.character!=self.id) return best;
    considerCivilizationDecision(best,bestExperimentDecision(world,self));
    considerCivilizationDecision(
        best,bestCraftDecisionAtPosition(world,self,authoritativePosition));
    considerCivilizationDecision(best,bestStoreDecision(world,self));
    considerCivilizationDecision(best,bestGatherDecision(world,self));
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

inline CivilizationExecutionResult executeCivilizationDecision(World& world,Character& self,const CivilizationUtilityDecision& decision)
{
    CivilizationExecutionResult result;
    const GridPos sanitationReference=civilizationSanitationReferencePosition(world);
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
                context.storageProblemRecognized=observePrimitiveStorageNeed(world,self).recognized;
            }else if(decision.experiment==ExperimentKind::SmeltCopperOre){
                context.smeltingOpportunityAvailable=copperSmeltingOpportunityAvailable(world,self);
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
                        world,self.id,decision.facilityKind,decision.facilityTargetPos);
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
                    if(!facilityOperationalAndActive(*facility)) return result;
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

inline void regenerateCivilizationEnvironment(World& world)
{
    for(auto& node:world.resourceNodes) node.regenerateDay();
}

} // namespace lifelens
