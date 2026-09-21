#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LLWorldStreamingSubsystem.generated.h"

/**
 * World v2 observer-interest coordinator.
 *
 * This subsystem owns presentation/observer interest only. It never materializes
 * Core chunks, resources or facilities. Simulation interest remains the
 * authoritative Core materialized-chunk registry.
 */
UCLASS()
class LIFELENS_API ULLWorldStreamingSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void SetObserverPresentationTarget(const FVector& TargetUU);
    void ClearObserverPresentationTarget();

    bool TryGetObserverPresentationTarget(FVector& OutTargetUU) const;

    FIntPoint ResolveObserverCenterChunk(
        int32 AnchorChunkX,
        int32 AnchorChunkY) const;

private:
    bool bHasObserverPresentationTarget = false;
    FVector ObserverPresentationTargetUU = FVector::ZeroVector;
};
