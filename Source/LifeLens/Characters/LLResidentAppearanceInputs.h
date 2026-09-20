#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Simulation/LLCoreReadTypes.h"
#include "LLResidentAppearanceInputs.generated.h"

// Presentation-side appearance inputs.
//
// Production resolution is Core-authoritative:
//   - stable ResidentId supplies deterministic non-heritable style identity;
//   - Core Sex/LifeStage supply lifecycle identity;
//   - FLLCoreGeneticsSnapshot supplies inherited phenotype axes.
//
// The struct is presentation-only and never persisted as a competing Save
// authority. MakeTemporaryAppearanceInputs() exists only for invalid-identity
// QA/error paths and should not be used by a normally spawned resident.
USTRUCT(BlueprintType)
struct FLLResidentAppearanceInputs
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    FGuid ResidentId;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    ELLCoreSex Sex = ELLCoreSex::Male;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    ELLCoreLifeStage LifeStage = ELLCoreLifeStage::Adult;

    // Deterministic seed all variation axes derive from.
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    int32 VisualSeed = 1;

    // Continuous axes in [0, 1].
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    float FaceAxis = 0.5f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    float SkinToneAxis = 0.5f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    float EyeColorAxis = 0.5f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    float HairColorAxis = 0.5f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    float HeightAxis = 0.5f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    float BuildAxis = 0.5f;

    // Discrete variants; the consumer wraps them into its own catalogue sizes.
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    int32 FaceVariant = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    int32 HairStyleVariant = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    int32 OutfitVariant = 0;

    // True only for the invalid-identity QA hash path.
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    bool bTemporaryPresentationSeed = false;
};

// Single producer of appearance inputs for the presentation layer.
UCLASS()
class LIFELENS_API ULLResidentAppearanceInputSource : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Fallback derivation (presentation-only) for residents without a stable
    // identity. Pure function of (WorldSeed, ResidentId, Sex, LifeStage).
    UFUNCTION(BlueprintPure, Category="LifeLens|Appearance")
    static FLLResidentAppearanceInputs MakeTemporaryAppearanceInputs(int32 WorldSeed, FGuid ResidentId, ELLCoreSex Sex, ELLCoreLifeStage LifeStage);

    // Stable-identity resolution without genetics. Production Core residents
    // normally use ResolveWithGenetics().
    UFUNCTION(BlueprintPure, Category="LifeLens|Appearance")
    static FLLResidentAppearanceInputs Resolve(int32 WorldSeed, FGuid ResidentId, ELLCoreSex Sex, ELLCoreLifeStage LifeStage);

    UFUNCTION(BlueprintPure, Category="LifeLens|Appearance")
    static FLLResidentAppearanceInputs ResolveWithGenetics(
        int32 WorldSeed,
        FGuid ResidentId,
        ELLCoreSex Sex,
        ELLCoreLifeStage LifeStage,
        const FLLCoreGeneticsSnapshot& Genetics);
};
