#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "GameFramework/PlayerController.h"
#include "LLObserverPlayerController.generated.h"

// Routes a single tap/click:
//   1. HUD chrome first (quick inspector card, detail tabs) - consumed there.
//   2. A resident actor under the pointer - select it.
//   3. Empty space - step one observation level back.
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
    void ApplyTap(const FVector2D& ScreenPosition, const FHitResult& Hit);
};
