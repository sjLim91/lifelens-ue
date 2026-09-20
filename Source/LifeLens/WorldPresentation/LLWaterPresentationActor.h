#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LLWaterPresentationActor.generated.h"

class AActor;

/**
 * Runtime projection of authoritative Core hydrology into Unreal Water bodies.
 *
 * This actor is presentation-only: it never creates Core water/resources,
 * never changes salinity/flow/availability, and disables gameplay collision on
 * the spawned Water actors. Coast/ocean use bounded Local Surface polygons;
 * the later Planetary layer may use a true globe/ocean representation without
 * changing Core hydrology authority.
 */
UCLASS(Config=Game, DefaultConfig)
class LIFELENS_API ALLWaterPresentationActor : public AActor
{
    GENERATED_BODY()

public:
    ALLWaterPresentationActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void RefreshFromCore(bool bForce);
    void ClearProjectedWater();
    void EnsureWaterZone();
    FVector GridToWorld(
        int32 GridX,
        int32 GridY,
        int32 InitialCenterGridX,
        int32 InitialCenterGridY,
        float SurfaceZUU) const;
    float WaterSurfaceZForChunk(
        class ULLCoreBridgeSubsystem* Bridge,
        const struct FLLCoreWorldGenerationObservation& World,
        int32 ChunkX,
        int32 ChunkY) const;
    int32 ChunkCoordForGrid(int32 GridCoordinate) const;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Water", meta=(ClampMin="0.25", ClampMax="10.0"))
    float RefreshIntervalSeconds = 2.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Water", meta=(ClampMin="-1000.0", ClampMax="1000.0"))
    float WaterSurfaceZUU = 4.0f;

    // Keep in visual lock-step with the current WorldPresentation macro relief.
    // Water uses Core terrain observations; this is rendering height only.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Water", meta=(ClampMin="0.0", ClampMax="500.0"))
    float TerrainReliefAmplitudeUU = 180.0f;

    // Must stay aligned with LLWorldPresentation regional relief policy so
    // a river continuing into an unmaterialized preview chunk follows the same
    // mountains/valleys instead of falling back to the flat local-water plane.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Water", meta=(ClampMin="0.0", ClampMax="5000.0"))
    float RegionalTerrainReliefAmplitudeUU = 1100.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Water", meta=(ClampMin="1", ClampMax="16"))
    int32 RegionalTerrainPreviewRadiusChunks = 8;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Water", meta=(ClampMin="0", ClampMax="15"))
    int32 RegionalTerrainInnerFlatRingChunks = 1;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Water", meta=(ClampMin="8000.0", ClampMax="250000.0"))
    float WaterZoneExtentUU = 96000.0f;

    float RefreshAccumulator = 0.0f;
    uint32 BuiltSignature = 0;
    bool bBuiltOnce = false;

    UPROPERTY()
    TArray<TObjectPtr<AActor>> SpawnedWaterActors;

    UPROPERTY()
    TObjectPtr<AActor> SpawnedWaterZone;
};
