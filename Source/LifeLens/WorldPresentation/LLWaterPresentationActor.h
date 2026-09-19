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
 * the spawned Water actors. Coast/ocean geometry remains deferred until the
 * planetary/coastline contract is rich enough to avoid flooding the local map.
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
        int32 InitialCenterGridY) const;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Water", meta=(ClampMin="0.25", ClampMax="10.0"))
    float RefreshIntervalSeconds = 2.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Water", meta=(ClampMin="-1000.0", ClampMax="1000.0"))
    float WaterSurfaceZUU = 4.0f;

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
