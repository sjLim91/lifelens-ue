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

// How strongly a piece of ambient dressing blocks the observer's view of the
// residents. Purely a presentation classification; Core knows nothing about it.
enum class ELLDressingLayer : uint8
{
    Canopy,       // full-height trees: the worst offenders
    Undergrowth,  // shrubs, ferns, tall grass: block at resident height
    GroundDetail, // rocks and pebbles: low enough to keep everywhere
};

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

    // ---- Settlement readability envelope -------------------------------------
    // Presentation-only rule. Residents live and act within a few thousand
    // units of the settlement, and natural forest density there hides both the
    // residents and what they are doing. Ambient dressing therefore thins out
    // near the settlement and returns to full natural density outside it.
    //
    // This changes nothing about what Core says exists: resident movement and
    // action range are untouched, and authoritative resource patches are never
    // removed. The reference point is the Core start region
    // (`InitialCenterGrid`), never a fixed world origin, and every value here
    // is presentation tuning that never reaches Core rules or the save
    // identity.
    //
    // Three bands, measured from the settlement reference:
    //   0 .. CoreClearRadiusUU        living core; ambient canopy and
    //                                 undergrowth almost fully suppressed
    //   CoreClear .. ActivityRadius   activity zone; density restored with
    //                                 distance, canopy last
    //   beyond ActivityRadius         untouched natural density
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0"))
    float CoreClearRadiusUU = 1200.0f;

    // Matches the observed resident activity range. Core movement is not
    // clamped to it; it only says how far the visual thinning reaches.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0"))
    float ActivityRadiusUU = 3000.0f;

    // Recovery curves across the activity band. A higher exponent keeps the
    // zone open for longer; canopy recovers latest because it blocks the most.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="1.0", ClampMax="6.0"))
    float CanopyRecoveryExponent = 2.6f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="1.0", ClampMax="6.0"))
    float UndergrowthRecoveryExponent = 1.5f;

    // A little dressing survives even in the living core so the settlement does
    // not read as a cut clearing. Low enough to stay see-through.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0", ClampMax="0.3"))
    float CoreZoneCanopyKeep = 0.04f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0", ClampMax="0.5"))
    float CoreZoneUndergrowthKeep = 0.10f;

    // Authoritative resource patches are never hidden. Inside the settlement
    // they are only drawn smaller so they read as present without blocking the
    // view of the residents.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.1", ClampMax="1.0"))
    float CoreZoneResourceScale = 0.55f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.1", ClampMax="1.0"))
    float ActivityZoneResourceScale = 0.8f;

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

    // Fraction of ambient dressing kept at a location for one layer: low in
    // the settlement core, restored with distance, 1 outside the envelope.
    // Deterministic and position-only.
    float AmbientDressingKeepFactor(const FVector2D& LocationUU, ELLDressingLayer Layer) const;

    // Presentation-space position of the Core start region centre
    // (`InitialCenterGrid`). Not assumed to be the world origin.
    FVector2D SettlementReferenceUU(const struct FLLCoreWorldGenerationObservation& World) const;

    // Size factor for an authoritative resource patch by band. Never zero: a
    // resource Core says exists is not hidden for readability.
    float ResourcePatchScaleFactor(const FVector2D& LocationUU) const;
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
    FVector2D CachedSettlementReferenceUU = FVector2D::ZeroVector;
};
