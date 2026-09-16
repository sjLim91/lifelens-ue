#include "Simulation/LLCoreBridgeSubsystem.h"

#include "lifelens/ContextAction.h"
#include "lifelens/Simulation.h"
#include "lifelens/ToolEffectiveness.h"

namespace
{
ELLCoreContextActionKind ContextActionToUnrealContextKind(lifelens::ContextActionKind Kind)
{
    switch (Kind)
    {
        case lifelens::ContextActionKind::Social: return ELLCoreContextActionKind::Social;
        case lifelens::ContextActionKind::Civilization: return ELLCoreContextActionKind::Civilization;
        case lifelens::ContextActionKind::Parenting: return ELLCoreContextActionKind::Parenting;
        case lifelens::ContextActionKind::None:
        default: return ELLCoreContextActionKind::None;
    }
}

ELLCoreSocialIntent ContextActionToUnrealSocialIntent(lifelens::SocialIntent Intent)
{
    switch (Intent)
    {
        case lifelens::SocialIntent::Approach: return ELLCoreSocialIntent::Approach;
        case lifelens::SocialIntent::Avoid: return ELLCoreSocialIntent::Avoid;
        case lifelens::SocialIntent::Repair: return ELLCoreSocialIntent::Repair;
        case lifelens::SocialIntent::Comfort: return ELLCoreSocialIntent::Comfort;
        case lifelens::SocialIntent::None:
        default: return ELLCoreSocialIntent::None;
    }
}

ELLCoreCivilizationAction ContextActionToUnrealCivilizationAction(lifelens::CivilizationIntent Intent)
{
    switch (Intent)
    {
        case lifelens::CivilizationIntent::Gather: return ELLCoreCivilizationAction::Gather;
        case lifelens::CivilizationIntent::Store: return ELLCoreCivilizationAction::Store;
        case lifelens::CivilizationIntent::Experiment: return ELLCoreCivilizationAction::Experiment;
        case lifelens::CivilizationIntent::Craft: return ELLCoreCivilizationAction::Craft;
        case lifelens::CivilizationIntent::None:
        default: return ELLCoreCivilizationAction::None;
    }
}

ELLCoreFacilityBuildAction ContextActionToUnrealFacilityAction(lifelens::FacilityBuildAction Action)
{
    switch (Action)
    {
        case lifelens::FacilityBuildAction::Plan: return ELLCoreFacilityBuildAction::Plan;
        case lifelens::FacilityBuildAction::DeliverMaterial: return ELLCoreFacilityBuildAction::DeliverMaterial;
        case lifelens::FacilityBuildAction::Work: return ELLCoreFacilityBuildAction::Work;
        case lifelens::FacilityBuildAction::None:
        default: return ELLCoreFacilityBuildAction::None;
    }
}

ELLCoreFacilityKind ContextActionToUnrealFacilityKind(lifelens::FacilityKind Kind)
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

ELLCoreToolCapability ContextActionToUnrealToolCapability(lifelens::ToolCapability Capability)
{
    switch (Capability)
    {
        case lifelens::ToolCapability::Cut: return ELLCoreToolCapability::Cut;
        case lifelens::ToolCapability::Chop: return ELLCoreToolCapability::Chop;
        case lifelens::ToolCapability::Dig: return ELLCoreToolCapability::Dig;
        case lifelens::ToolCapability::Strike: return ELLCoreToolCapability::Strike;
        case lifelens::ToolCapability::Carry: return ELLCoreToolCapability::Carry;
        case lifelens::ToolCapability::Heat: return ELLCoreToolCapability::Heat;
        case lifelens::ToolCapability::None:
        default: return ELLCoreToolCapability::None;
    }
}

ELLCoreParentingAction ContextActionToUnrealParentingAction(lifelens::ParentingAction Action)
{
    switch (Action)
    {
        case lifelens::ParentingAction::Feed: return ELLCoreParentingAction::Feed;
        case lifelens::ParentingAction::PutToSleep: return ELLCoreParentingAction::PutToSleep;
        case lifelens::ParentingAction::Bathe: return ELLCoreParentingAction::Bathe;
        case lifelens::ParentingAction::ToiletAssist: return ELLCoreParentingAction::ToiletAssist;
        case lifelens::ParentingAction::Hold: return ELLCoreParentingAction::Hold;
        case lifelens::ParentingAction::Play: return ELLCoreParentingAction::Play;
        case lifelens::ParentingAction::Educate: return ELLCoreParentingAction::Educate;
        case lifelens::ParentingAction::Discipline: return ELLCoreParentingAction::Discipline;
        case lifelens::ParentingAction::Comfort: return ELLCoreParentingAction::Comfort;
        case lifelens::ParentingAction::HealthCare: return ELLCoreParentingAction::HealthCare;
        default: return ELLCoreParentingAction::None;
    }
}

ELLCoreMaterialKind ContextActionToUnrealMaterial(lifelens::MaterialKind Kind)
{
    switch (Kind)
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
        case lifelens::MaterialKind::Unknown:
        default: return ELLCoreMaterialKind::Unknown;
    }
}

ELLCoreItemKind ContextActionToUnrealItem(lifelens::ItemKind Kind)
{
    switch (Kind)
    {
        case lifelens::ItemKind::SharpFlake: return ELLCoreItemKind::SharpFlake;
        case lifelens::ItemKind::StoneCuttingTool: return ELLCoreItemKind::StoneCuttingTool;
        case lifelens::ItemKind::Cordage: return ELLCoreItemKind::Cordage;
        case lifelens::ItemKind::SimpleContainer: return ELLCoreItemKind::SimpleContainer;
        case lifelens::ItemKind::FuelBundle: return ELLCoreItemKind::FuelBundle;
        case lifelens::ItemKind::RawMaterial:
        default: return ELLCoreItemKind::RawMaterial;
    }
}

ELLCoreTechniqueId ContextActionToUnrealTechnique(lifelens::TechniqueId Technique)
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
        case lifelens::TechniqueId::None:
        default: return ELLCoreTechniqueId::None;
    }
}
}

bool ULLCoreBridgeSubsystem::GetResidentPendingContextDirective(
    FGuid ResidentId,
    FLLCoreActionDirective& OutDirective) const
{
    OutDirective = FLLCoreActionDirective{};
    if (!CoreSimulation || !ResidentId.IsValid())
    {
        return false;
    }

    const uint64* CoreCharacterId = GuidToCore.Find(ResidentId);
    if (!CoreCharacterId)
    {
        return false;
    }

    const lifelens::CharacterId CharacterId =
        static_cast<lifelens::CharacterId>(*CoreCharacterId);
    const lifelens::PendingContextActionObservation Pending =
        CoreSimulation->observePendingContextAction(CharacterId);
    if (!Pending.active || Pending.token == 0
        || Pending.token > static_cast<uint64>(TNumericLimits<int64>::Max()))
    {
        return false;
    }

    OutDirective.bAlive = true;
    OutDirective.ContextActionKind = ContextActionToUnrealContextKind(Pending.kind);
    OutDirective.ContextActionToken = static_cast<int64>(Pending.token);
    OutDirective.ContextActionDurationTicks = FMath::Max(1, Pending.durationTicks);

    if (Pending.targetResident != 0)
    {
        OutDirective.TargetResidentId =
            MakeStableResidentGuid(static_cast<uint64>(Pending.targetResident));
    }

    switch (Pending.kind)
    {
        case lifelens::ContextActionKind::Social:
            OutDirective.ActivityKind = ELLCoreObservedActivityKind::Social;
            OutDirective.SocialIntent = ContextActionToUnrealSocialIntent(Pending.socialIntent);
            break;

        case lifelens::ContextActionKind::Civilization:
            OutDirective.CivilizationAction = ContextActionToUnrealCivilizationAction(Pending.civilizationIntent);
            OutDirective.CivilizationMaterial = ContextActionToUnrealMaterial(Pending.material);
            OutDirective.CivilizationItem = ContextActionToUnrealItem(Pending.item);
            OutDirective.CivilizationTechnique = ContextActionToUnrealTechnique(Pending.technique);
            OutDirective.CivilizationQuantity = Pending.quantity;
            OutDirective.CivilizationActionMinute = Pending.issuedMinute;
            OutDirective.CivilizationResourceNodeId = static_cast<int64>(Pending.resourceNode);
            OutDirective.CivilizationStorageId = static_cast<int64>(Pending.storage);
            OutDirective.CivilizationFacilityAction = ContextActionToUnrealFacilityAction(Pending.facilityAction);
            OutDirective.CivilizationFacilityId = static_cast<int64>(Pending.facility);
            OutDirective.CivilizationFacilityKind = ContextActionToUnrealFacilityKind(Pending.facilityKind);
            OutDirective.bCivilizationActionSucceeded = false;
            OutDirective.bHasCivilizationSpatialTarget = Pending.hasSpatialTarget;
            OutDirective.CivilizationTargetGridX = Pending.targetPos.x;
            OutDirective.CivilizationTargetGridY = Pending.targetPos.y;
            OutDirective.CivilizationSanitationSiteId = static_cast<int64>(Pending.sanitationSiteId);

            if (Pending.civilizationIntent == lifelens::CivilizationIntent::Gather)
            {
                const lifelens::Character* Character = nullptr;
                for (const lifelens::Character& Candidate : CoreSimulation->world().characters)
                {
                    if (Candidate.id == CharacterId)
                    {
                        Character = &Candidate;
                        break;
                    }
                }

                if (Character)
                {
                    const lifelens::GatherToolUseProfile Tool =
                        lifelens::inspectGatherTool(
                            Character->civilization.inventory,
                            Pending.material);
                    if (Tool.available)
                    {
                        OutDirective.bHasCivilizationTool = true;
                        OutDirective.CivilizationToolItem = ContextActionToUnrealItem(Tool.tool.kind);
                        OutDirective.CivilizationToolCapability =
                            ContextActionToUnrealToolCapability(Tool.capability);
                        OutDirective.CivilizationToolQuality =
                            static_cast<float>(Tool.tool.quality);
                        OutDirective.CivilizationToolDurability =
                            static_cast<float>(Tool.durabilityBefore);
                        OutDirective.CivilizationToolQuantityMultiplier =
                            static_cast<float>(Tool.quantityMultiplier);
                    }
                }
            }
            break;

        case lifelens::ContextActionKind::Parenting:
            OutDirective.ParentingAction = ContextActionToUnrealParentingAction(Pending.parentingAction);
            break;

        case lifelens::ContextActionKind::None:
        default:
            return false;
    }

    return true;
}

bool ULLCoreBridgeSubsystem::CompleteResidentContextAction(
    FGuid ResidentId,
    int64 ContextActionToken,
    int32 ResolvedGridX,
    int32 ResolvedGridY)
{
    if (!CoreSimulation || !ResidentId.IsValid() || ContextActionToken <= 0)
    {
        return false;
    }

    const uint64* CoreCharacterId = GuidToCore.Find(ResidentId);
    if (!CoreCharacterId)
    {
        return false;
    }

    const lifelens::GridPos ResolvedPosition{
        static_cast<int>(ResolvedGridX),
        static_cast<int>(ResolvedGridY)};
    const bool bCompleted = CoreSimulation->completeExternalContextAction(
        static_cast<lifelens::CharacterId>(*CoreCharacterId),
        static_cast<uint64>(ContextActionToken),
        ResolvedPosition);
    if (bCompleted)
    {
        RebuildGuidIndex();
        OnCoreRuntimeStateChanged.Broadcast();
    }
    return bCompleted;
}
