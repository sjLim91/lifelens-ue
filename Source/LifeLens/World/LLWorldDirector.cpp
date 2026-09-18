#include "World/LLWorldDirector.h"
#include "World/LLActivityAnchor.h"
#include "World/LLEnvironmentalResidueVisualizerComponent.h"
#include "Characters/LLResidentCharacter.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLSimulationSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/SceneComponent.h"

ALLWorldDirector::ALLWorldDirector()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    EnvironmentalResidueVisualizer = CreateDefaultSubobject<ULLEnvironmentalResidueVisualizerComponent>(
        TEXT("EnvironmentalResidueVisualizer"));
    EnvironmentalResidueVisualizer->SetupAttachment(SceneRoot);
}

void ALLWorldDirector::BeginPlay()
{
    Super::BeginPlay();

    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    Simulation = GameInstance ? GameInstance->GetSubsystem<ULLSimulationSubsystem>() : nullptr;
    CoreBridge = GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
    if (!Simulation || !CoreBridge)
    {
        return;
    }

    if (!Simulation->LoadGame())
    {
        Simulation->NewGame();
    }

    CoreBridge->SetExternalPhysicalExecutionEnabled(true);
    RefreshCorePresentationOrigin();
    CollectActivityAnchors();
    SpawnResidents();
    if (EnvironmentalResidueVisualizer)
    {
        EnvironmentalResidueVisualizer->RefreshFromCore(*CoreBridge, CoreGridCellSizeUU, true,
            CorePresentationOriginGrid.X, CorePresentationOriginGrid.Y);
    }
}

void ALLWorldDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!Simulation || !CoreBridge)
    {
        return;
    }

    CoreBridge->SetExternalPhysicalExecutionEnabled(true);

    const float RealDeltaSeconds = FMath::Max(0.0f, DeltaSeconds);
    const float SpeedMultiplier = FMath::Max(0.0f, Simulation->GetSimulationSpeedMultiplier());

    bool bAdvancedSimulation = false;
    if (SpeedMultiplier > KINDA_SMALL_NUMBER)
    {
        // Core time consumes scaled real time, while visual refresh below stays
        // on the render clock. This keeps Pause/1x/4x/16x/64x from creating a
        // second simulation truth in Presentation.
        SimulationClockAccumulator += RealDeltaSeconds * SpeedMultiplier;
        const float StepSeconds = FMath::Max(0.01f, RealSecondsPerSimulationMinute);
        const int32 StepBudget = FMath::Max(1, MaxSimulationMinutesPerFrame);
        int32 StepsThisFrame = 0;
        while (SimulationClockAccumulator >= StepSeconds && StepsThisFrame < StepBudget)
        {
            SimulationClockAccumulator -= StepSeconds;
            Simulation->AdvanceSimulationMinutes(1);
            ++StepsThisFrame;
            bAdvancedSimulation = true;

            if ((Simulation->GetSimulationMinute() % 60) == 0)
            {
                Simulation->SaveGame();
            }
        }
    }

    if (bAdvancedSimulation)
    {
        CollectActivityAnchors();
        SpawnResidents();
    }

    // Physical presentation follows the same Observer speed so authoritative
    // external actions do not become a real-time bottleneck at high speed.
    // Pausing disables resident movement ticks without stopping the render/UI
    // clock. Core itself remains frozen because no simulation minutes advance.
    const bool bPaused = SpeedMultiplier <= KINDA_SMALL_NUMBER;
    const float SimulationDeltaSeconds = RealDeltaSeconds * SpeedMultiplier;
    for (ALLResidentCharacter* Character : SpawnedResidents)
    {
        if (IsValid(Character))
        {
            Character->SetActorTickEnabled(!bPaused);
            Character->CustomTimeDilation = bPaused ? 1.0f : SpeedMultiplier;
            UpdateResident(*Character, SimulationDeltaSeconds);
        }
    }

    for (auto& Pair : RuntimeStates)
    {
        if (!FindResidentActor(Pair.Key))
        {
            ReleasePhysicalReservation(Pair.Key, Pair.Value);
        }
    }

    EnvironmentalVisualRefreshAccumulator += RealDeltaSeconds;
    const float VisualRefreshInterval = FMath::Max(0.05f, EnvironmentalVisualRefreshIntervalSeconds);
    if (EnvironmentalResidueVisualizer
        && EnvironmentalVisualRefreshAccumulator >= VisualRefreshInterval)
    {
        EnvironmentalVisualRefreshAccumulator = FMath::Fmod(
            EnvironmentalVisualRefreshAccumulator, VisualRefreshInterval);
        EnvironmentalResidueVisualizer->RefreshFromCore(*CoreBridge, CoreGridCellSizeUU, false,
            CorePresentationOriginGrid.X, CorePresentationOriginGrid.Y);
    }
}

ALLResidentCharacter* ALLWorldDirector::FindResidentActor(FGuid ResidentId) const
{
    for (ALLResidentCharacter* Character : SpawnedResidents)
    {
        if (IsValid(Character) && Character->GetResidentId() == ResidentId)
        {
            return Character;
        }
    }
    return nullptr;
}

ELLActionIntent ALLWorldDirector::GetResidentIntent(FGuid ResidentId) const
{
    const ALLResidentCharacter* Character = FindResidentActor(ResidentId);
    return Character ? Character->GetCurrentIntent() : ELLActionIntent::Idle;
}

ELLWorldAffordanceTier ALLWorldDirector::GetResidentAffordanceTier(FGuid ResidentId) const
{
    const FLLResidentRuntimeState* Runtime = RuntimeStates.Find(ResidentId);
    return Runtime ? Runtime->ActiveAffordanceTier : ELLWorldAffordanceTier::Unavailable;
}

bool ALLWorldDirector::IsResidentUsingEmergencyFallback(FGuid ResidentId) const
{
    const FLLResidentRuntimeState* Runtime = RuntimeStates.Find(ResidentId);
    return Runtime
        && Runtime->ActiveAffordanceTier == ELLWorldAffordanceTier::Emergency;
}

int32 ALLWorldDirector::GetEnvironmentalResidueVisualCount() const
{
    return EnvironmentalResidueVisualizer
        ? EnvironmentalResidueVisualizer->GetVisualInstanceCount()
        : 0;
}

void ALLWorldDirector::RefreshCorePresentationOrigin()
{
    CorePresentationOriginGrid = FIntPoint::ZeroValue;
    if (!CoreBridge)
    {
        return;
    }

    const FLLCoreWorldGenerationObservation Genesis = CoreBridge->GetWorldGenerationObservation();
    if (Genesis.bAvailable && Genesis.bHasInitialStartRegion)
    {
        CorePresentationOriginGrid = FIntPoint(
            Genesis.InitialCenterGridX,
            Genesis.InitialCenterGridY);
    }
}

void ALLWorldDirector::CollectActivityAnchors()
{
    ActivityAnchors.Reset();
    for (TActorIterator<ALLActivityAnchor> It(GetWorld()); It; ++It)
    {
        if (IsValid(*It))
        {
            ActivityAnchors.Add(*It);
        }
    }
}

void ALLWorldDirector::SpawnResidents()
{
    if (!Simulation || !CoreBridge || !GetWorld())
    {
        return;
    }

    SpawnedResidents.RemoveAll([](const TObjectPtr<ALLResidentCharacter>& Character)
    {
        return !IsValid(Character.Get());
    });

    const TArray<FLLResidentData> Residents = Simulation->GetResidents();
    TSet<FGuid> ProjectedResidentIds;
    ProjectedResidentIds.Reserve(Residents.Num());

    for (int32 Index = 0; Index < Residents.Num(); ++Index)
    {
        const FLLResidentData& Resident = Residents[Index];
        if (!Resident.ResidentId.IsValid())
        {
            continue;
        }

        ProjectedResidentIds.Add(Resident.ResidentId);

        if (ALLResidentCharacter* Existing = FindResidentActor(Resident.ResidentId))
        {
            Existing->BindResident(Resident);
            RuntimeStates.FindOrAdd(Resident.ResidentId);
            continue;
        }

        int32 GridX = 0;
        int32 GridY = 0;
        CoreBridge->GetResidentRuntimeGridPosition(Resident.ResidentId, GridX, GridY);
        const FVector SpawnLocation = CoreGridToWorldSpawnLocation(GridX, GridY, Index);

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        ALLResidentCharacter* Character = GetWorld()->SpawnActor<ALLResidentCharacter>(
            ALLResidentCharacter::StaticClass(), SpawnLocation, FRotator::ZeroRotator, Params);
        if (!Character)
        {
            continue;
        }

        Character->BindResident(Resident);
        SpawnedResidents.Add(Character);
        RuntimeStates.FindOrAdd(Resident.ResidentId);
    }

    for (int32 Index = SpawnedResidents.Num() - 1; Index >= 0; --Index)
    {
        ALLResidentCharacter* Character = SpawnedResidents[Index].Get();
        if (!IsValid(Character))
        {
            SpawnedResidents.RemoveAtSwap(Index);
            continue;
        }

        const FGuid ResidentId = Character->GetResidentId();
        if (ProjectedResidentIds.Contains(ResidentId))
        {
            continue;
        }

        if (FLLResidentRuntimeState* Runtime = RuntimeStates.Find(ResidentId))
        {
            ReleasePhysicalReservation(ResidentId, *Runtime);
        }
        RuntimeStates.Remove(ResidentId);
        Character->Destroy();
        SpawnedResidents.RemoveAtSwap(Index);
    }

    for (auto It = RuntimeStates.CreateIterator(); It; ++It)
    {
        if (!ProjectedResidentIds.Contains(It.Key()))
        {
            ReleasePhysicalReservation(It.Key(), It.Value());
            It.RemoveCurrent();
        }
    }
}

void ALLWorldDirector::UpdateResident(ALLResidentCharacter& Character, float DeltaSeconds)
{
    FLLResidentRuntimeState* Runtime = RuntimeStates.Find(Character.GetResidentId());
    if (!Runtime || !CoreBridge)
    {
        return;
    }

    FLLCoreActionDirective PendingDirective;
    if (CoreBridge->GetResidentPendingContextDirective(Character.GetResidentId(), PendingDirective))
    {
        ApplyPendingContextDirective(Character, *Runtime, PendingDirective, DeltaSeconds);
        return;
    }

    Runtime->ActiveContextActionToken = 0;
    Runtime->ContextUseElapsedSeconds = 0.0f;

    FLLCoreActionDirective Directive;
    if (!CoreBridge->GetResidentActionDirective(Character.GetResidentId(), Directive))
    {
        ReleasePhysicalReservation(Character.GetResidentId(), *Runtime);
        Character.ClearMovementTarget();
        Character.SetCurrentIntent(ELLActionIntent::Idle);
        Runtime->bPerformingAction = false;
        return;
    }

    ApplyCoreDirective(Character, *Runtime, Directive, DeltaSeconds);
}

void ALLWorldDirector::ApplyCoreDirective(
    ALLResidentCharacter& Character,
    FLLResidentRuntimeState& Runtime,
    const FLLCoreActionDirective& Directive,
    float DeltaSeconds)
{
    const bool bDirectiveChanged =
        !Runtime.bInitialized
        || Runtime.LastActivityKind != Directive.ActivityKind
        || Runtime.LastPhysicalIntent != Directive.PhysicalIntent
        || Runtime.LastSocialIntent != Directive.SocialIntent
        || Runtime.LastTargetId != Directive.TargetResidentId;

    if (bDirectiveChanged)
    {
        ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
        Runtime.bInitialized = true;
        Runtime.bPerformingAction = false;
        Runtime.PhysicalUseElapsedSeconds = 0.0f;
        Runtime.LastActivityKind = Directive.ActivityKind;
        Runtime.LastPhysicalIntent = Directive.PhysicalIntent;
        Runtime.LastSocialIntent = Directive.SocialIntent;
        Runtime.LastTargetId = Directive.TargetResidentId;
        Character.ClearMovementTarget();
    }

    if (!Directive.bAlive || Directive.ActivityKind == ELLCoreObservedActivityKind::Idle)
    {
        ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
        Runtime.bPerformingAction = false;
        Character.ClearMovementTarget();
        Character.SetCurrentIntent(ELLActionIntent::Idle);
        return;
    }

    if (Directive.ActivityKind == ELLCoreObservedActivityKind::Physical)
    {
        const ELLActionIntent Intent = ToPresentationIntent(Directive.PhysicalIntent);
        Character.SetCurrentIntent(Intent);

        if (Intent == ELLActionIntent::Idle)
        {
            ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
            Runtime.bPerformingAction = false;
            Character.ClearMovementTarget();
            return;
        }

        ALLActivityAnchor* Anchor = EnsurePhysicalReservation(Character, Runtime, Intent);

        // A Core-owned designated sanitation area is a Primitive-tier affordance.
        // Preferred/Primitive authored anchors may win; Natural anchors may not
        // mask an authoritative designated site that should be selected first.
        if (Anchor && Intent == ELLActionIntent::Toilet
            && static_cast<uint8>(Anchor->AffordanceTier)
                > static_cast<uint8>(ELLWorldAffordanceTier::Primitive))
        {
            int32 GridX = 0;
            int32 GridY = 0;
            bool bDesignated = false;
            int64 SiteId = 0;
            if (CoreBridge->GetSanitationUseTarget(
                    Character.GetResidentId(), GridX, GridY, bDesignated, SiteId)
                && bDesignated)
            {
                ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
                Anchor = nullptr;
            }
        }

        FTransform UseTransform = FTransform::Identity;
        const bool bUsesAnchor = Anchor != nullptr;

        if (bUsesAnchor)
        {
            UseTransform = Anchor->GetUseTransform();
        }
        else if (!EnsureEmergencyFallback(Character, Runtime, Intent, UseTransform))
        {
            Runtime.bPerformingAction = false;
            Runtime.PhysicalUseElapsedSeconds = 0.0f;
            Runtime.ActiveAffordanceTier = ELLWorldAffordanceTier::Unavailable;
            Character.ClearMovementTarget();
            return;
        }

        const FVector DesiredLocation = UseTransform.GetLocation();
        const double DistanceSquared = FVector::DistSquared2D(Character.GetActorLocation(), DesiredLocation);
        const bool bCoreSanitationGridTarget =
            Intent == ELLActionIntent::Toilet
            && (Runtime.bUsingEmergencyFallback || Runtime.bUsingDesignatedSanitationSite);
        const float ArrivalRadius = bCoreSanitationGridTarget
            ? FMath::Max(1.0f, FMath::Min(45.0f, CoreGridCellSizeUU * 0.45f))
            : 110.0f;
        const bool bAtUsePoint = DistanceSquared <= FMath::Square(ArrivalRadius);

        if (!bAtUsePoint)
        {
            Runtime.bPerformingAction = false;
            Runtime.PhysicalUseElapsedSeconds = 0.0f;
            Character.SetMovementTarget(DesiredLocation);
            return;
        }

        Character.ClearMovementTarget();
        if (bUsesAnchor && !Anchor->MarkInUse(Character.GetResidentId()))
        {
            ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
            Runtime.bPerformingAction = false;
            return;
        }

        Runtime.bPerformingAction = true;
        const FRotator UseRotation = UseTransform.GetRotation().Rotator();
        Character.SetActorRotation(FRotator(0.0f, UseRotation.Yaw, 0.0f));

        Runtime.PhysicalUseElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
        const int32 DurationTicks = FMath::Max(
            1,
            CoreBridge->GetPhysicalActionDurationTicks(
                Directive.PhysicalIntent,
                Runtime.bUsingEmergencyFallback,
                Runtime.bUsingDesignatedSanitationSite));
        const float RequiredUseSeconds = FMath::Max(
            0.1f,
            static_cast<float>(DurationTicks) * FMath::Max(0.1f, RealSecondsPerSimulationMinute));

        if (Runtime.PhysicalUseElapsedSeconds < RequiredUseSeconds)
        {
            return;
        }

        const bool bEmergencyFallback = Runtime.bUsingEmergencyFallback;
        const int64 SanitationSiteId = Runtime.bUsingDesignatedSanitationSite
            ? Runtime.CoreSanitationSiteId
            : 0;
        const FIntPoint ResolvedGrid = WorldLocationToCoreGrid(Character.GetActorLocation());
        const bool bAcknowledged = CoreBridge->CompleteResidentPhysicalAction(
            Character.GetResidentId(),
            bEmergencyFallback,
            ResolvedGrid.X,
            ResolvedGrid.Y,
            SanitationSiteId);

        if (bAcknowledged)
        {
            ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
            Runtime.bPerformingAction = false;
            Character.ClearMovementTarget();
            Character.SetCurrentIntent(ELLActionIntent::Idle);
        }
        else if (Runtime.bUsingDesignatedSanitationSite)
        {
            // The Core site may have been invalidated while the resident moved.
            // Release the stale target; the next tick re-resolves through Core
            // and naturally falls back to another valid tier if necessary.
            ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
            Runtime.bPerformingAction = false;
            Character.ClearMovementTarget();
        }
        else
        {
            Runtime.PhysicalUseElapsedSeconds = 0.0f;
        }
        return;
    }

    ReleasePhysicalReservation(Character.GetResidentId(), Runtime);

    if (Directive.ActivityKind == ELLCoreObservedActivityKind::Social)
    {
        Character.SetCurrentIntent(ELLActionIntent::Socialize);

        ALLResidentCharacter* Target = FindResidentActor(Directive.TargetResidentId);
        if (!Target || Directive.SocialIntent == ELLCoreSocialIntent::None)
        {
            Runtime.bPerformingAction = false;
            Character.ClearMovementTarget();
            return;
        }

        const FVector DesiredLocation = ResolveSocialTargetLocation(Character, *Target, Directive.SocialIntent);
        const double DistanceToDesired = FVector::DistSquared2D(Character.GetActorLocation(), DesiredLocation);
        const bool bAtDesiredLocation = DistanceToDesired <= FMath::Square(110.0);

        if (!bAtDesiredLocation)
        {
            Runtime.bPerformingAction = false;
            Character.SetMovementTarget(DesiredLocation);
        }
        else
        {
            Runtime.bPerformingAction = true;
            Character.ClearMovementTarget();

            if (Directive.SocialIntent != ELLCoreSocialIntent::Avoid)
            {
                FVector LookDirection = Target->GetActorLocation() - Character.GetActorLocation();
                LookDirection.Z = 0.0f;
                if (!LookDirection.IsNearlyZero())
                {
                    Character.SetActorRotation(FRotator(0.0f, LookDirection.Rotation().Yaw, 0.0f));
                }
            }
        }
    }
}

ELLActionIntent ALLWorldDirector::ToPresentationIntent(ELLCorePhysicalIntent Intent) const
{
    switch (Intent)
    {
        case ELLCorePhysicalIntent::Eat: return ELLActionIntent::Eat;
        case ELLCorePhysicalIntent::Drink: return ELLActionIntent::Drink;
        case ELLCorePhysicalIntent::Sleep: return ELLActionIntent::Sleep;
        case ELLCorePhysicalIntent::Toilet: return ELLActionIntent::Toilet;
        case ELLCorePhysicalIntent::Hygiene: return ELLActionIntent::Hygiene;
        case ELLCorePhysicalIntent::None:
        default:
            return ELLActionIntent::Idle;
    }
}

ALLActivityAnchor* ALLWorldDirector::FindBestUsableAnchor(
    const ALLResidentCharacter& Character,
    ELLActionIntent Intent,
    ELLWorldAffordanceTier& OutTier) const
{
    ALLActivityAnchor* BestAnchor = nullptr;
    ELLWorldAffordanceTier BestTier = ELLWorldAffordanceTier::Unavailable;
    double BestDistanceSquared = TNumericLimits<double>::Max();
    FString BestPath;

    for (ALLActivityAnchor* Anchor : ActivityAnchors)
    {
        if (!IsValid(Anchor)
            || !Anchor->SupportsIntent(Intent)
            || !Anchor->CanBeUsedBy(Character.GetResidentId())
            || Anchor->AffordanceTier == ELLWorldAffordanceTier::Unavailable)
        {
            continue;
        }

        const ELLWorldAffordanceTier CandidateTier = Anchor->AffordanceTier;
        const double DistanceSquared = FVector::DistSquared2D(
            Character.GetActorLocation(), Anchor->GetUseLocation());
        const FString AnchorPath = Anchor->GetPathName();

        const uint8 CandidateTierValue = static_cast<uint8>(CandidateTier);
        const uint8 BestTierValue = static_cast<uint8>(BestTier);
        const bool bBetterTier = CandidateTierValue < BestTierValue;
        const bool bSameTier = CandidateTier == BestTier;
        const bool bCloser = bSameTier && DistanceSquared + KINDA_SMALL_NUMBER < BestDistanceSquared;
        const bool bStableTieBreak =
            bSameTier
            && FMath::IsNearlyEqual(DistanceSquared, BestDistanceSquared)
            && (BestAnchor == nullptr || AnchorPath.Compare(BestPath, ESearchCase::CaseSensitive) < 0);

        if (bBetterTier || bCloser || bStableTieBreak)
        {
            BestAnchor = Anchor;
            BestTier = CandidateTier;
            BestDistanceSquared = DistanceSquared;
            BestPath = AnchorPath;
        }
    }

    OutTier = BestTier;
    return BestAnchor;
}

ALLActivityAnchor* ALLWorldDirector::EnsurePhysicalReservation(
    ALLResidentCharacter& Character,
    FLLResidentRuntimeState& Runtime,
    ELLActionIntent Intent)
{
    if (Runtime.ReservedAnchor.IsValid())
    {
        ALLActivityAnchor* Existing = Runtime.ReservedAnchor.Get();
        if (Runtime.ReservedIntent == Intent
            && Existing->SupportsIntent(Intent)
            && Existing->IsClaimedBy(Character.GetResidentId())
            && Existing->CanBeUsedBy(Character.GetResidentId()))
        {
            Runtime.ActiveAffordanceTier = Existing->AffordanceTier;
            Runtime.bUsingEmergencyFallback = false;
            Runtime.bUsingDesignatedSanitationSite = false;
            Runtime.CoreSanitationSiteId = 0;
            return Existing;
        }

        ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
    }

    ELLWorldAffordanceTier CandidateTier = ELLWorldAffordanceTier::Unavailable;
    ALLActivityAnchor* Candidate = FindBestUsableAnchor(Character, Intent, CandidateTier);
    if (!Candidate || !Candidate->TryReserve(Character.GetResidentId()))
    {
        return nullptr;
    }

    Runtime.ReservedAnchor = Candidate;
    Runtime.ReservedIntent = Intent;
    Runtime.ActiveAffordanceTier = CandidateTier;
    Runtime.bUsingEmergencyFallback = false;
    Runtime.bUsingDesignatedSanitationSite = false;
    Runtime.CoreSanitationSiteId = 0;
    Runtime.EmergencyUseTransform = FTransform::Identity;
    Runtime.PhysicalUseElapsedSeconds = 0.0f;
    return Candidate;
}

bool ALLWorldDirector::EnsureEmergencyFallback(
    const ALLResidentCharacter& Character,
    FLLResidentRuntimeState& Runtime,
    ELLActionIntent Intent,
    FTransform& OutUseTransform) const
{
    if (!SupportsEmergencyFallback(Intent))
    {
        Runtime.ActiveAffordanceTier = ELLWorldAffordanceTier::Unavailable;
        Runtime.bUsingEmergencyFallback = false;
        Runtime.bUsingDesignatedSanitationSite = false;
        Runtime.CoreSanitationSiteId = 0;
        return false;
    }

    const bool bHasCachedTarget = Runtime.ReservedIntent == Intent
        && (Intent == ELLActionIntent::Toilet
            ? (Runtime.bUsingEmergencyFallback || Runtime.bUsingDesignatedSanitationSite)
            : Runtime.bUsingEmergencyFallback);

    if (!bHasCachedTarget)
    {
        Runtime.ReservedIntent = Intent;
        Runtime.bUsingEmergencyFallback = false;
        Runtime.bUsingDesignatedSanitationSite = false;
        Runtime.CoreSanitationSiteId = 0;

        if (Intent == ELLActionIntent::Toilet)
        {
            int32 RecommendedGridX = 0;
            int32 RecommendedGridY = 0;
            bool bDesignatedSite = false;
            int64 SanitationSiteId = 0;
            if (!CoreBridge || !CoreBridge->GetSanitationUseTarget(
                    Character.GetResidentId(),
                    RecommendedGridX,
                    RecommendedGridY,
                    bDesignatedSite,
                    SanitationSiteId))
            {
                Runtime.ActiveAffordanceTier = ELLWorldAffordanceTier::Unavailable;
                Runtime.EmergencyUseTransform = FTransform::Identity;
                Runtime.PhysicalUseElapsedSeconds = 0.0f;
                return false;
            }

            Runtime.bUsingDesignatedSanitationSite = bDesignatedSite;
            Runtime.bUsingEmergencyFallback = !bDesignatedSite;
            Runtime.CoreSanitationSiteId = bDesignatedSite ? SanitationSiteId : 0;
            Runtime.ActiveAffordanceTier = bDesignatedSite
                ? ELLWorldAffordanceTier::Primitive
                : ELLWorldAffordanceTier::Emergency;

            const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
            FVector RecommendedLocation = GetActorLocation()
                + FVector(
                    static_cast<float>(RecommendedGridX - CorePresentationOriginGrid.X) * CellSize,
                    static_cast<float>(RecommendedGridY - CorePresentationOriginGrid.Y) * CellSize,
                    0.0f);
            RecommendedLocation.Z = Character.GetActorLocation().Z;

            FVector FacingDirection = RecommendedLocation - Character.GetActorLocation();
            FacingDirection.Z = 0.0f;
            const FRotator RecommendedRotation = FacingDirection.IsNearlyZero()
                ? Character.GetActorRotation()
                : FacingDirection.Rotation();

            Runtime.EmergencyUseTransform = FTransform(
                RecommendedRotation,
                RecommendedLocation,
                FVector::OneVector);
        }
        else if (Intent == ELLActionIntent::Sleep)
        {
            int32 SleepGridX = 0;
            int32 SleepGridY = 0;
            int64 SleepFacilityId = 0;
            if (CoreBridge && CoreBridge->GetSettlementSleepUseTarget(
                    Character.GetResidentId(),
                    SleepGridX,
                    SleepGridY,
                    SleepFacilityId))
            {
                Runtime.ActiveAffordanceTier = ELLWorldAffordanceTier::Primitive;
                Runtime.bUsingEmergencyFallback = true;

                const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
                FVector SleepLocation = GetActorLocation()
                    + FVector(
                        static_cast<float>(SleepGridX - CorePresentationOriginGrid.X) * CellSize,
                        static_cast<float>(SleepGridY - CorePresentationOriginGrid.Y) * CellSize,
                        0.0f);
                SleepLocation.Z = Character.GetActorLocation().Z;

                FVector FacingDirection = SleepLocation - Character.GetActorLocation();
                FacingDirection.Z = 0.0f;
                const FRotator SleepRotation = FacingDirection.IsNearlyZero()
                    ? Character.GetActorRotation()
                    : FacingDirection.Rotation();

                Runtime.EmergencyUseTransform = FTransform(
                    SleepRotation,
                    SleepLocation,
                    FVector::OneVector);
            }
            else
            {
                Runtime.ActiveAffordanceTier = ELLWorldAffordanceTier::Emergency;
                Runtime.bUsingEmergencyFallback = true;
                Runtime.EmergencyUseTransform = ResolveEmergencyFallbackTransform(Character, Intent);
            }
        }
        else
        {
            Runtime.ActiveAffordanceTier = ELLWorldAffordanceTier::Emergency;
            Runtime.bUsingEmergencyFallback = true;
            Runtime.EmergencyUseTransform = ResolveEmergencyFallbackTransform(Character, Intent);
        }

        Runtime.PhysicalUseElapsedSeconds = 0.0f;
    }

    OutUseTransform = Runtime.EmergencyUseTransform;
    return true;
}

bool ALLWorldDirector::SupportsEmergencyFallback(ELLActionIntent Intent) const
{
    switch (Intent)
    {
        case ELLActionIntent::Eat:
        case ELLActionIntent::Drink:
        case ELLActionIntent::Sleep:
        case ELLActionIntent::Toilet:
        case ELLActionIntent::Hygiene:
            return true;
        case ELLActionIntent::Idle:
        case ELLActionIntent::Socialize:
        case ELLActionIntent::HaveFun:
        default:
            return false;
    }
}

FTransform ALLWorldDirector::ResolveEmergencyFallbackTransform(
    const ALLResidentCharacter& Character,
    ELLActionIntent Intent) const
{
    const FVector Location = Character.GetActorLocation();
    const FRotator Rotation = Character.GetActorRotation();
    return FTransform(Rotation, Location, FVector::OneVector);
}

FIntPoint ALLWorldDirector::WorldLocationToCoreGrid(const FVector& WorldLocation) const
{
    const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
    const FVector Relative = WorldLocation - GetActorLocation();
    return FIntPoint(
        CorePresentationOriginGrid.X + FMath::RoundToInt(Relative.X / CellSize),
        CorePresentationOriginGrid.Y + FMath::RoundToInt(Relative.Y / CellSize));
}

FVector ALLWorldDirector::CoreGridToWorldSpawnLocation(
    int32 GridX,
    int32 GridY,
    int32 PresentationSlot) const
{
    const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
    const float MaxSubCellRadius = CellSize * 0.42f;

    FVector SubCellOffset = FVector::ZeroVector;
    const int32 SafeSlot = FMath::Max(0, PresentationSlot);
    if (SafeSlot < 4)
    {
        const float XSign = (SafeSlot % 2) == 0 ? -1.0f : 1.0f;
        const float YSign = SafeSlot < 2 ? -1.0f : 1.0f;
        SubCellOffset = FVector(
            XSign * MaxSubCellRadius,
            YSign * MaxSubCellRadius,
            0.0f);
    }
    else
    {
        const int32 RingSlot = (SafeSlot - 4) % 8;
        const int32 Ring = ((SafeSlot - 4) / 8) + 1;
        const float AngleDegrees = static_cast<float>(RingSlot) * 45.0f
            + ((Ring % 2) == 1 ? 22.5f : 0.0f);
        const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
        const float RadiusScale = FMath::Min(1.0f, 0.55f + static_cast<float>(Ring - 1) * 0.12f);
        const float Radius = MaxSubCellRadius * RadiusScale;
        SubCellOffset = FVector(
            FMath::Cos(AngleRadians) * Radius,
            FMath::Sin(AngleRadians) * Radius,
            0.0f);
    }

    return GetActorLocation()
        + FVector(
            static_cast<float>(GridX - CorePresentationOriginGrid.X) * CellSize,
            static_cast<float>(GridY - CorePresentationOriginGrid.Y) * CellSize,
            90.0f)
        + SubCellOffset;
}

void ALLWorldDirector::ReleasePhysicalReservation(
    FGuid ResidentId,
    FLLResidentRuntimeState& Runtime)
{
    if (Runtime.ReservedAnchor.IsValid())
    {
        Runtime.ReservedAnchor->Release(ResidentId);
    }

    Runtime.ReservedAnchor.Reset();
    Runtime.ReservedIntent = ELLActionIntent::Idle;
    Runtime.ActiveAffordanceTier = ELLWorldAffordanceTier::Unavailable;
    Runtime.bUsingEmergencyFallback = false;
    Runtime.bUsingDesignatedSanitationSite = false;
    Runtime.CoreSanitationSiteId = 0;
    Runtime.EmergencyUseTransform = FTransform::Identity;
    Runtime.PhysicalUseElapsedSeconds = 0.0f;
}

FVector ALLWorldDirector::ResolveSocialTargetLocation(
    const ALLResidentCharacter& Character,
    const ALLResidentCharacter& Target,
    ELLCoreSocialIntent SocialIntent) const
{
    FVector AwayDirection = Character.GetActorLocation() - Target.GetActorLocation();
    AwayDirection.Z = 0.0f;
    if (AwayDirection.IsNearlyZero())
    {
        const bool bPositive = (GetTypeHash(Character.GetResidentId()) & 1u) == 0u;
        AwayDirection = bPositive ? FVector(1.0f, 0.0f, 0.0f) : FVector(0.0f, 1.0f, 0.0f);
    }
    AwayDirection.Normalize();

    if (SocialIntent == ELLCoreSocialIntent::Avoid)
    {
        return Character.GetActorLocation() + AwayDirection * 420.0f;
    }

    return Target.GetActorLocation() + AwayDirection * 120.0f;
}
