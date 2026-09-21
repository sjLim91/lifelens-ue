#include "World/LLWorldStreamingSubsystem.h"

#include "World/LLWorldSpatialContract.h"

void ULLWorldStreamingSubsystem::SetObserverPresentationTarget(
    const FVector& TargetUU)
{
    if (!FMath::IsFinite(TargetUU.X)
        || !FMath::IsFinite(TargetUU.Y)
        || !FMath::IsFinite(TargetUU.Z))
    {
        return;
    }

    ObserverPresentationTargetUU = TargetUU;
    bHasObserverPresentationTarget = true;
}

void ULLWorldStreamingSubsystem::ClearObserverPresentationTarget()
{
    ObserverPresentationTargetUU = FVector::ZeroVector;
    bHasObserverPresentationTarget = false;
}

bool ULLWorldStreamingSubsystem::TryGetObserverPresentationTarget(
    FVector& OutTargetUU) const
{
    if (!bHasObserverPresentationTarget)
    {
        return false;
    }

    OutTargetUU = ObserverPresentationTargetUU;
    return true;
}

FIntPoint ULLWorldStreamingSubsystem::ResolveObserverCenterChunk(
    int32 AnchorChunkX,
    int32 AnchorChunkY) const
{
    if (!bHasObserverPresentationTarget)
    {
        return FIntPoint(AnchorChunkX, AnchorChunkY);
    }

    return LLWorldSpatialContract::LogicalChunkForPresentationLocation(
        AnchorChunkX,
        AnchorChunkY,
        FVector2D(
            ObserverPresentationTargetUU.X,
            ObserverPresentationTargetUU.Y));
}
