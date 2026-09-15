#include "World/LLWorldDirector.h"
#include "World/LLActivityAnchor.h"
#include "Characters/LLResidentCharacter.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLSimulationSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ALLWorldDirector::ALLWorldDirector()
{
    PrimaryActorTick.bCanEverTick = true;
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

    // Unreal owns physical movement/arrival/use timing. Core keeps the intent
    // pending and applies authoritative outcomes only after the world ACK.
    CoreBridge->SetExternalPhysicalExecutionEnabled(true);

    // World affordances must come from the actual world/civilization state.
    // Never synthesize beds, toilets, food stations, etc. just to satisfy an intent.
    CollectActivityAnchors();
    SpawnResidents();
}

void ALLWorldDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!Simulation || !CoreBridge)
    {
        return;
    }

    bool bAdvancedSimulation = false;
    SimulationClockAccumulator += DeltaSeconds;
    const float StepSeconds = FMath::Max(0.1f, RealSecondsPerSimulationMinute);
    while (SimulationClockAccumulator >= StepSeconds)
    {
        SimulationClockAccumulator -= StepSeconds;
        Simulation->AdvanceSimulationMinutes(1);
        bAdvancedSimulation = true;

        if ((Simulation->GetSimulationMinute() % 60) == 0)
        {
            Simulation->SaveGame();
        }
    }

    if (bAdvancedSimulation)
    {
        // Core may create residents (births) and civilization/world presentation may
        // add/remove physical affordances at runtime. Reconcile both after each
        // simulation advance instead of treating BeginPlay as a permanent snapshot.
        CollectActivityAnchors();
        SpawnResidents();
    }

    for (ALLResidentCharacter* Character : SpawnedResidents)
    {
        if (IsValid(Character))
        {
            UpdateResident(*Character, DeltaSeconds);
        }
    }

    for (auto& Pair : RuntimeStates)
    {
        if (!FindResidentActor(Pair.Key))
        {
            ReleasePhysicalReservation(Pair.Key, Pair.Value);
        }
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
    return Runtime && Runtime->bUsingEmergencyFallback;
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
    if (!Simulation || !GetWorld())
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

    const FVector Base = GetActorLocation() + FVector(0.0f, 0.0f, 90.0f);
    const TArray<FVector> FounderSpawnOffsets = {
        FVector(-240.0f, -160.0f, 0.0f),
        FVector( 240.0f, -160.0f, 0.0f),
        FVector(-240.0f,  160.0f, 0.0f),
        FVector( 240.0f,  160.0f, 0.0f)
    };

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

        FVector SpawnOffset = FVector::ZeroVector;
        if (Index < FounderSpawnOffsets.Num())
        {
            SpawnOffset = FounderSpawnOffsets[Index];
        }
        else
        {
            // New generations must never wrap back onto the original four spawn
            // points. Allocate stable expanding rings so load/reconciliation can
            // represent populations larger than the founder set without overlap.
            const int32 ExtraIndex = Index - FounderSpawnOffsets.Num();
            const int32 Ring = (ExtraIndex / 8) + 1;
            const int32 Slot = ExtraIndex % 8;
            const float AngleDegrees = static_cast<float>(Slot) * 45.0f
                + ((Ring % 2) == 1 ? 22.5f : 0.0f);
            const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
            const float Radius = 520.0f + static_cast<float>(Ring - 1) * 180.0f;
            SpawnOffset = FVector(
                FMath::Cos(AngleRadians) * Radius,
                FMath::Sin(AngleRadians) * Radius,
                0.0f);
        }

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        ALLResidentCharacter* Character = GetWorld()->SpawnActor<ALLResidentCharacter>(
            ALLResidentCharacter::StaticClass(), Base + SpawnOffset, FRotator::ZeroRotator, Params);
        if (!Character)
        {
            continue;
        }

        Character->BindResident(Resident);
        SpawnedResidents.Add(Character);
        RuntimeStates.FindOrAdd(Resident.ResidentId);
    }

    // If a projection ever removes a resident entirely, clean up the presentation
    // actor and any outstanding reservation. Existing living/deceased Core residents
    // remain projected, so this is identity reconciliation rather than a death policy.
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
        const bool bAtUsePoint = DistanceSquared <= FMath::Square(110.0);

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
                Runtime.bUsingEmergencyFallback));
        const float RequiredUseSeconds = FMath::Max(
            0.1f,
            static_cast<float>(DurationTicks) * FMath::Max(0.1f, RealSecondsPerSimulationMinute));

        if (Runtime.PhysicalUseElapsedSeconds < RequiredUseSeconds)
        {
            return;
        }

        const bool bEmergencyFallback = Runtime.bUsingEmergencyFallback;
        const FIntPoint ResolvedGrid = WorldLocationToCoreGrid(Character.GetActorLocation());
        const bool bAcknowledged = CoreBridge->CompleteResidentPhysicalAction(
            Character.GetResidentId(),
            bEmergencyFallback,
            ResolvedGrid.X,
            ResolvedGrid.Y);

        if (bAcknowledged)
        {
            ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
            Runtime.bPerformingAction = false;
            Character.ClearMovementTarget();
            Character.SetCurrentIntent(ELLActionIntent::Idle);
        }
        else
        {
            // Avoid hammering Core every frame when a completion precondition
            // (for example a real provision) is no longer satisfied.
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
        return false;
    }

    if (!Runtime.bUsingEmergencyFallback || Runtime.ReservedIntent != Intent)
    {
        Runtime.ReservedIntent = Intent;
        Runtime.ActiveAffordanceTier = ELLWorldAffordanceTier::Emergency;
        Runtime.bUsingEmergencyFallback = true;
        Runtime.EmergencyUseTransform = ResolveEmergencyFallbackTransform(Character, Intent);
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
    FVector Location = Character.GetActorLocation();
    FRotator Rotation = Character.GetActorRotation();

    if (Intent == ELLActionIntent::Toilet)
    {
        // Until terrain/resource semantics are available, pick a deterministic
        // outdoor fallback point away from the settlement origin. This is not a
        // toilet object and therefore does not fake civilization progress.
        const uint32 StableHash = HashCombine(
            GetTypeHash(Character.GetResidentId()),
            static_cast<uint32>(Intent));
        const float AngleDegrees = static_cast<float>(StableHash % 360u);
        const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
        const FVector Direction(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians), 0.0f);
        Location = GetActorLocation() + Direction * 650.0f;
        Location.Z = Character.GetActorLocation().Z;
        Rotation = Direction.Rotation();
    }

    // Sleep -> ground rest in place.
    // Eat/Drink -> consume what Core has already made available, in place.
    // Hygiene -> minimal no-facility fallback in place.
    return FTransform(Rotation, Location, FVector::OneVector);
}

FIntPoint ALLWorldDirector::WorldLocationToCoreGrid(const FVector& WorldLocation) const
{
    const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
    const FVector Relative = WorldLocation - GetActorLocation();
    return FIntPoint(
        FMath::RoundToInt(Relative.X / CellSize),
        FMath::RoundToInt(Relative.Y / CellSize));
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
