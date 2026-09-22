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
struct FLLCoreTerrainPresentationObservation;

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

    // Visual-only far world envelope. These instances never represent
    // interactable Core resources and carry no collision/navigation authority.
#if PLATFORM_ANDROID
    static constexpr int32 MaxFarTreeInstances = 180;
    static constexpr int32 MaxFarRockInstances = 96;
#else
    static constexpr int32 MaxFarTreeInstances = 320;
    static constexpr int32 MaxFarRockInstances = 160;
#endif
    static constexpr float FarDressingCullStartUU = 10000.0f;
    static constexpr float FarDressingCullEndUU = 72000.0f;

    static constexpr float TreeCullStartUU = 12000.0f;
    static constexpr float TreeCullEndUU   = 26000.0f;
    // High-poly photoreal hero canopy is intentionally local-view only.
    static constexpr float HeroTreeCullStartUU = 5500.0f;
    static constexpr float HeroTreeCullEndUU = 14000.0f;
    static constexpr float SmallCullStartUU = 3800.0f;
    static constexpr float SmallCullEndUU   = 10500.0f;
    static constexpr float FacilityCullStartUU = 7000.0f;
    static constexpr float FacilityCullEndUU = 18000.0f;

    static constexpr float RefreshIntervalSeconds = 0.5f;

    // ---- Platform visual density --------------------------------------------
#if PLATFORM_ANDROID
    static constexpr int32 MaxTreeInstances  = 620;
    static constexpr int32 MaxHeroTreeInstances = 0;
    static constexpr int32 MaxShrubInstances = 760;
    static constexpr int32 MaxGrassInstances = 1800;
    static constexpr int32 MaxRockInstances  = 720;

    static constexpr int32 MaxTreesPerChunk  = 96;
    static constexpr int32 MaxHeroTreesPerChunk = 0;
    static constexpr int32 MaxShrubsPerChunk = 115;
    static constexpr int32 MaxGrassPerChunk  = 300;
    static constexpr int32 MaxRocksPerChunk  = 105;
#else
    // Desktop cinematic tier keeps substantially more ambient dressing while
    // HISM culling and the observer readability envelope bound visible cost.
    static constexpr int32 MaxTreeInstances  = 1480;
    // island_tree_02 is a ~2M-triangle hero source mesh. Keep it very sparse
    // and never use it for the far-world ring.
    static constexpr int32 MaxHeroTreeInstances = 8;
    static constexpr int32 MaxShrubInstances = 2100;
    static constexpr int32 MaxGrassInstances = 6400;
    static constexpr int32 MaxRockInstances  = 1650;

    static constexpr int32 MaxTreesPerChunk  = 180;
    static constexpr int32 MaxHeroTreesPerChunk = 1;
    static constexpr int32 MaxShrubsPerChunk = 250;
    static constexpr int32 MaxGrassPerChunk  = 780;
    static constexpr int32 MaxRocksPerChunk  = 210;
#endif

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

    // ---- Authority-derived readability -------------------------------------
    // The initial spawn region is only a coordinate frame / world-entry point.
    // It must not become a presentation-owned living zone.
    //
    // Natural dressing is thinned only around authoritative facilities, while
    // camera-to-resident canopy occlusion is handled by the reversible dynamic
    // visibility path. The legacy radius properties remain serialized for
    // compatibility with existing configs but are intentionally presentation
    // no-ops and must not regain authority.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0"))
    float CoreClearRadiusUU = 360.0f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0"))
    float ActivityRadiusUU = 1350.0f;

    // Facility-local recovery curves. Canopy recovers latest because it blocks
    // residents and their actions most strongly.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="1.0", ClampMax="6.0"))
    float CanopyRecoveryExponent = 1.7f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="1.0", ClampMax="6.0"))
    float UndergrowthRecoveryExponent = 1.2f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0", ClampMax="0.8"))
    float CoreZoneCanopyKeep = 0.30f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.0", ClampMax="1.0"))
    float CoreZoneUndergrowthKeep = 0.48f;

    // Authoritative resource patches are never hidden. They may be drawn
    // smaller only inside an actual facility readability envelope.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.1", ClampMax="1.0"))
    float CoreZoneResourceScale = 0.32f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability", meta=(ClampMin="0.1", ClampMax="1.0"))
    float ActivityZoneResourceScale = 0.62f;

    // Actual facilities open local readability envelopes. Their radii are
    // shared with Water/Desktop terrain through LLTerrainPresentationContract.

    // ---- Visual-only far world envelope --------------------------------------
    // The authoritative simulation only materializes nearby chunks. The
    // observer camera can see much farther, so a separate collision-free
    // horizon surface prevents the bootstrap/materialized square from floating
    // in grey void. It never creates Core chunks/resources/facilities.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|FarWorld", meta=(ClampMin="16.0", ClampMax="96.0"))
    float FarGroundMinSpanChunks = 72.0f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|FarWorld", meta=(ClampMin="2.0", ClampMax="12.0"))
    float FarGroundActiveSpanMultiplier = 6.0f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|FarWorld", meta=(ClampMin="0.0", ClampMax="8.0"))
    float FarGroundDropUU = 2.0f;

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|FarWorld", meta=(ClampMin="0.2", ClampMax="1.0"))
    float FarDressingOuterRadiusFraction = 0.47f;

    // Regional relief radius/amplitude/inner ring live in
    // LLTerrainPresentationContract so Water and horizon terrain share one
    // presentation shape contract.

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|FarWorld", meta=(ClampMin="1", ClampMax="3"))
    int32 RegionalTerrainTilesPerChunk = 2;

    // ---- Gentle authoritative terrain relief --------------------------------
    // The bootstrap collision plane and resident locomotion remain flat around
    // the active settlement. Outside that readability envelope, Core macro
    // elevation is projected into gentle visual relief so the world no longer
    // reads as a perfectly flat board.
    // Local terrain height/flattening values live in
    // LLTerrainPresentationContract so terrain, water and desktop projection
    // cannot silently diverge.

    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Terrain", meta=(ClampMin="0.0", ClampMax="12.0"))
    float TerrainMaxTiltDegrees = 5.5f;

    // ---- Legacy initial sight-line compatibility -----------------------------
    // Kept serialized so old configs load cleanly. The runtime function now
    // returns 1.0 unconditionally; initial spawn/camera never clears ecology.
    UPROPERTY(EditAnywhere, Category="LifeLens|WorldPresentation|Readability")
    bool bClearInitialSightlineCanopy = false;

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
    void AddPhotorealStructureLog(const FVector& CenterUU,
                                  const FVector& DirectionUU,
                                  float LengthUU,
                                  float DiameterUU);
    void AddPhotorealFurnaceStone(const FVector& CenterUU,
                                  float DiameterUU,
                                  float YawDegrees,
                                  float VerticalScale = 1.0f);

    void RefreshFromCore(bool bForce);
    void ClearInstances();
    void ClearFacilityInstances();
    void ApplyFacilityMaterialPalette();
    void BuildGround(
        const struct FLLCoreWorldGenerationObservation& World,
        const TArray<FLLCoreNaturalChunkObservation>& MaterializedChunks,
        const TArray<FLLCoreTerrainPresentationObservation>& RegionalTerrains,
        const FIntPoint& ObserverCenterChunk);
    void BuildRegionalTerrainPreview(
        const struct FLLCoreWorldGenerationObservation& World,
        const TArray<FLLCoreTerrainPresentationObservation>& RegionalTerrains,
        const TArray<FLLCoreNaturalChunkObservation>& MaterializedChunks);
    void BuildChunkGround(
        const struct FLLCoreWorldGenerationObservation& World,
        const FLLCoreNaturalChunkObservation& Chunk,
        const FLLCoreTerrainPresentationObservation& Terrain);
    void BuildFarEnvironment(
        const struct FLLCoreWorldGenerationObservation& World,
        float ActiveGroundSpanUU,
        float FarGroundSpanUU,
        const TArray<FLLCoreTerrainPresentationObservation>& RegionalTerrains,
        const FIntPoint& ObserverCenterChunk,
        const FVector2D& ObserverCenterUU);
    void BuildChunkDressing(
        const struct FLLCoreWorldGenerationObservation& World,
        const FLLCoreNaturalChunkObservation& Chunk,
        const FLLCoreTerrainPresentationObservation& Terrain);
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
    FIntPoint ResolveObserverCenterChunk(
        const struct FLLCoreWorldGenerationObservation& World) const;
    float TerrainReliefBlend(const FVector2D& LocationUU) const;
    float TerrainSurfaceZUU(
        const struct FLLCoreWorldGenerationObservation& World,
        const FLLCoreTerrainPresentationObservation& Terrain,
        const FVector2D& LocationUU) const;
    FRotator TerrainTileRotation(
        const struct FLLCoreWorldGenerationObservation& World,
        const FLLCoreTerrainPresentationObservation& Terrain,
        const FVector2D& CenterUU,
        float SampleSpanUU) const;

    float RegionalTerrainSurfaceZUU(
        const struct FLLCoreWorldGenerationObservation& World,
        const FLLCoreTerrainPresentationObservation& Terrain,
        const FVector2D& LocationUU) const;
    FRotator RegionalTerrainTileRotation(
        const struct FLLCoreWorldGenerationObservation& World,
        const FLLCoreTerrainPresentationObservation& Terrain,
        const FVector2D& CenterUU,
        float SampleSpanUU) const;

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
    // Desktop-only heavyweight canopy catalogue; left empty on Android.
    UPROPERTY() TArray<TObjectPtr<UStaticMesh>> HeroTreeMeshes;
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
    UPROPERTY() TObjectPtr<UStaticMeshComponent> FarGround;

    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RegionalTerrainTileInstances;

    // Authoritative materialized-chunk surface overlay. The broad Ground/FarGround
    // remain continuity underlays; these HISM tiles project each Core chunk's
    // actual moisture/biome surface material so the local world is not one flat
    // initial-chunk carpet.
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GroundGrassTileInstances;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GroundDryTileInstances;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GroundTransitionTileInstances;

    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> TreeInstances;
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HeroTreeInstances;
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> FarTreeInstances;
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> ShrubInstances;
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> GrassInstances;
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> RockInstances;
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> FarRockInstances;

    // Construction-progress and not-yet-upgraded facility structure still uses
    // lightweight modular composition. Approved completed hero props use the
    // photoreal CC0 components below; missing hero art must not silently regress
    // to an obvious Engine primitive substitute. All components are visual-only
    // and collision-free.
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FacilityFoundationInstances;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FacilityPostInstances;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FacilityRoofInstances;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FacilityCargoInstances;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FacilityAccentInstances;

    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> PhotorealFirePitInstances;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> PhotorealStorageBasketInstances;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> PhotorealWorkToolInstances;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> PhotorealStructureLogInstances;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> PhotorealFurnaceStoneInstances;

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
    int32 BuiltObserverCenterChunkX = MAX_int32;
    int32 BuiltObserverCenterChunkY = MAX_int32;
    int32 BuiltChunkCount = -1;
    uint32 BuiltNaturalChunkSignature = 0;
    uint32 BuiltTerrainPresentationSignature = 0;
    uint32 BuiltRegionalTerrainSignature = 0;
    uint32 BuiltFacilitySignature = 0;
    uint32 BuiltFacilityLayoutSignature = 0;
    uint32 BuiltResourceQuantitySignature = 0;
    bool bBuiltFacilityPresentation = false;
    int32 PlacedTrees = 0;
    int32 PlacedHeroTrees = 0;
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
