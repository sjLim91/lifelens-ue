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
        if (Character.MotionComponent) Character.MotionComponent->SetSocialInteractionActive(bActive);
    };
    auto SetWorkPresentation = [&](ELLResidentWorkPresentationMode Mode)
    {
        if (Character.MotionComponent) Character.MotionComponent->SetWorkPresentationMode(Mode);
    };
    auto SetHeldToolPresentation = [&](ELLResidentHeldToolPresentation Tool)
    {
        if (Character.MotionComponent) Character.MotionComponent->SetHeldToolPresentation(Tool);
    };
    auto ClearContextPresentation = [&]()
    {
        SetTalkingPresentation(false);
        SetWorkPresentation(ELLResidentWorkPresentationMode::None);
        SetHeldToolPresentation(ELLResidentHeldToolPresentation::None);
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
        1.0f,FMath::Max(1.0f, CoreGridCellSizeUU) * 1.40f);
    FVector DesiredLocation = Character.GetActorLocation();
    ALLResidentCharacter* TargetResident = nullptr;
    float ArrivalRadius = FMath::Min(
        FMath::Max(1.0f, ContextWorldTargetArrivalRadiusUU),AckSafeArrivalRadius);
    bool bFaceTarget = false;
    bool bFaceWorldTarget = false;
    bool bTalkAtTarget = false;
    ELLResidentWorkPresentationMode WorkAtTarget = ELLResidentWorkPresentationMode::None;
    ELLResidentHeldToolPresentation HeldToolAtTarget = ELLResidentHeldToolPresentation::None;

    auto ResolveAuthoritativeResidentTarget = [&](FGuid TargetResidentId, FVector& OutLocation) -> bool
    {
        int32 TargetGridX = 0;
        int32 TargetGridY = 0;
        if (!CoreBridge->GetResidentRuntimeGridPosition(TargetResidentId, TargetGridX, TargetGridY)) return false;
        const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
        OutLocation = GetActorLocation()
            + FVector(static_cast<float>(TargetGridX - CorePresentationOriginGrid.X) * CellSize,
                      static_cast<float>(TargetGridY - CorePresentationOriginGrid.Y) * CellSize,0.0f);
        OutLocation.Z = Character.GetActorLocation().Z;
        return true;
    };

    switch (Directive.ContextActionKind)
    {
        case ELLCoreContextActionKind::Social:
            TargetResident = FindResidentActor(Directive.TargetResidentId);
            if (!TargetResident || Directive.SocialIntent == ELLCoreSocialIntent::None)
            {
                ClearContextPresentation(); Runtime.bPerformingAction = false; Character.ClearMovementTarget(); return;
            }
            if (Directive.SocialIntent == ELLCoreSocialIntent::Avoid)
            {
                FVector AwayDirection = Character.GetActorLocation() - TargetResident->GetActorLocation();
                AwayDirection.Z = 0.0f;
                if (AwayDirection.IsNearlyZero())
                {
                    const bool bPositive = (GetTypeHash(Character.GetResidentId()) & 1u) == 0u;
                    AwayDirection = bPositive ? FVector(1.0f,0.0f,0.0f) : FVector(0.0f,1.0f,0.0f);
                }
                AwayDirection.Normalize();
                DesiredLocation = Character.GetActorLocation() + AwayDirection * 420.0f;
                ArrivalRadius = FMath::Max(1.0f, ContextResidentArrivalRadiusUU);
            }
            else
            {
                if (!ResolveAuthoritativeResidentTarget(Directive.TargetResidentId, DesiredLocation))
                {
                    ClearContextPresentation(); Runtime.bPerformingAction = false; Character.ClearMovementTarget(); return;
                }
                ArrivalRadius = FMath::Min(FMath::Max(1.0f, ContextResidentArrivalRadiusUU),AckSafeArrivalRadius);
                bFaceTarget = true;
                bTalkAtTarget = true;
            }
            Character.SetCurrentIntent(ELLActionIntent::Socialize);
            break;

        case ELLCoreContextActionKind::KnowledgeTeaching:
            TargetResident = FindResidentActor(Directive.TargetResidentId);
            if (!TargetResident
                || !ResolveAuthoritativeResidentTarget(Directive.TargetResidentId, DesiredLocation))
            {
                ClearContextPresentation(); Runtime.bPerformingAction = false; Character.ClearMovementTarget(); return;
            }
            ArrivalRadius = FMath::Min(FMath::Max(1.0f, ContextResidentArrivalRadiusUU),AckSafeArrivalRadius);
            Character.SetCurrentIntent(ELLActionIntent::Socialize);
            bFaceTarget = true;
            bTalkAtTarget = true;
            break;

        case ELLCoreContextActionKind::Parenting:
            TargetResident = FindResidentActor(Directive.TargetResidentId);
            if (!TargetResident || Directive.ParentingAction == ELLCoreParentingAction::None
                || !ResolveAuthoritativeResidentTarget(Directive.TargetResidentId, DesiredLocation))
            {
                ClearContextPresentation(); Runtime.bPerformingAction = false; Character.ClearMovementTarget(); return;
            }
            ArrivalRadius = FMath::Min(FMath::Max(1.0f, ContextResidentArrivalRadiusUU),AckSafeArrivalRadius);
            Character.SetCurrentIntent(ELLActionIntent::Socialize);
            bFaceTarget = true;
            break;

        case ELLCoreContextActionKind::Civilization:
            Character.SetCurrentIntent(ELLActionIntent::Idle);
            if (Directive.CivilizationAction == ELLCoreCivilizationAction::Gather)
            {
                WorkAtTarget = ELLResidentWorkPresentationMode::Gather;
                bFaceWorldTarget = true;
                if (Directive.bHasCivilizationTool)
                {
                    switch (Directive.CivilizationToolItem)
                    {
                        case ELLCoreItemKind::SharpFlake: HeldToolAtTarget = ELLResidentHeldToolPresentation::SharpFlake; break;
                        case ELLCoreItemKind::StoneCuttingTool: HeldToolAtTarget = ELLResidentHeldToolPresentation::StoneCuttingTool; break;
                        case ELLCoreItemKind::SimpleContainer: HeldToolAtTarget = ELLResidentHeldToolPresentation::SimpleContainer; break;
                        case ELLCoreItemKind::DiggingStick: HeldToolAtTarget = ELLResidentHeldToolPresentation::DiggingStick; break;
                        case ELLCoreItemKind::StoneHammer: HeldToolAtTarget = ELLResidentHeldToolPresentation::StoneHammer; break;
                        default: break;
                    }
                }
            }
            else if (Directive.CivilizationAction == ELLCoreCivilizationAction::Experiment
                && Directive.CivilizationTechnique == ELLCoreTechniqueId::CopperSmelting)
            {
                bFaceWorldTarget = true;
                WorkAtTarget = ELLResidentWorkPresentationMode::Interact;
            }
            else if (Directive.CivilizationTechnique == ELLCoreTechniqueId::PrimitiveStorage)
            {
                bFaceWorldTarget = true;
                switch (Directive.CivilizationFacilityAction)
                {
                    case ELLCoreFacilityBuildAction::Plan:
                    case ELLCoreFacilityBuildAction::DeliverMaterial:
                        WorkAtTarget = ELLResidentWorkPresentationMode::Interact; break;
                    case ELLCoreFacilityBuildAction::Work:
                        WorkAtTarget = ELLResidentWorkPresentationMode::Build; break;
                    default: break;
                }
            }
            else if (Directive.CivilizationTechnique == ELLCoreTechniqueId::FireMaking
                && Directive.CivilizationFacilityKind == ELLCoreFacilityKind::FirePit)
            {
                bFaceWorldTarget = true;
                switch (Directive.CivilizationFacilityAction)
                {
                    case ELLCoreFacilityBuildAction::Work:
                        WorkAtTarget = ELLResidentWorkPresentationMode::Build; break;
                    case ELLCoreFacilityBuildAction::Plan:
                    case ELLCoreFacilityBuildAction::DeliverMaterial:
                    case ELLCoreFacilityBuildAction::Fuel:
                    case ELLCoreFacilityBuildAction::Ignite:
                    case ELLCoreFacilityBuildAction::CollectCharcoal:
                        WorkAtTarget = ELLResidentWorkPresentationMode::Interact; break;
                    default: break;
                }
            }
            else if (Directive.CivilizationFacilityKind == ELLCoreFacilityKind::Furnace
                && (Directive.CivilizationTechnique == ELLCoreTechniqueId::FireMaking
                    || Directive.CivilizationTechnique == ELLCoreTechniqueId::CopperSmelting))
            {
                bFaceWorldTarget = true;
                switch (Directive.CivilizationFacilityAction)
                {
                    case ELLCoreFacilityBuildAction::Work:
                        WorkAtTarget = ELLResidentWorkPresentationMode::Build; break;
                    case ELLCoreFacilityBuildAction::Plan:
                    case ELLCoreFacilityBuildAction::DeliverMaterial:
                    case ELLCoreFacilityBuildAction::LoadSmeltCharge:
                    case ELLCoreFacilityBuildAction::Ignite:
                    case ELLCoreFacilityBuildAction::CollectMetal:
                        WorkAtTarget = ELLResidentWorkPresentationMode::Interact; break;
                    default: break;
                }
            }

            if (Directive.bHasCivilizationSpatialTarget)
            {
                const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
                DesiredLocation = GetActorLocation()
                    + FVector(static_cast<float>(Directive.CivilizationTargetGridX - CorePresentationOriginGrid.X) * CellSize,
                              static_cast<float>(Directive.CivilizationTargetGridY - CorePresentationOriginGrid.Y) * CellSize,0.0f);
                DesiredLocation.Z = Character.GetActorLocation().Z;
                ArrivalRadius = FMath::Min(FMath::Max(1.0f, ContextWorldTargetArrivalRadiusUU),AckSafeArrivalRadius);
            }
            break;

        case ELLCoreContextActionKind::None:
        default:
            ClearContextPresentation();
            return;
    }

    const double DistanceSquared = FVector::DistSquared2D(Character.GetActorLocation(), DesiredLocation);
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
    SetHeldToolPresentation(HeldToolAtTarget);

    if (bFaceTarget && TargetResident)
    {
        FVector LookDirection = TargetResident->GetActorLocation() - Character.GetActorLocation();
        LookDirection.Z = 0.0f;
        if (!LookDirection.IsNearlyZero()) Character.SetActorRotation(FRotator(0.0f, LookDirection.Rotation().Yaw, 0.0f));
    }
    else if (bFaceWorldTarget)
    {
        FVector LookDirection = DesiredLocation - Character.GetActorLocation();
        LookDirection.Z = 0.0f;
        if (!LookDirection.IsNearlyZero()) Character.SetActorRotation(FRotator(0.0f, LookDirection.Rotation().Yaw, 0.0f));
    }

    Runtime.ContextUseElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
    const float RequiredUseSeconds = FMath::Max(
        0.1f,static_cast<float>(FMath::Max(1, Directive.ContextActionDurationTicks))
            * FMath::Max(0.1f, RealSecondsPerSimulationMinute));
    if (Runtime.ContextUseElapsedSeconds < RequiredUseSeconds) return;

    const FIntPoint ResolvedGrid = WorldLocationToCoreGrid(Character.GetActorLocation());
    const bool bAcknowledged = CoreBridge->CompleteResidentContextAction(
        Character.GetResidentId(),Directive.ContextActionToken,ResolvedGrid.X,ResolvedGrid.Y);

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
