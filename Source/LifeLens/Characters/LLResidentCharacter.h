#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/LLTypes.h"
#include "LLResidentCharacter.generated.h"

class ULLDecisionComponent;

UCLASS(Blueprintable)
class LIFELENS_API ALLResidentCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ALLResidentCharacter();

    UFUNCTION(BlueprintCallable, Category="LifeLens|Resident")
    void BindResident(const FLLResidentData& ResidentData);

    UFUNCTION(BlueprintPure, Category="LifeLens|Resident")
    FGuid GetResidentId() const { return ResidentId; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Resident")
    FText GetResidentDisplayName() const { return ResidentDisplayName; }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LifeLens|AI")
    TObjectPtr<ULLDecisionComponent> DecisionComponent;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LifeLens|Resident")
    FGuid ResidentId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LifeLens|Resident")
    FText ResidentDisplayName;
};
