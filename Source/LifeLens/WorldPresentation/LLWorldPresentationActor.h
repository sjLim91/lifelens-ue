#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LLWorldPresentationActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class ULLCoreBridgeSubsystem;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
struct FLLCoreCivilizationWorldObservation;
struct FLLCoreNaturalChunkObservation;

// Generated-world presentation. Natural dressing and constructed facilities are
// read-only projections of authoritative Core observations. Presentation never
// invents simulation objects, changes Core coordinates, or grants facility use.
UCLASS()
class LIFELENS_API ALLWorldPresentationActor : public AActor
{
    GENERATED_BODY()

public:
    ALLWorldPresentationActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    // ---- Android budget ------------------------------------------------------
    static constexpr int32 MaxTreeInstances  = 700;
    static constexpr int32 MaxShrubInstances = 900;
    static constexpr int32 MaxGrassInstances = 2600;
    static constexpr int32 MaxRockInstances  = 900;

    static constexpr int32 MaxTreesPerChunk  = 110;
    static constexpr int32 MaxShrubsPerChunk = 140;
    static constexpr int32 MaxGrassPerChunk  = 420;
    static constexpr int32 MaxRocksPerChunk  = 130;

    static constexpr float TreeCullStartUU = 14000.0f;
    static constexpr float TreeCullEndUU   = 30000.0f;
    static constexpr float SmallCullStartUU = 4500.0f;
    static constexpr float SmallCullEndUU   = 13000.0f;
    static constexpr float FacilityCullStartUU = 7000.0f;
    static constexpr float FacilityCullEndUU = 18000.0f;

    static constexpr float RefreshIntervalSeconds = 2.0f;

    // ---- Start-region readability -------------------------------------------
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0"))
    float StartRegionClearRadiusUU = 800.0f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0"))
    float StartRegionClearFalloffUU = 700.0f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.1", ClampMax="1.0"))
    float StartRegionResourceScale = 0.55f;

private:
    UHierarchicalInstancedStaticMeshComponent* AddInstancedComponent(
        const TCHAR* Name, UStaticMesh* Mesh, float CullStartUU, float CullEndUU, bool bCastShadow);

    void RefreshFromCore(bool bForce);
    void ClearInstances();
    void ClearFacilityInstances();
    void BuildGround(const struct FLLCoreWorldGenerationObservation& World);
    void BuildChunkDressing(const struct FLLCoreWorldGenerationObservation& World,
                            const FLLCoreNaturalChunkObservation& Chunk);
    void BuildFacilities(const struct FLLCoreWorldGenerationObservation& World,
                         const FLLCoreCivilizationWorldObservation& Civilization);
    uint32 FacilitySignature(const FLLCoreCivilizationWorldObservation& Civilization) const;
    FVector ChunkOriginUU(const struct FLLCoreWorldGenerationObservation& World,
                          int32 ChunkX, int32 ChunkY) const;

    float AmbientDressingKeepFactor(const FVector2D& LocationUU) const;
    UMaterialInterface* GroundMaterialForChunk(const FLLCoreNaturalChunkObservation& Chunk) const;

    // Catalogue (referenced in the constructor so it is cooked).
    UPROPERTY() TArray<TObjectPtr<UStaticMesh>> TreeMeshes;
    UPROPERTY() TArray<TObjectPtr<UStaticMesh>> ShrubMeshes;
    UPROPERTY() TArray<TObjectPtr<UStaticMesh>> GrassMeshes;
    UPROPERTY() TArray<TObjectPtr<UStaticMesh>> RockMeshes;
    UPROPERTY() TObjectPtr<UStaticMesh> GroundMesh;
    UPROPERTY() TObjectPtr<UMaterialInterface> GroundGrass;
    UPROPERTY() TObjectPtr<UMaterialInterface> GroundDry;
    UPROPERTY() TObjectPtr<UMaterialInterface> GroundTransition;

    // Natural runtime presentation.
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Ground;
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> TreeInstances;
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> ShrubInstances;
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> GrassInstances;
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> RockInstances;

    // Facility presentation uses simple Engine cube composition until a suitable
    // CC0 prop set is added. These components are visual-only and collision-free.
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FacilityFoundationInstances;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FacilityPostInstances;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FacilityRoofInstances;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FacilityCargoInstances;

    float RefreshAccumulator = 0.0f;
    int64 BuiltWorldSeed = 0;
    int32 BuiltGenerationVersion = -1;
    int32 BuiltChunkCount = -1;
    uint32 BuiltFacilitySignature = 0;
    bool bBuiltFacilityPresentation = false;
    int32 PlacedTrees = 0;
    int32 PlacedShrubs = 0;
    int32 PlacedGrass = 0;
    int32 PlacedRocks = 0;
    int32 SuppressedDressing = 0;
};
