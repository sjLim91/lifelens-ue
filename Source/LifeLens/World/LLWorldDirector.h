#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/LLTypes.h"
#include "Simulation/LLCoreActionTypes.h"
#include "LLWorldDirector.generated.h"

class ALLActivityAnchor;
class ALLResidentCharacter;
class ULLSimulationSubsystem;
class ULLCoreBridgeSubsystem;

struct FLLResidentRuntimeState
{
    bool bInitialized = false;
    bool bPerformingAction = false;
    ELLCoreObservedActivityKind LastActivityKind = ELLCoreObservedActivityKind::Idle;
    ELLCorePhysicalIntent LastPhysicalIntent = ELLCorePhysicalIntent::None;
    ELLCoreSocialIntent LastSocialIntent = ELLCoreSocialIntent::None;
    FGuid LastTargetId;
    TWeakObjectPtr<ALLActivityAnchor> ReservedAnchor;
    ELLActionIntent ReservedIntent = ELLActionIntent::Idle;
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
    void ApplyCoreDirective(
        ALLResidentCharacter& Character,
        FLLResidentRuntimeState& Runtime,
        const FLLCoreActionDirective& Directive);
    ELLActionIntent ToPresentationIntent(ELLCorePhysicalIntent Intent) const;
    ALLActivityAnchor* FindBestUsableAnchor(
        const ALLResidentCharacter& Character,
        ELLActionIntent Intent) const;
    ALLActivityAnchor* EnsurePhysicalReservation(
        ALLResidentCharacter& Character,
        FLLResidentRuntimeState& Runtime,
        ELLActionIntent Intent);
    void ReleasePhysicalReservation(FGuid ResidentId, FLLResidentRuntimeState& Runtime);
    FVector ResolveSocialTargetLocation(
        const ALLResidentCharacter& Character,
        const ALLResidentCharacter& Target,
        ELLCoreSocialIntent SocialIntent) const;

    UPROPERTY()
    TObjectPtr<ULLSimulationSubsystem> Simulation;

    UPROPERTY()
    TObjectPtr<ULLCoreBridgeSubsystem> CoreBridge;

    UPROPERTY()
    TArray<TObjectPtr<ALLResidentCharacter>> SpawnedResidents;

    UPROPERTY()
    TArray<TObjectPtr<ALLActivityAnchor>> ActivityAnchors;

    TMap<FGuid, FLLResidentRuntimeState> RuntimeStates;

    float SimulationClockAccumulator = 0.0f;

    UPROPERTY(EditAnywhere, Category="LifeLens|Time")
    float RealSecondsPerSimulationMinute = 0.6f;
};
