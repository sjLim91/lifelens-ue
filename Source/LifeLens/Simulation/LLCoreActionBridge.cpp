#include "Simulation/LLCoreBridgeSubsystem.h"

#include "lifelens/CivilizationSpatial.h"
#include "lifelens/ObserverReadModel.h"
#include "lifelens/Simulation.h"

namespace
{
ELLCorePhysicalIntent ToUnrealPhysicalIntent(lifelens::Goal Goal)
{
    switch (Goal)
    {
        case lifelens::Goal::Eat: return ELLCorePhysicalIntent::Eat;
        case lifelens::Goal::Drink: return ELLCorePhysicalIntent::Drink;
        case lifelens::Goal::Sleep: return ELLCorePhysicalIntent::Sleep;
        case lifelens::Goal::UseToilet: return ELLCorePhysicalIntent::Toilet;
        case lifelens::Goal::Wash: return ELLCorePhysicalIntent::Hygiene;
        case lifelens::Goal::Idle:
        default:
            return ELLCorePhysicalIntent::None;
    }
}

lifelens::Goal ToCorePhysicalGoal(ELLCorePhysicalIntent Intent)
{
    switch (Intent)
    {
        case ELLCorePhysicalIntent::Eat: return lifelens::Goal::Eat;
        case ELLCorePhysicalIntent::Drink: return lifelens::Goal::Drink;
        case ELLCorePhysicalIntent::Sleep: return lifelens::Goal::Sleep;
        case ELLCorePhysicalIntent::Toilet: return lifelens::Goal::UseToilet;
        case ELLCorePhysicalIntent::Hygiene: return lifelens::Goal::Wash;
        case ELLCorePhysicalIntent::None:
        default:
            return lifelens::Goal::Idle;
    }
}

ELLCoreSocialIntent ToUnrealSocialIntent(lifelens::SocialIntent Intent)
{
    switch (Intent)
    {
        case lifelens::SocialIntent::Approach: return ELLCoreSocialIntent::Approach;
        case lifelens::SocialIntent::Avoid: return ELLCoreSocialIntent::Avoid;
        case lifelens::SocialIntent::Repair: return ELLCoreSocialIntent::Repair;
        case lifelens::SocialIntent::Comfort: return ELLCoreSocialIntent::Comfort;
        case lifelens::SocialIntent::None:
        default:
            return ELLCoreSocialIntent::None;
    }
}

ELLCoreCivilizationAction ToUnrealCivilizationAction(lifelens::CivilizationActivityKind Kind)
{
    switch (Kind)
    {
        case lifelens::CivilizationActivityKind::Gather: return ELLCoreCivilizationAction::Gather;
        case lifelens::CivilizationActivityKind::Store: return ELLCoreCivilizationAction::Store;
        case lifelens::CivilizationActivityKind::Retrieve: return ELLCoreCivilizationAction::Retrieve;
        case lifelens::CivilizationActivityKind::Experiment: return ELLCoreCivilizationAction::Experiment;
        case lifelens::CivilizationActivityKind::Craft: return ELLCoreCivilizationAction::Craft;
        case lifelens::CivilizationActivityKind::None:
        default:
            return ELLCoreCivilizationAction::None;
    }
}

ELLCoreMaterialKind ToUnrealMaterialKind(lifelens::MaterialKind Kind)
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
        default:
            return ELLCoreMaterialKind::Unknown;
    }
}

ELLCoreItemKind ToUnrealItemKind(lifelens::ItemKind Kind)
{
    switch (Kind)
    {
        case lifelens::ItemKind::SharpFlake: return ELLCoreItemKind::SharpFlake;
        case lifelens::ItemKind::StoneCuttingTool: return ELLCoreItemKind::StoneCuttingTool;
        case lifelens::ItemKind::Cordage: return ELLCoreItemKind::Cordage;
        case lifelens::ItemKind::SimpleContainer: return ELLCoreItemKind::SimpleContainer;
        case lifelens::ItemKind::FuelBundle: return ELLCoreItemKind::FuelBundle;
        case lifelens::ItemKind::RawMaterial:
        default:
            return ELLCoreItemKind::RawMaterial;
    }
}

ELLCoreTechniqueId ToUnrealTechniqueId(lifelens::TechniqueId Technique)
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
        case lifelens::TechniqueId::None:
        default:
            return ELLCoreTechniqueId::None;
    }
}
}

void ULLCoreBridgeSubsystem::SetExternalPhysicalExecutionEnabled(bool bEnabled)
{
    if (CoreSimulation)
    {
        CoreSimulation->setExternalPhysicalExecution(bEnabled);
    }
}

int32 ULLCoreBridgeSubsystem::GetPhysicalActionDurationTicks(
    ELLCorePhysicalIntent Intent,
    bool bEmergencyFallback,
    bool bDesignatedSanitationSite) const
{
    if (bDesignatedSanitationSite && Intent == ELLCorePhysicalIntent::Toilet)
    {
        return static_cast<int32>(lifelens::designatedSanitationUseDurationTicks());
    }

    const lifelens::Goal Goal = ToCorePhysicalGoal(Intent);
    return bEmergencyFallback
        ? static_cast<int32>(lifelens::emergencyUseDurationTicks(Goal))
        : static_cast<int32>(lifelens::facilityUseDurationTicks(Goal));
}

bool ULLCoreBridgeSubsystem::CompleteResidentPhysicalAction(
    FGuid ResidentId,
    bool bEmergencyFallback,
    int32 ResolvedGridX,
    int32 ResolvedGridY,
    int64 SanitationSiteId)
{
    if (!CoreSimulation || !ResidentId.IsValid() || SanitationSiteId < 0)
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

    const bool bCompleted = CoreSimulation->completeExternalPhysicalAction(
        static_cast<lifelens::CharacterId>(*CoreCharacterId),
        bEmergencyFallback,
        ResolvedPosition,
        static_cast<lifelens::SanitationSiteId>(SanitationSiteId));
    if (bCompleted)
    {
        RebuildGuidIndex();
        OnCoreRuntimeStateChanged.Broadcast();
    }
    return bCompleted;
}

bool ULLCoreBridgeSubsystem::GetResidentRuntimeGridPosition(
    FGuid ResidentId,
    int32& OutGridX,
    int32& OutGridY) const
{
    OutGridX = 0;
    OutGridY = 0;
    if (!CoreSimulation || !ResidentId.IsValid())
    {
        return false;
    }

    const uint64* CoreCharacterId = GuidToCore.Find(ResidentId);
    if (!CoreCharacterId)
    {
        return false;
    }

    lifelens::GridPos Position{};
    if (!CoreSimulation->runtimePosition(
        static_cast<lifelens::CharacterId>(*CoreCharacterId), Position))
    {
        return false;
    }

    OutGridX = static_cast<int32>(Position.x);
    OutGridY = static_cast<int32>(Position.y);
    return true;
}

bool ULLCoreBridgeSubsystem::GetRecommendedOutdoorReliefGridPosition(
    FGuid ResidentId,
    int32& OutGridX,
    int32& OutGridY) const
{
    OutGridX = 0;
    OutGridY = 0;
    if (!CoreSimulation || !ResidentId.IsValid())
    {
        return false;
    }

    const uint64* CoreCharacterId = GuidToCore.Find(ResidentId);
    if (!CoreCharacterId)
    {
        return false;
    }

    lifelens::GridPos Position{};
    if (!CoreSimulation->recommendedOutdoorReliefPosition(
        static_cast<lifelens::CharacterId>(*CoreCharacterId), Position))
    {
        return false;
    }

    OutGridX = static_cast<int32>(Position.x);
    OutGridY = static_cast<int32>(Position.y);
    return true;
}

bool ULLCoreBridgeSubsystem::GetSanitationUseTarget(
    FGuid ResidentId,
    int32& OutGridX,
    int32& OutGridY,
    bool& bOutDesignatedSite,
    int64& OutSanitationSiteId) const
{
    OutGridX = 0;
    OutGridY = 0;
    bOutDesignatedSite = false;
    OutSanitationSiteId = 0;

    if (!CoreSimulation || !ResidentId.IsValid())
    {
        return false;
    }

    const uint64* CoreCharacterId = GuidToCore.Find(ResidentId);
    if (!CoreCharacterId)
    {
        return false;
    }

    lifelens::SanitationUseTarget Target;
    if (!CoreSimulation->sanitationUseTarget(
        static_cast<lifelens::CharacterId>(*CoreCharacterId), Target))
    {
        return false;
    }

    OutGridX = static_cast<int32>(Target.pos.x);
    OutGridY = static_cast<int32>(Target.pos.y);
    bOutDesignatedSite = Target.kind == lifelens::SanitationUseTargetKind::DesignatedArea;
    OutSanitationSiteId = static_cast<int64>(Target.siteId);
    return true;
}

bool ULLCoreBridgeSubsystem::GetSettlementSleepUseTarget(
    FGuid ResidentId,
    int32& OutGridX,
    int32& OutGridY,
    int64& OutFacilityId) const
{
    OutGridX = 0;
    OutGridY = 0;
    OutFacilityId = 0;
    if (!CoreSimulation || !ResidentId.IsValid())
    {
        return false;
    }

    const uint64* CoreCharacterId = GuidToCore.Find(ResidentId);
    if (!CoreCharacterId)
    {
        return false;
    }

    lifelens::GridPos Position{};
    lifelens::FacilityId FacilityId = 0;
    if (!CoreSimulation->settlementSleepTarget(
            static_cast<lifelens::CharacterId>(*CoreCharacterId),
            Position,
            FacilityId))
    {
        return false;
    }

    OutGridX = static_cast<int32>(Position.x);
    OutGridY = static_cast<int32>(Position.y);
    OutFacilityId = static_cast<int64>(FacilityId);
    return OutFacilityId > 0;
}


bool ULLCoreBridgeSubsystem::GetResidentActionDirective(
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

    const lifelens::CharacterId CharacterId = static_cast<lifelens::CharacterId>(*CoreCharacterId);
    const lifelens::Character* Character =
        lifelens::findObservedCharacter(CoreSimulation->world(), CharacterId);
    if (!Character)
    {
        return false;
    }

    const lifelens::ResidentObservation Observation = CoreSimulation->observeResident(CharacterId);
    OutDirective.bAlive = Character->alive;

    switch (Observation.activityKind)
    {
        case lifelens::ObservedActivityKind::Physical:
            OutDirective.ActivityKind = ELLCoreObservedActivityKind::Physical;
            OutDirective.PhysicalIntent = ToUnrealPhysicalIntent(Observation.physicalGoal);
            break;
        case lifelens::ObservedActivityKind::Social:
            OutDirective.ActivityKind = ELLCoreObservedActivityKind::Social;
            OutDirective.SocialIntent = ToUnrealSocialIntent(Observation.socialIntent);
            if (Observation.activityTargetId != 0)
            {
                OutDirective.TargetResidentId =
                    MakeStableResidentGuid(static_cast<uint64>(Observation.activityTargetId));
            }
            break;
        case lifelens::ObservedActivityKind::Idle:
        default:
            OutDirective.ActivityKind = ELLCoreObservedActivityKind::Idle;
            break;
    }

    // Physical/social authority wins if a new action already replaced the
    // short-lived civilization presentation context.
    if (Observation.activityKind == lifelens::ObservedActivityKind::Idle)
    {
        const lifelens::ResidentCivilizationActivityObservation Civilization =
            CoreSimulation->observeResidentCivilizationActivity(CharacterId);
        if (Civilization.active)
        {
            OutDirective.CivilizationAction = ToUnrealCivilizationAction(Civilization.kind);
            OutDirective.CivilizationMaterial = ToUnrealMaterialKind(Civilization.material);
            OutDirective.CivilizationItem = ToUnrealItemKind(Civilization.item);
            OutDirective.CivilizationTechnique = ToUnrealTechniqueId(Civilization.technique);
            OutDirective.CivilizationQuantity = static_cast<int32>(Civilization.quantity);
            OutDirective.CivilizationActionMinute = static_cast<int64>(Civilization.minute);
            OutDirective.CivilizationResourceNodeId = static_cast<int64>(Civilization.resourceNode);
            OutDirective.CivilizationStorageId = static_cast<int64>(Civilization.storage);
            OutDirective.bCivilizationActionSucceeded = Civilization.success;
            OutDirective.bHasCivilizationSpatialTarget = Civilization.hasSpatialTarget;
            OutDirective.CivilizationTargetGridX = static_cast<int32>(Civilization.targetGridX);
            OutDirective.CivilizationTargetGridY = static_cast<int32>(Civilization.targetGridY);
            OutDirective.CivilizationSanitationSiteId = static_cast<int64>(Civilization.sanitationSiteId);

            // Sanitation already carries its real site position through the
            // runtime observation. Gather/Store use stable Core entity ids, so
            // resolve their authoritative sites here rather than letting
            // Character presentation guess a nearby scenery object.
            if (!OutDirective.bHasCivilizationSpatialTarget)
            {
                lifelens::GridPos Target{};
                bool bResolved = false;
                if (Civilization.kind == lifelens::CivilizationActivityKind::Gather)
                {
                    bResolved = lifelens::resolveCivilizationResourceAccessGridPosition(
                        CoreSimulation->world(), Civilization.resourceNode, Target);
                }
                else if (Civilization.kind == lifelens::CivilizationActivityKind::Store)
                {
                    bResolved = lifelens::resolveCivilizationStorageGridPosition(
                        CoreSimulation->world(), Civilization.storage, Target);
                }

                if (bResolved)
                {
                    OutDirective.bHasCivilizationSpatialTarget = true;
                    OutDirective.CivilizationTargetGridX = static_cast<int32>(Target.x);
                    OutDirective.CivilizationTargetGridY = static_cast<int32>(Target.y);
                }
            }
        }
    }

    return true;
}
