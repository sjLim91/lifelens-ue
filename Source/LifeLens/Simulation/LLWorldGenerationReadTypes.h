#pragma once

#include "CoreMinimal.h"
#include "LLWorldGenerationReadTypes.generated.h"

UENUM(BlueprintType)
enum class ELLCoreNaturalObstacleKind : uint8
{
    Tree,
    Rock
};

UENUM(BlueprintType)
enum class ELLCoreSurfaceWaterKind : uint8
{
    None,
    Spring,
    Stream,
    River,
    Lake,
    Wetland,
    Coast,
    Ocean
};

UENUM(BlueprintType)
enum class ELLCoreWaterSalinity : uint8
{
    Fresh,
    Brackish,
    Salt
};

USTRUCT(BlueprintType)
struct FLLCoreHydrologyObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology") bool bAvailable = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology") int32 ChunkX = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology") int32 ChunkY = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology") ELLCoreSurfaceWaterKind SurfaceKind = ELLCoreSurfaceWaterKind::None;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology") ELLCoreWaterSalinity Salinity = ELLCoreWaterSalinity::Fresh;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology") int64 SurfaceWaterId = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology") float SurfaceAvailability = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology") float FlowPotential = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology") float GroundwaterPotential = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology") float RechargePotential = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology") float RunoffPotential = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology") bool bHasDownstream = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology") int32 DownstreamChunkX = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology") int32 DownstreamChunkY = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology") bool bFreshSurfaceWater = false;
};

/**
 * Deterministic presentation projection derived from authoritative Core
 * hydrology. Grid positions/size hints help Unreal Water build a stable local
 * surface representation, but they do not create or mutate water simulation.
 */
USTRUCT(BlueprintType)
struct FLLCoreSurfaceWaterPresentationObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology|Presentation") bool bAvailable = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology|Presentation") int64 SurfaceWaterId = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology|Presentation") ELLCoreSurfaceWaterKind SurfaceKind = ELLCoreSurfaceWaterKind::None;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology|Presentation") ELLCoreWaterSalinity Salinity = ELLCoreWaterSalinity::Fresh;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology|Presentation") int32 ChunkX = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology|Presentation") int32 ChunkY = 0;

    // Stable local-surface anchor at the authoritative chunk center.
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology|Presentation") int32 CenterGridX = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology|Presentation") int32 CenterGridY = 0;

    // Linear surface water (spring/stream/river) can connect toward the
    // authoritative downstream chunk. Consumers must ignore these when false.
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology|Presentation") bool bLinearChannel = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology|Presentation") bool bHasDownstreamTarget = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology|Presentation") int32 DownstreamCenterGridX = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology|Presentation") int32 DownstreamCenterGridY = 0;

    // Rendering hints only. They are deterministic functions of authoritative
    // water kind/availability/flow and are not gameplay width/depth claims.
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology|Presentation") float SuggestedChannelWidthCells = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology|Presentation") float SuggestedAreaRadiusCells = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology|Presentation") float SurfaceAvailability = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology|Presentation") float FlowPotential = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Hydrology|Presentation") bool bFreshSurfaceWater = false;
};

USTRUCT(BlueprintType)
struct FLLCoreNaturalObstacleObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Obstacle") int64 ObstacleId = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Obstacle") ELLCoreNaturalObstacleKind Kind = ELLCoreNaturalObstacleKind::Tree;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Obstacle") int64 SourceResourceNodeId = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Obstacle") int32 GridX = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Obstacle") int32 GridY = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Obstacle") float OffsetXCells = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Obstacle") float OffsetYCells = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Obstacle") float HalfExtentXCells = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Obstacle") float HalfExtentYCells = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Obstacle") float HalfHeightCells = 0.0f;
};

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

    // Stable deterministic presentation seed projected from the Core patch
    // detail stream. PCG may use it for decorative scatter without becoming
    // resource authority.
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Presentation") int64 VisualSeed = 0;
};

USTRUCT(BlueprintType)
struct FLLCoreNaturalChunkObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration") bool bMaterialized = false;

    // Stable deterministic presentation seed derived from the authoritative
    // generated chunk seed. Intended for repeatable PCG decoration only.
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Presentation") int64 VisualSeed = 0;
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
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|WorldGeneration|Obstacle") TArray<FLLCoreNaturalObstacleObservation> PhysicalObstacles;
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
