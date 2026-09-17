#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Characters/LLResidentAppearanceInputs.h"
#include "LLResidentAppearanceComponent.generated.h"

class UAnimSequence;
class UMaterialInstanceDynamic;
class USkeletalMesh;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture;

// Character Appearance v1 — Track B (Quaternius CC0).
//
// Builds the resident's human body from the imported Universal Base
// Characters assets: sex-specific skeletal mesh on the shared skeleton, skin
// tone (texture variant + tint), eye tint, hair style/colour (static mesh on
// the Head bone), height/build scale, and an idle animation. Everything is
// chosen from FLLResidentAppearanceInputs, so the same WorldSeed + ResidentId
// always produce the same look and Save/Load keeps it.
//
// Presentation only: reads Bridge/compat data, never decides actions and
// never writes authoritative state. Asset paths stay inside this component
// (vendor choice is not part of the inputs contract).
UCLASS(ClassGroup=(LifeLens), meta=(BlueprintSpawnableComponent))
class LIFELENS_API ULLResidentAppearanceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULLResidentAppearanceComponent();

    virtual void BeginPlay() override;

    // Builds the body once; safe to call from other components' BeginPlay.
    void EnsureBuilt();

    bool HasBody() const { return Body != nullptr; }

    // True when the minimum default clothing layer was attached to the body.
    bool HasOutfit() const { return Outfit != nullptr; }

    // Height of the scaled body above the actor origin, in world units
    // (used by the presentation layer to place the label).
    float GetVisualTopOffset() const;

    // Distance from the actor origin down to the feet of the body.
    float GetFeetOffset() const { return FeetOffset; }

    // Lifecycle presentation can resize the authoritative character capsule.
    // Keep the visual body, its cached height, outfit/hair children and label
    // calculations aligned without rebuilding identity/genetics/materials.
    void ApplyLifecyclePresentationScale(float NewFeetOffset, const FVector& NewBodyScale)
    {
        FeetOffset = FMath::Max(1.0f, NewFeetOffset);
        BodyScaleZ = FMath::Max(0.01f, NewBodyScale.Z);
        if (Body)
        {
            Body->SetRelativeLocation(FVector(0.0f, 0.0f, -FeetOffset));
            Body->SetRelativeScale3D(NewBodyScale);
        }
    }

    const FLLResidentAppearanceInputs& GetInputs() const { return Inputs; }

    USkeletalMeshComponent* GetBodyComponent() const { return Body; }

    // ---- Tuning ---------------------------------------------------------------
    // Height factor per Core LifeStage (index = ELLCoreLifeStage).
    static constexpr float StageHeightFactor[8] = { 0.40f, 0.52f, 0.65f, 0.85f, 0.98f, 1.0f, 1.0f, 0.96f };
    static constexpr float HeightAxisRange = 0.08f; // +-8 % around the stage height
    static constexpr float BuildAxisRange  = 0.06f; // +-6 % lateral scale

private:
    void ResolveInputs();
    void BuildBody();
    void ApplySkin();
    void ApplyEyes();
    void ApplyHair();
    void ApplyOutfit();
    void ApplyScale();
    void PlayIdle();

    UStaticMesh* PickHairMesh(bool& bOutWithBeard) const;
    UMaterialInstanceDynamic* MakeSlotMaterial(const TCHAR* SlotNameContains);

    // Catalogue (referenced in the constructor so it is cooked).
    UPROPERTY() TObjectPtr<USkeletalMesh> MaleMesh;
    UPROPERTY() TObjectPtr<USkeletalMesh> FemaleMesh;
    UPROPERTY() TObjectPtr<UTexture> MaleSkinLight;
    UPROPERTY() TObjectPtr<UTexture> MaleSkinDark;
    UPROPERTY() TObjectPtr<UTexture> FemaleSkinLight;
    UPROPERTY() TObjectPtr<UTexture> FemaleSkinDark;
    UPROPERTY() TArray<TObjectPtr<UStaticMesh>> MaleHair;
    UPROPERTY() TArray<TObjectPtr<UStaticMesh>> FemaleHair;
    UPROPERTY() TObjectPtr<UStaticMesh> BeardMesh;
    UPROPERTY() TObjectPtr<UAnimSequence> IdleAnim;
    // Minimum default clothing (Modular Character Outfits - Fantasy, Peasant),
    // skinned to the shared UBC skeleton; follows the body pose.
    // Head-only bodies worn under the clothing. The outfit covers pelvis ->
    // neck_01 and the whole limbs, so the full body would clip through it.
    UPROPERTY() TObjectPtr<USkeletalMesh> MaleHeadMesh;
    UPROPERTY() TObjectPtr<USkeletalMesh> FemaleHeadMesh;
    UPROPERTY() TObjectPtr<USkeletalMesh> MalePeasantMesh;
    UPROPERTY() TObjectPtr<USkeletalMesh> FemalePeasantMesh;
    UPROPERTY() TObjectPtr<UTexture> PeasantAltBaseColor;

    // Runtime.
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Body;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Hair;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Beard;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SkinMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> EyeMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> HairMaterial;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Outfit;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> OutfitMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> OutfitSkinMaterial;

    FLLResidentAppearanceInputs Inputs;
    float FeetOffset = 88.0f;
    float MeshHeight = 0.0f;   // unscaled bind-pose height of the chosen mesh
    float BodyScaleZ = 1.0f;
    FLinearColor SkinTintColor = FLinearColor::White; // shared by body skin and outfit-exposed skin
    bool bClothed = false;  // head-only body + outfit instead of the bare full body
    bool bBuilt = false;
};
