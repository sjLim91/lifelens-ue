#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LLDesktopTerrainPresentationActor.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;
struct FLLCoreNaturalChunkObservation;
struct FLLCoreTerrainPresentationObservation;
struct FLLCoreWorldGenerationObservation;
struct FLLCoreCivilizationWorldObservation;

/**
 * Desktop-only smooth terrain overlay built from authoritative Core terrain
 * presentation observations.
 *
 * It is visual-only: no collision, navigation or terrain gameplay authority.
 * The existing broad/Android ground path remains the fallback.
 */
UCLASS(Config=Game, DefaultConfig)
class LIFELENS_API ALLDesktopTerrainPresentationActor : public AActor
{
    GENERATED_BODY()

public:
    ALLDesktopTerrainPresentationActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    void RefreshFromCore(bool bForce);
    uint32 BuildSignature(
        const TArray<FLLCoreTerrainPresentationObservation>& Terrains) const;
    float ReliefBlend(
        const FVector2D& LocationUU,
        const TArray<FVector2D>& FacilityCentersUU) const;
    float SurfaceZUU(
        const FLLCoreWorldGenerationObservation& World,
        const FLLCoreTerrainPresentationObservation& Terrain,
        const FVector2D& LocationUU,
        const TArray<FVector2D>& FacilityCentersUU) const;
    UMaterialInterface* MaterialForChunk(
        const FLLCoreNaturalChunkObservation& Chunk) const;

    // Not a UPROPERTY on purpose: the component is an Actor default subobject
    // already owned by AActor, while avoiding a generated Android reference to
    // the desktop-only ProceduralMeshComponent module.
    UProceduralMeshComponent* TerrainMesh = nullptr;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> GroundGrass;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> GroundDry;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> GroundTransition;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Terrain", meta=(ClampMin="2", ClampMax="24"))
    int32 SubdivisionsPerChunk = 10;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Terrain", meta=(ClampMin="0.0", ClampMax="500.0"))
    float TerrainReliefAmplitudeUU = 180.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Terrain", meta=(ClampMin="0.0", ClampMax="5000.0"))
    float SettlementFlattenRadiusUU = 2400.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Terrain", meta=(ClampMin="100.0", ClampMax="6000.0"))
    float SettlementBlendBandUU = 1800.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Terrain", meta=(ClampMin="0.0", ClampMax="2000.0"))
    float FacilityFlattenRadiusUU = 360.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Terrain", meta=(ClampMin="50.0", ClampMax="2500.0"))
    float FacilityBlendBandUU = 520.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Terrain", meta=(ClampMin="0.0", ClampMax="10.0"))
    float SurfaceLiftUU = 1.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|WorldPresentation|Terrain", meta=(ClampMin="0.2", ClampMax="10.0"))
    float RefreshIntervalSeconds = 2.0f;

    float RefreshAccumulator = 0.0f;
    uint32 LastSignature = 0;
    bool bBuiltOnce = false;
};
