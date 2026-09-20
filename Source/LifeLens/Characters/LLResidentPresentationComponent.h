#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/LLTypes.h"
#include "LLResidentPresentationComponent.generated.h"

class ULLResidentAppearanceComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

// Presentation layer of a resident (DQ-02).
// Production visuals come from ULLResidentAppearanceComponent. This component
// owns observer readability:
// - an optional QA-only primitive silhouette fallback (disabled by default);
// - a selection ring at the feet, visible only for the observed resident,
//   brighter at LEVEL 2 than LEVEL 1;
// - name label LOD by camera distance (near: name + LifeStage badge,
//   mid: name, far: hidden), always facing the camera, size scaled by distance.
// Reads resident data through ULLSimulationSubsystem / ULLObservationSubsystem only.
UCLASS(Config=Game, DefaultConfig, ClassGroup=(LifeLens), meta=(BlueprintSpawnableComponent))
class LIFELENS_API ULLResidentPresentationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULLResidentPresentationComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Called once the owner knows which resident it represents. The human body
    // can only be built then; a QA silhouette, if explicitly enabled, is
    // removed once the human body becomes available.
    void OnResidentBound();

    // ---- Silhouette (adult, unscaled; multiplied by the LifeStage factor) ----
    // Reference is the DebugBody cube: 55 x 55 x 90, centred on the actor
    // origin. Adult silhouette height (torso + head) equals 90 and its width
    // stays inside 55. Engine Cylinder is 100 tall with radius 50; Sphere radius 50.
    static constexpr float ReferenceBodyHeight = 90.0f;
    static constexpr float ReferenceBodyWidth  = 55.0f;
    // The silhouette occupies the DebugBody's box (origin-centred), so the
    // opaque cube is hidden at runtime while the silhouette is shown. The
    // character's DebugBody component itself is untouched.
    static constexpr bool bHideDebugBody = true;

    // Production policy: never mask a missing human asset with low-quality
    // Engine primitives. Enable explicitly only for QA diagnostics.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Presentation|QA")
    bool bAllowPrimitiveSilhouetteFallback = false;
    static constexpr float SilhouetteBaseZ = -ReferenceBodyHeight * 0.5f;
    static constexpr float HeadRadius        = 8.0f;
    static constexpr float TorsoHeight       = ReferenceBodyHeight - 2.0f * HeadRadius;
    static constexpr float TorsoRadiusMale   = 19.0f;
    static constexpr float TorsoRadiusFemale = 17.0f;

    static constexpr float StageHeightFactor[5] = { 0.45f, 0.65f, 0.85f, 1.0f, 0.95f };

    // ---- Selection ring ------------------------------------------------------
    static constexpr float RingOuterRadius = 36.0f;
    static constexpr float RingInnerRadius = 29.0f;
    static constexpr float RingThickness   = 2.0f;
    static constexpr float RingQuickIntensity  = 0.55f;
    static constexpr float RingDetailIntensity = 1.0f;
    static constexpr float RingDetailRadiusScale = 1.14f;
    static constexpr float RingPulseFraction = 0.055f;
    static constexpr float RingPulseRadiansPerSecond = 4.2f;

    // ---- Name label LOD (camera distance, world units) ----------------------
    static constexpr float LabelNearDistance = 1500.0f;
    static constexpr float LabelMidDistance  = 4500.0f;
    static constexpr float SelectedLabelMaxDistance = 7000.0f;
    static constexpr float LabelReferenceDistance = 1900.0f;
    static constexpr float LabelBaseWorldSize = 14.0f;
    static constexpr float LabelMinScale = 0.6f;
    static constexpr float LabelMaxScale = 2.0f;
    static constexpr float LabelMinLifeStageScale = 0.82f;
    static constexpr float SelectedLabelSizeMultiplier = 1.12f;
    static constexpr float LabelAboveHead = 10.0f;
    static constexpr float CrowdLabelSeparationPixels = 72.0f;
    static constexpr float CrowdLabelRefreshSeconds = 0.30f;
    static constexpr int32 MaxCrowdLabelComparisons = 64;

    static constexpr float DataRefreshSeconds = 1.0f;

private:
    void BuildSilhouette();
    void BuildRing();
    UStaticMeshComponent* AddMesh(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInstanceDynamic*& OutMaterial, const FLinearColor& Color);
    void RefreshResidentData();
    void ApplySilhouetteScale();
    void UpdateRing();
    void RefreshCrowdLabelSuppression();
    void UpdateLabel();

    float FeetOffset() const;
    void HideDebugBody();
    static FString LifeStageBadge(ELLLifeStage Stage);

    UPROPERTY()
    TObjectPtr<UStaticMesh> CylinderMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> SphereMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> UnlitMaterial;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> Torso;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> Head;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> RingOuter;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> RingInner;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> SilhouetteMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> RingOuterMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> RingInnerMaterial;

    UPROPERTY()
    TObjectPtr<UTextRenderComponent> Label;

    // When the owner's appearance component has built a human body, the label
    // sits above that body. Missing production art fails closed; primitive
    // silhouette presentation requires explicit QA opt-in.
    UPROPERTY()
    TObjectPtr<ULLResidentAppearanceComponent> Appearance;

    ELLLifeStage LifeStage = ELLLifeStage::Adult;
    ELLSex Sex = ELLSex::Male;
    int32 AgeYears = 0;
    FString DisplayName;
    float StageFactor = 1.0f;
    float DataRefreshTimer = 0.0f;
    float CrowdLabelRefreshTimer = 0.0f;
    bool bCrowdLabelSuppressed = false;
    bool bResidentDataValid = false;
};
