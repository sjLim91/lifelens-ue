#include "Characters/LLResidentCharacter.h"
#include "Characters/LLResidentAppearanceComponent.h"
#include "Characters/LLResidentMotionComponent.h"
#include "Characters/LLResidentPresentationComponent.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameInstance.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    float StableAvoidanceSign(const FGuid& ResidentId)
    {
        const uint32 Mixed = ResidentId.A ^ ResidentId.B ^ ResidentId.C ^ ResidentId.D;
        return (Mixed & 1u) != 0u ? 1.0f : -1.0f;
    }

    FVector StableSideDirection(const FVector& Forward, const FGuid& ResidentId)
    {
        FVector Side(-Forward.Y, Forward.X, 0.0f);
        if (!Side.Normalize())
        {
            Side = FVector::RightVector;
        }
        return Side * StableAvoidanceSign(ResidentId);
    }
}

ALLResidentCharacter::ALLResidentCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    DebugBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugBody"));
    DebugBody->SetupAttachment(RootComponent);
    DebugBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DebugBody->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.9f));
    DebugBody->SetCastShadow(false);

    // Never expose the bootstrap cube while resident identity/appearance binds.
    // Presentation may build a proper human body or its explicit silhouette
    // fallback after BindResident, but the Engine cube is diagnostics-only.
    DebugBody->SetVisibility(false, true);
    DebugBody->SetHiddenInGame(true, true);

    // Diagnostics-only bootstrap geometry. The production human body is built
    // by ULLResidentAppearanceComponent after stable resident identity binds;
    // this hidden cube is retained only as a dependency-light QA sentinel.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (BodyMesh.Succeeded())
    {
        DebugBody->SetStaticMesh(BodyMesh.Object);
    }

    NameLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameLabel"));
    NameLabel->SetupAttachment(RootComponent);
    NameLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 125.0f));
    NameLabel->SetHorizontalAlignment(EHTA_Center);
    NameLabel->SetWorldSize(28.0f);
    NameLabel->SetTextRenderColor(FColor::White);

    // Appearance builds the human body first; Presentation (ring, label,
    // silhouette fallback) reads it in its own BeginPlay.
    AppearanceComponent = CreateDefaultSubobject<ULLResidentAppearanceComponent>(TEXT("AppearanceComponent"));
    PresentationComponent = CreateDefaultSubobject<ULLResidentPresentationComponent>(TEXT("PresentationComponent"));

    // Motion Bootstrap: reflects the actual movement of this actor in the body
    // animation. Presentation only; it never chooses movement or actions.
    MotionComponent = CreateDefaultSubobject<ULLResidentMotionComponent>(TEXT("MotionComponent"));
}

void ALLResidentCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bHasMovementTarget)
    {
        return;
    }

    const FVector StartLocation = GetActorLocation();
    const bool bHasRouteWaypoint =
        MovementWaypointIndex >= 0
        && MovementWaypointIndex < MovementWaypoints.Num();
    FVector FlatTarget = bHasRouteWaypoint
        ? MovementWaypoints[MovementWaypointIndex]
        : MovementTarget;
    FlatTarget.Z = StartLocation.Z;

    FVector ToTarget = FlatTarget - StartLocation;
    ToTarget.Z = 0.0f;
    const float DistanceToTarget = ToTarget.Size2D();
    const bool bIntermediateWaypoint =
        bHasRouteWaypoint
        && MovementWaypointIndex < MovementWaypoints.Num() - 1;
    const float AcceptanceRadius = bIntermediateWaypoint
        ? FMath::Max(1.0f, PathWaypointAcceptanceRadius)
        : FMath::Max(1.0f, TargetAcceptanceRadius);
    if (DistanceToTarget <= AcceptanceRadius)
    {
        if (bIntermediateWaypoint)
        {
            ++MovementWaypointIndex;
        }
        else
        {
            bHasMovementTarget = false;
            MovementWaypoints.Reset();
            MovementWaypointIndex = 0;
        }
        return;
    }

    // Preserve the existing constant-speed movement contract, then sweep the
    // resulting frame step so authoritative blockers can redirect it.
    const FVector ForwardDestination = FMath::VInterpConstantTo(
        StartLocation,
        FlatTarget,
        FMath::Max(0.0f, DeltaSeconds),
        FMath::Max(0.0f, RuntimeMoveSpeed));
    FVector ForwardStep = ForwardDestination - StartLocation;
    ForwardStep.Z = 0.0f;
    const float StepDistance = ForwardStep.Size2D();
    if (StepDistance <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const FVector Forward = ForwardStep.GetSafeNormal2D();

    FHitResult ForwardHit;
    SetActorLocation(ForwardDestination, true, &ForwardHit, ETeleportType::None);

    if (ForwardHit.bBlockingHit)
    {
        const FVector ImpactLocation = GetActorLocation();
        const float ForwardTravelDistance = FMath::Clamp(
            (ImpactLocation - StartLocation).Size2D(),
            0.0f,
            StepDistance);
        const float RemainingStepDistance = FMath::Max(0.0f, StepDistance - ForwardTravelDistance);

        if (RemainingStepDistance > KINDA_SMALL_NUMBER)
        {
            FVector Remaining = FlatTarget - ImpactLocation;
            Remaining.Z = 0.0f;

            FVector SlideDirection = FVector::VectorPlaneProject(Remaining, ForwardHit.ImpactNormal);
            SlideDirection.Z = 0.0f;
            const bool bHadProjectedSlide = SlideDirection.Normalize();
            if (!bHadProjectedSlide)
            {
                SlideDirection = StableSideDirection(Forward, ResidentId);
            }

            const FVector BeforeSlide = GetActorLocation();
            FHitResult SlideHit;
            SetActorLocation(
                BeforeSlide + SlideDirection * RemainingStepDistance,
                true,
                &SlideHit,
                ETeleportType::None);

            // A near head-on hit against a flat box can yield a tangent that is
            // immediately blocked by a neighbouring proxy. If the first sidestep
            // made effectively no progress, try the stable tangent on the other
            // side rather than repeating the same blocked direction or oscillating.
            // The alternate attempt reuses the same remaining frame budget.
            const float SlideProgressSquared = FVector::DistSquared2D(BeforeSlide, GetActorLocation());
            if (SlideProgressSquared <= FMath::Square(1.0f))
            {
                const FVector StableSide = StableSideDirection(Forward, ResidentId);
                const FVector AlternateSide = FVector::DotProduct(SlideDirection, StableSide) >= 0.0f
                    ? -StableSide
                    : StableSide;
                SetActorLocation(
                    BeforeSlide + AlternateSide * RemainingStepDistance,
                    true,
                    nullptr,
                    ETeleportType::None);
            }
        }
    }

    const FVector ActualMove = GetActorLocation() - StartLocation;
    if (ActualMove.SizeSquared2D() > FMath::Square(0.5f))
    {
        SetActorRotation(FRotator(0.0f, ActualMove.Rotation().Yaw, 0.0f));
    }
}

void ALLResidentCharacter::SetPresentationGroundOffsetUU(float OffsetUU)
{
    if (AppearanceComponent)
    {
        AppearanceComponent->SetPresentationGroundOffsetUU(OffsetUU);
    }
}

void ALLResidentCharacter::BindResident(const FLLResidentData& ResidentData)
{
    const bool bIdentityChanged = ResidentId.IsValid() && ResidentId != ResidentData.ResidentId;
    ResidentId = ResidentData.ResidentId;
    ResidentDisplayName = FText::FromString(ResidentData.DisplayName);

    if (bIdentityChanged)
    {
        // Actors should normally stay bound to one stable Core resident. Reset
        // presentation caches defensively if pooling/rebinding is introduced.
        bLifecyclePresentationInitialized = false;
        LastLifecycleStageIndex = INDEX_NONE;
        AdultCapsuleHalfHeight = 0.0f;
        AdultCapsuleRadius = 0.0f;
        AdultRuntimeMoveSpeed = 0.0f;
        AdultBodyScale = FVector::OneVector;
    }

    if (NameLabel)
    {
        NameLabel->SetText(ResidentDisplayName);
    }

    // Appearance is deterministic per ResidentId, so it can only be built once
    // the identity is known. Rebinding refreshes only lifecycle scale; identity,
    // genetics, skin, hair and outfit are never rerolled.
    if (AppearanceComponent)
    {
        AppearanceComponent->EnsureBuilt();
        RefreshLifecyclePresentation();
    }
    if (PresentationComponent)
    {
        PresentationComponent->OnResidentBound();
    }
}

void ALLResidentCharacter::RefreshLifecyclePresentation()
{
    if (!AppearanceComponent || !AppearanceComponent->HasBody() || !ResidentId.IsValid())
    {
        return;
    }

    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    ULLCoreBridgeSubsystem* Bridge = GameInstance
        ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>()
        : nullptr;
    if (!Bridge || !Bridge->IsCoreRunning())
    {
        return;
    }

    FLLCoreResidentObservation Observation;
    if (!Bridge->GetResidentObservation(ResidentId, Observation) || !Observation.bAlive)
    {
        return;
    }

    UCapsuleComponent* Capsule = GetCapsuleComponent();
    USkeletalMeshComponent* Body = AppearanceComponent->GetBodyComponent();
    if (!Capsule || !Body)
    {
        return;
    }

    const int32 StageIndex = FMath::Clamp(static_cast<int32>(Observation.LifeStage), 0, 7);
    const float StageFactor = ULLResidentAppearanceComponent::StageHeightFactor[StageIndex];
    const float StageWidthFactor = ULLResidentAppearanceComponent::StageWidthFactor[StageIndex];

    // Hair greying / child hair volume / beard visibility may change while the
    // broad LifeStage remains the same, so update this before the stage-size
    // early return below.
    AppearanceComponent->ApplyLifecycleAgePresentation(
        Observation.LifeStage,
        Observation.AgeYears);

    if (!bLifecyclePresentationInitialized)
    {
        AdultCapsuleHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
        AdultCapsuleRadius = Capsule->GetUnscaledCapsuleRadius();
        // RuntimeMoveSpeed is authored as the adult/reference presentation
        // speed. Capture it once so stage transitions never compound scaling.
        AdultRuntimeMoveSpeed = FMath::Max(0.0f, RuntimeMoveSpeed);

        const int32 BuiltStageIndex = FMath::Clamp(
            static_cast<int32>(AppearanceComponent->GetInputs().LifeStage), 0, 7);
        const float BuiltStageFactor = FMath::Max(
            0.01f,
            ULLResidentAppearanceComponent::StageHeightFactor[BuiltStageIndex]);
        const float BuiltStageWidthFactor = FMath::Max(
            0.01f,
            ULLResidentAppearanceComponent::StageWidthFactor[BuiltStageIndex]);

        // Normalize the initially built stage back to the resident's adult
        // genetic/body baseline. XY contains both stage height and stage-width
        // shaping, while Z contains stage height only. Dividing the whole vector
        // by BuiltStageFactor used to bake the birth/child width factor into the
        // baseline and multiply it again on every later life-stage transition.
        AdultBodyScale = Body->GetRelativeScale3D();
        AdultBodyScale.X /= BuiltStageFactor * BuiltStageWidthFactor;
        AdultBodyScale.Y /= BuiltStageFactor * BuiltStageWidthFactor;
        AdultBodyScale.Z /= BuiltStageFactor;
        bLifecyclePresentationInitialized = true;
    }

    // Core already defines lifecycle locomotion pacing (child 0.82, teen 0.95,
    // elderly 0.78, etc.). Consume that DTO instead of making every rendered
    // resident move at the same adult speed. Direct-care babies/toddlers remain
    // governed by Core and normally have no autonomous plan.
    RuntimeMoveSpeed = AdultRuntimeMoveSpeed * FMath::Clamp(
        Observation.MovementScale,
        0.05f,
        2.0f);

    if (LastLifecycleStageIndex == StageIndex)
    {
        return;
    }

    const float OldScaledHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    const float GroundZ = GetActorLocation().Z - OldScaledHalfHeight;

    const float NewRadius = FMath::Max(
        1.0f,
        AdultCapsuleRadius * StageFactor * StageWidthFactor);
    const float NewHalfHeight = FMath::Max(
        NewRadius,
        AdultCapsuleHalfHeight * StageFactor);
    Capsule->SetCapsuleSize(NewRadius, NewHalfHeight, true);

    const float NewScaledHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    FVector NewActorLocation = GetActorLocation();
    NewActorLocation.Z = GroundZ + NewScaledHalfHeight;
    SetActorLocation(NewActorLocation, false, nullptr, ETeleportType::TeleportPhysics);

    FVector NewBodyScale = AdultBodyScale;
    NewBodyScale.X *= StageFactor * StageWidthFactor;
    NewBodyScale.Y *= StageFactor * StageWidthFactor;
    NewBodyScale.Z *= StageFactor;
    AppearanceComponent->ApplyLifecyclePresentationScale(NewScaledHalfHeight, NewBodyScale);
    LastLifecycleStageIndex = StageIndex;

    UE_LOG(LogTemp, Log,
        TEXT("LifeLens lifecycle presentation: resident=%s stage=%d factor=%.2f capsule=(%.1f, %.1f)"),
        *ResidentId.ToString(EGuidFormats::DigitsWithHyphens),
        StageIndex,
        StageFactor,
        NewRadius,
        NewHalfHeight);
}

void ALLResidentCharacter::SetMovementTarget(const FVector& TargetLocation)
{
    if (IsMovingToward(TargetLocation))
    {
        return;
    }

    MovementTarget = TargetLocation;
    MovementWaypoints.Reset();
    MovementWaypointIndex = 0;
    bHasMovementTarget = true;
}

void ALLResidentCharacter::SetMovementPath(
    const TArray<FVector>& PathPoints,
    const FVector& FinalTarget)
{
    if (IsMovingToward(FinalTarget))
    {
        return;
    }

    MovementTarget = FinalTarget;
    MovementWaypoints = PathPoints;
    MovementWaypointIndex = 0;

    if (MovementWaypoints.Num() == 0
        || FVector::DistSquared2D(MovementWaypoints.Last(), FinalTarget)
            > FMath::Square(1.0f))
    {
        MovementWaypoints.Add(FinalTarget);
    }

    bHasMovementTarget = true;
}

void ALLResidentCharacter::ClearMovementTarget()
{
    bHasMovementTarget = false;
    MovementWaypoints.Reset();
    MovementWaypointIndex = 0;
}

bool ALLResidentCharacter::IsMovingToward(
    const FVector& TargetLocation,
    float ToleranceUU) const
{
    return bHasMovementTarget
        && FVector::DistSquared2D(MovementTarget, TargetLocation)
            <= FMath::Square(FMath::Max(0.0f, ToleranceUU));
}

bool ALLResidentCharacter::HasReachedMovementTarget() const
{
    if (!bHasMovementTarget)
    {
        return true;
    }

    return FVector::DistSquared2D(GetActorLocation(), MovementTarget) <= FMath::Square(TargetAcceptanceRadius);
}
