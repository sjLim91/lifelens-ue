#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LLWaterPresentationActor.generated.h"

class AActor;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

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
    void EnsureFallbackWaterMaterial();
    void AddFallbackWaterChannel(
        const FVector& Start,
        const FVector& End,
        float WidthUU);
    void AddFallbackWaterArea(
        const FVector& Center,
        float RadiusUU);
    FVector GridToWorld(
        int32 GridX,
        int32 GridY,
        int32 InitialCenterGridX,
        int32 InitialCenterGridY,
        float SurfaceZUU) const;
    float WaterSurfaceZForGrid(
        class ULLCoreBridgeSubsystem* Bridge,
        const struct FLLCoreWorldGenerationObservation& World,
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

    // Lightweight safety net underneath Unreal Water bodies. It never owns
    // gameplay collision or hydrology; it only prevents surface water from
    // disappearing completely when a platform Water material/zone fails.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Water|Fallback")
    bool bEnableVisibleWaterFallback = true;

    // The authored/plugin water plane is lifted 4 UU above the terrain. Keep
    // the safety surface slightly below that water plane but still above the
    // opaque terrain; a depth >= WaterSurfaceZUU can bury the fallback.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Water|Fallback", meta=(ClampMin="1.0", ClampMax="40.0"))
    float FallbackWaterDepthBelowSurfaceUU = 2.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Water|Fallback", meta=(ClampMin="1", ClampMax="12"))
    int32 FallbackRiverSegments = 4;

    float RefreshAccumulator = 0.0f;
    uint32 BuiltSignature = 0;
    bool bBuiltOnce = false;

    UPROPERTY()
    TArray<TObjectPtr<AActor>> SpawnedWaterActors;

    UPROPERTY(VisibleAnywhere, Category="LifeLens|WorldPresentation|Water|Fallback")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FallbackChannelInstances;

    UPROPERTY(VisibleAnywhere, Category="LifeLens|WorldPresentation|Water|Fallback")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FallbackAreaInstances;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> FallbackWaterMaterial;

    UPROPERTY()
    TObjectPtr<AActor> SpawnedWaterZone;

    // True only when this presentation actor created the WaterZone. Authored
    // map zones may be reused but must never be destroyed by this actor.
    bool bOwnsWaterZone = false;
};
