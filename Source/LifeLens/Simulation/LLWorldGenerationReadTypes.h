#pragma once

#include "CoreMinimal.h"
#include "LLWorldGenerationReadTypes.generated.h"

USTRUCT(BlueprintType)
struct FLLCoreNaturalResourcePatchObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int64 ResourceNodeId = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") FName Material;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 GridX = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 GridY = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 CurrentQuantity = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 MaxQuantity = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") bool bRenewable = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 RegenerationPerDay = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float VisualDensity = 0.0f;
};

USTRUCT(BlueprintType)
struct FLLCoreNaturalChunkObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") bool bMaterialized = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 ChunkX = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 ChunkY = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") FName Biome;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") FName Surface;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float Elevation = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float Moisture = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float Temperature = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float WaterPotential = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float FertilityPotential = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float TraversalEase = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float HazardPotential = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 MaterializedMinute = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") TArray<FLLCoreNaturalResourcePatchObservation> ResourcePatches;
};

USTRUCT(BlueprintType)
struct FLLCoreWorldGenerationObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") bool bAvailable = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int64 WorldSeed = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 GenerationVersion = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") bool bHasInitialStartRegion = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 InitialChunkX = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 InitialChunkY = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 InitialCenterGridX = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 InitialCenterGridY = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") float InitialViability = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") int32 MaterializedChunkCount = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") FLLCoreNaturalChunkObservation InitialChunk;
};
