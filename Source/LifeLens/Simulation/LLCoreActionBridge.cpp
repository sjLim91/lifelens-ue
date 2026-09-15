#include "Simulation/LLCoreBridgeSubsystem.h"

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
    bool bEmergencyFallback) const
{
    const lifelens::Goal Goal = ToCorePhysicalGoal(Intent);
    return bEmergencyFallback
        ? static_cast<int32>(lifelens::emergencyUseDurationTicks(Goal))
        : static_cast<int32>(lifelens::facilityUseDurationTicks(Goal));
}

bool ULLCoreBridgeSubsystem::CompleteResidentPhysicalAction(
    FGuid ResidentId,
    bool bEmergencyFallback,
    int32 ResolvedGridX,
    int32 ResolvedGridY)
{
    if (!CoreSimulation || !ResidentId.IsValid())
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
        ResolvedPosition);
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

    return true;
}
