#include "Characters/LLResidentAppearanceComponent.h"
#include "Characters/LLResidentCharacter.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLSimulationSubsystem.h"
#include "Animation/AnimSequence.h"
#include "AnimationRuntime.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ReferenceSkeleton.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    // Interchange glTF material instance parameters (parent MI_Default_Opaque_DS).
    const FName ParamBaseColorTexture(TEXT("BaseColorTexture"));
    const FName ParamBaseColorFactor(TEXT("BaseColorFactor"));

    const FName HeadBoneName(TEXT("Head"));

    // Skin tint over the light/dark textures: warm to cool, slightly darker at the high end.
    FLinearColor SkinTint(float Axis)
    {
        const FLinearColor Light(1.00f, 0.96f, 0.92f, 1.0f);
        const FLinearColor Deep(0.80f, 0.66f, 0.56f, 1.0f);
        return FMath::Lerp(Light, Deep, Axis);
    }

    // Eye tint over the brown base texture.
    FLinearColor EyeTint(float Axis)
    {
        static const FLinearColor Palette[] = {
            FLinearColor(0.55f, 0.35f, 0.20f), // brown
            FLinearColor(0.35f, 0.25f, 0.15f), // dark brown
            FLinearColor(0.45f, 0.55f, 0.40f), // hazel-green
            FLinearColor(0.45f, 0.60f, 0.80f), // blue
            FLinearColor(0.60f, 0.60f, 0.62f), // grey
        };
        const int32 Count = UE_ARRAY_COUNT(Palette);
        return Palette[FMath::Clamp(static_cast<int32>(Axis * Count), 0, Count - 1)];
    }

    // Hair tint over the hair base texture.
    FLinearColor HairTint(float Axis)
    {
        static const FLinearColor Palette[] = {
            FLinearColor(0.08f, 0.06f, 0.05f), // black
            FLinearColor(0.25f, 0.15f, 0.10f), // dark brown
            FLinearColor(0.45f, 0.30f, 0.18f), // brown
            FLinearColor(0.60f, 0.25f, 0.12f), // auburn
            FLinearColor(0.85f, 0.70f, 0.45f), // blond
            FLinearColor(0.75f, 0.75f, 0.72f), // grey
        };
        const int32 Count = UE_ARRAY_COUNT(Palette);
        return Palette[FMath::Clamp(static_cast<int32>(Axis * Count), 0, Count - 1)];
    }

    float MeshBindPoseHeight(const USkeletalMesh* Mesh)
    {
        if (!Mesh)
        {
            return 0.0f;
        }
        const FBoxSphereBounds Bounds = Mesh->GetBounds();
        return static_cast<float>(Bounds.BoxExtent.Z * 2.0);
    }
}

ULLResidentAppearanceComponent::ULLResidentAppearanceComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // Track B catalogue. Paths follow Content/Characters/Quaternius/PROVENANCE.md.
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MaleFinder(TEXT("/Game/Characters/Quaternius/UBC/Male/Superhero_Male_FullBody/SkeletalMeshes/Superhero_Male_FullBody.Superhero_Male_FullBody"));
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> FemaleFinder(TEXT("/Game/Characters/Quaternius/UBC/Female/Superhero_Female_FullBody/SkeletalMeshes/Superhero_Female_FullBody.Superhero_Female_FullBody"));
    static ConstructorHelpers::FObjectFinder<UTexture> MaleLightFinder(TEXT("/Game/Characters/Quaternius/UBC/Textures/T_Superhero_Male_Ligh.T_Superhero_Male_Ligh"));
    static ConstructorHelpers::FObjectFinder<UTexture> MaleDarkFinder(TEXT("/Game/Characters/Quaternius/UBC/Male/Superhero_Male_FullBody/Textures/T_Superhero_Male_Dark.T_Superhero_Male_Dark"));
    static ConstructorHelpers::FObjectFinder<UTexture> FemaleLightFinder(TEXT("/Game/Characters/Quaternius/UBC/Textures/T_Superhero_Female_Light_BaseColor.T_Superhero_Female_Light_BaseColor"));
    static ConstructorHelpers::FObjectFinder<UTexture> FemaleDarkFinder(TEXT("/Game/Characters/Quaternius/UBC/Female/Superhero_Female_FullBody/Textures/T_Superhero_Female_Dark_BaseColor.T_Superhero_Female_Dark_BaseColor"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HairBuzzedFinder(TEXT("/Game/Characters/Quaternius/UBC/Hair/Hair_Buzzed.Hair_Buzzed"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HairSimplePartedFinder(TEXT("/Game/Characters/Quaternius/UBC/Hair/Hair_SimpleParted.Hair_SimpleParted"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HairLongFinder(TEXT("/Game/Characters/Quaternius/UBC/Hair/Hair_Long.Hair_Long"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HairBunsFinder(TEXT("/Game/Characters/Quaternius/UBC/Hair/Hair_Buns.Hair_Buns"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HairBuzzedFemaleFinder(TEXT("/Game/Characters/Quaternius/UBC/Hair/Hair_BuzzedFemale.Hair_BuzzedFemale"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BeardFinder(TEXT("/Game/Characters/Quaternius/UBC/Hair/Hair_Beard.Hair_Beard"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleFinder(TEXT("/Game/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Idle_Loop.Idle_Loop"));

    MaleMesh = MaleFinder.Succeeded() ? MaleFinder.Object : nullptr;
    FemaleMesh = FemaleFinder.Succeeded() ? FemaleFinder.Object : nullptr;
    MaleSkinLight = MaleLightFinder.Succeeded() ? MaleLightFinder.Object : nullptr;
    MaleSkinDark = MaleDarkFinder.Succeeded() ? MaleDarkFinder.Object : nullptr;
    FemaleSkinLight = FemaleLightFinder.Succeeded() ? FemaleLightFinder.Object : nullptr;
    FemaleSkinDark = FemaleDarkFinder.Succeeded() ? FemaleDarkFinder.Object : nullptr;

    if (HairBuzzedFinder.Succeeded()) { MaleHair.Add(HairBuzzedFinder.Object); }
    if (HairSimplePartedFinder.Succeeded()) { MaleHair.Add(HairSimplePartedFinder.Object); FemaleHair.Add(HairSimplePartedFinder.Object); }
    if (HairLongFinder.Succeeded()) { MaleHair.Add(HairLongFinder.Object); FemaleHair.Add(HairLongFinder.Object); }
    if (HairBunsFinder.Succeeded()) { FemaleHair.Add(HairBunsFinder.Object); }
    if (HairBuzzedFemaleFinder.Succeeded()) { FemaleHair.Add(HairBuzzedFemaleFinder.Object); }
    BeardMesh = BeardFinder.Succeeded() ? BeardFinder.Object : nullptr;
    IdleAnim = IdleFinder.Succeeded() ? IdleFinder.Object : nullptr;
}

void ULLResidentAppearanceComponent::BeginPlay()
{
    Super::BeginPlay();
    EnsureBuilt();
}

void ULLResidentAppearanceComponent::EnsureBuilt()
{
    if (bBuilt)
    {
        return;
    }
    bBuilt = true;

    ResolveInputs();
    BuildBody();
    if (!Body)
    {
        return;
    }
    ApplySkin();
    ApplyEyes();
    ApplyHair();
    ApplyScale();
    PlayIdle();

    // One line per built resident so Save/Load continuity can be checked in
    // the Output Log: the same ResidentId must print the same seed/variants
    // before and after a load.
    const ALLResidentCharacter* Resident = Cast<ALLResidentCharacter>(GetOwner());
    UE_LOG(LogTemp, Log, TEXT("LLAppearance %s id=%s seed=%d sex=%d stage=%d skin=%.3f eye=%.3f hair=%d/%.3f height=%.3f build=%.3f temp=%d"),
        Resident ? *Resident->GetResidentDisplayName().ToString() : TEXT("?"),
        *Inputs.ResidentId.ToString(EGuidFormats::DigitsWithHyphens),
        Inputs.VisualSeed, static_cast<int32>(Inputs.Sex), static_cast<int32>(Inputs.LifeStage),
        Inputs.SkinToneAxis, Inputs.EyeColorAxis, Inputs.HairStyleVariant, Inputs.HairColorAxis,
        Inputs.HeightAxis, Inputs.BuildAxis, Inputs.bTemporaryPresentationSeed ? 1 : 0);
}

float ULLResidentAppearanceComponent::GetVisualTopOffset() const
{
    return Body ? (-FeetOffset + MeshHeight * BodyScaleZ) : 0.0f;
}

void ULLResidentAppearanceComponent::ResolveInputs()
{
    const ALLResidentCharacter* Resident = Cast<ALLResidentCharacter>(GetOwner());
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!Resident || !GameInstance)
    {
        return;
    }

    const FGuid ResidentId = Resident->GetResidentId();
    int32 WorldSeed = 0;
    ELLCoreSex Sex = ELLCoreSex::Male;
    ELLCoreLifeStage LifeStage = ELLCoreLifeStage::Adult;

    // Authoritative source first: Core Bridge read DTO.
    bool bResolved = false;
    if (ULLCoreBridgeSubsystem* Bridge = GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>())
    {
        WorldSeed = Bridge->GetRuntimeSeed();
        FLLCoreResidentObservation Observation;
        if (Bridge->IsCoreRunning() && Bridge->GetResidentObservation(ResidentId, Observation))
        {
            Sex = Observation.Sex;
            LifeStage = Observation.LifeStage;
            bResolved = true;
        }
    }

    // Compatibility projection fallback.
    if (!bResolved)
    {
        if (ULLSimulationSubsystem* Simulation = GameInstance->GetSubsystem<ULLSimulationSubsystem>())
        {
            if (WorldSeed == 0)
            {
                WorldSeed = Simulation->GetWorldSeed();
            }
            FLLResidentData Data;
            if (Simulation->FindResidentById(ResidentId, Data))
            {
                Sex = Data.Sex == ELLSex::Female ? ELLCoreSex::Female : ELLCoreSex::Male;
                switch (Data.LifeStage)
                {
                    case ELLLifeStage::Infant: LifeStage = ELLCoreLifeStage::Baby; break;
                    case ELLLifeStage::Child:  LifeStage = ELLCoreLifeStage::Child; break;
                    case ELLLifeStage::Teen:   LifeStage = ELLCoreLifeStage::Teen; break;
                    case ELLLifeStage::Elder:  LifeStage = ELLCoreLifeStage::Elderly; break;
                    case ELLLifeStage::Adult:
                    default:                   LifeStage = ELLCoreLifeStage::Adult; break;
                }
            }
        }
    }

    Inputs = ULLResidentAppearanceInputSource::Resolve(WorldSeed, ResidentId, Sex, LifeStage);
}

void ULLResidentAppearanceComponent::BuildBody()
{
    AActor* Owner = GetOwner();
    USkeletalMesh* Mesh = Inputs.Sex == ELLCoreSex::Female ? FemaleMesh.Get() : MaleMesh.Get();
    if (!Owner || !Owner->GetRootComponent() || !Mesh)
    {
        return;
    }

    if (const ACharacter* Character = Cast<ACharacter>(Owner))
    {
        if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
        {
            FeetOffset = Capsule->GetScaledCapsuleHalfHeight();
        }
    }

    Body = NewObject<USkeletalMeshComponent>(Owner, TEXT("AppearanceBody"));
    Body->SetSkeletalMesh(Mesh);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetCastShadow(true);
    Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
    Body->SetupAttachment(Owner->GetRootComponent());
    Body->SetRelativeLocation(FVector(0.0f, 0.0f, -FeetOffset));
    Body->RegisterComponent();

    MeshHeight = MeshBindPoseHeight(Mesh);
}

UMaterialInstanceDynamic* ULLResidentAppearanceComponent::MakeSlotMaterial(const TCHAR* SlotNameContains)
{
    if (!Body)
    {
        return nullptr;
    }
    const TArray<FName> SlotNames = Body->GetMaterialSlotNames();
    for (int32 Index = 0; Index < SlotNames.Num(); ++Index)
    {
        if (SlotNames[Index].ToString().Contains(SlotNameContains))
        {
            return Body->CreateAndSetMaterialInstanceDynamic(Index);
        }
    }
    return nullptr;
}

void ULLResidentAppearanceComponent::ApplySkin()
{
    // Slot names come from the glTF material names (MI_Superhero_Male / MI_Superhero_Female).
    SkinMaterial = MakeSlotMaterial(TEXT("Superhero"));
    if (!SkinMaterial)
    {
        return;
    }

    const bool bFemale = Inputs.Sex == ELLCoreSex::Female;
    UTexture* Light = bFemale ? FemaleSkinLight.Get() : MaleSkinLight.Get();
    UTexture* Dark = bFemale ? FemaleSkinDark.Get() : MaleSkinDark.Get();
    UTexture* Base = Inputs.SkinToneAxis < 0.5f ? Light : Dark;
    if (Base)
    {
        SkinMaterial->SetTextureParameterValue(ParamBaseColorTexture, Base);
    }
    // Tint varies within each half of the axis so four residents rarely match.
    const float Local = FMath::Frac(Inputs.SkinToneAxis * 2.0f);
    SkinMaterial->SetVectorParameterValue(ParamBaseColorFactor, SkinTint(Local));
}

void ULLResidentAppearanceComponent::ApplyEyes()
{
    EyeMaterial = MakeSlotMaterial(TEXT("Eyes"));
    if (EyeMaterial)
    {
        EyeMaterial->SetVectorParameterValue(ParamBaseColorFactor, EyeTint(Inputs.EyeColorAxis));
    }
}

UStaticMesh* ULLResidentAppearanceComponent::PickHairMesh(bool& bOutWithBeard) const
{
    const TArray<TObjectPtr<UStaticMesh>>& Catalogue = Inputs.Sex == ELLCoreSex::Female ? FemaleHair : MaleHair;
    bOutWithBeard = Inputs.Sex == ELLCoreSex::Male && (Inputs.HairStyleVariant / FMath::Max(1, Catalogue.Num())) % 3 == 0;
    if (Catalogue.Num() == 0)
    {
        return nullptr;
    }
    return Catalogue[Inputs.HairStyleVariant % Catalogue.Num()].Get();
}

void ULLResidentAppearanceComponent::ApplyHair()
{
    if (!Body)
    {
        return;
    }

    // The hair meshes are authored with their origin at the character origin
    // ("Origin at 0"), so attach to the Head bone with the inverse of the head's
    // bind-pose transform: aligned in bind pose, then follows the head.
    const USkeletalMesh* Mesh = Body->GetSkeletalMeshAsset();
    const FReferenceSkeleton& RefSkeleton = Mesh->GetRefSkeleton();
    const int32 HeadIndex = RefSkeleton.FindBoneIndex(HeadBoneName);
    FTransform HeadBind = FTransform::Identity;
    if (HeadIndex != INDEX_NONE)
    {
        HeadBind = FAnimationRuntime::GetComponentSpaceTransform(RefSkeleton, RefSkeleton.GetRefBonePose(), HeadIndex);
    }
    const FTransform HairRelative = HeadIndex != INDEX_NONE ? HeadBind.Inverse() : FTransform::Identity;
    const FName AttachBone = HeadIndex != INDEX_NONE ? HeadBoneName : NAME_None;

    auto Attach = [&](const TCHAR* Name, UStaticMesh* HairMesh) -> UStaticMeshComponent*
    {
        if (!HairMesh)
        {
            return nullptr;
        }
        UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(GetOwner(), Name);
        Component->SetStaticMesh(HairMesh);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCastShadow(false);
        Component->SetupAttachment(Body, AttachBone);
        Component->SetRelativeTransform(HairRelative);
        Component->RegisterComponent();
        return Component;
    };

    bool bWithBeard = false;
    Hair = Attach(TEXT("AppearanceHair"), PickHairMesh(bWithBeard));
    if (bWithBeard)
    {
        Beard = Attach(TEXT("AppearanceBeard"), BeardMesh.Get());
    }

    const FLinearColor Tint = HairTint(Inputs.HairColorAxis);

    // Eyebrows are part of the body mesh (slot MI_Hair_1 / MI_Hair_2); tint them like the hair.
    if (UMaterialInstanceDynamic* BrowMaterial = MakeSlotMaterial(TEXT("Hair")))
    {
        BrowMaterial->SetVectorParameterValue(ParamBaseColorFactor, Tint);
    }

    if (Hair)
    {
        HairMaterial = Hair->CreateAndSetMaterialInstanceDynamic(0);
        if (HairMaterial)
        {
            HairMaterial->SetVectorParameterValue(ParamBaseColorFactor, Tint);
        }
    }
    if (Beard)
    {
        if (UMaterialInstanceDynamic* BeardMaterial = Beard->CreateAndSetMaterialInstanceDynamic(0))
        {
            BeardMaterial->SetVectorParameterValue(ParamBaseColorFactor, Tint);
        }
    }
}

void ULLResidentAppearanceComponent::ApplyScale()
{
    if (!Body || MeshHeight <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    // Adult bind-pose height maps to the capsule height; LifeStage and the
    // continuous axes vary around it.
    const float CapsuleHeight = FeetOffset * 2.0f;
    const int32 StageIndex = FMath::Clamp(static_cast<int32>(Inputs.LifeStage), 0, 7);
    const float Stage = StageHeightFactor[StageIndex];
    const float Height = 1.0f + (Inputs.HeightAxis - 0.5f) * 2.0f * HeightAxisRange;
    const float Build = 1.0f + (Inputs.BuildAxis - 0.5f) * 2.0f * BuildAxisRange;

    BodyScaleZ = (CapsuleHeight / MeshHeight) * Stage * Height;
    const float ScaleXY = BodyScaleZ * Build;
    Body->SetRelativeScale3D(FVector(ScaleXY, ScaleXY, BodyScaleZ));
}

void ULLResidentAppearanceComponent::PlayIdle()
{
    if (Body && IdleAnim)
    {
        Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Body->PlayAnimation(IdleAnim, true);
    }
}
