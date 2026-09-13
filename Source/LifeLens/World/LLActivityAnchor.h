#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/LLTypes.h"
#include "LLActivityAnchor.generated.h"

UCLASS(Blueprintable)
class LIFELENS_API ALLActivityAnchor : public AActor
{
    GENERATED_BODY()

public:
    ALLActivityAnchor();

    UFUNCTION(BlueprintPure, Category="LifeLens|World")
    FVector GetUseLocation() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LifeLens|World")
    ELLActionIntent SupportedIntent = ELLActionIntent::Idle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LifeLens|World")
    FVector LocalUseOffset = FVector::ZeroVector;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;
};
