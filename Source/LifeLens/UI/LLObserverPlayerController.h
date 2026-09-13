#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LLObserverPlayerController.generated.h"

UCLASS()
class LIFELENS_API ALLObserverPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ALLObserverPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

private:
    void HandlePrimarySelect();
    void HandleTouchPressed(ETouchIndex::Type FingerIndex, FVector Location);
    void ApplySelectionFromHit(const FHitResult& Hit);
};
