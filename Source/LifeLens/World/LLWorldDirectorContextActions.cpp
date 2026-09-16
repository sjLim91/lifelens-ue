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

    const float AckSafeArrivalRadius = FMath::Max(
        1.0f,
        FMath::Max(1.0f, CoreGridCellSizeUU) * 1.40f);
    FVector DesiredLocation = Character.GetActorLocation();
    ALLResidentCharacter* TargetResident = nullptr;
    float ArrivalRadius = FMath::Min(
        FMath::Max(1.0f, ContextWorldTargetArrivalRadiusUU),
        AckSafeArrivalRadius);
    bool bFaceTarget = false;

    auto ResolveAuthoritativeResidentTarget = [&](FGuid TargetResidentId, FVector& OutLocation) -> bool
    {
        int32 TargetGridX = 0;
        int32 TargetGridY = 0;
        if (!CoreBridge->GetResidentRuntimeGridPosition(TargetResidentId, TargetGridX, TargetGridY))
        {
            return false;
        }

        const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
        OutLocation = GetActorLocation()
            + FVector(
                static_cast<float>(TargetGridX - CorePresentationOriginGrid.X) * CellSize,
                static_cast<float>(TargetGridY - CorePresentationOriginGrid.Y) * CellSize,
                0.0f);
        OutLocation.Z = Character.GetActorLocation().Z;
        return true;
    };

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
                // Avoid must always increase separation from the current point;
                // targeting a fixed radius around the other resident could make
                // an already-distant resident walk back toward that resident.
                DesiredLocation = Character.GetActorLocation() + AwayDirection * 420.0f;
                ArrivalRadius = FMath::Max(1.0f, ContextResidentArrivalRadiusUU);
            }
            else
            {
                if (!ResolveAuthoritativeResidentTarget(Directive.TargetResidentId, DesiredLocation))
                {
                    Runtime.bPerformingAction = false;
                    Character.ClearMovementTarget();
                    return;
                }
                ArrivalRadius = FMath::Min(
                    FMath::Max(1.0f, ContextResidentArrivalRadiusUU),
                    AckSafeArrivalRadius);
                bFaceTarget = true;
            }
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
            if (!ResolveAuthoritativeResidentTarget(Directive.TargetResidentId, DesiredLocation))
            {
                Runtime.bPerformingAction = false;
                Character.ClearMovementTarget();
                return;
            }
            ArrivalRadius = FMath::Min(
                FMath::Max(1.0f, ContextResidentArrivalRadiusUU),
                AckSafeArrivalRadius);
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
                ArrivalRadius = FMath::Min(
                    FMath::Max(1.0f, ContextWorldTargetArrivalRadiusUU),
                    AckSafeArrivalRadius);
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
