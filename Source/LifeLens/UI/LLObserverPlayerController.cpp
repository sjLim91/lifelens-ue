#include "UI/LLObserverPlayerController.h"
#include "UI/LLObservationSubsystem.h"
#include "UI/LLObserverHUD.h"
#include "Characters/LLResidentCharacter.h"
#include "Core/LLLifeLensGameMode.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Camera/CameraActor.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"

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

    bTouch1Tracked = false;
    bTouchGesture = false;
    bTouchHadSecondFinger = false;
    bTwoFingerActive = false;

    if (bTap)
    {
        // Near-resident screen-space picking in ApplyTap is authoritative for
        // touch. ExactHit is optional and deliberately omitted on release.
        ApplyTap(ReleasePosition, nullptr);
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
