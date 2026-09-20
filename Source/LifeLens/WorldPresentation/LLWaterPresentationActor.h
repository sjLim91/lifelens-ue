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
    float WaterSurfaceZForGrid(
        class ULLCoreBridgeSubsystem* Bridge,
        const struct FLLCoreWorldGenerationObservation& World,
        const TArray<struct FLLCoreTerrainPresentationObservation>& RegionalTerrains,
        int32 GridX,
        int32 GridY,
        const TArray<FVector2D>& FacilityCentersUU) const;
    int32 ChunkCoordForGrid(int32 GridCoordinate) const;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Water", meta=(ClampMin="0.25", ClampMax="10.0"))
    float RefreshIntervalSeconds = 2.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Water", meta=(ClampMin="-1000.0", ClampMax="1000.0"))
    float WaterSurfaceZUU = 4.0f;

    // Local terrain height/flattening is centralized in
    // LLTerrainPresentationContract; Water only owns its small surface lift.

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
