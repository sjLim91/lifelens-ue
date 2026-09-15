#include "UI/LLObserverPlayerController.h"
#include "UI/LLObservationSubsystem.h"
#include "UI/LLObserverHUD.h"
#include "Characters/LLResidentCharacter.h"
#include "Components/InputComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
    // QA aid for the visual milestones. The production observer camera is
    // owned by the game mode lane; these commands never change it, they only
    // point the local view somewhere else so spawn placement, environment
    // feedback and resident visuals can be inspected. Off unless typed.
    //
    //   ll.ViewResidents        frame every resident
    //   ll.ViewResidents 2      frame resident #2 closely
    //   ll.ViewReset            hand the view back to the game mode camera
    TWeakObjectPtr<ACameraActor> GDebugViewCamera;
    TWeakObjectPtr<AActor> GOriginalViewTarget;

    APlayerController* FirstLocalController(UWorld* World)
    {
        return World ? World->GetFirstPlayerController() : nullptr;
    }

    void FrameResidents(UWorld* World, const TArray<FString>& Args)
    {
        APlayerController* Controller = FirstLocalController(World);
        if (!Controller)
        {
            return;
        }

        TArray<ALLResidentCharacter*> Residents;
        for (TActorIterator<ALLResidentCharacter> It(World); It; ++It)
        {
            if (IsValid(*It))
            {
                Residents.Add(*It);
            }
        }
        if (Residents.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("ll.ViewResidents: no residents in the world"));
            return;
        }

        int32 Index = INDEX_NONE;
        if (Args.Num() > 0)
        {
            Index = FCString::Atoi(*Args[0]);
        }

        FBox Bounds(ForceInit);
        if (Residents.IsValidIndex(Index))
        {
            Bounds += Residents[Index]->GetActorLocation();
        }
        else
        {
            for (const ALLResidentCharacter* Resident : Residents)
            {
                Bounds += Resident->GetActorLocation();
            }
        }

        const FVector Centre = Bounds.GetCenter();
        // Enough distance to keep the whole spread in frame, with a floor so a
        // single resident is still viewed from a readable distance.
        const float Spread = FMath::Max(Bounds.GetSize().Size2D(), 400.0f);
        const float Distance = FMath::Clamp(Spread * 1.6f, 900.0f, 20000.0f);
        const float Height = Distance * 0.75f;

        if (!GDebugViewCamera.IsValid())
        {
            GDebugViewCamera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass());
            GOriginalViewTarget = Controller->GetViewTarget();
        }
        ACameraActor* Camera = GDebugViewCamera.Get();
        if (!Camera)
        {
            return;
        }

        const FVector CameraLocation = Centre + FVector(0.0f, -Distance, Height);
        Camera->SetActorLocation(CameraLocation);
        Camera->SetActorRotation((Centre - CameraLocation).Rotation());
        if (UCameraComponent* CameraComponent = Camera->GetCameraComponent())
        {
            CameraComponent->SetFieldOfView(70.0f);
        }
        Controller->SetViewTarget(Camera);

        UE_LOG(LogTemp, Log,
            TEXT("ll.ViewResidents: %d resident(s), centre=%s distance=%.0f height=%.0f"),
            Residents.Num(), *Centre.ToCompactString(), Distance, Height);
    }

    void ResetView(UWorld* World)
    {
        APlayerController* Controller = FirstLocalController(World);
        if (Controller && GOriginalViewTarget.IsValid())
        {
            Controller->SetViewTarget(GOriginalViewTarget.Get());
            UE_LOG(LogTemp, Log, TEXT("ll.ViewReset: view returned to the game mode camera"));
        }
        if (GDebugViewCamera.IsValid())
        {
            GDebugViewCamera->Destroy();
            GDebugViewCamera.Reset();
        }
        GOriginalViewTarget.Reset();
    }

    static FAutoConsoleCommandWithWorldAndArgs CVarViewResidents(
        TEXT("ll.ViewResidents"),
        TEXT("Debug/QA only: point the view at the residents. Optional index frames one resident. Does not change the production observer camera."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
            [](const TArray<FString>& Args, UWorld* World) { FrameResidents(World, Args); }));

    static FAutoConsoleCommandWithWorld CVarViewReset(
        TEXT("ll.ViewReset"),
        TEXT("Debug/QA only: hand the view back to the game mode observer camera."),
        FConsoleCommandWithWorldDelegate::CreateStatic([](UWorld* World) { ResetView(World); }));
}

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
    // position used by GetMousePosition.
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
    if (!GetMousePosition(MouseX, MouseY))
    {
        return;
    }

    FHitResult Hit;
    ALLResidentCharacter* ExactHit = nullptr;
    if (GetHitResultUnderCursor(ECC_Pawn, true, Hit))
    {
        ExactHit = Cast<ALLResidentCharacter>(Hit.GetActor());
    }
    ApplyTap(FVector2D(MouseX, MouseY), ExactHit);
}

void ALLObserverPlayerController::HandleTouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
    FHitResult Hit;
    ALLResidentCharacter* ExactHit = nullptr;
    if (GetHitResultUnderFinger(FingerIndex, ECC_Pawn, true, Hit))
    {
        ExactHit = Cast<ALLResidentCharacter>(Hit.GetActor());
    }
    ApplyTap(FVector2D(Location.X, Location.Y), ExactHit);
}

ALLResidentCharacter* ALLObserverPlayerController::FindResidentAtScreenPosition(const FVector2D& ScreenPosition, float RadiusPixels, float& OutDistance) const
{
    OutDistance = -1.0f;
    ALLResidentCharacter* Picked = nullptr;
    float PickedDistance = -1.0f;

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<ALLResidentCharacter> It(World); It; ++It)
    {
        ALLResidentCharacter* Resident = *It;
        FBox2D Bounds;
        FBox2D Tap;
        if (!Resident || !ALLObserverHUD::ProjectResidentTapRect(this, Resident, RadiusPixels, Bounds, Tap))
        {
            continue;
        }

        const float Distance = static_cast<float>(FMath::Sqrt(Bounds.ComputeSquaredDistanceToPoint(ScreenPosition)));
        if (OutDistance < 0.0f || Distance < OutDistance)
        {
            OutDistance = Distance;
        }

        if (Tap.IsInside(ScreenPosition) && (PickedDistance < 0.0f || Distance < PickedDistance))
        {
            Picked = Resident;
            PickedDistance = Distance;
        }
    }

    return Picked;
}

void ALLObserverPlayerController::ApplyTap(const FVector2D& ScreenPosition, ALLResidentCharacter* ExactHit)
{
    UGameInstance* GameInstance = GetGameInstance();
    ULLObservationSubsystem* Observation = GameInstance ? GameInstance->GetSubsystem<ULLObservationSubsystem>() : nullptr;
    if (!Observation)
    {
        return;
    }

    ALLObserverHUD* ObserverHUD = GetHUD<ALLObserverHUD>();

    int32 ViewportX = 0;
    int32 ViewportY = 0;
    GetViewportSize(ViewportX, ViewportY);
    const FVector2D ViewportSize(ViewportX, ViewportY);

    const float Radius = ALLObserverHUD::TouchTargetRadiusPixels(this);
    float NearestDistance = -1.0f;
    ALLResidentCharacter* NearResident = FindResidentAtScreenPosition(ScreenPosition, Radius, NearestDistance);

    const FVector2D CanvasPosition = ObserverHUD ? ObserverHUD->ViewportToCanvas(ScreenPosition, ViewportSize) : ScreenPosition;

    // 1. HUD chrome (quick inspector card, detail tabs).
    const bool bHUDConsumed = ObserverHUD && ObserverHUD->HandleTap(ScreenPosition, ViewportSize);

    UE_LOG(LogTemp, Log, TEXT("LLObserver tap viewport (%.0f, %.0f) canvas (%.0f, %.0f) viewportSize (%.0f, %.0f) level %d hud %s nearestResident %.0f px radius %.0f px exactHit %s"),
        ScreenPosition.X, ScreenPosition.Y, CanvasPosition.X, CanvasPosition.Y, ViewportSize.X, ViewportSize.Y,
        static_cast<int32>(Observation->GetObservationLevel()), bHUDConsumed ? TEXT("consumed") : TEXT("none"),
        NearestDistance, Radius, ExactHit ? TEXT("yes") : TEXT("no"));

    if (bHUDConsumed)
    {
        return;
    }

    // 2. A resident within the touch target, else the resident directly under the pointer.
    if (ALLResidentCharacter* Picked = NearResident ? NearResident : ExactHit)
    {
        Observation->ObserveResident(Picked->GetResidentId());
        return;
    }

    // 3. Empty space: one level back (LEVEL 2 -> 1 -> 0).
    Observation->StepBack();
}
