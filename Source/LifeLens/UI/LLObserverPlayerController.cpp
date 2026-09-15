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
    //   ll.ViewResidents        frame the densest resident cluster
    //   ll.ViewResidents 2      frame resident #2 closely
    //   ll.ViewReset            hand the view back to the game mode camera
    TWeakObjectPtr<ACameraActor> GDebugViewCamera;
    TWeakObjectPtr<AActor> GOriginalViewTarget;

    // Residents that are further apart than this are not framed together: the
    // camera would have to pull back so far that nobody is readable.
    constexpr float ClusterRadiusUU = 3000.0f;
    // Smallest subject radius, so one resident is still viewed from a sane
    // distance rather than from inside their own capsule.
    constexpr float SingleResidentRadiusUU = 260.0f;
    constexpr float MinViewDistanceUU = 700.0f;
    constexpr float MaxViewDistanceUU = 9000.0f;
    constexpr float ResidentEyeHeightUU = 90.0f;

    APlayerController* FirstLocalController(UWorld* World)
    {
        return World ? World->GetFirstPlayerController() : nullptr;
    }

    // Framing distance for a bounding sphere at a given vertical FOV, with a
    // margin so the subject does not touch the screen edge.
    float FramingDistanceUU(float RadiusUU, float FieldOfViewDegrees)
    {
        const float HalfAngle = FMath::DegreesToRadians(FMath::Clamp(FieldOfViewDegrees, 20.0f, 120.0f) * 0.5f);
        const float Distance = RadiusUU / FMath::Max(FMath::Tan(HalfAngle), KINDA_SMALL_NUMBER);
        return Distance * 1.25f;
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

        FVector Centre = FVector::ZeroVector;
        float Radius = SingleResidentRadiusUU;
        int32 Framed = 1;
        int32 Excluded = 0;

        if (Residents.IsValidIndex(Index))
        {
            Centre = Residents[Index]->GetActorLocation();
        }
        else
        {
            // Residents can wander far apart. Framing the full bounding box then
            // puts the camera kilometres away and everything becomes a speck, so
            // the view centres on the densest cluster instead and reports who was
            // left out.
            FVector Best = Residents[0]->GetActorLocation();
            int32 BestCount = 0;
            for (const ALLResidentCharacter* Candidate : Residents)
            {
                const FVector CandidateLocation = Candidate->GetActorLocation();
                int32 Count = 0;
                for (const ALLResidentCharacter* Other : Residents)
                {
                    if (FVector::DistSquared2D(CandidateLocation, Other->GetActorLocation())
                        <= FMath::Square(ClusterRadiusUU))
                    {
                        ++Count;
                    }
                }
                if (Count > BestCount)
                {
                    BestCount = Count;
                    Best = CandidateLocation;
                }
            }

            FBox Cluster(ForceInit);
            for (const ALLResidentCharacter* Resident : Residents)
            {
                const FVector Location = Resident->GetActorLocation();
                if (FVector::DistSquared2D(Best, Location) <= FMath::Square(ClusterRadiusUU))
                {
                    Cluster += Location;
                }
                else
                {
                    ++Excluded;
                }
            }
            Centre = Cluster.GetCenter();
            Framed = Residents.Num() - Excluded;
            Radius = FMath::Max(Cluster.GetSize().Size2D() * 0.5f, SingleResidentRadiusUU);
        }

        const float FieldOfView = 70.0f;
        const float Distance = FMath::Clamp(FramingDistanceUU(Radius, FieldOfView),
                                            MinViewDistanceUU, MaxViewDistanceUU);
        // A 35 degree look-down keeps both the residents and the ground around
        // them in frame.
        const float Height = Distance * 0.70f;

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

        const FVector Focus = Centre + FVector(0.0f, 0.0f, ResidentEyeHeightUU);
        const FVector CameraLocation = Focus + FVector(0.0f, -Distance, Height);
        Camera->SetActorLocation(CameraLocation);
        Camera->SetActorRotation((Focus - CameraLocation).Rotation());
        if (UCameraComponent* CameraComponent = Camera->GetCameraComponent())
        {
            CameraComponent->SetFieldOfView(FieldOfView);
        }
        Controller->SetViewTarget(Camera);

        UE_LOG(LogTemp, Log,
            TEXT("ll.ViewResidents: framed %d of %d (excluded %d beyond %.0f), centre=%s radius=%.0f distance=%.0f height=%.0f"),
            Framed, Residents.Num(), Excluded, ClusterRadiusUU,
            *Centre.ToCompactString(), Radius, Distance, Height);
        if (Excluded > 0)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ll.ViewResidents: %d resident(s) are outside the cluster. Use ll.ViewResidents <index> to look at one."),
                Excluded);
        }
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
