#include "UI/LLObserverPlayerController.h"
#include "UI/LLObservationSubsystem.h"
#include "Characters/LLResidentCharacter.h"
#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"

ALLObserverPlayerController::ALLObserverPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableTouchEvents = true;
}

void ALLObserverPlayerController::BeginPlay()
{
    Super::BeginPlay();
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
    FHitResult Hit;
    if (GetHitResultUnderCursor(ECC_Pawn, true, Hit))
    {
        ApplySelectionFromHit(Hit);
    }
    else
    {
        ApplySelectionFromHit(FHitResult());
    }
}

void ALLObserverPlayerController::HandleTouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
    FHitResult Hit;
    if (GetHitResultUnderFinger(FingerIndex, ECC_Pawn, true, Hit))
    {
        ApplySelectionFromHit(Hit);
    }
    else
    {
        ApplySelectionFromHit(FHitResult());
    }
}

void ALLObserverPlayerController::ApplySelectionFromHit(const FHitResult& Hit)
{
    UGameInstance* GameInstance = GetGameInstance();
    ULLObservationSubsystem* Observation = GameInstance ? GameInstance->GetSubsystem<ULLObservationSubsystem>() : nullptr;
    if (!Observation)
    {
        return;
    }

    if (const ALLResidentCharacter* Resident = Cast<ALLResidentCharacter>(Hit.GetActor()))
    {
        Observation->ObserveResident(Resident->GetResidentId());
    }
    else
    {
        Observation->ClearObservedResident();
    }
}
