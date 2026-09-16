#include "UI/LLObserverPlayerController.h"
#include "UI/LLObservationSubsystem.h"
#include "UI/LLObserverHUD.h"
#include "Characters/LLResidentCharacter.h"
#include "Core/LLLifeLensGameMode.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Camera/CameraActor.h"
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
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ALLObserverPlayerController::HandleRightMousePressed);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &ALLObserverPlayerController::HandleRightMouseReleased);
    InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &ALLObserverPlayerController::HandleMiddleMousePressed);
    InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Released, this, &ALLObserverPlayerController::HandleMiddleMouseReleased);
    InputComponent->BindAxisKey(EKeys::MouseWheelAxis, this, &ALLObserverPlayerController::HandleMouseWheel);

    // Touch selection is intentionally deferred until release. Pressing a
    // finger is not a selection because the same gesture may become an orbit,
    // pinch or two-finger pan.
    InputComponent->BindTouch(IE_Pressed, this, &ALLObserverPlayerController::HandleTouchPressed);
    InputComponent->BindTouch(IE_Released, this, &ALLObserverPlayerController::HandleTouchReleased);
}

void ALLObserverPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    EnsureCameraInitialized();
    if (!bCameraInitialized)
    {
        return;
    }

    UpdateMouseCameraInput();
    UpdateTouchCameraInput();
    ApplyCameraTransform(DeltaTime);
}

void ALLObserverPlayerController::EnsureCameraInitialized()
{
    ACameraActor* Camera = Cast<ACameraActor>(GetViewTarget());
    if (!Camera)
    {
        ObserverCamera.Reset();
        bCameraInitialized = false;
        return;
    }

    if (bCameraInitialized && ObserverCamera.Get() == Camera)
    {
        return;
    }

    ObserverCamera = Camera;

    float TargetHeight = 0.0f;
    if (const UWorld* World = GetWorld())
    {
        if (const ALLLifeLensGameMode* GameMode = World->GetAuthGameMode<ALLLifeLensGameMode>())
        {
            TargetHeight = GameMode->GetObserverCameraTargetHeightUU();
        }
    }

    const FVector CameraLocation = Camera->GetActorLocation();
    const FVector CameraForward = Camera->GetActorForwardVector();

    // Preserve the existing production framing by intersecting the current
    // camera forward ray with the configured target-height plane.
    FVector InitialTarget(0.0f, 0.0f, TargetHeight);
    if (FMath::Abs(CameraForward.Z) > KINDA_SMALL_NUMBER)
    {
        const float RayDistance = (TargetHeight - CameraLocation.Z) / CameraForward.Z;
        if (RayDistance > 0.0f)
        {
            InitialTarget = CameraLocation + CameraForward * RayDistance;
            InitialTarget.Z = TargetHeight;
        }
    }

    const FVector Offset = CameraLocation - InitialTarget;
    const float HorizontalDistance = FVector2D(Offset.X, Offset.Y).Size();
    const float RawDistance = Offset.Size();
    if (RawDistance <= KINDA_SMALL_NUMBER)
    {
        bCameraInitialized = false;
        return;
    }

    const float MinDistance = FMath::Max(100.0f, CameraMinDistanceUU);
    const float MaxDistance = FMath::Max(MinDistance, CameraMaxDistanceUU);
    const float MinElevation = FMath::Clamp(CameraMinElevationDegrees, 1.0f, 89.0f);
    const float MaxElevation = FMath::Clamp(CameraMaxElevationDegrees, MinElevation, 89.0f);

    CurrentOrbitTarget = InitialTarget;
    DesiredOrbitTarget = InitialTarget;
    CurrentOrbitYawDegrees = FMath::RadiansToDegrees(FMath::Atan2(Offset.Y, Offset.X));
    DesiredOrbitYawDegrees = CurrentOrbitYawDegrees;
    CurrentOrbitElevationDegrees = FMath::Clamp(
        FMath::RadiansToDegrees(FMath::Atan2(Offset.Z, FMath::Max(HorizontalDistance, KINDA_SMALL_NUMBER))),
        MinElevation,
        MaxElevation);
    DesiredOrbitElevationDegrees = CurrentOrbitElevationDegrees;
    CurrentOrbitDistanceUU = FMath::Clamp(RawDistance, MinDistance, MaxDistance);
    DesiredOrbitDistanceUU = CurrentOrbitDistanceUU;
    bCameraInitialized = true;
}

void ALLObserverPlayerController::HandleRightMousePressed()
{
    bRightMouseDragging = true;
}

void ALLObserverPlayerController::HandleRightMouseReleased()
{
    bRightMouseDragging = false;
}

void ALLObserverPlayerController::HandleMiddleMousePressed()
{
    bMiddleMouseDragging = true;
}

void ALLObserverPlayerController::HandleMiddleMouseReleased()
{
    bMiddleMouseDragging = false;
}

void ALLObserverPlayerController::HandleMouseWheel(float AxisValue)
{
    if (FMath::IsNearlyZero(AxisValue))
    {
        return;
    }

    EnsureCameraInitialized();
    if (!bCameraInitialized)
    {
        return;
    }

    // Positive wheel = zoom in, negative = zoom out.
    const float PerNotchScale = FMath::Clamp(1.0f - MouseWheelZoomFraction, 0.2f, 0.99f);
    ZoomByScale(FMath::Pow(PerNotchScale, AxisValue));
}

void ALLObserverPlayerController::UpdateMouseCameraInput()
{
    if (!bRightMouseDragging && !bMiddleMouseDragging)
    {
        return;
    }

    float DeltaX = 0.0f;
    float DeltaY = 0.0f;
    GetInputMouseDelta(DeltaX, DeltaY);
    const FVector2D Delta(DeltaX, DeltaY);
    if (Delta.IsNearlyZero())
    {
        return;
    }

    if (bRightMouseDragging)
    {
        RotateByScreenDelta(Delta, MouseRotateDegreesPerPixel);
    }
    if (bMiddleMouseDragging)
    {
        PanByScreenDelta(Delta);
    }
}

void ALLObserverPlayerController::RotateByScreenDelta(const FVector2D& Delta, float DegreesPerPixel)
{
    DesiredOrbitYawDegrees = FMath::UnwindDegrees(DesiredOrbitYawDegrees + Delta.X * DegreesPerPixel);

    const float MinElevation = FMath::Clamp(CameraMinElevationDegrees, 1.0f, 89.0f);
    const float MaxElevation = FMath::Clamp(CameraMaxElevationDegrees, MinElevation, 89.0f);
    DesiredOrbitElevationDegrees = FMath::Clamp(
        DesiredOrbitElevationDegrees - Delta.Y * DegreesPerPixel,
        MinElevation,
        MaxElevation);
}

void ALLObserverPlayerController::PanByScreenDelta(const FVector2D& Delta, float ScaleMultiplier)
{
    ACameraActor* Camera = ObserverCamera.Get();
    if (!Camera)
    {
        return;
    }

    FVector Right = Camera->GetActorRightVector();
    FVector Forward = Camera->GetActorForwardVector();
    Right.Z = 0.0f;
    Forward.Z = 0.0f;
    if (!Right.Normalize() || !Forward.Normalize())
    {
        return;
    }

    const float DistanceForScale = FMath::Max(CurrentOrbitDistanceUU, DesiredOrbitDistanceUU);
    const float WorldPerPixel = DistanceForScale * FMath::Max(0.0001f, PanWorldPerPixelDistanceFactor) * ScaleMultiplier;

    // Grab-style pan: moving the pointer/fingers right drags the world right,
    // so the camera target moves left. Vertical screen movement maps to the
    // camera's ground-projected forward axis.
    DesiredOrbitTarget += (-Right * Delta.X + Forward * Delta.Y) * WorldPerPixel;
}

void ALLObserverPlayerController::ZoomByScale(float Scale)
{
    const float MinDistance = FMath::Max(100.0f, CameraMinDistanceUU);
    const float MaxDistance = FMath::Max(MinDistance, CameraMaxDistanceUU);
    DesiredOrbitDistanceUU = FMath::Clamp(DesiredOrbitDistanceUU * FMath::Max(0.01f, Scale), MinDistance, MaxDistance);
}

float ALLObserverPlayerController::TouchDragThresholdPixels() const
{
    const float DPIScale = FMath::Max(1.0f, UWidgetLayoutLibrary::GetViewportScale(this));
    return FMath::Max(1.0f, TouchDragThresholdLogicalPixels) * DPIScale;
}

void ALLObserverPlayerController::HandlePrimarySelect()
{
    if (bRightMouseDragging || bMiddleMouseDragging)
    {
        return;
    }

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
    const FVector2D Position(Location.X, Location.Y);

    if (FingerIndex == ETouchIndex::Touch1)
    {
        bTouch1Tracked = true;
        bTouchGesture = false;
        bTouchHadSecondFinger = false;
        bTwoFingerActive = false;
        TouchStart1 = Position;
        LastTouch1 = Position;
        LastTouch2 = FVector2D::ZeroVector;
    }
    else if (FingerIndex == ETouchIndex::Touch2 && bTouch1Tracked)
    {
        bTouchHadSecondFinger = true;
        bTouchGesture = true;
        bTwoFingerActive = true;
        LastTouch2 = Position;
    }
}

void ALLObserverPlayerController::HandleTouchReleased(ETouchIndex::Type FingerIndex, FVector Location)
{
    if (FingerIndex == ETouchIndex::Touch2)
    {
        bTouchHadSecondFinger = true;
        bTouchGesture = true;
        bTwoFingerActive = false;
        return;
    }

    if (FingerIndex != ETouchIndex::Touch1)
    {
        return;
    }

    const FVector2D ReleasePosition(Location.X, Location.Y);
    const bool bReleaseStayedWithinTapThreshold =
        (ReleasePosition - TouchStart1).Size() <= TouchDragThresholdPixels();
    const bool bTap = bTouch1Tracked
        && !bTouchGesture
        && !bTouchHadSecondFinger
        && bReleaseStayedWithinTapThreshold;

    ALLResidentCharacter* ExactHit = nullptr;
    if (bTap)
    {
        FHitResult Hit;
        if (GetHitResultUnderFinger(FingerIndex, ECC_Pawn, true, Hit))
        {
            ExactHit = Cast<ALLResidentCharacter>(Hit.GetActor());
        }
    }

    bTouch1Tracked = false;
    bTouchGesture = false;
    bTouchHadSecondFinger = false;
    bTwoFingerActive = false;

    if (bTap)
    {
        ApplyTap(ReleasePosition, ExactHit);
    }
}

void ALLObserverPlayerController::UpdateTouchCameraInput()
{
    float X1 = 0.0f;
    float Y1 = 0.0f;
    bool bTouch1Pressed = false;
    GetInputTouchState(ETouchIndex::Touch1, X1, Y1, bTouch1Pressed);

    float X2 = 0.0f;
    float Y2 = 0.0f;
    bool bTouch2Pressed = false;
    GetInputTouchState(ETouchIndex::Touch2, X2, Y2, bTouch2Pressed);

    if (!bTouch1Pressed)
    {
        return;
    }

    const FVector2D Current1(X1, Y1);
    if (!bTouch1Tracked)
    {
        // Defensive path for platforms that do not deliver the pressed event
        // before the first PlayerTick.
        bTouch1Tracked = true;
        TouchStart1 = Current1;
        LastTouch1 = Current1;
    }

    if (bTouch2Pressed)
    {
        const FVector2D Current2(X2, Y2);
        if (!bTwoFingerActive)
        {
            bTouchHadSecondFinger = true;
            bTouchGesture = true;
            bTwoFingerActive = true;
            LastTouch1 = Current1;
            LastTouch2 = Current2;
            return;
        }

        const float PreviousPinchDistance = (LastTouch1 - LastTouch2).Size();
        const float CurrentPinchDistance = (Current1 - Current2).Size();
        if (PreviousPinchDistance > 1.0f && CurrentPinchDistance > 1.0f)
        {
            const float RawScale = PreviousPinchDistance / CurrentPinchDistance;
            ZoomByScale(FMath::Pow(RawScale, FMath::Max(0.1f, PinchZoomSensitivity)));
        }

        const FVector2D PreviousMid = (LastTouch1 + LastTouch2) * 0.5f;
        const FVector2D CurrentMid = (Current1 + Current2) * 0.5f;
        PanByScreenDelta(CurrentMid - PreviousMid);

        LastTouch1 = Current1;
        LastTouch2 = Current2;
        return;
    }

    if (bTwoFingerActive)
    {
        bTwoFingerActive = false;
        bTouchGesture = true;
        LastTouch1 = Current1;
        return;
    }

    const FVector2D Delta = Current1 - LastTouch1;
    if ((Current1 - TouchStart1).Size() > TouchDragThresholdPixels())
    {
        bTouchGesture = true;
    }

    if (bTouchGesture && !Delta.IsNearlyZero())
    {
        RotateByScreenDelta(Delta, TouchRotateDegreesPerPixel);
    }
    LastTouch1 = Current1;
}

void ALLObserverPlayerController::ApplyCameraTransform(float DeltaTime)
{
    ACameraActor* Camera = ObserverCamera.Get();
    if (!Camera)
    {
        return;
    }

    const float Alpha = 1.0f - FMath::Exp(-FMath::Max(0.1f, CameraSmoothingSpeed) * FMath::Max(0.0f, DeltaTime));

    CurrentOrbitTarget = FMath::Lerp(CurrentOrbitTarget, DesiredOrbitTarget, Alpha);
    CurrentOrbitYawDegrees = FMath::UnwindDegrees(
        CurrentOrbitYawDegrees + FMath::FindDeltaAngleDegrees(CurrentOrbitYawDegrees, DesiredOrbitYawDegrees) * Alpha);
    CurrentOrbitElevationDegrees = FMath::Lerp(CurrentOrbitElevationDegrees, DesiredOrbitElevationDegrees, Alpha);
    CurrentOrbitDistanceUU = FMath::Lerp(CurrentOrbitDistanceUU, DesiredOrbitDistanceUU, Alpha);

    const float YawRadians = FMath::DegreesToRadians(CurrentOrbitYawDegrees);
    const float ElevationRadians = FMath::DegreesToRadians(CurrentOrbitElevationDegrees);
    const float CosElevation = FMath::Cos(ElevationRadians);
    const FVector OffsetDirection(
        CosElevation * FMath::Cos(YawRadians),
        CosElevation * FMath::Sin(YawRadians),
        FMath::Sin(ElevationRadians));

    const FVector CameraLocation = CurrentOrbitTarget + OffsetDirection * CurrentOrbitDistanceUU;
    const FRotator CameraRotation = (CurrentOrbitTarget - CameraLocation).Rotation();
    Camera->SetActorLocationAndRotation(CameraLocation, CameraRotation);
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
