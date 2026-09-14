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

void ALLWorldDirector::SpawnResidents()
{
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
            Runtime.bPerformingAction = false;
            Character.ClearMovementTarget();
            return;
        }

        if (!Runtime.bPerformingAction && (bDirectiveChanged || Character.HasReachedMovementTarget()))
        {
            if (bDirectiveChanged)
            {
                Character.SetMovementTarget(ResolveTargetLocation(Intent, Character.GetResidentId()));
            }

            if (Character.HasReachedMovementTarget())
            {
                Character.ClearMovementTarget();
                Runtime.bPerformingAction = true;
            }
        }
        return;
    }

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
        const float DistanceToDesired = FVector::DistSquared2D(Character.GetActorLocation(), DesiredLocation);
        const bool bAtDesiredLocation = DistanceToDesired <= FMath::Square(110.0f);

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

FVector ALLWorldDirector::ResolveTargetLocation(ELLActionIntent Intent, FGuid ResidentId) const
{
    for (const ALLActivityAnchor* Anchor : ActivityAnchors)
    {
        if (IsValid(Anchor) && Anchor->SupportedIntent == Intent)
        {
            return Anchor->GetUseLocation();
        }
    }

    FVector Base;
    switch (Intent)
    {
        case ELLActionIntent::Eat: Base = FVector(-520.0f, -300.0f, 90.0f); break;
        case ELLActionIntent::Drink: Base = FVector(-520.0f, 300.0f, 90.0f); break;
        case ELLActionIntent::Sleep: Base = FVector(520.0f, -300.0f, 90.0f); break;
        case ELLActionIntent::Hygiene: Base = FVector(-520.0f, 300.0f, 90.0f); break;
        case ELLActionIntent::Toilet: Base = FVector(520.0f, 300.0f, 90.0f); break;
        case ELLActionIntent::HaveFun: Base = FVector(0.0f, 520.0f, 90.0f); break;
        case ELLActionIntent::Socialize: Base = FVector::ZeroVector; break;
        case ELLActionIntent::Idle:
        default: Base = FVector(0.0f, -100.0f, 90.0f); break;
    }

    const float LaneOffset = static_cast<float>(GetTypeHash(ResidentId) % 5) * 42.0f - 84.0f;
    return GetActorLocation() + Base + FVector(0.0f, LaneOffset, 0.0f);
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
