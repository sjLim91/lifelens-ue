#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LLWorldPresentationActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class ULLCoreBridgeSubsystem;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;
struct FLLCoreCivilizationWorldObservation;
struct FLLCoreNaturalChunkObservation;

// How strongly a piece of ambient dressing blocks the observer's view of the
// residents. Purely a presentation classification; Core knows nothing about it.
enum class ELLDressingLayer : uint8
{
    Canopy,       // full-height trees: the worst offenders
    Undergrowth,  // shrubs, ferns, tall grass: block at resident height
    GroundDetail, // rocks and pebbles: low enough to keep everywhere
};

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
    // Visual Catch-up v2 tightens the ambient HISM envelope before adding
    // weather VFX. Resource/facility truth is untouched; these values only
    // cap presentation density and distance so precipitation/post-process has
    // headroom on the Android target.
    static constexpr int32 MaxTreeInstances  = 620;
    static constexpr int32 MaxShrubInstances = 760;
    static constexpr int32 MaxGrassInstances = 1800;
    static constexpr int32 MaxRockInstances  = 720;

    static constexpr int32 MaxTreesPerChunk  = 96;
    static constexpr int32 MaxShrubsPerChunk = 115;
    static constexpr int32 MaxGrassPerChunk  = 300;
    static constexpr int32 MaxRocksPerChunk  = 105;

    static constexpr float TreeCullStartUU = 12000.0f;
    static constexpr float TreeCullEndUU   = 26000.0f;
    static constexpr float SmallCullStartUU = 3800.0f;
    static constexpr float SmallCullEndUU   = 10500.0f;
    static constexpr float FacilityCullStartUU = 7000.0f;
    static constexpr float FacilityCullEndUU = 18000.0f;

    static constexpr float RefreshIntervalSeconds = 2.0f;

    // ---- Dynamic observer canopy visibility ---------------------------------
    // Presentation-only. Ambient canopy that crosses the current camera ->
    // resident sightline is reversibly collapsed, then restored as the observer
    // moves. Authoritative resource-patch trees are intentionally NOT registered
    // here, so this never hides or mutates resource truth.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability")
    bool bDynamicObserverCanopyVisibility = true;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="40.0", ClampMax="800.0"))
    float DynamicCanopyHideRadiusUU = 220.0f;

    // Hysteresis prevents a tree from flickering as a camera/resident sightline
    // skims the edge of the hide corridor.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="40.0", ClampMax="1200.0"))
    float DynamicCanopyRestoreRadiusUU = 300.0f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.001", ClampMax="0.20"))
    float DynamicCanopyHiddenScale = 0.02f;

    // Bounds the Android cost when population becomes large. Nearest residents
    // to the observer get readability priority; Core population is untouched.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="1", ClampMax="64"))
    int32 MaxDynamicVisibilityTargets = 24;

    // ---- Settlement readability envelope -------------------------------------
    // Presentation-only. Residents live and act within a few thousand units of
    // the settlement, and natural forest density there hides both the residents
    // and what they are doing. Ambient dressing thins out near the settlement
    // and returns to full natural density outside it.
    //
    // Core is untouched: resident movement and action range are unchanged, and
    // authoritative resource patches are never removed. The reference point is
    // the Core start region (`InitialCenterGrid`), never a fixed world origin.
    //
    //   0 .. CoreClearRadiusUU        living core; canopy and undergrowth
    //                                 almost fully suppressed
    //   CoreClear .. ActivityRadius   activity zone; density restored with
    //                                 distance, canopy last
    //   beyond ActivityRadius         untouched natural density
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0"))
    float CoreClearRadiusUU = 1200.0f;

    // Matches the observed resident activity range. Core movement is not
    // clamped to it; it only says how far the visual thinning reaches.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0"))
    float ActivityRadiusUU = 3000.0f;

    // Recovery curves across the activity band. Canopy recovers latest because
    // it blocks the most.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="1.0", ClampMax="6.0"))
    float CanopyRecoveryExponent = 2.6f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="1.0", ClampMax="6.0"))
    float UndergrowthRecoveryExponent = 1.5f;

    // A little dressing survives in the living core so the settlement does not
    // read as a cut clearing.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0", ClampMax="0.3"))
    float CoreZoneCanopyKeep = 0.04f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0", ClampMax="0.5"))
    float CoreZoneUndergrowthKeep = 0.10f;

    // Authoritative resource patches are never hidden; inside the settlement
    // they are only drawn smaller.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.1", ClampMax="1.0"))
    float CoreZoneResourceScale = 0.55f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.1", ClampMax="1.0"))
    float ActivityZoneResourceScale = 0.8f;

    // As civilization expands away from the initial settlement center, each
    // authoritative facility opens a smaller local readability envelope.
    // This prevents later neighborhoods from disappearing inside untouched
    // ambient forest without inventing roads, zoning or simulation clearing.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0", ClampMax="1600.0"))
    float FacilityClearRadiusUU = 420.0f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0", ClampMax="3000.0"))
    float FacilityActivityRadiusUU = 1050.0f;

    // ---- Initial sight line (IR-E-1 mitigation) --------------------------------
    // Residents are visible the moment play starts and then disappear behind the
    // canopy that loads between the observer camera and the settlement. This
    // clears canopy inside a cone from the initial camera position toward the
    // settlement reference.
    //
    // Frozen at the initial camera pose, so it is now only the fallback used
    // when Dynamic Observer Canopy Visibility is disabled. The default runtime
    // path keeps all ambient canopy instances and reversibly collapses only the
    // ones currently between the camera and a resident.
    //
    // The camera is read and never moved; Core and world-generation facts are
    // untouched and resource patches are never removed.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability")
    bool bClearInitialSightlineCanopy = true;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="2.0", ClampMax="60.0"))
    float InitialSightlineHalfAngleDegrees = 16.0f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0", ClampMax="40.0"))
    float InitialSightlineEdgeFalloffDegrees = 9.0f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0", ClampMax="0.4"))
    float InitialSightlineCanopyKeep = 0.05f;

private:
    struct FDynamicCanopyInstance
    {
        TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent> Component;
        int32 InstanceIndex = INDEX_NONE;
        FTransform BaseTransform = FTransform::Identity;
        bool bSuppressed = false;
    };

    UHierarchicalInstancedStaticMeshComponent* AddInstancedComponent(
        const TCHAR* Name, UStaticMesh* Mesh, float CullStartUU, float CullEndUU, bool bCastShadow);

    void RefreshFromCore(bool bForce);
    void ClearInstances();
    void ClearFacilityInstances();
    void ApplyFacilityMaterialPalette();
    void BuildGround(const struct FLLCoreWorldGenerationObservation& World);
    void BuildChunkDressing(const struct FLLCoreWorldGenerationObservation& World,
                            const FLLCoreNaturalChunkObservation& Chunk);
    void BuildFacilities(const struct FLLCoreWorldGenerationObservation& World,
                         const FLLCoreCivilizationWorldObservation& Civilization,
                         bool bNightPresentation);
    uint32 FacilitySignature(const FLLCoreCivilizationWorldObservation& Civilization) const;
    uint32 FacilityLayoutSignature(const FLLCoreCivilizationWorldObservation& Civilization) const;
    uint32 ResourceQuantitySignature(const FLLCoreCivilizationWorldObservation& Civilization) const;
    void RefreshFacilityReadabilityReferences(
        const struct FLLCoreWorldGenerationObservation& World,
        const FLLCoreCivilizationWorldObservation& Civilization);
    FVector ChunkOriginUU(const struct FLLCoreWorldGenerationObservation& World,
                          int32 ChunkX, int32 ChunkY) const;

    float AmbientDressingKeepFactor(const FVector2D& LocationUU, ELLDressingLayer Layer) const;
    float FacilityDressingKeepFactor(const FVector2D& LocationUU, ELLDressingLayer Layer) const;
    FVector2D SettlementReferenceUU(const struct FLLCoreWorldGenerationObservation& World) const;
    float ResourcePatchScaleFactor(const FVector2D& LocationUU) const;
    float InitialSightlineKeepFactor(const FVector2D& LocationUU) const;
    bool CaptureInitialViewOrigin();
    void RegisterDynamicCanopyInstance(UHierarchicalInstancedStaticMeshComponent* Component,
                                       int32 InstanceIndex, const FTransform& BaseTransform);
    void UpdateDynamicObserverCanopyVisibility();
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
    UPROPERTY() TObjectPtr<UMaterialInterface> FacilitySurfaceMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> FacilityAccentMaterial;

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
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FacilityAccentInstances;

    // Shared palette materials keep facility silhouettes readable without
    // multiplying draw components per facility. Shape/state still comes solely
    // from authoritative facility DTOs.
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FacilityFoundationMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FacilityPostMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FacilityRoofMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FacilityCargoMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FacilityAccentDynamicMaterial;

    float RefreshAccumulator = 0.0f;
    int64 BuiltWorldSeed = 0;
    int32 BuiltGenerationVersion = -1;
    int32 BuiltChunkCount = -1;
    uint32 BuiltFacilitySignature = 0;
    uint32 BuiltFacilityLayoutSignature = 0;
    uint32 BuiltResourceQuantitySignature = 0;
    bool bBuiltFacilityPresentation = false;
    int32 PlacedTrees = 0;
    int32 PlacedShrubs = 0;
    int32 PlacedGrass = 0;
    int32 PlacedRocks = 0;
    int32 SuppressedDressing = 0;
    FVector2D CachedSettlementReferenceUU = FVector2D::ZeroVector;
    TArray<FVector2D> CachedFacilityReadabilityCentersUU;
    FVector2D InitialViewOriginUU = FVector2D::ZeroVector;
    bool bInitialViewCaptured = false;
    int32 SightlineCleared = 0;
    int32 DynamicCanopySuppressed = 0;
    TArray<FDynamicCanopyInstance> DynamicCanopyInstances;
};
