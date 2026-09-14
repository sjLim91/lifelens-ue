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

    CollectActivityAnchors();
    EnsureBootstrapActivityAnchors();
    SpawnResidents();
}

void ALLWorldDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!Simulation || !CoreBridge)
    {
        return;
    }

    SimulationClockAccumulator += DeltaSeconds;
    const float StepSeconds = FMath::Max(0.1f, RealSecondsPerSimulationMinute);
    while (SimulationClockAccumulator >= StepSeconds)
    {
        SimulationClockAccumulator -= StepSeconds;
        Simulation->AdvanceSimulationMinutes(1);

        if ((Simulation->GetSimulationMinute() % 60) == 0)
        {
            Simulation->SaveGame();
        }
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

void ALLWorldDirector::CollectActivityAnchors()
{
    ActivityAnchors.Reset();
    for (TActorIterator<ALLActivityAnchor> It(GetWorld()); It; ++It)
    {
        ActivityAnchors.Add(*It);
    }
}

void ALLWorldDirector::EnsureBootstrapActivityAnchors()
{
    if (!GetWorld())
    {
        return;
    }

    struct FBootstrapAnchorSpec
    {
        ELLActionIntent Intent;
        FVector Offset;
        FRotator Rotation;
    };

    const FBootstrapAnchorSpec Specs[] = {
        { ELLActionIntent::Eat,    FVector(-520.0f, -300.0f, 90.0f), FRotator(0.0f,   0.0f, 0.0f) },
        { ELLActionIntent::Drink,  FVector(-520.0f,  300.0f, 90.0f), FRotator(0.0f,   0.0f, 0.0f) },
        { ELLActionIntent::Sleep,  FVector( 520.0f, -300.0f, 90.0f), FRotator(0.0f, 180.0f, 0.0f) },
        { ELLActionIntent::Toilet, FVector( 520.0f,  300.0f, 90.0f), FRotator(0.0f, 180.0f, 0.0f) },
        { ELLActionIntent::Hygiene,FVector(-220.0f,  520.0f, 90.0f), FRotator(0.0f, -90.0f, 0.0f) }
    };

    for (const FBootstrapAnchorSpec& Spec : Specs)
    {
        bool bAlreadyProvided = false;
        for (ALLActivityAnchor* Anchor : ActivityAnchors)
        {
            if (IsValid(Anchor) && Anchor->SupportedIntent == Spec.Intent)
            {
                bAlreadyProvided = true;
                break;
            }
        }

        if (bAlreadyProvided)
        {
            continue;
        }

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Params.Name = MakeUniqueObjectName(GetWorld(), ALLActivityAnchor::StaticClass(), TEXT("LLBootstrapActivityAnchor"));

        ALLActivityAnchor* Anchor = GetWorld()->SpawnActor<ALLActivityAnchor>(
            ALLActivityAnchor::StaticClass(),
            GetActorLocation() + Spec.Offset,
            Spec.Rotation,
            Params);

        if (!Anchor)
        {
            continue;
        }

        Anchor->SupportedIntent = Spec.Intent;
        ActivityAnchors.Add(Anchor);
    }
}

void ALLWorldDirector::SpawnResidents()
{
    for (auto& Pair : RuntimeStates)
    {
        ReleasePhysicalReservation(Pair.Key, Pair.Value);
    }

    SpawnedResidents.Reset();
    RuntimeStates.Reset();

    if (!Simulation || !GetWorld())
    {
        return;
    }

    const TArray<FLLResidentData> Residents = Simulation->GetResidents();
    const FVector Base = GetActorLocation() + FVector(0.0f, 0.0f, 90.0f);
    const TArray<FVector> SpawnOffsets = {
        FVector(-240.0f, -160.0f, 0.0f),
        FVector( 240.0f, -160.0f, 0.0f),
        FVector(-240.0f,  160.0f, 0.0f),
        FVector( 240.0f,  160.0f, 0.0f)
    };

    for (int32 Index = 0; Index < Residents.Num(); ++Index)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        const FVector SpawnLocation = Base + SpawnOffsets[Index % SpawnOffsets.Num()];
        ALLResidentCharacter* Character = GetWorld()->SpawnActor<ALLResidentCharacter>(
            ALLResidentCharacter::StaticClass(), SpawnLocation, FRotator::ZeroRotator, Params);

        if (!Character)
        {
            continue;
        }

        Character->BindResident(Residents[Index]);
        SpawnedResidents.Add(Character);
        RuntimeStates.Add(Residents[Index].ResidentId, FLLResidentRuntimeState());
    }
}

void ALLWorldDirector::UpdateResident(ALLResidentCharacter& Character, float DeltaSeconds)
{
    (void)DeltaSeconds;

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

    ApplyCoreDirective(Character, *Runtime, Directive);
}

void ALLWorldDirector::ApplyCoreDirective(
    ALLResidentCharacter& Character,
    FLLResidentRuntimeState& Runtime,
    const FLLCoreActionDirective& Directive)
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
        if (!Anchor)
        {
            Runtime.bPerformingAction = false;
            Character.ClearMovementTarget();
            return;
        }

        const FTransform UseTransform = Anchor->GetUseTransform();
        const FVector DesiredLocation = UseTransform.GetLocation();
        const double DistanceSquared = FVector::DistSquared2D(Character.GetActorLocation(), DesiredLocation);
        const bool bAtUsePoint = DistanceSquared <= FMath::Square(110.0);

        if (!bAtUsePoint)
        {
            Runtime.bPerformingAction = false;
            Character.SetMovementTarget(DesiredLocation);
            return;
        }

        Character.ClearMovementTarget();
        if (!Anchor->MarkInUse(Character.GetResidentId()))
        {
            ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
            Runtime.bPerformingAction = false;
            return;
        }

        Runtime.bPerformingAction = true;
        const FRotator UseRotation = UseTransform.GetRotation().Rotator();
        Character.SetActorRotation(FRotator(0.0f, UseRotation.Yaw, 0.0f));
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
    ELLActionIntent Intent) const
{
    ALLActivityAnchor* BestAnchor = nullptr;
    double BestDistanceSquared = TNumericLimits<double>::Max();
    FString BestPath;

    for (ALLActivityAnchor* Anchor : ActivityAnchors)
    {
        if (!IsValid(Anchor)
            || Anchor->SupportedIntent != Intent
            || !Anchor->CanBeUsedBy(Character.GetResidentId()))
        {
            continue;
        }

        const double DistanceSquared = FVector::DistSquared2D(
            Character.GetActorLocation(), Anchor->GetUseLocation());
        const FString AnchorPath = Anchor->GetPathName();
        const bool bCloser = DistanceSquared + KINDA_SMALL_NUMBER < BestDistanceSquared;
        const bool bStableTieBreak =
            FMath::IsNearlyEqual(DistanceSquared, BestDistanceSquared)
            && (BestAnchor == nullptr || AnchorPath.Compare(BestPath, ESearchCase::CaseSensitive) < 0);

        if (bCloser || bStableTieBreak)
        {
            BestAnchor = Anchor;
            BestDistanceSquared = DistanceSquared;
            BestPath = AnchorPath;
        }
    }

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
            && Existing->SupportedIntent == Intent
            && Existing->IsClaimedBy(Character.GetResidentId())
            && Existing->CanBeUsedBy(Character.GetResidentId()))
        {
            return Existing;
        }

        ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
    }

    ALLActivityAnchor* Candidate = FindBestUsableAnchor(Character, Intent);
    if (!Candidate || !Candidate->TryReserve(Character.GetResidentId()))
    {
        return nullptr;
    }

    Runtime.ReservedAnchor = Candidate;
    Runtime.ReservedIntent = Intent;
    return Candidate;
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
