#include "World/LLActivityAnchor.h"
#include "Components/SceneComponent.h"

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

bool ALLActivityAnchor::CanBeUsedBy(FGuid ResidentId) const
{
    if (!bEnabled || !ResidentId.IsValid())
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
    if (!IsClaimedBy(ResidentId))
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
