#include "Characters/LLResidentPresentationComponent.h"
#include "Characters/LLResidentCharacter.h"
#include "Simulation/LLSimulationSubsystem.h"
#include "UI/LLObservationSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    // One matte colour for every silhouette; colour is reserved for the ring.
    const FLinearColor SilhouetteColor(0.72f, 0.72f, 0.70f, 1.0f);
    const FLinearColor RingColor(0.35f, 0.80f, 1.00f, 1.0f);
    const FLinearColor RingInnerColor(0.02f, 0.02f, 0.02f, 1.0f);
    const FName ColorParameter(TEXT("Color"));
}

ULLResidentPresentationComponent::ULLResidentPresentationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    // Engine primitives and an unlit colour material, referenced here so they
    // are cooked with the project.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
    if (CylinderFinder.Succeeded())
    {
        CylinderMesh = CylinderFinder.Object;
    }
    if (SphereFinder.Succeeded())
    {
        SphereMesh = SphereFinder.Object;
    }
    if (MaterialFinder.Succeeded())
    {
        UnlitMaterial = MaterialFinder.Object;
    }
}

void ULLResidentPresentationComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* Owner = GetOwner())
    {
        Label = Owner->FindComponentByClass<UTextRenderComponent>();
    }

    BuildSilhouette();
    BuildRing();
    RefreshResidentData();
    ApplySilhouetteScale();
    UpdateRing();
    UpdateLabel();
}

void ULLResidentPresentationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    DataRefreshTimer -= DeltaTime;
    if (DataRefreshTimer <= 0.0f)
    {
        DataRefreshTimer = DataRefreshSeconds;
        RefreshResidentData();
        ApplySilhouetteScale();
    }

    UpdateRing();
    UpdateLabel();
}

float ULLResidentPresentationComponent::FeetOffset() const
{
    if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
        {
            return Capsule->GetScaledCapsuleHalfHeight();
        }
    }
    return 88.0f;
}

UStaticMeshComponent* ULLResidentPresentationComponent::AddMesh(const TCHAR* Name, UStaticMesh* Mesh, UMaterialInstanceDynamic*& OutMaterial, const FLinearColor& Color)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->GetRootComponent() || !Mesh)
    {
        return nullptr;
    }

    UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(Owner, Name);
    Component->SetStaticMesh(Mesh);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetCastShadow(false);
    Component->SetupAttachment(Owner->GetRootComponent());
    Component->RegisterComponent();

    if (UnlitMaterial)
    {
        OutMaterial = UMaterialInstanceDynamic::Create(UnlitMaterial, Component);
        OutMaterial->SetVectorParameterValue(ColorParameter, Color);
        Component->SetMaterial(0, OutMaterial);
    }
    return Component;
}

void ULLResidentPresentationComponent::BuildSilhouette()
{
    UMaterialInstanceDynamic* TorsoMaterial = nullptr;
    UMaterialInstanceDynamic* HeadMaterial = nullptr;
    Torso = AddMesh(TEXT("PresentationTorso"), CylinderMesh, TorsoMaterial, SilhouetteColor);
    Head = AddMesh(TEXT("PresentationHead"), SphereMesh, HeadMaterial, SilhouetteColor);
    SilhouetteMaterial = TorsoMaterial;
    if (Head && SilhouetteMaterial)
    {
        Head->SetMaterial(0, SilhouetteMaterial);
    }
}

void ULLResidentPresentationComponent::BuildRing()
{
    UMaterialInstanceDynamic* OuterMaterial = nullptr;
    UMaterialInstanceDynamic* InnerMaterial = nullptr;
    RingOuter = AddMesh(TEXT("PresentationRingOuter"), CylinderMesh, OuterMaterial, RingColor * RingQuickIntensity);
    RingInner = AddMesh(TEXT("PresentationRingInner"), CylinderMesh, InnerMaterial, RingInnerColor);
    RingOuterMaterial = OuterMaterial;
    RingInnerMaterial = InnerMaterial;

    const float Feet = FeetOffset();
    if (RingOuter)
    {
        RingOuter->SetRelativeScale3D(FVector(RingOuterRadius / 50.0f, RingOuterRadius / 50.0f, RingThickness / 100.0f));
        RingOuter->SetRelativeLocation(FVector(0.0f, 0.0f, -Feet + RingThickness * 0.5f));
        RingOuter->SetVisibility(false);
    }
    if (RingInner)
    {
        RingInner->SetRelativeScale3D(FVector(RingInnerRadius / 50.0f, RingInnerRadius / 50.0f, RingThickness / 100.0f));
        RingInner->SetRelativeLocation(FVector(0.0f, 0.0f, -Feet + RingThickness * 0.5f + 0.5f));
        RingInner->SetVisibility(false);
    }
}

void ULLResidentPresentationComponent::RefreshResidentData()
{
    const ALLResidentCharacter* Resident = Cast<ALLResidentCharacter>(GetOwner());
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    ULLSimulationSubsystem* Simulation = GameInstance ? GameInstance->GetSubsystem<ULLSimulationSubsystem>() : nullptr;
    if (!Resident || !Simulation)
    {
        return;
    }

    FLLResidentData Data;
    if (Simulation->FindResidentById(Resident->GetResidentId(), Data))
    {
        LifeStage = Data.LifeStage;
        Sex = Data.Sex;
        DisplayName = Data.DisplayName;
        const int32 StageIndex = FMath::Clamp(static_cast<int32>(LifeStage), 0, 4);
        StageFactor = StageHeightFactor[StageIndex];
        bResidentDataValid = true;
    }
}

void ULLResidentPresentationComponent::ApplySilhouetteScale()
{
    const float Feet = FeetOffset();
    const float S = StageFactor;
    const float TorsoRadius = (Sex == ELLSex::Female ? TorsoRadiusFemale : TorsoRadiusMale) * S;
    const float Height = TorsoHeight * S;

    if (Torso)
    {
        Torso->SetRelativeScale3D(FVector(TorsoRadius / 50.0f, TorsoRadius / 50.0f, Height / 100.0f));
        Torso->SetRelativeLocation(FVector(0.0f, 0.0f, -Feet + Height * 0.5f));
    }
    if (Head)
    {
        const float Radius = HeadRadius * S;
        Head->SetRelativeScale3D(FVector(Radius / 50.0f));
        Head->SetRelativeLocation(FVector(0.0f, 0.0f, -Feet + Height + Radius));
    }
    if (Label)
    {
        Label->SetRelativeLocation(FVector(0.0f, 0.0f, -Feet + Height + 2.0f * HeadRadius * S + LabelAboveHead));
    }
}

void ULLResidentPresentationComponent::UpdateRing()
{
    const ALLResidentCharacter* Resident = Cast<ALLResidentCharacter>(GetOwner());
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    ULLObservationSubsystem* Observation = GameInstance ? GameInstance->GetSubsystem<ULLObservationSubsystem>() : nullptr;

    bool bSelected = false;
    float Intensity = RingQuickIntensity;
    if (Resident && Observation && Observation->HasObservedResident()
        && Observation->GetObservedResidentId() == Resident->GetResidentId())
    {
        bSelected = true;
        Intensity = Observation->IsDetailOpen() ? RingDetailIntensity : RingQuickIntensity;
    }

    if (RingOuter)
    {
        RingOuter->SetVisibility(bSelected);
    }
    if (RingInner)
    {
        RingInner->SetVisibility(bSelected);
    }
    if (bSelected && RingOuterMaterial)
    {
        RingOuterMaterial->SetVectorParameterValue(ColorParameter, RingColor * Intensity);
    }
}

FString ULLResidentPresentationComponent::LifeStageBadge(ELLLifeStage Stage)
{
    switch (Stage)
    {
        case ELLLifeStage::Infant: return TEXT("Infant");
        case ELLLifeStage::Child:  return TEXT("Child");
        case ELLLifeStage::Teen:   return TEXT("Teen");
        case ELLLifeStage::Elder:  return TEXT("Elder");
        case ELLLifeStage::Adult:
        default:                   return TEXT("Adult");
    }
}

void ULLResidentPresentationComponent::UpdateLabel()
{
    if (!Label)
    {
        return;
    }

    const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
    if (!Camera)
    {
        return;
    }

    const FVector CameraLocation = Camera->GetCameraLocation();
    const FVector LabelLocation = Label->GetComponentLocation();
    const float Distance = static_cast<float>(FVector::Dist(CameraLocation, LabelLocation));

    if (Distance > LabelMidDistance)
    {
        Label->SetVisibility(false);
        return;
    }
    Label->SetVisibility(true);

    // Text content by LOD.
    const FString Name = bResidentDataValid ? DisplayName : Label->Text.ToString();
    const FString Wanted = Distance <= LabelNearDistance
        ? Name + TEXT("\n") + LifeStageBadge(LifeStage)
        : Name;
    if (!Label->Text.ToString().Equals(Wanted))
    {
        Label->SetText(FText::FromString(Wanted));
    }

    // Face the camera; scale with distance.
    const FVector ToCamera = CameraLocation - LabelLocation;
    if (!ToCamera.IsNearlyZero())
    {
        Label->SetWorldRotation(FRotationMatrix::MakeFromX(ToCamera).Rotator());
    }
    const float Scale = FMath::Clamp(Distance / LabelReferenceDistance, LabelMinScale, LabelMaxScale);
    Label->SetWorldSize(LabelBaseWorldSize * Scale);
}
