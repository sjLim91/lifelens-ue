#include "Characters/LLResidentPresentationComponent.h"
#include "Characters/LLResidentAppearanceComponent.h"
#include "Characters/LLResidentCharacter.h"
#include "Simulation/LLSimulationSubsystem.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "UI/LLObservationSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    const FLinearColor SilhouetteColor(0.72f, 0.72f, 0.70f, 1.0f);
    const FLinearColor RingColor(0.35f, 0.80f, 1.00f, 1.0f);
    const FLinearColor RingInnerColor(0.02f, 0.02f, 0.02f, 1.0f);
    const FName ColorParameter(TEXT("Color"));
}

ULLResidentPresentationComponent::ULLResidentPresentationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

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
        Appearance = Owner->FindComponentByClass<ULLResidentAppearanceComponent>();
    }

    // Human body from Character Appearance is the production path. Primitive
    // silhouette geometry is QA-only and defaults off so missing art cannot
    // silently regress the shipped local view to Cylinder/Sphere people.
    if (Appearance)
    {
        Appearance->EnsureBuilt();
    }
    // The world director binds the resident right after spawning the actor, so
    // at BeginPlay the identity is usually still unknown and the appearance
    // cannot be built yet. Wait for OnResidentBound() instead of falling back
    // to a silhouette that would be discarded one call later.
    const ALLResidentCharacter* Resident = Cast<ALLResidentCharacter>(GetOwner());
    const bool bAwaitingIdentity = Resident && !Resident->GetResidentId().IsValid();

    const bool bHasHumanBody = Appearance && Appearance->HasBody();
    if (!bHasHumanBody && !bAwaitingIdentity && bAllowPrimitiveSilhouetteFallback)
    {
        BuildSilhouette();
    }
    BuildRing();
    if (bHasHumanBody || Torso)
    {
        HideDebugBody();
    }
    RefreshResidentData();
    ApplySilhouetteScale();
    UpdateRing();
    UpdateLabel();
}

void ULLResidentPresentationComponent::OnResidentBound()
{
    if (!Appearance)
    {
        return;
    }
    Appearance->EnsureBuilt();
    if (!Appearance->HasBody())
    {
        // Fail closed in production. A missing human asset must remain visible
        // through observer label/ring diagnostics rather than becoming an
        // obvious Engine-primitive person.
        if (bAllowPrimitiveSilhouetteFallback && !Torso)
        {
            BuildSilhouette();
        }
    }
    else if (Torso)
    {
        // A human body arrived after a QA silhouette had already been built;
        // drop the diagnostic meshes.
        Torso->DestroyComponent();
        Torso = nullptr;
        if (Head)
        {
            Head->DestroyComponent();
            Head = nullptr;
        }
        SilhouetteMaterial = nullptr;
    }

    HideDebugBody();
    RefreshResidentData();
    ApplySilhouetteScale();
    UpdateRing();
    UpdateLabel();
}

void ULLResidentPresentationComponent::HideDebugBody()
{
    const bool bHasHumanBody = Appearance && Appearance->HasBody();
    if (!bHideDebugBody || (!Torso && !bHasHumanBody))
    {
        return;
    }
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }
    TArray<UStaticMeshComponent*> Meshes;
    Owner->GetComponents<UStaticMeshComponent>(Meshes);
    for (UStaticMeshComponent* Mesh : Meshes)
    {
        if (Mesh && Mesh->GetFName() == TEXT("DebugBody"))
        {
            Mesh->SetVisibility(false);
        }
    }
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

    CrowdLabelRefreshTimer -= DeltaTime;
    if (CrowdLabelRefreshTimer <= 0.0f)
    {
        CrowdLabelRefreshTimer = CrowdLabelRefreshSeconds;
        RefreshCrowdLabelSuppression();
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
        AgeYears = Data.AgeYears;
        DisplayName = Data.DisplayName;
        const int32 StageIndex = FMath::Clamp(static_cast<int32>(LifeStage), 0, 4);
        StageFactor = StageHeightFactor[StageIndex];
        bResidentDataValid = true;
    }
}

void ULLResidentPresentationComponent::ApplySilhouetteScale()
{
    const float Base = SilhouetteBaseZ;
    const float S = StageFactor;
    const float TorsoRadius = (Sex == ELLSex::Female ? TorsoRadiusFemale : TorsoRadiusMale) * S;
    const float Height = TorsoHeight * S;
    const float Radius = HeadRadius * S;

    if (Torso)
    {
        Torso->SetRelativeScale3D(FVector(TorsoRadius / 50.0f, TorsoRadius / 50.0f, Height / 100.0f));
        Torso->SetRelativeLocation(FVector(0.0f, 0.0f, Base + Height * 0.5f));
    }
    if (Head)
    {
        Head->SetRelativeScale3D(FVector(Radius / 50.0f));
        Head->SetRelativeLocation(FVector(0.0f, 0.0f, Base + Height + Radius));
    }
    if (Label)
    {
        const float LabelZ = (Appearance && Appearance->HasBody())
            ? Appearance->GetVisualTopOffset() + LabelAboveHead
            : Base + Height + 2.0f * Radius + LabelAboveHead;
        Label->SetRelativeLocation(FVector(0.0f, 0.0f, LabelZ));
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

    const float Feet = FeetOffset();
    const float GroundOffset = Appearance
        ? Appearance->GetPresentationGroundOffsetUU()
        : 0.0f;
    const bool bDetail = bSelected && Observation && Observation->IsDetailOpen();
    const float TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    const float Pulse = bSelected
        ? 1.0f + FMath::Sin(TimeSeconds * RingPulseRadiansPerSecond) * RingPulseFraction
        : 1.0f;
    const float DetailScale = bDetail ? RingDetailRadiusScale : 1.0f;
    const float OuterRadius = RingOuterRadius * Pulse * DetailScale;
    const float InnerRadius = RingInnerRadius * Pulse * DetailScale;

    if (RingOuter)
    {
        RingOuter->SetRelativeScale3D(FVector(
            OuterRadius / 50.0f,
            OuterRadius / 50.0f,
            RingThickness / 100.0f));
        RingOuter->SetRelativeLocation(FVector(
            0.0f, 0.0f, -Feet + GroundOffset + RingThickness * 0.5f));
        RingOuter->SetVisibility(bSelected);
    }
    if (RingInner)
    {
        RingInner->SetRelativeScale3D(FVector(
            InnerRadius / 50.0f,
            InnerRadius / 50.0f,
            RingThickness / 100.0f));
        RingInner->SetRelativeLocation(FVector(
            0.0f, 0.0f, -Feet + GroundOffset + RingThickness * 0.5f + 0.5f));
        RingInner->SetVisibility(bSelected);
    }
    if (bSelected && RingOuterMaterial)
    {
        const float PulseBrightness = FMath::Lerp(0.92f, 1.08f, (Pulse - (1.0f - RingPulseFraction)) / (2.0f * RingPulseFraction));
        RingOuterMaterial->SetVectorParameterValue(
            ColorParameter,
            RingColor * Intensity * PulseBrightness);
    }
}

void ULLResidentPresentationComponent::RefreshCrowdLabelSuppression()
{
    bCrowdLabelSuppressed = false;

    const ALLResidentCharacter* Resident = Cast<ALLResidentCharacter>(GetOwner());
    UWorld* World = GetWorld();
    APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
    APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
    if (!Resident || !Label || !World || !PlayerController || !Camera)
    {
        return;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    ULLObservationSubsystem* Observation =
        GameInstance ? GameInstance->GetSubsystem<ULLObservationSubsystem>() : nullptr;
    const FGuid SelfId = Resident->GetResidentId();
    const bool bSelected = Observation && Observation->HasObservedResident()
        && Observation->GetObservedResidentId() == SelfId;
    if (bSelected)
    {
        return;
    }

    FVector2D SelfScreen;
    if (!PlayerController->ProjectWorldLocationToScreen(
            Label->GetComponentLocation(), SelfScreen, false))
    {
        return;
    }

    const FVector CameraLocation = Camera->GetCameraLocation();
    const float SelfDistanceSq = FVector::DistSquared(
        CameraLocation,
        Resident->GetActorLocation());
    const float SeparationSq = FMath::Square(CrowdLabelSeparationPixels);
    int32 Comparisons = 0;

    for (TActorIterator<ALLResidentCharacter> It(World); It; ++It)
    {
        const ALLResidentCharacter* Other = *It;
        if (!Other || Other == Resident || ++Comparisons > MaxCrowdLabelComparisons)
        {
            if (Comparisons > MaxCrowdLabelComparisons)
            {
                break;
            }
            continue;
        }

        FVector BoundsOrigin = FVector::ZeroVector;
        FVector BoundsExtent = FVector::ZeroVector;
        Other->GetActorBounds(false, BoundsOrigin, BoundsExtent, false);
        const FVector OtherLabelPoint =
            BoundsOrigin + FVector(0.0f, 0.0f, BoundsExtent.Z + LabelAboveHead);

        FVector2D OtherScreen;
        if (!PlayerController->ProjectWorldLocationToScreen(
                OtherLabelPoint, OtherScreen, false)
            || FVector2D::DistSquared(SelfScreen, OtherScreen) > SeparationSq)
        {
            continue;
        }

        const FGuid OtherId = Other->GetResidentId();
        const bool bOtherSelected = Observation && Observation->HasObservedResident()
            && Observation->GetObservedResidentId() == OtherId;
        const float OtherDistanceSq = FVector::DistSquared(
            CameraLocation,
            Other->GetActorLocation());
        const bool bOtherClearlyCloser = OtherDistanceSq + 100.0f < SelfDistanceSq;
        const bool bTieBreakWins =
            FMath::IsNearlyEqual(OtherDistanceSq, SelfDistanceSq, 100.0f)
            && GetTypeHash(OtherId) < GetTypeHash(SelfId);

        if (bOtherSelected || bOtherClearlyCloser || bTieBreakWins)
        {
            bCrowdLabelSuppressed = true;
            return;
        }
    }
}

FString ULLResidentPresentationComponent::LifeStageBadge(ELLLifeStage Stage)
{
    switch (Stage)
    {
        case ELLLifeStage::Infant: return TEXT("영아");
        case ELLLifeStage::Child:  return TEXT("아동");
        case ELLLifeStage::Teen:   return TEXT("청소년");
        case ELLLifeStage::Elder:  return TEXT("노년");
        case ELLLifeStage::Adult:
        default:                   return TEXT("성인");
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

    const ALLResidentCharacter* Resident = Cast<ALLResidentCharacter>(GetOwner());
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    ULLObservationSubsystem* Observation =
        GameInstance ? GameInstance->GetSubsystem<ULLObservationSubsystem>() : nullptr;
    ULLCoreBridgeSubsystem* Bridge =
        GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
    const float Daylight01 = Bridge && Bridge->IsCoreRunning()
        ? FMath::Clamp(Bridge->GetTimeObservation().Daylight01, 0.0f, 1.0f)
        : 1.0f;
    const float NightReadability01 = 1.0f - Daylight01;
    const bool bSelected = Resident && Observation && Observation->HasObservedResident()
        && Observation->GetObservedResidentId() == Resident->GetResidentId();

    if (!bSelected && bCrowdLabelSuppressed)
    {
        Label->SetVisibility(false);
        return;
    }

    const FVector CameraLocation = Camera->GetCameraLocation();
    const FVector LabelLocation = Label->GetComponentLocation();
    const float Distance = static_cast<float>(FVector::Dist(CameraLocation, LabelLocation));
    const float MaxVisibleDistance = bSelected ? SelectedLabelMaxDistance : LabelMidDistance;

    if (Distance > MaxVisibleDistance)
    {
        Label->SetVisibility(false);
        return;
    }
    Label->SetVisibility(true);

    if (bResidentDataValid)
    {
        // The selected resident keeps the identity badge visible throughout the
        // focus range; unselected residents only show it up close to reduce
        // world-space text clutter.
        const bool bShowIdentityBadge = bSelected || Distance <= LabelNearDistance;
        FString Wanted = DisplayName;
        if (bShowIdentityBadge)
        {
            Wanted += TEXT(" · ") + LifeStageBadge(LifeStage);
            if (bSelected)
            {
                Wanted += FString::Printf(TEXT(" · %d세"), FMath::Max(0, AgeYears));
            }
        }
        if (!Label->Text.ToString().Equals(Wanted))
        {
            Label->SetText(FText::FromString(Wanted));
        }
    }

    const float FadeBand = FMath::Max(300.0f, MaxVisibleDistance * 0.24f);
    const float FadeStart = FMath::Max(0.0f, MaxVisibleDistance - FadeBand);
    const float DistanceFade = Distance <= FadeStart
        ? 1.0f
        : 1.0f - FMath::Clamp(
            (Distance - FadeStart) / FMath::Max(FadeBand, KINDA_SMALL_NUMBER),
            0.0f,
            1.0f);
    const uint8 BaseAlpha = bSelected
        ? 255
        : static_cast<uint8>(FMath::RoundToInt(
            FMath::Lerp(220.0f, 245.0f, NightReadability01)));
    const uint8 PresentedAlpha = static_cast<uint8>(
        FMath::Clamp(
            FMath::RoundToInt(static_cast<float>(BaseAlpha) * DistanceFade),
            0,
            255));

    const FLinearColor DayUnselected(0.92f, 0.94f, 0.96f, 1.0f);
    const FLinearColor NightUnselected(0.86f, 0.94f, 1.0f, 1.0f);
    const FLinearColor UnselectedColor = FMath::Lerp(
        DayUnselected,
        NightUnselected,
        NightReadability01);
    const FColor PresentedColor = bSelected
        ? FColor(130, 224, 255, PresentedAlpha)
        : FColor(
            static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(UnselectedColor.R * 255.0f), 0, 255)),
            static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(UnselectedColor.G * 255.0f), 0, 255)),
            static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(UnselectedColor.B * 255.0f), 0, 255)),
            PresentedAlpha);
    Label->SetTextRenderColor(PresentedColor);

    const FVector ToCamera = CameraLocation - LabelLocation;
    if (!ToCamera.IsNearlyZero())
    {
        Label->SetWorldRotation(FRotationMatrix::MakeFromX(ToCamera).Rotator());
    }

    const float DistanceScale = FMath::Clamp(
        Distance / LabelReferenceDistance,
        LabelMinScale,
        LabelMaxScale);
    // Label legibility should not shrink in lockstep with body height. Babies
    // and children stay physically smaller, but their names remain readable.
    const float LifeStageLabelScale = FMath::Clamp(
        StageFactor,
        LabelMinLifeStageScale,
        1.0f);
    const float SelectionScale = bSelected ? SelectedLabelSizeMultiplier : 1.0f;
    const float NightScale = FMath::Lerp(1.0f, bSelected ? 1.08f : 1.04f, NightReadability01);
    Label->SetWorldSize(
        LabelBaseWorldSize
        * LifeStageLabelScale
        * DistanceScale
        * SelectionScale
        * NightScale);
}
