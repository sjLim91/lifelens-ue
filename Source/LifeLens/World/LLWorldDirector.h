#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/LLTypes.h"
#include "LLWorldDirector.generated.h"

class ALLActivityAnchor;
class ALLResidentCharacter;
class ULLSimulationSubsystem;

struct FLLResidentRuntimeState
{
    bool bPerformingAction = false;
    float ActionSecondsRemaining = 0.0f;
    float DecisionCooldown = 0.0f;
    FGuid SocialTargetId;
};

UCLASS()
class LIFELENS_API ALLWorldDirector : public AActor
{
    GENERATED_BODY()

public:
    ALLWorldDirector();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category="LifeLens|World")
    ALLResidentCharacter* FindResidentActor(FGuid ResidentId) const;

    UFUNCTION(BlueprintPure, Category="LifeLens|World")
    ELLActionIntent GetResidentIntent(FGuid ResidentId) const;

protected:
    virtual void BeginPlay() override;

private:
    void CollectActivityAnchors();
    void SpawnResidents();
    void UpdateResident(ALLResidentCharacter& Character, float DeltaSeconds);
    void StartNextAction(ALLResidentCharacter& Character, FLLResidentRuntimeState& Runtime);
    void CompleteAction(ALLResidentCharacter& Character, FLLResidentRuntimeState& Runtime);
    ALLResidentCharacter* ChooseSocialTarget(const ALLResidentCharacter& Character) const;
    FVector ResolveTargetLocation(ELLActionIntent Intent, FGuid ResidentId) const;
    float GetActionDuration(ELLActionIntent Intent) const;

    UPROPERTY()
    TObjectPtr<ULLSimulationSubsystem> Simulation;

    UPROPERTY()
    TArray<TObjectPtr<ALLResidentCharacter>> SpawnedResidents;

    UPROPERTY()
    TArray<TObjectPtr<ALLActivityAnchor>> ActivityAnchors;

    TMap<FGuid, FLLResidentRuntimeState> RuntimeStates;

    float SimulationClockAccumulator = 0.0f;

    UPROPERTY(EditAnywhere, Category="LifeLens|Time")
    float RealSecondsPerSimulationMinute = 0.6f;
};
