#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "GameFramework/PlayerController.h"
#include "LLObserverPlayerController.generated.h"

class ALLResidentCharacter;

// Routes a single tap/click. All positions are viewport pixels (the space
// GetMousePosition, touch events and ProjectWorldLocationToScreen share):
//   1. HUD chrome (quick inspector card, detail tabs) - consumed there.
//   2. The resident whose projected screen position is nearest the tap,
//      within the touch-target radius - select it.
//   3. Otherwise - step one observation level back.
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
    // ExactHit: the resident directly under the pointer (trace), used when no
    // resident is within the touch-target radius.
    void ApplyTap(const FVector2D& ScreenPosition, ALLResidentCharacter* ExactHit);

    // Resident whose projected render bounds, grown by RadiusPixels, contain
    // the tap; the closest bounds rectangle wins. OutDistance is the distance
    // from the tap to the nearest resident's bounds rectangle in pixels
    // (0 when inside), or -1 when no resident projects on screen.
    ALLResidentCharacter* FindResidentAtScreenPosition(const FVector2D& ScreenPosition, float RadiusPixels, float& OutDistance) const;
};
