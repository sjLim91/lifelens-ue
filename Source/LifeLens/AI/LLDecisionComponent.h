#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/LLTypes.h"
#include "LLDecisionComponent.generated.h"

USTRUCT(BlueprintType)
struct FLLDecisionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    ELLActionIntent Intent = ELLActionIntent::Idle;

    UPROPERTY(BlueprintReadOnly)
    float Score = 0.0f;
};

UCLASS(ClassGroup=(LifeLens), meta=(BlueprintSpawnableComponent))
class LIFELENS_API ULLDecisionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULLDecisionComponent();

    UFUNCTION(BlueprintPure, Category="LifeLens|AI")
    FLLDecisionResult ChooseAction(const FLLResidentData& Resident) const;

private:
    static float Deficit(float NeedValue);
};
