#include "Characters/LLResidentCharacter.h"
#include "AI/LLDecisionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"

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
}

void ALLResidentCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bHasMovementTarget)
    {
        return;
    }

    const FVector Current = GetActorLocation();
    FVector FlatTarget = MovementTarget;
    FlatTarget.Z = Current.Z;

    const FVector Delta = FlatTarget - Current;
    if (Delta.SizeSquared2D() <= FMath::Square(TargetAcceptanceRadius))
    {
        bHasMovementTarget = false;
        return;
    }

    const FVector Next = FMath::VInterpConstantTo(Current, FlatTarget, DeltaSeconds, RuntimeMoveSpeed);
    SetActorLocation(Next, true);

    if (!Delta.IsNearlyZero())
    {
        SetActorRotation(FRotator(0.0f, Delta.Rotation().Yaw, 0.0f));
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
