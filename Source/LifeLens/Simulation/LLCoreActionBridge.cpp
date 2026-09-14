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

bool ULLCoreBridgeSubsystem::CompleteResidentPhysicalAction(FGuid ResidentId)
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

    const bool bCompleted = CoreSimulation->completeExternalPhysicalAction(
        static_cast<lifelens::CharacterId>(*CoreCharacterId));
    if (bCompleted)
    {
        RebuildGuidIndex();
        OnCoreRuntimeStateChanged.Broadcast();
    }
    return bCompleted;
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
