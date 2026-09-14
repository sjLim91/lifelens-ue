#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Simulation/LLCoreReadTypes.h"
#include "LLResidentAppearanceInputs.generated.h"

// Presentation-side appearance inputs (Character Appearance v1, Track B).
//
// This struct is the ONLY thing the appearance component consumes. It mirrors
// the field set of the Bridge contract `FLLAppearanceProfile` (PR #65) so the
// producer can be swapped without touching the consumer:
//
//   - Until PR #65 is on main: `MakeTemporaryAppearanceInputs()` derives the
//     values from hash(WorldSeed, ResidentId). TEMPORARY presentation-only
//     values; not authoritative, never saved, never read back into Core.
//   - After PR #65: replace the body of `ULLResidentAppearanceInputSource::
//     Resolve()` with `ULLAppearanceProfileLibrary::MakeDeterministicAppearanceProfile`
//     and map its fields 1:1 here. No other file needs to change.
//
// Determinism: same WorldSeed + ResidentId always yields the same inputs, so
// NEW GAME residents differ per seed and Save/Load keeps each resident's look.
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

    // True when produced by the temporary hash path rather than the Bridge contract.
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    bool bTemporaryPresentationSeed = false;
};

// Single producer of appearance inputs for the presentation layer.
UCLASS()
class LIFELENS_API ULLResidentAppearanceInputSource : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // TEMPORARY presentation-only derivation used until PR #65's
    // FLLAppearanceProfile contract is available on main. Pure function of
    // (WorldSeed, ResidentId, Sex, LifeStage); no authoritative state.
    UFUNCTION(BlueprintPure, Category="LifeLens|Appearance")
    static FLLResidentAppearanceInputs MakeTemporaryAppearanceInputs(int32 WorldSeed, FGuid ResidentId, ELLCoreSex Sex, ELLCoreLifeStage LifeStage);

    // Resolution point the appearance component calls. Swap the implementation
    // to the Bridge contract here when it lands.
    UFUNCTION(BlueprintPure, Category="LifeLens|Appearance")
    static FLLResidentAppearanceInputs Resolve(int32 WorldSeed, FGuid ResidentId, ELLCoreSex Sex, ELLCoreLifeStage LifeStage);
};
