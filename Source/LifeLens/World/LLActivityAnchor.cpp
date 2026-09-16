#include "World/LLActivityAnchor.h"

#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Simulation/LLCivilizationReadTypes.h"
#include "Simulation/LLCoreBridgeSubsystem.h"

ALLActivityAnchor::ALLActivityAnchor()
{
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
}

FVector ALLActivityAnchor::GetUseLocation() const
{
    return GetActorTransform().TransformPosition(LocalUseOffset);
}

FTransform ALLActivityAnchor::GetUseTransform() const
{
    const FTransform ActorTransform = GetActorTransform();
    const FVector WorldLocation = ActorTransform.TransformPosition(LocalUseOffset);
    const FQuat WorldRotation = ActorTransform.TransformRotation(LocalUseRotation.Quaternion());
    return FTransform(WorldRotation, WorldLocation, FVector::OneVector);
}

bool ALLActivityAnchor::SupportsIntent(ELLActionIntent Intent) const
{
    return SupportedIntent == Intent || AdditionalSupportedIntents.Contains(Intent);
}

bool ALLActivityAnchor::RequiresCoreFacilityBinding() const
{
    return AffordanceTier == ELLWorldAffordanceTier::Preferred
        || AffordanceTier == ELLWorldAffordanceTier::Primitive;
}

bool ALLActivityAnchor::HasOperationalCoreFacilityBinding() const
{
    if (!RequiresCoreFacilityBinding())
    {
        return true;
    }
    if (CoreFacilityId <= 0)
    {
        return false;
    }

    const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const ULLCoreBridgeSubsystem* Bridge = GameInstance
        ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>()
        : nullptr;
    if (!Bridge || !Bridge->IsCoreRunning())
    {
        return false;
    }

    const FLLCoreCivilizationWorldObservation World =
        Bridge->GetCivilizationWorldObservation(0);
    for (const FLLCoreCivilizationFacilityObservation& Facility : World.Facilities)
    {
        if (Facility.FacilityId == CoreFacilityId)
        {
            return Facility.State == ELLCoreFacilityState::Operational
                && Facility.bActive;
        }
    }
    return false;
}

bool ALLActivityAnchor::CanBeUsedBy(FGuid ResidentId) const
{
    if (!bEnabled || !ResidentId.IsValid() || !HasOperationalCoreFacilityBinding())
    {
        return false;
    }

    return State == ELLActivityAnchorState::Available || ClaimedResidentId == ResidentId;
}

bool ALLActivityAnchor::IsClaimedBy(FGuid ResidentId) const
{
    return ResidentId.IsValid()
        && ClaimedResidentId == ResidentId
        && State != ELLActivityAnchorState::Available;
}

bool ALLActivityAnchor::TryReserve(FGuid ResidentId)
{
    if (!CanBeUsedBy(ResidentId))
    {
        return false;
    }

    ClaimedResidentId = ResidentId;
    if (State == ELLActivityAnchorState::Available)
    {
        State = ELLActivityAnchorState::Reserved;
    }
    return true;
}

bool ALLActivityAnchor::MarkInUse(FGuid ResidentId)
{
    if (!IsClaimedBy(ResidentId) || !HasOperationalCoreFacilityBinding())
    {
        return false;
    }

    State = ELLActivityAnchorState::InUse;
    return true;
}

void ALLActivityAnchor::Release(FGuid ResidentId)
{
    if (!IsClaimedBy(ResidentId))
    {
        return;
    }

    ClaimedResidentId.Invalidate();
    State = ELLActivityAnchorState::Available;
}
