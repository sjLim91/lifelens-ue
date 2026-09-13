#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LLObservationSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLLObservedResidentChanged, FGuid, ResidentId);

UCLASS()
class LIFELENS_API ULLObservationSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="LifeLens|Observation")
    void ObserveResident(FGuid ResidentId);

    UFUNCTION(BlueprintCallable, Category="LifeLens|Observation")
    void ClearObservedResident();

    UFUNCTION(BlueprintPure, Category="LifeLens|Observation")
    bool HasObservedResident() const { return ObservedResidentId.IsValid(); }

    UFUNCTION(BlueprintPure, Category="LifeLens|Observation")
    FGuid GetObservedResidentId() const { return ObservedResidentId; }

    UPROPERTY(BlueprintAssignable, Category="LifeLens|Observation")
    FLLObservedResidentChanged OnObservedResidentChanged;

private:
    UPROPERTY()
    FGuid ObservedResidentId;
};
