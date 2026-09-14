#pragma once

#include "CoreMinimal.h"
#include "LLEnvironmentReadTypes.generated.h"

UENUM(BlueprintType)
enum class ELLCoreEnvironmentalResidueKind : uint8
{
    HumanWaste
};

USTRUCT(BlueprintType)
struct FLLCoreEnvironmentalResidueObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") int64 ResidueId = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") ELLCoreEnvironmentalResidueKind Kind = ELLCoreEnvironmentalResidueKind::HumanWaste;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") int32 GridX = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") int32 GridY = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") FGuid SourceResidentId;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") int32 AgeMinutes = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") float Amount = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") float Intensity = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") int32 RadiusTiles = 1;
};

USTRUCT(BlueprintType)
struct FLLCoreEnvironmentObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") int64 SimulationMinute = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") int32 TotalResidues = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") int32 HumanWasteResidues = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") float AggregateAmount = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") float PeakIntensity = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") TArray<FLLCoreEnvironmentalResidueObservation> Residues;
};
