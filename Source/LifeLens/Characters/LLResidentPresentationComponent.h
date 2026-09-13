#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/LLTypes.h"
#include "LLResidentPresentationComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

// Presentation layer of a resident (DQ-02). Adds, never replaces:
// - a silhouette (cylinder torso + sphere head, one matte unlit colour),
//   height scaled by LifeStage, torso width by sex;
// - a selection ring at the feet, visible only for the observed resident,
//   brighter at LEVEL 2 than LEVEL 1;
// - name label LOD by camera distance (near: name + LifeStage badge,
//   mid: name, far: hidden), always facing the camera, size scaled by distance.
// Reads resident data through ULLSimulationSubsystem / ULLObservationSubsystem only.
UCLASS(ClassGroup=(LifeLens), meta=(BlueprintSpawnableComponent))
class LIFELENS_API ULLResidentPresentationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULLResidentPresentationComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ---- Silhouette (adult, unscaled; multiplied by the LifeStage factor) ----
    // Engine Cylinder is 100 tall with radius 50; Sphere radius 50.
    static constexpr float TorsoHeight       = 138.0f;
    static constexpr float TorsoRadiusMale   = 31.0f;
    static constexpr float TorsoRadiusFemale = 28.0f;
    static constexpr float HeadRadius        = 17.0f;

    // Height factor per LifeStage (index = ELLLifeStage).
    static constexpr float StageHeightFactor[5] = { 0.45f, 0.65f, 0.85f, 1.0f, 0.95f };

    // ---- Selection ring ------------------------------------------------------
    static constexpr float RingOuterRadius = 65.0f;
    static constexpr float RingInnerRadius = 53.0f;
    static constexpr float RingThickness   = 2.0f;
    static constexpr float RingQuickIntensity  = 0.55f;
    static constexpr float RingDetailIntensity = 1.0f;

    // ---- Name label LOD (camera distance, world units) ----------------------
    static constexpr float LabelNearDistance = 2400.0f; // name + LifeStage badge
    static constexpr float LabelMidDistance  = 4500.0f; // name only; beyond: hidden
    static constexpr float LabelReferenceDistance = 1900.0f;
    static constexpr float LabelBaseWorldSize = 28.0f;
    static constexpr float LabelMinScale = 0.6f;
    static constexpr float LabelMaxScale = 2.0f;
    static constexpr float LabelAboveHead = 30.0f;

    static constexpr float DataRefreshSeconds = 1.0f;

private:
    void BuildSilhouette();
    void BuildRing();
    UStaticMeshComponent* AddMesh(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInstanceDynamic*& OutMaterial, const FLinearColor& Color);
    void RefreshResidentData();
    void ApplySilhouetteScale();
    void UpdateRing();
    void UpdateLabel();

    float FeetOffset() const; // distance from the actor origin down to the feet
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

    ELLLifeStage LifeStage = ELLLifeStage::Adult;
    ELLSex Sex = ELLSex::Male;
    FString DisplayName;
    float StageFactor = 1.0f;
    float DataRefreshTimer = 0.0f;
    bool bResidentDataValid = false;
};
