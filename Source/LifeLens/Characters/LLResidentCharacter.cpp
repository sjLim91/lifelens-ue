#include "Characters/LLResidentCharacter.h"
#include "AI/LLDecisionComponent.h"
#include "Characters/LLResidentAppearanceComponent.h"
#include "Characters/LLResidentMotionComponent.h"
#include "Characters/LLResidentPresentationComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
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

    DecisionComponent = CreateDefaultSubobject<ULLDecisionComponent>(TEXT("DecisionComponent"));

    DebugBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugBody"));
    DebugBody->SetupAttachment(RootComponent);
    DebugBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DebugBody->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.9f));

    // /Engine/BasicShapes/Capsule is not present in the UE 5.6 slim build image.
    // Cube is a stable engine asset already used by the runtime smoke world, so
    // keep the placeholder body dependency-free until real character meshes land.
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
    FVector FlatTarget = MovementTarget;
    FlatTarget.Z = StartLocation.Z;

    FVector ToTarget = FlatTarget - StartLocation;
    ToTarget.Z = 0.0f;
    const float DistanceToTarget = ToTarget.Size2D();
    if (DistanceToTarget <= FMath::Max(1.0f, TargetAcceptanceRadius))
    {
        bHasMovementTarget = false;
        return;
    }

    // Preserve the existing constant-speed movement contract, then sweep the
    // resulting frame step so visible solid dressing can block and redirect it.
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
            // made effectively no progress, try the resident's stable opposite
            // side rather than oscillating left/right every frame. The alternate
            // attempt uses the same remaining frame budget, never an extra step.
            const float SlideProgressSquared = FVector::DistSquared2D(BeforeSlide, GetActorLocation());
            if (SlideProgressSquared <= FMath::Square(1.0f))
            {
                const FVector FallbackSide = StableSideDirection(Forward, ResidentId);
                const FVector AlternateSide = bHadProjectedSlide
                    ? FallbackSide
                    : -FallbackSide;
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

void ALLResidentCharacter::BindResident(const FLLResidentData& ResidentData)
{
    ResidentId = ResidentData.ResidentId;
    ResidentDisplayName = FText::FromString(ResidentData.DisplayName);

    if (NameLabel)
    {
        NameLabel->SetText(ResidentDisplayName);
    }

    // Appearance is deterministic per ResidentId, so it can only be built once
    // the identity is known. Both calls are idempotent and re-binding the same
    // resident does not rebuild.
    if (AppearanceComponent)
    {
        AppearanceComponent->EnsureBuilt();
    }
    if (PresentationComponent)
    {
        PresentationComponent->OnResidentBound();
    }
}

void ALLResidentCharacter::SetMovementTarget(const FVector& TargetLocation)
{
    MovementTarget = TargetLocation;
    bHasMovementTarget = true;
}

void ALLResidentCharacter::ClearMovementTarget()
{
    bHasMovementTarget = false;
}

bool ALLResidentCharacter::HasReachedMovementTarget() const
{
    if (!bHasMovementTarget)
    {
        return true;
    }

    return FVector::DistSquared2D(GetActorLocation(), MovementTarget) <= FMath::Square(TargetAcceptanceRadius);
}
