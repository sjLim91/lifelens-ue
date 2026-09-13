#include "UI/LLObserverPlayerController.h"
#include "UI/LLObservationSubsystem.h"
#include "UI/LLObserverHUD.h"
#include "Characters/LLResidentCharacter.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"

ALLObserverPlayerController::ALLObserverPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableTouchEvents = true;
}

void ALLObserverPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Observer game: visible cursor, no viewport lock, capture only while a
    // button is held. Permanent capture hides the cursor and switches Slate to
    // high-precision (relative) mouse mode, which freezes the cached cursor
    // position used by GetMousePosition / GetHitResultUnderCursor.
    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);
    bShowMouseCursor = true;
}

void ALLObserverPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (!InputComponent)
    {
        return;
    }

    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ALLObserverPlayerController::HandlePrimarySelect);
    InputComponent->BindTouch(IE_Pressed, this, &ALLObserverPlayerController::HandleTouchPressed);
}

void ALLObserverPlayerController::HandlePrimarySelect()
{
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    GetMousePosition(MouseX, MouseY);
    const FVector2D ScreenPosition(MouseX, MouseY);

    FHitResult Hit;
    if (!GetHitResultUnderCursor(ECC_Pawn, true, Hit))
    {
        Hit = FHitResult();
    }
    ApplyTap(ScreenPosition, Hit);
}

void ALLObserverPlayerController::HandleTouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
    const FVector2D ScreenPosition(Location.X, Location.Y);

    FHitResult Hit;
    if (!GetHitResultUnderFinger(FingerIndex, ECC_Pawn, true, Hit))
    {
        Hit = FHitResult();
    }
    ApplyTap(ScreenPosition, Hit);
}

void ALLObserverPlayerController::ApplyTap(const FVector2D& ScreenPosition, const FHitResult& Hit)
{
    UGameInstance* GameInstance = GetGameInstance();
    ULLObservationSubsystem* Observation = GameInstance ? GameInstance->GetSubsystem<ULLObservationSubsystem>() : nullptr;
    if (!Observation)
    {
        return;
    }

    // 1. HUD chrome (quick inspector card, detail tabs). The viewport size is
    //    passed so the HUD can map the tap into its canvas space if the two
    //    ever differ.
    if (ALLObserverHUD* ObserverHUD = GetHUD<ALLObserverHUD>())
    {
        int32 ViewportX = 0;
        int32 ViewportY = 0;
        GetViewportSize(ViewportX, ViewportY);
        if (ObserverHUD->HandleTap(ScreenPosition, FVector2D(ViewportX, ViewportY)))
        {
            return;
        }
    }

    // 2. A resident in the world.
    if (const ALLResidentCharacter* Resident = Cast<ALLResidentCharacter>(Hit.GetActor()))
    {
        Observation->ObserveResident(Resident->GetResidentId());
        return;
    }

    // 3. Empty space: one level back (LEVEL 2 -> 1 -> 0).
    Observation->StepBack();
}
