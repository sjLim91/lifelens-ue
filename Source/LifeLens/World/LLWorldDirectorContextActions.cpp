#include "World/LLWorldDirector.h"

#include "Characters/LLResidentCharacter.h"
#include "Simulation/LLCoreBridgeSubsystem.h"

void ALLWorldDirector::ApplyPendingContextDirective(
    ALLResidentCharacter& Character,
    FLLResidentRuntimeState& Runtime,
    const FLLCoreActionDirective& Directive,
    float DeltaSeconds)
{
    if (!CoreBridge
        || Directive.ContextActionKind == ELLCoreContextActionKind::None
        || Directive.ContextActionToken <= 0)
    {
        Runtime.ActiveContextActionToken = 0;
        Runtime.ContextUseElapsedSeconds = 0.0f;
        Runtime.bPerformingAction = false;
        Character.ClearMovementTarget();
        return;
    }

    if (Runtime.ActiveContextActionToken != Directive.ContextActionToken)
    {
        ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
        Runtime.ActiveContextActionToken = Directive.ContextActionToken;
        Runtime.ContextUseElapsedSeconds = 0.0f;
        Runtime.bPerformingAction = false;
        Character.ClearMovementTarget();
    }

    FVector DesiredLocation = Character.GetActorLocation();
    ALLResidentCharacter* TargetResident = nullptr;
    float ArrivalRadius = FMath::Max(1.0f, ContextWorldTargetArrivalRadiusUU);
    bool bFaceTarget = false;

    switch (Directive.ContextActionKind)
    {
        case ELLCoreContextActionKind::Social:
            TargetResident = FindResidentActor(Directive.TargetResidentId);
            if (!TargetResident || Directive.SocialIntent == ELLCoreSocialIntent::None)
            {
                Runtime.bPerformingAction = false;
                Character.ClearMovementTarget();
                return;
            }
            if (Directive.SocialIntent == ELLCoreSocialIntent::Avoid)
            {
                FVector AwayDirection = Character.GetActorLocation() - TargetResident->GetActorLocation();
                AwayDirection.Z = 0.0f;
                if (AwayDirection.IsNearlyZero())
                {
                    const bool bPositive = (GetTypeHash(Character.GetResidentId()) & 1u) == 0u;
                    AwayDirection = bPositive
                        ? FVector(1.0f, 0.0f, 0.0f)
                        : FVector(0.0f, 1.0f, 0.0f);
                }
                AwayDirection.Normalize();
                DesiredLocation = TargetResident->GetActorLocation() + AwayDirection * 420.0f;
            }
            else
            {
                DesiredLocation = ResolveSocialTargetLocation(
                    Character, *TargetResident, Directive.SocialIntent);
                bFaceTarget = true;
            }
            ArrivalRadius = FMath::Max(1.0f, ContextResidentArrivalRadiusUU);
            Character.SetCurrentIntent(ELLActionIntent::Socialize);
            break;

        case ELLCoreContextActionKind::Parenting:
            TargetResident = FindResidentActor(Directive.TargetResidentId);
            if (!TargetResident || Directive.ParentingAction == ELLCoreParentingAction::None)
            {
                Runtime.bPerformingAction = false;
                Character.ClearMovementTarget();
                return;
            }
            DesiredLocation = ResolveSocialTargetLocation(
                Character, *TargetResident, ELLCoreSocialIntent::Approach);
            ArrivalRadius = FMath::Max(1.0f, ContextResidentArrivalRadiusUU);
            Character.SetCurrentIntent(ELLActionIntent::Socialize);
            bFaceTarget = true;
            break;

        case ELLCoreContextActionKind::Civilization:
            Character.SetCurrentIntent(ELLActionIntent::Idle);
            if (Directive.bHasCivilizationSpatialTarget)
            {
                const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
                DesiredLocation = GetActorLocation()
                    + FVector(
                        static_cast<float>(
                            Directive.CivilizationTargetGridX - CorePresentationOriginGrid.X) * CellSize,
                        static_cast<float>(
                            Directive.CivilizationTargetGridY - CorePresentationOriginGrid.Y) * CellSize,
                        0.0f);
                DesiredLocation.Z = Character.GetActorLocation().Z;
            }
            break;

        case ELLCoreContextActionKind::None:
        default:
            return;
    }

    const double DistanceSquared = FVector::DistSquared2D(
        Character.GetActorLocation(), DesiredLocation);
    if (DistanceSquared > FMath::Square(ArrivalRadius))
    {
        Runtime.bPerformingAction = false;
        Runtime.ContextUseElapsedSeconds = 0.0f;
        Character.SetMovementTarget(DesiredLocation);
        return;
    }

    Character.ClearMovementTarget();
    Runtime.bPerformingAction = true;

    if (bFaceTarget && TargetResident)
    {
        FVector LookDirection = TargetResident->GetActorLocation() - Character.GetActorLocation();
        LookDirection.Z = 0.0f;
        if (!LookDirection.IsNearlyZero())
        {
            Character.SetActorRotation(FRotator(0.0f, LookDirection.Rotation().Yaw, 0.0f));
        }
    }

    Runtime.ContextUseElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
    const float RequiredUseSeconds = FMath::Max(
        0.1f,
        static_cast<float>(FMath::Max(1, Directive.ContextActionDurationTicks))
            * FMath::Max(0.1f, RealSecondsPerSimulationMinute));
    if (Runtime.ContextUseElapsedSeconds < RequiredUseSeconds)
    {
        return;
    }

    const FIntPoint ResolvedGrid = WorldLocationToCoreGrid(Character.GetActorLocation());
    const bool bAcknowledged = CoreBridge->CompleteResidentContextAction(
        Character.GetResidentId(),
        Directive.ContextActionToken,
        ResolvedGrid.X,
        ResolvedGrid.Y);

    Runtime.ContextUseElapsedSeconds = 0.0f;
    if (bAcknowledged)
    {
        Runtime.ActiveContextActionToken = 0;
        Runtime.bPerformingAction = false;
        Character.ClearMovementTarget();
        Character.SetCurrentIntent(ELLActionIntent::Idle);
    }
}
