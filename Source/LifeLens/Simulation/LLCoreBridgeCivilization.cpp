#include "Simulation/LLCoreBridgeSubsystem.h"

#include "lifelens/CivilizationObserverReadModel.h"
#include "lifelens/Simulation.h"

namespace
{
int32 SafeCivilizationCount(std::size_t Count)
{
    return Count > static_cast<std::size_t>(MAX_int32)
        ? MAX_int32
        : static_cast<int32>(Count);
}

FString StableFactId(lifelens::SocialFactId FactId)
{
    return FString::Printf(TEXT("%llu"), static_cast<unsigned long long>(FactId));
}

ELLCoreMaterialKind ToUnrealMaterial(lifelens::MaterialKind Material)
{
    switch (Material)
    {
        case lifelens::MaterialKind::Stone: return ELLCoreMaterialKind::Stone;
        case lifelens::MaterialKind::Flint: return ELLCoreMaterialKind::Flint;
        case lifelens::MaterialKind::Wood: return ELLCoreMaterialKind::Wood;
        case lifelens::MaterialKind::Fiber: return ELLCoreMaterialKind::Fiber;
        case lifelens::MaterialKind::Clay: return ELLCoreMaterialKind::Clay;
        case lifelens::MaterialKind::Water: return ELLCoreMaterialKind::Water;
        case lifelens::MaterialKind::PlantFood: return ELLCoreMaterialKind::PlantFood;
        case lifelens::MaterialKind::Bone: return ELLCoreMaterialKind::Bone;
        case lifelens::MaterialKind::Hide: return ELLCoreMaterialKind::Hide;
        case lifelens::MaterialKind::CopperOre: return ELLCoreMaterialKind::CopperOre;
        case lifelens::MaterialKind::TinOre: return ELLCoreMaterialKind::TinOre;
        case lifelens::MaterialKind::IronOre: return ELLCoreMaterialKind::IronOre;
        case lifelens::MaterialKind::Charcoal: return ELLCoreMaterialKind::Charcoal;
        case lifelens::MaterialKind::CopperMetal: return ELLCoreMaterialKind::CopperMetal;
        case lifelens::MaterialKind::Unknown:
        default:
            return ELLCoreMaterialKind::Unknown;
    }
}

ELLCoreItemKind ToUnrealItem(lifelens::ItemKind Item)
{
    switch (Item)
    {
        case lifelens::ItemKind::SharpFlake: return ELLCoreItemKind::SharpFlake;
        case lifelens::ItemKind::StoneCuttingTool: return ELLCoreItemKind::StoneCuttingTool;
        case lifelens::ItemKind::Cordage: return ELLCoreItemKind::Cordage;
        case lifelens::ItemKind::SimpleContainer: return ELLCoreItemKind::SimpleContainer;
        case lifelens::ItemKind::FuelBundle: return ELLCoreItemKind::FuelBundle;
        case lifelens::ItemKind::DiggingStick: return ELLCoreItemKind::DiggingStick;
        case lifelens::ItemKind::StoneHammer: return ELLCoreItemKind::StoneHammer;
        case lifelens::ItemKind::RawMaterial:
        default:
            return ELLCoreItemKind::RawMaterial;
    }
}

ELLCoreTechniqueId ToUnrealTechnique(lifelens::TechniqueId Technique)
{
    switch (Technique)
    {
        case lifelens::TechniqueId::SharpFlake: return ELLCoreTechniqueId::SharpFlake;
        case lifelens::TechniqueId::ChippedStoneTool: return ELLCoreTechniqueId::ChippedStoneTool;
        case lifelens::TechniqueId::FireMaking: return ELLCoreTechniqueId::FireMaking;
        case lifelens::TechniqueId::FiberCordage: return ELLCoreTechniqueId::FiberCordage;
        case lifelens::TechniqueId::SimpleContainer: return ELLCoreTechniqueId::SimpleContainer;
        case lifelens::TechniqueId::DesignatedSanitationArea: return ELLCoreTechniqueId::DesignatedSanitationArea;
        case lifelens::TechniqueId::DugSanitationPit: return ELLCoreTechniqueId::DugSanitationPit;
        case lifelens::TechniqueId::PrimitiveStorage: return ELLCoreTechniqueId::PrimitiveStorage;
        case lifelens::TechniqueId::DiggingStick: return ELLCoreTechniqueId::DiggingStick;
        case lifelens::TechniqueId::StoneHammer: return ELLCoreTechniqueId::StoneHammer;
        case lifelens::TechniqueId::CopperSmelting: return ELLCoreTechniqueId::CopperSmelting;
        case lifelens::TechniqueId::None:
        default:
            return ELLCoreTechniqueId::None;
    }
}

ELLCoreKnowledgeLevel ToUnrealKnowledgeLevel(lifelens::KnowledgeLevel Level)
{
    switch (Level)
    {
        case lifelens::KnowledgeLevel::Observed: return ELLCoreKnowledgeLevel::Observed;
        case lifelens::KnowledgeLevel::Hypothesized: return ELLCoreKnowledgeLevel::Hypothesized;
        case lifelens::KnowledgeLevel::Understood: return ELLCoreKnowledgeLevel::Understood;
        case lifelens::KnowledgeLevel::Reproducible: return ELLCoreKnowledgeLevel::Reproducible;
        case lifelens::KnowledgeLevel::Practiced: return ELLCoreKnowledgeLevel::Practiced;
        case lifelens::KnowledgeLevel::Mastered: return ELLCoreKnowledgeLevel::Mastered;
        case lifelens::KnowledgeLevel::Unknown:
        default:
            return ELLCoreKnowledgeLevel::Unknown;
    }
}

ELLCoreKnowledgeSource ToUnrealKnowledgeSource(lifelens::CivilizationKnowledgeSource Source)
{
    switch (Source)
    {
        case lifelens::CivilizationKnowledgeSource::SelfDiscovery: return ELLCoreKnowledgeSource::SelfDiscovery;
        case lifelens::CivilizationKnowledgeSource::DirectWitness: return ELLCoreKnowledgeSource::DirectWitness;
        case lifelens::CivilizationKnowledgeSource::Teaching: return ELLCoreKnowledgeSource::Teaching;
        case lifelens::CivilizationKnowledgeSource::Unknown:
        default:
            return ELLCoreKnowledgeSource::Unknown;
    }
}

ELLCoreFacilityKind ToUnrealFacilityKind(lifelens::FacilityKind Kind)
{
    switch (Kind)
    {
        case lifelens::FacilityKind::PrimitiveStorage: return ELLCoreFacilityKind::PrimitiveStorage;
        case lifelens::FacilityKind::FirePit: return ELLCoreFacilityKind::FirePit;
        case lifelens::FacilityKind::WorkSurface: return ELLCoreFacilityKind::WorkSurface;
        case lifelens::FacilityKind::SleepingPlace: return ELLCoreFacilityKind::SleepingPlace;
        case lifelens::FacilityKind::Shelter: return ELLCoreFacilityKind::Shelter;
        case lifelens::FacilityKind::Furnace: return ELLCoreFacilityKind::Furnace;
        default: return ELLCoreFacilityKind::PrimitiveStorage;
    }
}

ELLCoreFacilityState ToUnrealFacilityState(lifelens::FacilityState State)
{
    switch (State)
    {
        case lifelens::FacilityState::Planned: return ELLCoreFacilityState::Planned;
        case lifelens::FacilityState::UnderConstruction: return ELLCoreFacilityState::UnderConstruction;
        case lifelens::FacilityState::Operational: return ELLCoreFacilityState::Operational;
        case lifelens::FacilityState::Ruined: return ELLCoreFacilityState::Ruined;
        default: return ELLCoreFacilityState::Planned;
    }
}

FLLCoreCivilizationItemStack ToUnrealItemStack(const lifelens::CivilizationItemObservation& Core)
{
    FLLCoreCivilizationItemStack Result;
    Result.Item = ToUnrealItem(Core.item);
    Result.Material = ToUnrealMaterial(Core.material);
    Result.Quantity = Core.quantity;
    Result.Quality = static_cast<float>(Core.quality);
    Result.Durability = static_cast<float>(Core.durability);
    return Result;
}
}

bool ULLCoreBridgeSubsystem::GetResidentCivilizationObservation(
    FGuid ResidentId,
    FLLCoreResidentCivilizationObservation& OutObservation) const
{
    OutObservation = FLLCoreResidentCivilizationObservation{};
    if (!CoreSimulation || !ResidentId.IsValid())
    {
        return false;
    }

    const uint64* CoreCharacterId = GuidToCore.Find(ResidentId);
    if (!CoreCharacterId)
    {
        return false;
    }

    const lifelens::CharacterId CharacterId = static_cast<lifelens::CharacterId>(*CoreCharacterId);
    const lifelens::Character* Character =
        lifelens::findObservedCharacter(CoreSimulation->world(), CharacterId);
    if (!Character)
    {
        return false;
    }

    const lifelens::ResidentCivilizationObservation Core =
        CoreSimulation->observeResidentCivilization(CharacterId);
    if (Core.residentId == 0)
    {
        return false;
    }

    OutObservation.ResidentId = ResidentId;
    OutObservation.DisplayName = UTF8_TO_TCHAR(Character->name.c_str());
    OutObservation.TotalInventoryUnits = Core.totalInventoryUnits;
    OutObservation.GatheringSkill = static_cast<float>(Core.gatheringSkill);
    OutObservation.CraftingSkill = static_cast<float>(Core.craftingSkill);
    OutObservation.LearningSkill = static_cast<float>(Core.learningSkill);
    OutObservation.KnownTechniqueCount = SafeCivilizationCount(Core.knownTechniqueCount);
    OutObservation.ReproducibleTechniqueCount = SafeCivilizationCount(Core.reproducibleTechniqueCount);
    OutObservation.LatestKnowledgeMinute = static_cast<int64>(Core.latestKnowledgeMinute);
    OutObservation.LatestTechnique = ToUnrealTechnique(Core.latestTechnique);

    OutObservation.Inventory.Reserve(SafeCivilizationCount(Core.inventory.size()));
    for (const lifelens::CivilizationItemObservation& Item : Core.inventory)
    {
        OutObservation.Inventory.Add(ToUnrealItemStack(Item));
    }

    OutObservation.Techniques.Reserve(SafeCivilizationCount(Core.techniques.size()));
    for (const lifelens::CivilizationTechniqueObservation& Technique : Core.techniques)
    {
        FLLCoreTechniqueKnowledgeObservation Read;
        Read.Technique = ToUnrealTechnique(Technique.technique);
        Read.Level = ToUnrealKnowledgeLevel(Technique.level);
        Read.Confidence = static_cast<float>(Technique.confidence);
        Read.SuccessfulUses = Technique.successfulUses;
        Read.bHasProvenance = Technique.hasProvenance;
        Read.Source = ToUnrealKnowledgeSource(Technique.source);
        Read.LearnedMinute = static_cast<int64>(Technique.learnedMinute);
        Read.HopCount = SafeCivilizationCount(Technique.hopCount);

        if (Technique.hasProvenance)
        {
            Read.ProvenanceFactId = StableFactId(Technique.factId);
            if (Technique.originResidentId != 0)
            {
                Read.OriginResidentId = MakeStableResidentGuid(static_cast<uint64>(Technique.originResidentId));
            }
            if (Technique.immediateSourceId != 0)
            {
                Read.ImmediateSourceResidentId = MakeStableResidentGuid(static_cast<uint64>(Technique.immediateSourceId));
            }
        }
        OutObservation.Techniques.Add(MoveTemp(Read));
    }

    return true;
}

FLLCoreCivilizationWorldObservation ULLCoreBridgeSubsystem::GetCivilizationWorldObservation(
    int32 MaxRecentDiscoveries) const
{
    FLLCoreCivilizationWorldObservation Result;
    if (!CoreSimulation)
    {
        return Result;
    }

    const int32 SafeRecentLimit = FMath::Clamp(MaxRecentDiscoveries, 0, 128);
    const lifelens::CivilizationWorldObservation Core =
        CoreSimulation->observeCivilizationWorld(static_cast<std::size_t>(SafeRecentLimit));

    Result.SimulationMinute = static_cast<int64>(Core.minute);
    Result.ResourceNodeCount = SafeCivilizationCount(Core.resourceNodeCount);
    Result.DepletedResourceNodeCount = SafeCivilizationCount(Core.depletedResourceNodeCount);
    Result.TotalResourceUnits = Core.totalResourceUnits;
    Result.StorageSiteCount = SafeCivilizationCount(Core.storageSiteCount);
    Result.TotalStoredUnits = Core.totalStoredUnits;
    Result.FacilityCount = SafeCivilizationCount(Core.facilityCount);
    Result.PlannedFacilityCount = SafeCivilizationCount(Core.plannedFacilityCount);
    Result.UnderConstructionFacilityCount = SafeCivilizationCount(Core.underConstructionFacilityCount);
    Result.OperationalFacilityCount = SafeCivilizationCount(Core.operationalFacilityCount);
    Result.TechniqueFactCount = SafeCivilizationCount(Core.techniqueFactCount);
    Result.TransmissionReceiptCount = SafeCivilizationCount(Core.transmissionReceiptCount);
    Result.UniqueKnownTechniqueTypes = SafeCivilizationCount(Core.uniqueKnownTechniqueTypes);
    Result.UniqueReproducibleTechniqueTypes = SafeCivilizationCount(Core.uniqueReproducibleTechniqueTypes);
    Result.KnownTechniqueOwners = SafeCivilizationCount(Core.knownTechniqueOwners);
    Result.ReproducibleTechniqueOwners = SafeCivilizationCount(Core.reproducibleTechniqueOwners);

    Result.Resources.Reserve(SafeCivilizationCount(Core.resources.size()));
    for (const lifelens::CivilizationResourceObservation& Resource : Core.resources)
    {
        FLLCoreCivilizationResourceObservation Read;
        Read.ResourceNodeId = static_cast<int64>(Resource.id);
        Read.Material = ToUnrealMaterial(Resource.material);
        Read.Quantity = Resource.quantity;
        Read.MaxQuantity = Resource.maxQuantity;
        Read.bRenewable = Resource.renewable;
        Read.RegenerationPerDay = Resource.regenerationPerDay;
        Result.Resources.Add(MoveTemp(Read));
    }

    Result.Storages.Reserve(SafeCivilizationCount(Core.storages.size()));
    for (const lifelens::CivilizationStorageObservation& Storage : Core.storages)
    {
        FLLCoreCivilizationStorageObservation Read;
        Read.StorageId = static_cast<int64>(Storage.id);
        Read.TotalUnits = Storage.totalUnits;
        Read.Inventory.Reserve(SafeCivilizationCount(Storage.inventory.size()));
        for (const lifelens::CivilizationItemObservation& Item : Storage.inventory)
        {
            Read.Inventory.Add(ToUnrealItemStack(Item));
        }
        Result.Storages.Add(MoveTemp(Read));
    }

    Result.Facilities.Reserve(SafeCivilizationCount(Core.facilities.size()));
    for (const lifelens::CivilizationFacilityObservation& Facility : Core.facilities)
    {
        FLLCoreCivilizationFacilityObservation Read;
        Read.FacilityId = static_cast<int64>(Facility.id);
        Read.Kind = ToUnrealFacilityKind(Facility.kind);
        Read.State = ToUnrealFacilityState(Facility.state);
        Read.GridX = Facility.pos.x;
        Read.GridY = Facility.pos.y;
        if (Facility.initiatedBy != 0)
        {
            Read.InitiatedByResidentId = MakeStableResidentGuid(static_cast<uint64>(Facility.initiatedBy));
        }
        if (Facility.lastWorkedBy != 0)
        {
            Read.LastWorkedByResidentId = MakeStableResidentGuid(static_cast<uint64>(Facility.lastWorkedBy));
        }
        Read.StartedMinute = static_cast<int64>(Facility.startedMinute);
        Read.CompletedMinute = static_cast<int64>(Facility.completedMinute);
        Read.ConstructionWork = static_cast<float>(Facility.constructionWork);
        Read.RequiredWork = static_cast<float>(Facility.requiredWork);
        Read.WorkProgress = static_cast<float>(Facility.workProgress);
        Read.Durability = static_cast<float>(Facility.durability);
        Read.bActive = Facility.active;
        Read.LinkedStorageId = static_cast<int64>(Facility.linkedStorage);
        Read.RequiredMaterialUnits = Facility.requiredMaterialUnits;
        Read.DeliveredMaterialUnits = Facility.deliveredMaterialUnits;
        Read.FuelUnits = Facility.fuelUnits;
        Read.CharcoalUnits = Facility.charcoalUnits;
        Read.OreUnits = Facility.oreUnits;
        Read.MetalUnits = Facility.metalUnits;
        Read.HeatLevel = static_cast<float>(Facility.heatLevel);
        Read.bLit = Facility.lit;
        Read.BurnMinutesRemaining = Facility.burnMinutesRemaining;
        Read.LastFireMinute = static_cast<int64>(Facility.lastFireMinute);
        Read.Requirements.Reserve(SafeCivilizationCount(Facility.requirements.size()));
        for (const lifelens::CivilizationFacilityRequirementObservation& Requirement : Facility.requirements)
        {
            FLLCoreCivilizationFacilityRequirementObservation RequirementRead;
            RequirementRead.Material = ToUnrealMaterial(Requirement.material);
            RequirementRead.Required = Requirement.required;
            RequirementRead.Delivered = Requirement.delivered;
            Read.Requirements.Add(MoveTemp(RequirementRead));
        }
        Result.Facilities.Add(MoveTemp(Read));
    }

    Result.RecentDiscoveries.Reserve(SafeCivilizationCount(Core.recentDiscoveries.size()));
    for (const lifelens::CivilizationDiscoveryObservation& Discovery : Core.recentDiscoveries)
    {
        FLLCoreCivilizationDiscoveryObservation Read;
        Read.FactId = StableFactId(Discovery.factId);
        Read.Technique = ToUnrealTechnique(Discovery.technique);
        if (Discovery.discovererId != 0)
        {
            Read.DiscovererResidentId = MakeStableResidentGuid(static_cast<uint64>(Discovery.discovererId));
        }
        Read.DiscovererName = UTF8_TO_TCHAR(Discovery.discovererName.c_str());
        Read.Minute = static_cast<int64>(Discovery.minute);
        Read.RecipientCount = SafeCivilizationCount(Discovery.recipientCount);
        Read.LivingKnowerCount = SafeCivilizationCount(Discovery.livingKnowerCount);
        Result.RecentDiscoveries.Add(MoveTemp(Read));
    }

    return Result;
}
