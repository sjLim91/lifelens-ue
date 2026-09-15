#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Simulation/LLCoreReadTypes.h"
#include "LLResidentAppearanceInputs.generated.h"

// Presentation-side appearance inputs (Character Appearance v1, Track B).
//
// This struct is the ONLY thing the appearance component consumes. It mirrors
// the field set of the Bridge contract `FLLAppearanceProfile`
// (Source/LifeLens/Simulation/LLAppearanceProfile.h, PR #65) so the producer
// stays in one place:
//
//   - `ULLResidentAppearanceInputSource::Resolve()` maps
//     `ULLAppearanceProfileLibrary::MakeDeterministicAppearanceProfile` 1:1.
//     The Bridge derives ResidentId from (WorldSeed, Core CharacterId), so the
//     same restored resident yields the same look without any cache.
//   - `MakeTemporaryAppearanceInputs()` is the fallback for an invalid
//     ResidentId only: hash(WorldSeed, ResidentId), presentation-only, not
//     authoritative, never saved.
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
    // Fallback derivation (presentation-only) for residents without a stable
    // identity. Pure function of (WorldSeed, ResidentId, Sex, LifeStage).
    UFUNCTION(BlueprintPure, Category="LifeLens|Appearance")
    static FLLResidentAppearanceInputs MakeTemporaryAppearanceInputs(int32 WorldSeed, FGuid ResidentId, ELLCoreSex Sex, ELLCoreLifeStage LifeStage);

    // Resolution point the appearance component calls: Bridge contract first,
    // temporary fallback otherwise.
    UFUNCTION(BlueprintPure, Category="LifeLens|Appearance")
    static FLLResidentAppearanceInputs Resolve(int32 WorldSeed, FGuid ResidentId, ELLCoreSex Sex, ELLCoreLifeStage LifeStage);
};
