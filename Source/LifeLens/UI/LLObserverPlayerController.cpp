#include "UI/LLObserverPlayerController.h"
#include "UI/LLObservationSubsystem.h"
#include "UI/LLObserverHUD.h"
#include "Characters/LLResidentCharacter.h"
#include "Core/LLLifeLensGameMode.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "World/LLWorldSpatialContract.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

namespace
{
    // Ported from Dagyeom PR #98 onto the current observer camera code.
    // These commands are QA-only and never alter production camera tuning.
    //
    //   ll.ViewResidents        frame the densest resident cluster
    //   ll.ViewResidents 2      frame resident #2 closely
    //   ll.ViewReset            return to the original view target
    TWeakObjectPtr<ACameraActor> GDebugViewCamera;
    TWeakObjectPtr<AActor> GOriginalViewTarget;

    constexpr float DebugClusterRadiusUU = 3000.0f;
    constexpr float DebugSingleResidentRadiusUU = 260.0f;
    constexpr float DebugMinViewDistanceUU = 700.0f;
    constexpr float DebugMaxViewDistanceUU = 9000.0f;
    constexpr float DebugResidentEyeHeightUU = 90.0f;

    APlayerController* DebugFirstLocalController(UWorld* World)
    {
        return World ? World->GetFirstPlayerController() : nullptr;
    }

    float DebugFramingDistanceUU(float RadiusUU, float FieldOfViewDegrees)
    {
        const float HalfAngle = FMath::DegreesToRadians(
            FMath::Clamp(FieldOfViewDegrees, 20.0f, 120.0f) * 0.5f);
        const float Distance = RadiusUU
            / FMath::Max(FMath::Tan(HalfAngle), KINDA_SMALL_NUMBER);
        return Distance * 1.25f;
    }

    void DebugFrameResidents(UWorld* World, const TArray<FString>& Args)
    {
        APlayerController* Controller = DebugFirstLocalController(World);
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

        int32 RequestedIndex = INDEX_NONE;
        if (Args.Num() > 0)
        {
            RequestedIndex = FCString::Atoi(*Args[0]);
        }

        FVector Centre = FVector::ZeroVector;
        float Radius = DebugSingleResidentRadiusUU;
        int32 Framed = 1;
        int32 Excluded = 0;

        if (Residents.IsValidIndex(RequestedIndex))
        {
            Centre = Residents[RequestedIndex]->GetActorLocation();
        }
        else
        {
            FVector Best = Residents[0]->GetActorLocation();
            int32 BestCount = 0;
            for (const ALLResidentCharacter* Candidate : Residents)
            {
                const FVector CandidateLocation = Candidate->GetActorLocation();
                int32 Count = 0;
                for (const ALLResidentCharacter* Other : Residents)
                {
                    if (FVector::DistSquared2D(
                            CandidateLocation,
                            Other->GetActorLocation())
                        <= FMath::Square(DebugClusterRadiusUU))
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
                if (FVector::DistSquared2D(Best, Location)
                    <= FMath::Square(DebugClusterRadiusUU))
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
            Radius = FMath::Max(
                Cluster.GetSize().Size2D() * 0.5f,
                DebugSingleResidentRadiusUU);
        }

        const float FieldOfView = 70.0f;
        const float Distance = FMath::Clamp(
            DebugFramingDistanceUU(Radius, FieldOfView),
            DebugMinViewDistanceUU,
            DebugMaxViewDistanceUU);
        const float Height = Distance * 0.70f;

        if (!GDebugViewCamera.IsValid())
        {
            GDebugViewCamera = World->SpawnActor<ACameraActor>(
                ACameraActor::StaticClass());
            GOriginalViewTarget = Controller->GetViewTarget();
        }

        ACameraActor* Camera = GDebugViewCamera.Get();
        if (!Camera)
        {
            return;
        }

        const FVector Focus = Centre
            + FVector(0.0f, 0.0f, DebugResidentEyeHeightUU);
        const FVector CameraLocation = Focus
            + FVector(0.0f, -Distance, Height);
        Camera->SetActorLocation(CameraLocation);
        Camera->SetActorRotation((Focus - CameraLocation).Rotation());
        if (UCameraComponent* CameraComponent = Camera->GetCameraComponent())
        {
            CameraComponent->SetFieldOfView(FieldOfView);
        }
        Controller->SetViewTarget(Camera);

        UE_LOG(LogTemp, Log,
            TEXT("ll.ViewResidents: framed %d of %d (excluded %d beyond %.0f), centre=%s radius=%.0f distance=%.0f height=%.0f"),
            Framed,
            Residents.Num(),
            Excluded,
            DebugClusterRadiusUU,
            *Centre.ToCompactString(),
            Radius,
            Distance,
            Height);
    }

    void DebugResetObserverView(UWorld* World)
    {
        APlayerController* Controller = DebugFirstLocalController(World);
        if (Controller && GOriginalViewTarget.IsValid())
        {
            Controller->SetViewTarget(GOriginalViewTarget.Get());
            UE_LOG(LogTemp, Log,
                TEXT("ll.ViewReset: view returned to the production observer camera"));
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
        TEXT("QA only: frame residents. Optional index frames one resident."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
            [](const TArray<FString>& Args, UWorld* World)
            {
                DebugFrameResidents(World, Args);
            }));

    static FAutoConsoleCommandWithWorld CVarViewReset(
        TEXT("ll.ViewReset"),
        TEXT("QA only: return to the production observer camera."),
        FConsoleCommandWithWorldDelegate::CreateStatic(
            [](UWorld* World)
            {
                DebugResetObserverView(World);
            }));
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

    // QA camera from Dagyeom PR #98 is intentionally isolated from the
    // production orbit/follow controller. Without this guard the next tick
    // would adopt the temporary CameraActor and immediately move it.
    if (GDebugViewCamera.IsValid() && GetViewTarget() == GDebugViewCamera.Get())
    {
        return;
    }

    EnsureCameraInitialized();
    if (!bCameraInitialized)
    {
        return;
    }

    SyncObservedResidentSelection();
    UpdateMouseCameraInput();
    UpdateTouchCameraInput();
    UpdateObservedResidentFocus(DeltaTime);
    ApplyCameraTransform(DeltaTime);
}

void ALLObserverPlayerController::EnsureCameraInitialized()
{
    static const FName ProductionObserverCameraTag(TEXT("LifeLens.ObserverCamera"));

    ACameraActor* Camera = Cast<ACameraActor>(GetViewTarget());
    const bool bProductionCameraActive =
        Camera && Camera->ActorHasTag(ProductionObserverCameraTag);

    if (!bProductionCameraActive)
    {
        Camera = nullptr;

        // Primary recovery path: GameMode retains the exact camera it spawned.
        if (UWorld* World = GetWorld())
        {
            if (ALLLifeLensGameMode* GameMode =
                    World->GetAuthGameMode<ALLLifeLensGameMode>())
            {
                Camera = GameMode->GetObserverCamera();
            }

            // Fallback for lifecycle/order edge cases: recover by stable tag.
            if (!IsValid(Camera))
            {
                for (TActorIterator<ACameraActor> It(World); It; ++It)
                {
                    if (IsValid(*It) && It->ActorHasTag(ProductionObserverCameraTag))
                    {
                        Camera = *It;
                        break;
                    }
                }
            }
        }

        if (IsValid(Camera))
        {
            SetViewTarget(Camera);
            UE_LOG(LogTemp, Warning,
                TEXT("LifeLens observer view recovered to the production camera."));
        }
    }

    if (!IsValid(Camera))
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

    if (!bWorldOverviewCaptured)
    {
        WorldOverviewTarget = InitialTarget;
        WorldOverviewYawDegrees = CurrentOrbitYawDegrees;
        WorldOverviewElevationDegrees = CurrentOrbitElevationDegrees;
        WorldOverviewDistanceUU = CurrentOrbitDistanceUU;
        bWorldOverviewCaptured = true;
    }

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

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (GetMousePosition(MouseX, MouseY))
    {
        int32 ViewportX = 0;
        int32 ViewportY = 0;
        GetViewportSize(ViewportX, ViewportY);
        if (ALLObserverHUD* ObserverHUD = GetHUD<ALLObserverHUD>();
            ObserverHUD && ObserverHUD->HandleDetailScrollWheel(
                FVector2D(MouseX, MouseY), AxisValue, FVector2D(ViewportX, ViewportY)))
        {
            return;
        }
    }

    EnsureCameraInitialized();
    if (!bCameraInitialized)
    {
        return;
    }

    // Positive wheel = zoom in, negative = zoom out when the cursor is not
    // over LEVEL 2 scroll content.
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
    SuspendObservedResidentFollow();
    bReturningToWorldOverview = false;
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
    SuspendObservedResidentFollow();
    bReturningToWorldOverview = false;
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
    FVector ProposedTarget =
        DesiredOrbitTarget
        + (-Right * Delta.X + Forward * Delta.Y) * WorldPerPixel;

    // Local/Regional presentation is currently centred on the captured opening
    // overview. Free pan must not escape that rendered envelope and reveal the
    // broad continuity underlay as if it were actual explored terrain.
    if (bWorldOverviewCaptured)
    {
        const float MaxPanRadiusUU =
            FMath::Max(0.0f, ManualPanMaxRadiusChunks)
            * LLWorldSpatialContract::ChunkSpanUU;
        if (MaxPanRadiusUU > KINDA_SMALL_NUMBER)
        {
            FVector2D Offset(
                ProposedTarget.X - WorldOverviewTarget.X,
                ProposedTarget.Y - WorldOverviewTarget.Y);
            Offset = Offset.GetClampedToMaxSize(MaxPanRadiusUU);
            ProposedTarget.X = WorldOverviewTarget.X + Offset.X;
            ProposedTarget.Y = WorldOverviewTarget.Y + Offset.Y;
        }
    }

    DesiredOrbitTarget = ProposedTarget;
}

void ALLObserverPlayerController::ZoomByScale(float Scale)
{
    SuspendObservedResidentFollow();
    bReturningToWorldOverview = false;
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
        bHUDDetailScrollTouchActive = false;
        TouchStart1 = Position;
        LastTouch1 = Position;
        LastTouch2 = FVector2D::ZeroVector;

        int32 ViewportX = 0;
        int32 ViewportY = 0;
        GetViewportSize(ViewportX, ViewportY);
        if (ALLObserverHUD* ObserverHUD = GetHUD<ALLObserverHUD>();
            ObserverHUD && ObserverHUD->BeginDetailScrollDrag(Position, FVector2D(ViewportX, ViewportY)))
        {
            // This is only a scroll *candidate* until movement crosses the same
            // drag threshold as camera gestures. Keeping tap eligibility here
            // lets linked resident rows inside LEVEL 2 content remain tappable
            // on mobile instead of every press being consumed as a scroll.
            bHUDDetailScrollTouchActive = true;
        }
    }
    else if (FingerIndex == ETouchIndex::Touch2 && bTouch1Tracked)
    {
        if (bHUDDetailScrollTouchActive)
        {
            bTouchHadSecondFinger = true;
            return;
        }

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
        if (bHUDDetailScrollTouchActive)
        {
            bTouchHadSecondFinger = true;
            return;
        }

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
    const bool bWasHUDDetailScrollTouchActive = bHUDDetailScrollTouchActive;
    if (bWasHUDDetailScrollTouchActive)
    {
        if (ALLObserverHUD* ObserverHUD = GetHUD<ALLObserverHUD>())
        {
            ObserverHUD->EndDetailScrollDrag();
        }
    }

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
    bHUDDetailScrollTouchActive = false;

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

    if (bHUDDetailScrollTouchActive)
    {
        const bool bCrossedScrollThreshold =
            (Current1 - TouchStart1).Size() > TouchDragThresholdPixels();
        if (bCrossedScrollThreshold)
        {
            bTouchGesture = true;

            int32 ViewportX = 0;
            int32 ViewportY = 0;
            GetViewportSize(ViewportX, ViewportY);
            if (ALLObserverHUD* ObserverHUD = GetHUD<ALLObserverHUD>())
            {
                ObserverHUD->UpdateDetailScrollDrag(
                    Current1,
                    FVector2D(ViewportX, ViewportY));
            }
        }
        LastTouch1 = Current1;
        return;
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

ALLResidentCharacter* ALLObserverPlayerController::FindResidentActor(FGuid ResidentId) const
{
    if (!ResidentId.IsValid() || !GetWorld())
    {
        return nullptr;
    }
    for (TActorIterator<ALLResidentCharacter> It(GetWorld()); It; ++It)
    {
        if (It->GetResidentId() == ResidentId)
        {
            return *It;
        }
    }
    return nullptr;
}

void ALLObserverPlayerController::SuspendObservedResidentFollow()
{
    bFollowObservedResident = false;
}

void ALLObserverPlayerController::ResolveObservedResidentFocusFraming(
    const ALLResidentCharacter* Resident,
    FVector& OutTarget,
    float& OutDistance) const
{
    OutTarget = Resident ? Resident->GetActorLocation() : FVector::ZeroVector;
    OutDistance = ObservedResidentFocusDistanceUU;
    if (!Resident)
    {
        return;
    }

    FVector Origin = Resident->GetActorLocation();
    FVector Extent = FVector::ZeroVector;
    Resident->GetActorBounds(false, Origin, Extent, false);

    const float RenderedHeight = FMath::Max(1.0f, Extent.Z * 2.0f);
    const float ReferenceHeight = FMath::Max(10.0f, ObservedResidentReferenceHeightUU);
    const float SizeScale = FMath::Clamp(
        RenderedHeight / ReferenceHeight,
        FMath::Clamp(ObservedResidentFocusMinDistanceScale, 0.25f, 1.0f),
        1.15f);

    const float MinDistance = FMath::Max(100.0f, CameraMinDistanceUU);
    const float MaxDistance = FMath::Max(MinDistance, CameraMaxDistanceUU);
    OutDistance = FMath::Clamp(
        ObservedResidentFocusDistanceUU * SizeScale,
        MinDistance,
        MaxDistance);

    // Scale the vertical aim point with the resident instead of looking above
    // small children at the same fixed adult offset.
    const float AdaptiveHeightOffset = ObservedResidentFocusHeightOffsetUU < 0.0f
        ? ObservedResidentFocusHeightOffsetUU
        : FMath::Clamp(
            Extent.Z * 0.75f,
            FMath::Min(24.0f, ObservedResidentFocusHeightOffsetUU),
            ObservedResidentFocusHeightOffsetUU);

    FVector CameraRight = FVector::RightVector;
    if (const ACameraActor* Camera = ObserverCamera.Get())
    {
        CameraRight = Camera->GetActorRightVector();
        CameraRight.Z = 0.0f;
        CameraRight.Normalize();
    }

    const float LateralFraction = FMath::Clamp(ObservedResidentFocusLateralFraction, 0.0f, 0.35f);

    FVector MovementLead = Resident->GetVelocity();
    MovementLead.Z = 0.0f;
    MovementLead *= FMath::Clamp(ObservedResidentMovementLeadSeconds, 0.0f, 2.0f);
    MovementLead = MovementLead.GetClampedToMaxSize(
        FMath::Max(0.0f, ObservedResidentMaxMovementLeadUU));

    OutTarget = Origin
        + FVector(0.0f, 0.0f, AdaptiveHeightOffset)
        + CameraRight * (OutDistance * LateralFraction)
        + MovementLead;
}

void ALLObserverPlayerController::FocusWorldLocation(
    const FVector& WorldLocation,
    float FocusDistanceUU)
{
    // Preserve the currently selected resident in the observer UI, but suspend
    // camera follow so SyncObservedResidentSelection does not immediately pull
    // the camera away from the event location. Re-selecting/tapping a resident
    // resumes normal resident framing.
    SuspendObservedResidentFollow();
    bReturningToWorldOverview = false;
    bWorldEventFocusActive = true;

    UGameInstance* GameInstance = GetGameInstance();
    ULLObservationSubsystem* Observation =
        GameInstance ? GameInstance->GetSubsystem<ULLObservationSubsystem>() : nullptr;
    if (Observation && Observation->HasObservedResident())
    {
        FocusedResidentId = Observation->GetObservedResidentId();
    }

    DesiredOrbitTarget = WorldLocation + FVector(0.0f, 0.0f, 55.0f);
    const float MinDistance = FMath::Max(100.0f, CameraMinDistanceUU);
    const float MaxDistance = FMath::Max(MinDistance, CameraMaxDistanceUU);
    DesiredOrbitDistanceUU = FMath::Clamp(
        FocusDistanceUU,
        MinDistance,
        MaxDistance);

    const float MinElevation = FMath::Clamp(CameraMinElevationDegrees, 1.0f, 89.0f);
    const float MaxElevation = FMath::Clamp(CameraMaxElevationDegrees, MinElevation, 89.0f);
    DesiredOrbitElevationDegrees = FMath::Clamp(
        50.0f,
        MinElevation,
        MaxElevation);
}

void ALLObserverPlayerController::FocusObservedResident(ALLResidentCharacter* Resident, bool bReframe)
{
    if (!Resident)
    {
        return;
    }

    FocusedResidentId = Resident->GetResidentId();
    bFollowObservedResident = FocusedResidentId.IsValid();
    bReturningToWorldOverview = false;
    bWorldEventFocusActive = false;

    float FocusDistance = ObservedResidentFocusDistanceUU;
    ResolveObservedResidentFocusFraming(Resident, DesiredOrbitTarget, FocusDistance);

    if (bReframe)
    {
        DesiredOrbitDistanceUU = FocusDistance;

        const float MinElevation = FMath::Clamp(CameraMinElevationDegrees, 1.0f, 89.0f);
        const float MaxElevation = FMath::Clamp(CameraMaxElevationDegrees, MinElevation, 89.0f);
        DesiredOrbitElevationDegrees = FMath::Clamp(
            ObservedResidentFocusElevationDegrees,
            MinElevation,
            MaxElevation);
    }
}

void ALLObserverPlayerController::SyncObservedResidentSelection()
{
    UGameInstance* GameInstance = GetGameInstance();
    ULLObservationSubsystem* Observation =
        GameInstance ? GameInstance->GetSubsystem<ULLObservationSubsystem>() : nullptr;
    if (!Observation)
    {
        return;
    }

    ULLCoreBridgeSubsystem* Bridge =
        GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
    if (Bridge)
    {
        const int64 RuntimeGeneration = Bridge->GetRuntimeGeneration();
        if (RuntimeGeneration != LastObservedCoreRuntimeGeneration)
        {
            LastObservedCoreRuntimeGeneration = RuntimeGeneration;

            // A selection is meaningful only if that stable resident identity
            // exists in the newly installed authoritative runtime. Preserve it
            // across save loads when the resident still exists (including dead
            // residents shown by lifecycle/history UI), but do not leave the
            // observer stuck in Quick/Detail for an ID from a different world.
            if (Observation->HasObservedResident())
            {
                FLLCoreResidentObservation Resident;
                if (!Bridge->GetResidentObservation(
                        Observation->GetObservedResidentId(), Resident))
                {
                    Observation->ClearObservedResident();
                    RestoreWorldOverview();
                    return;
                }
            }
        }
    }

    // Selection can now originate outside this controller (lifecycle cards,
    // future family/history surfaces). Reframe only when the observed resident
    // actually changes so manual pan/orbit still suspends follow for the same
    // selected resident.
    if (!Observation->HasObservedResident())
    {
        if (FocusedResidentId.IsValid())
        {
            RestoreWorldOverview();
        }
        return;
    }

    const FGuid ObservedId = Observation->GetObservedResidentId();
    if (!ObservedId.IsValid() || ObservedId == FocusedResidentId)
    {
        return;
    }

    if (ALLResidentCharacter* Resident = FindResidentActor(ObservedId))
    {
        FocusObservedResident(Resident, true);
    }
}

void ALLObserverPlayerController::UpdateObservedResidentFocus(float DeltaTime)
{
    if (!bFollowObservedResident)
    {
        return;
    }

    UGameInstance* GameInstance = GetGameInstance();
    ULLObservationSubsystem* Observation =
        GameInstance ? GameInstance->GetSubsystem<ULLObservationSubsystem>() : nullptr;
    if (!Observation || !Observation->HasObservedResident())
    {
        bFollowObservedResident = false;
        FocusedResidentId.Invalidate();
        return;
    }

    const FGuid ObservedId = Observation->GetObservedResidentId();
    ALLResidentCharacter* Resident = FindResidentActor(ObservedId);
    if (!Resident)
    {
        bFollowObservedResident = false;
        FocusedResidentId.Invalidate();
        return;
    }

    FocusedResidentId = ObservedId;
    FVector ResolvedTarget = DesiredOrbitTarget;
    float FocusDistance = DesiredOrbitDistanceUU;
    ResolveObservedResidentFocusFraming(Resident, ResolvedTarget, FocusDistance);

    // Follow the authoritative resident position without feeding every tiny
    // velocity/sample change directly into the orbit target. This is camera
    // damping only; resident movement itself remains untouched.
    const float FollowSpeed = FMath::Max(0.1f, ObservedResidentFollowSmoothingSpeed);
    DesiredOrbitTarget = FMath::VInterpTo(
        DesiredOrbitTarget,
        ResolvedTarget,
        FMath::Max(0.0f, DeltaTime),
        FollowSpeed);
}

void ALLObserverPlayerController::RestoreWorldOverview()
{
    bFollowObservedResident = false;
    bWorldEventFocusActive = false;
    bReturningToWorldOverview = true;
    FocusedResidentId.Invalidate();

    if (!bWorldOverviewCaptured)
    {
        return;
    }

    DesiredOrbitTarget = WorldOverviewTarget;
    DesiredOrbitYawDegrees = WorldOverviewYawDegrees;
    DesiredOrbitElevationDegrees = WorldOverviewElevationDegrees;
    DesiredOrbitDistanceUU = WorldOverviewDistanceUU;
}

void ALLObserverPlayerController::ApplyCameraTransform(float DeltaTime)
{
    ACameraActor* Camera = ObserverCamera.Get();
    if (!Camera)
    {
        return;
    }

    const float EffectiveSmoothingSpeed = bReturningToWorldOverview
        ? OverviewTransitionSmoothingSpeed
        : ((bFollowObservedResident || bWorldEventFocusActive)
            ? FocusTransitionSmoothingSpeed
            : CameraSmoothingSpeed);
    const float Alpha = 1.0f - FMath::Exp(
        -FMath::Max(0.1f, EffectiveSmoothingSpeed)
        * FMath::Max(0.0f, DeltaTime));

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

    if (bReturningToWorldOverview)
    {
        const bool bTargetSettled =
            FVector::DistSquared(CurrentOrbitTarget, DesiredOrbitTarget) <= FMath::Square(2.0f);
        const bool bDistanceSettled =
            FMath::Abs(CurrentOrbitDistanceUU - DesiredOrbitDistanceUU) <= 2.0f;
        const bool bYawSettled =
            FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentOrbitYawDegrees, DesiredOrbitYawDegrees)) <= 0.15f;
        const bool bElevationSettled =
            FMath::Abs(CurrentOrbitElevationDegrees - DesiredOrbitElevationDegrees) <= 0.15f;
        if (bTargetSettled && bDistanceSettled && bYawSettled && bElevationSettled)
        {
            bReturningToWorldOverview = false;
        }
    }
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
        FocusObservedResident(Picked, true);
        return;
    }

    // 3. A facility/world-event focus is a temporary camera excursion, not
    // an observation-level change. The first empty tap returns to the selected
    // resident instead of accidentally closing that resident's inspector.
    if (bWorldEventFocusActive && Observation->HasObservedResident())
    {
        if (ALLResidentCharacter* Resident =
                FindResidentActor(Observation->GetObservedResidentId()))
        {
            FocusObservedResident(Resident, true);
            return;
        }
        bWorldEventFocusActive = false;
    }

    // 4. Empty space: one level back (LEVEL 2 -> 1 -> 0). Returning to
    // LEVEL 0 restores the captured production overview framing.
    Observation->StepBack();
    if (Observation->GetObservationLevel() == ELLObservationLevel::World)
    {
        RestoreWorldOverview();
    }
}
