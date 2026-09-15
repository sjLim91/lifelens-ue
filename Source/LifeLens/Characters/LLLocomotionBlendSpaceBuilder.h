#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LLLocomotionBlendSpaceBuilder.generated.h"

class UAnimSequence;
class UBlendSpace;

// Editor-time helper for building the resident locomotion blend space from a
// script (Content/Characters/Quaternius/Import/make_locomotion_blendspace.py).
//
// Setting `SampleData` directly from Python produces an asset that has samples
// but no triangulation, so `GetSamplesFromBlendInput` resolves 0 samples at
// runtime and the mesh falls back to the reference (T) pose. The editor-only
// `AddSample` / `ValidateSampleData` / `ResampleData` path is what builds the
// runtime data, and it is not exposed to Python; this library exposes it.
UCLASS()
class LIFELENS_API ULLLocomotionBlendSpaceBuilder : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Replaces the blend space contents with one sample per animation, at the
    // matching speed on the X axis, and rebuilds the runtime blend data.
    // Returns the number of usable samples (0 on failure).
    UFUNCTION(BlueprintCallable, Category="LifeLens|Motion")
    // The axis range itself is set by the caller through the asset's
    // `blend_parameters` property, which is script-writable.
    static int32 BuildSpeedBlendSpace(UBlendSpace* BlendSpace,
                                      const TArray<UAnimSequence*>& Animations,
                                      const TArray<float>& Speeds);

    // Number of samples the blend space resolves for a given speed. Used by the
    // build script to assert that the asset actually works before saving.
    UFUNCTION(BlueprintCallable, Category="LifeLens|Motion")
    static int32 CountResolvedSamples(UBlendSpace* BlendSpace, float Speed);
};
