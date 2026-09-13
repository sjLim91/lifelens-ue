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
