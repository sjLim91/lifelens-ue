#include "World/LLWorldDirector.h"
#include "World/LLActivityAnchor.h"
#include "Characters/LLResidentCharacter.h"
#include "AI/LLDecisionComponent.h"
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
    if (!Simulation)
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

    if (!Simulation)
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
    FLLResidentRuntimeState* Runtime = RuntimeStates.Find(Character.GetResidentId());
    if (!Runtime)
    {
        return;
    }

    Runtime->DecisionCooldown = FMath::Max(0.0f, Runtime->DecisionCooldown - DeltaSeconds);

    if (Runtime->bPerformingAction)
    {
        Runtime->ActionSecondsRemaining -= DeltaSeconds;
        if (Runtime->ActionSecondsRemaining <= 0.0f)
        {
            CompleteAction(Character, *Runtime);
        }
        return;
    }

    if (Character.GetCurrentIntent() == ELLActionIntent::Socialize && Runtime->SocialTargetId.IsValid())
    {
        if (ALLResidentCharacter* Target = FindResidentActor(Runtime->SocialTargetId))
        {
            const FVector Approach = Target->GetActorLocation() + FVector(85.0f, 0.0f, 0.0f);
            Character.SetMovementTarget(Approach);
        }
        else
        {
            Runtime->SocialTargetId.Invalidate();
            Character.ClearMovementTarget();
            Character.SetCurrentIntent(ELLActionIntent::Idle);
        }
    }

    if (Character.GetCurrentIntent() != ELLActionIntent::Idle && Character.HasReachedMovementTarget())
    {
        Runtime->bPerformingAction = true;
        Runtime->ActionSecondsRemaining = GetActionDuration(Character.GetCurrentIntent());
        return;
    }

    if (Character.GetCurrentIntent() == ELLActionIntent::Idle && Runtime->DecisionCooldown <= 0.0f)
    {
        StartNextAction(Character, *Runtime);
    }
}

void ALLWorldDirector::StartNextAction(ALLResidentCharacter& Character, FLLResidentRuntimeState& Runtime)
{
    if (!Simulation || !Character.DecisionComponent)
    {
        return;
    }

    FLLResidentData Resident;
    if (!Simulation->FindResidentById(Character.GetResidentId(), Resident))
    {
        return;
    }

    const FLLDecisionResult Decision = Character.DecisionComponent->ChooseAction(Resident);
    ELLActionIntent Intent = Decision.Intent;

    Runtime.SocialTargetId.Invalidate();
    Character.SetCurrentIntent(Intent);

    if (Intent == ELLActionIntent::Socialize)
    {
        ALLResidentCharacter* SocialTarget = ChooseSocialTarget(Character);
        if (SocialTarget)
        {
            Runtime.SocialTargetId = SocialTarget->GetResidentId();
            Character.SetMovementTarget(SocialTarget->GetActorLocation() + FVector(85.0f, 0.0f, 0.0f));
            return;
        }

        Intent = ELLActionIntent::HaveFun;
        Character.SetCurrentIntent(Intent);
    }

    if (Intent == ELLActionIntent::Idle)
    {
        Character.ClearMovementTarget();
        Runtime.bPerformingAction = true;
        Runtime.ActionSecondsRemaining = GetActionDuration(Intent);
        return;
    }

    Character.SetMovementTarget(ResolveTargetLocation(Intent, Character.GetResidentId()));
}

void ALLWorldDirector::CompleteAction(ALLResidentCharacter& Character, FLLResidentRuntimeState& Runtime)
{
    if (!Simulation)
    {
        return;
    }

    const ELLActionIntent CompletedIntent = Character.GetCurrentIntent();

    if (CompletedIntent == ELLActionIntent::Socialize && Runtime.SocialTargetId.IsValid())
    {
        ALLResidentCharacter* TargetCharacter = FindResidentActor(Runtime.SocialTargetId);
        if (TargetCharacter && FVector::DistSquared2D(Character.GetActorLocation(), TargetCharacter->GetActorLocation()) <= FMath::Square(300.0f))
        {
            FLLResidentData A;
            FLLResidentData B;
            if (Simulation->FindResidentById(Character.GetResidentId(), A)
                && Simulation->FindResidentById(TargetCharacter->GetResidentId(), B))
            {
                const float Agreeability = (A.Personality.Agreeableness + B.Personality.Agreeableness) / 200.0f;
                const float Stability = (A.Personality.EmotionalStability + B.Personality.EmotionalStability) / 200.0f;
                const float OpennessCompatibility = 1.0f - FMath::Abs(A.Personality.Openness - B.Personality.Openness) / 100.0f;

                Simulation->ApplySocialInteraction(
                    A.ResidentId,
                    B.ResidentId,
                    2.5f + Agreeability * 4.0f,
                    1.5f + Stability * 3.0f,
                    0.2f + FMath::Clamp(OpennessCompatibility, 0.0f, 1.0f) * 1.2f);

                Simulation->ApplyActionOutcome(B.ResidentId, ELLActionIntent::Socialize, 0.55f);
            }
        }
    }

    Simulation->ApplyActionOutcome(Character.GetResidentId(), CompletedIntent);

    Runtime.bPerformingAction = false;
    Runtime.ActionSecondsRemaining = 0.0f;
    Runtime.DecisionCooldown = 0.8f;
    Runtime.SocialTargetId.Invalidate();
    Character.ClearMovementTarget();
    Character.SetCurrentIntent(ELLActionIntent::Idle);
}

ALLResidentCharacter* ALLWorldDirector::ChooseSocialTarget(const ALLResidentCharacter& Character) const
{
    ALLResidentCharacter* BestTarget = nullptr;
    float BestDistanceSquared = TNumericLimits<float>::Max();

    for (ALLResidentCharacter* Candidate : SpawnedResidents)
    {
        if (!IsValid(Candidate) || Candidate == &Character)
        {
            continue;
        }

        const float DistanceSquared = FVector::DistSquared2D(Character.GetActorLocation(), Candidate->GetActorLocation());
        if (DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestTarget = Candidate;
        }
    }

    return BestTarget;
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

float ALLWorldDirector::GetActionDuration(ELLActionIntent Intent) const
{
    switch (Intent)
    {
        case ELLActionIntent::Sleep: return 4.5f;
        case ELLActionIntent::Eat: return 2.5f;
        case ELLActionIntent::Socialize: return 3.0f;
        case ELLActionIntent::Hygiene: return 2.8f;
        case ELLActionIntent::Toilet: return 1.8f;
        case ELLActionIntent::HaveFun: return 3.5f;
        case ELLActionIntent::Idle:
        default: return 1.2f;
    }
}
