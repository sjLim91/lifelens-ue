#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LLObservationSubsystem.generated.h"

// Observer UI levels (SPEC 55).
UENUM(BlueprintType)
enum class ELLObservationLevel : uint8
{
    World  UMETA(DisplayName = "Level 0 - World"),
    Quick  UMETA(DisplayName = "Level 1 - Quick Inspector"),
    Detail UMETA(DisplayName = "Level 2 - Character Detail")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLLObservedResidentChanged, FGuid, ResidentId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLLObservationLevelChanged, ELLObservationLevel, Level);

// Holds what the observer is looking at and how deep. Pure UI state; it never
// touches the simulation.
UCLASS()
class LIFELENS_API ULLObservationSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Select a resident. A new resident opens at LEVEL 1; re-selecting the
    // current resident keeps the current level.
    UFUNCTION(BlueprintCallable, Category="LifeLens|Observation")
    void ObserveResident(FGuid ResidentId);

    // Back to LEVEL 0.
    UFUNCTION(BlueprintCallable, Category="LifeLens|Observation")
    void ClearObservedResident();

    // LEVEL 1 -> LEVEL 2. No-op without a selected resident.
    UFUNCTION(BlueprintCallable, Category="LifeLens|Observation")
    void OpenDetail();

    // LEVEL 2 -> LEVEL 1.
    UFUNCTION(BlueprintCallable, Category="LifeLens|Observation")
    void CloseDetail();

    // One level up: LEVEL 2 -> 1, LEVEL 1 -> 0. Used for taps on empty space.
    UFUNCTION(BlueprintCallable, Category="LifeLens|Observation")
    void StepBack();

    UFUNCTION(BlueprintPure, Category="LifeLens|Observation")
    bool HasObservedResident() const { return ObservedResidentId.IsValid(); }

    UFUNCTION(BlueprintPure, Category="LifeLens|Observation")
    FGuid GetObservedResidentId() const { return ObservedResidentId; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Observation")
    ELLObservationLevel GetObservationLevel() const { return Level; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Observation")
    bool IsDetailOpen() const { return Level == ELLObservationLevel::Detail; }

    UPROPERTY(BlueprintAssignable, Category="LifeLens|Observation")
    FLLObservedResidentChanged OnObservedResidentChanged;

    UPROPERTY(BlueprintAssignable, Category="LifeLens|Observation")
    FLLObservationLevelChanged OnObservationLevelChanged;

private:
    void SetLevel(ELLObservationLevel NewLevel);

    UPROPERTY()
    FGuid ObservedResidentId;

    UPROPERTY()
    ELLObservationLevel Level = ELLObservationLevel::World;
};
