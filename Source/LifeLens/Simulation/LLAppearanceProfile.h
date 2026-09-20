#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Simulation/LLCoreReadTypes.h"
#include "LLAppearanceProfile.generated.h"

/**
 * Stable visual projection for a resident.
 *
 * ResidentId is already derived from WorldSeed + Core CharacterId by the Core bridge,
 * so style identity remains stable across Save/Load without creating a second authority.
 * Inheritable phenotype axes come from the authoritative persisted Core GeneticsProfile
 * when a resident observation is available. Asset-specific presentation maps these
 * vendor-neutral values to meshes, materials, hair, clothing and future face morphs.
 */
USTRUCT(BlueprintType)
struct FLLAppearanceProfile
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    FGuid ResidentId;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    ELLCoreSex Sex = ELLCoreSex::Male;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    ELLCoreLifeStage LifeStage = ELLCoreLifeStage::Adult;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    int32 VisualSeed = 1;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance", meta=(ClampMin="0.0", ClampMax="1.0"))
    float FaceAxis = 0.5f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance", meta=(ClampMin="0.0", ClampMax="1.0"))
    float SkinToneAxis = 0.5f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance", meta=(ClampMin="0.0", ClampMax="1.0"))
    float EyeColorAxis = 0.5f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance", meta=(ClampMin="0.0", ClampMax="1.0"))
    float HairColorAxis = 0.5f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance", meta=(ClampMin="0.0", ClampMax="1.0"))
    float HeightAxis = 0.5f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance", meta=(ClampMin="0.0", ClampMax="1.0"))
    float BuildAxis = 0.5f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    int32 FaceVariant = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    int32 HairStyleVariant = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Appearance")
    int32 OutfitVariant = 0;
};

UCLASS()
class LIFELENS_API ULLAppearanceProfileLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="LifeLens|Appearance")
    static FLLAppearanceProfile MakeDeterministicAppearanceProfile(
        FGuid ResidentId,
        ELLCoreSex Sex,
        ELLCoreLifeStage LifeStage);

    UFUNCTION(BlueprintPure, Category="LifeLens|Appearance")
    static FLLAppearanceProfile MakeGeneticAppearanceProfile(
        FGuid ResidentId,
        ELLCoreSex Sex,
        ELLCoreLifeStage LifeStage,
        const FLLCoreGeneticsSnapshot& Genetics);
};
