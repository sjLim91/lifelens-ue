#pragma once

#include "CoreMinimal.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "LLEnvironmentalResidueVisualizerComponent.generated.h"

class ULLCoreBridgeSubsystem;
struct FLLCoreEnvironmentObservation;

/**
 * Read-only presentation of authoritative environmental residues.
 *
 * One HISM component represents many residue records.  It never creates,
 * mutates, cleans or acknowledges simulation state; it only rebuilds from the
 * Core Bridge read DTO.  Per-instance custom data reserves four floats for
 * presentation materials: intensity, normalized amount, normalized radius,
 * and normalized age.
 */
UCLASS(ClassGroup=(LifeLens), meta=(BlueprintSpawnableComponent))
class LIFELENS_API ULLEnvironmentalResidueVisualizerComponent
    : public UHierarchicalInstancedStaticMeshComponent
{
    GENERATED_BODY()

public:
    ULLEnvironmentalResidueVisualizerComponent();

    void RefreshFromCore(
        const ULLCoreBridgeSubsystem& CoreBridge,
        float CoreGridCellSizeUU,
        bool bForce = false,
        int32 CoreOriginGridX = 0,
        int32 CoreOriginGridY = 0);

    UFUNCTION(BlueprintPure, Category="LifeLens|World|Environment")
    int32 GetVisualInstanceCount() const { return GetInstanceCount(); }

private:
    uint32 BuildVisualSignature(const FLLCoreEnvironmentObservation& Environment) const;
    FVector ResolveSurfaceLocation(int32 GridX, int32 GridY, float CoreGridCellSizeUU, int32 CoreOriginGridX, int32 CoreOriginGridY) const;

    uint32 LastVisualSignature = 0;
    bool bHasVisualSignature = false;

    UPROPERTY(EditAnywhere, Category="LifeLens|Environment|Visual", meta=(ClampMin="1", ClampMax="512"))
    int32 MaxResidueInstances = 128;

    UPROPERTY(EditAnywhere, Category="LifeLens|Environment|Visual", meta=(ClampMin="0.0"))
    float SurfaceOffsetUU = 1.5f;

    UPROPERTY(EditAnywhere, Category="LifeLens|Environment|Visual", meta=(ClampMin="0"))
    int32 StartCullDistanceUU = 2500;

    UPROPERTY(EditAnywhere, Category="LifeLens|Environment|Visual", meta=(ClampMin="0"))
    int32 EndCullDistanceUU = 12000;
};
