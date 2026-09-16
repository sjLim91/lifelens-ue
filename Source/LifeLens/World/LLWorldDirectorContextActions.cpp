#include "World/LLWorldDirector.h"

#include "Characters/LLResidentCharacter.h"
#include "Characters/LLResidentMotionComponent.h"
#include "Simulation/LLCoreBridgeSubsystem.h"

void ALLWorldDirector::ApplyPendingContextDirective(
    ALLResidentCharacter& Character,
    FLLResidentRuntimeState& Runtime,
    const FLLCoreActionDirective& Directive,
    float DeltaSeconds)
{
    auto SetTalkingPresentation = [&](bool bActive)
    {
        if (Character.MotionComponent)
        {
            Character.MotionComponent->SetSocialInteractionActive(bActive);
        }
    };
    auto SetWorkPresentation = [&](ELLResidentWorkPresentationMode Mode)
    {
        if (Character.MotionComponent)
        {
            Character.MotionComponent->SetWorkPresentationMode(Mode);
        }
    };
    auto ClearContextPresentation = [&]()
    {
        SetTalkingPresentation(false);
        SetWorkPresentation(ELLResidentWorkPresentationMode::None);
    };

    if (!CoreBridge
        || Directive.ContextActionKind == ELLCoreContextActionKind::None
        || Directive.ContextActionToken <= 0)
    {
        ClearContextPresentation();
        Runtime.ActiveContextActionToken = 0;
        Runtime.ContextUseElapsedSeconds = 0.0f;
        Runtime.bPerformingAction = false;
        Character.ClearMovementTarget();
        return;
    }

    if (Runtime.ActiveContextActionToken != Directive.ContextActionToken)
    {
        ClearContextPresentation();
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
    bool bTalkAtTarget = false;
    ELLResidentWorkPresentationMode WorkAtTarget = ELLResidentWorkPresentationMode::None;

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
                ClearContextPresentation();
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
                DesiredLocation = Character.GetActorLocation() + AwayDirection * 420.0f;
                ArrivalRadius = FMath::Max(1.0f, ContextResidentArrivalRadiusUU);
            }
            else
            {
                if (!ResolveAuthoritativeResidentTarget(Directive.TargetResidentId, DesiredLocation))
                {
                    ClearContextPresentation();
                    Runtime.bPerformingAction = false;
                    Character.ClearMovementTarget();
                    return;
                }
                ArrivalRadius = FMath::Min(
                    FMath::Max(1.0f, ContextResidentArrivalRadiusUU),
                    AckSafeArrivalRadius);
                bFaceTarget = true;
                bTalkAtTarget = true;
            }
            Character.SetCurrentIntent(ELLActionIntent::Socialize);
            break;

        case ELLCoreContextActionKind::Parenting:
            TargetResident = FindResidentActor(Directive.TargetResidentId);
            if (!TargetResident || Directive.ParentingAction == ELLCoreParentingAction::None)
            {
                ClearContextPresentation();
                Runtime.bPerformingAction = false;
                Character.ClearMovementTarget();
                return;
            }
            if (!ResolveAuthoritativeResidentTarget(Directive.TargetResidentId, DesiredLocation))
            {
                ClearContextPresentation();
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
            if (Directive.CivilizationTechnique == ELLCoreTechniqueId::PrimitiveStorage)
            {
                switch (Directive.CivilizationFacilityAction)
                {
                    case ELLCoreFacilityBuildAction::Plan:
                    case ELLCoreFacilityBuildAction::DeliverMaterial:
                        WorkAtTarget = ELLResidentWorkPresentationMode::Interact;
                        break;
                    case ELLCoreFacilityBuildAction::Work:
                        WorkAtTarget = ELLResidentWorkPresentationMode::Build;
                        break;
                    case ELLCoreFacilityBuildAction::None:
                    default:
                        break;
                }
            }
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
            ClearContextPresentation();
            return;
    }

    const double DistanceSquared = FVector::DistSquared2D(
        Character.GetActorLocation(), DesiredLocation);
    if (DistanceSquared > FMath::Square(ArrivalRadius))
    {
        ClearContextPresentation();
        Runtime.bPerformingAction = false;
        Runtime.ContextUseElapsedSeconds = 0.0f;
        Character.SetMovementTarget(DesiredLocation);
        return;
    }

    Character.ClearMovementTarget();
    Runtime.bPerformingAction = true;
    SetTalkingPresentation(bTalkAtTarget);
    SetWorkPresentation(WorkAtTarget);

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
        ClearContextPresentation();
        Runtime.ActiveContextActionToken = 0;
        Runtime.bPerformingAction = false;
        Character.ClearMovementTarget();
        Character.SetCurrentIntent(ELLActionIntent::Idle);
    }
}
