#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LLWorldPresentationActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class ULLCoreBridgeSubsystem;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
struct FLLCoreNaturalChunkObservation;

// World Visual Milestone A — generated-world natural presentation.
//
// Presentation only. Every placement fact comes from the authoritative world
// generation read contract (`ULLCoreBridgeSubsystem::GetWorldGenerationObservation`
// / `GetNaturalChunkObservation`) through the shared spatial contract in
// `World/LLWorldSpatialContract.h`. This actor never writes simulation state,
// never decides that a resource exists, and never moves Core coordinates to
// fit the visuals.
//
// Determinism: the same WorldSeed + GenerationVersion + chunk coordinates
// always produce the same dressing, so a reloaded world looks identical. The
// population seed is deliberately not an input.
UCLASS()
class LIFELENS_API ALLWorldPresentationActor : public AActor
{
    GENERATED_BODY()

public:
    ALLWorldPresentationActor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    // ---- Android budget ------------------------------------------------------
    // Hard caps so a large materialized region cannot grow the instance count
    // without bound.
    static constexpr int32 MaxTreeInstances  = 700;
    static constexpr int32 MaxShrubInstances = 900;
    static constexpr int32 MaxGrassInstances = 2600;
    static constexpr int32 MaxRockInstances  = 900;

    // Per-chunk dressing budget, scaled by the chunk's own fertility/moisture.
    static constexpr int32 MaxTreesPerChunk  = 110;
    static constexpr int32 MaxShrubsPerChunk = 140;
    static constexpr int32 MaxGrassPerChunk  = 420;
    static constexpr int32 MaxRocksPerChunk  = 130;

    static constexpr float TreeCullStartUU = 14000.0f;
    static constexpr float TreeCullEndUU   = 30000.0f;
    static constexpr float SmallCullStartUU = 4500.0f;
    static constexpr float SmallCullEndUU   = 13000.0f;

    // How often the presentation re-reads the generation observation. Chunks
    // are materialized rarely, so this stays cheap.
    static constexpr float RefreshIntervalSeconds = 2.0f;

    // ---- Start-region readability -------------------------------------------
    // Presentation-only rule: ambient dressing vegetation is not placed right
    // on top of the founders, so the observer can see them. It changes nothing
    // about what Core says exists. Authoritative resource patches are never
    // suppressed by it; they are only drawn smaller inside the radius.
    //
    // The centre is the Core start region (`InitialCenterGrid`), never the
    // Unreal world origin, and the radius is presentation tuning: it is not
    // part of any Core rule and never reaches the save identity.
    // Tuned against the authoritative chunk span: one chunk is 32 cells, so the
    // start chunk is 3,200 UU across. A fully clear 800 UU circle plus a 700 UU
    // fade leaves the outer part of the start chunk dressed.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0"))
    float StartRegionClearRadiusUU = 800.0f;

    // Dressing fades back in over this band instead of stopping at a hard edge.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0"))
    float StartRegionClearFalloffUU = 700.0f;

    // Authoritative resource patches stay visible inside the radius at this
    // fraction of their normal size, so they read as present but do not block
    // the view of the founders.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.1", ClampMax="1.0"))
    float StartRegionResourceScale = 0.55f;

private:
    UHierarchicalInstancedStaticMeshComponent* AddInstancedComponent(
        const TCHAR* Name, UStaticMesh* Mesh, float CullStartUU, float CullEndUU, bool bCastShadow);

    void RefreshFromCore(bool bForce);
    void ClearInstances();
    void BuildGround(const struct FLLCoreWorldGenerationObservation& World);
    void BuildChunkDressing(const struct FLLCoreWorldGenerationObservation& World,
                            const FLLCoreNaturalChunkObservation& Chunk);
    FVector ChunkOriginUU(const struct FLLCoreWorldGenerationObservation& World,
                          int32 ChunkX, int32 ChunkY) const;

    // Fraction of ambient dressing kept at a location: 0 next to the founders,
    // 1 outside the readability band. Deterministic and position-only.
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

    // Runtime.
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Ground;
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> TreeInstances;
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> ShrubInstances;
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> GrassInstances;
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> RockInstances;

    float RefreshAccumulator = 0.0f;
    int64 BuiltWorldSeed = 0;
    int32 BuiltGenerationVersion = -1;
    int32 BuiltChunkCount = -1;
    int32 PlacedTrees = 0;
    int32 PlacedShrubs = 0;
    int32 PlacedGrass = 0;
    int32 PlacedRocks = 0;
    int32 SuppressedDressing = 0;
};
