#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "GameFramework/PlayerController.h"
#include "LLObserverPlayerController.generated.h"

class ACameraActor;
class ALLResidentCharacter;

// Routes observer selection and camera input.
//
// Selection:
//   1. HUD chrome (quick inspector card, detail tabs) - consumed there.
//   2. The resident whose projected screen position is nearest the tap/click,
//      within the touch-target radius - select it.
//   3. Otherwise - step one observation level back.
//
// Camera (docs/OBSERVER_CAMERA_CONTROL_v1.md):
//   PC: wheel zoom, right-drag orbit, middle-drag pan.
//   Android: tap selects, one-finger drag orbits, pinch zooms, two-finger drag pans.
//   LEVEL 2 detail content consumes wheel / one-finger drag before camera input.
UCLASS(Config=Game, DefaultConfig)
class LIFELENS_API ALLObserverPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ALLObserverPlayerController();

    // Presentation surfaces such as lifecycle/facility event cards can move
    // the observer to an authoritative world location without inventing a
    // resident selection or mutating simulation state.
    void FocusWorldLocation(const FVector& WorldLocation, float FocusDistanceUU = 4200.0f);

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaTime) override;

private:
    // Selection input.
    void HandlePrimarySelect();
    void HandleTouchPressed(ETouchIndex::Type FingerIndex, FVector Location);
    void HandleTouchReleased(ETouchIndex::Type FingerIndex, FVector Location);

    // Mouse camera input.
    void HandleRightMousePressed();
    void HandleRightMouseReleased();
    void HandleMiddleMousePressed();
    void HandleMiddleMouseReleased();
    void HandleMouseWheel(float AxisValue);

    // Camera runtime.
    void EnsureCameraInitialized();
    void UpdateMouseCameraInput();
    void UpdateTouchCameraInput();
    void ApplyCameraTransform(float DeltaTime);
    void SyncObservedResidentSelection();
    void UpdateObservedResidentFocus(float DeltaTime);
    void FocusObservedResident(ALLResidentCharacter* Resident, bool bReframe);
    void ResolveObservedResidentFocusFraming(const ALLResidentCharacter* Resident, FVector& OutTarget, float& OutDistance) const;
    void RestoreWorldOverview();
    void SuspendObservedResidentFollow();
    ALLResidentCharacter* FindResidentActor(FGuid ResidentId) const;
    void RotateByScreenDelta(const FVector2D& Delta, float DegreesPerPixel);
    void PanByScreenDelta(const FVector2D& Delta, float ScaleMultiplier = 1.0f);
    void ZoomByScale(float Scale);
    float TouchDragThresholdPixels() const;

    // ExactHit: the resident directly under the pointer (trace), used when no
    // resident is within the touch-target radius.
    void ApplyTap(const FVector2D& ScreenPosition, ALLResidentCharacter* ExactHit);

    // Resident whose projected render bounds, grown by RadiusPixels, contain
    // the tap; the closest bounds rectangle wins. OutDistance is the distance
    // from the tap to the nearest resident's bounds rectangle in pixels
    // (0 when inside), or -1 when no resident projects on screen.
    ALLResidentCharacter* FindResidentAtScreenPosition(const FVector2D& ScreenPosition, float RadiusPixels, float& OutDistance) const;

    // Product tuning. Config/DefaultGame.ini is the normal source of truth.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera", meta=(ClampMin="100.0"))
    float CameraMinDistanceUU = 1000.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera", meta=(ClampMin="1000.0"))
    float CameraMaxDistanceUU = 24000.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera", meta=(ClampMin="1.0", ClampMax="89.0"))
    float CameraMinElevationDegrees = 20.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera", meta=(ClampMin="1.0", ClampMax="89.0"))
    float CameraMaxElevationDegrees = 80.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera", meta=(ClampMin="0.01"))
    float MouseRotateDegreesPerPixel = 0.18f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera", meta=(ClampMin="0.01"))
    float TouchRotateDegreesPerPixel = 0.16f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera", meta=(ClampMin="0.0001"))
    float PanWorldPerPixelDistanceFactor = 0.0015f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera", meta=(ClampMin="0.01", ClampMax="0.8"))
    float MouseWheelZoomFraction = 0.12f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera", meta=(ClampMin="0.1", ClampMax="3.0"))
    float PinchZoomSensitivity = 1.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera", meta=(ClampMin="1.0"))
    float TouchDragThresholdLogicalPixels = 12.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera", meta=(ClampMin="0.1"))
    float CameraSmoothingSpeed = 10.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera", meta=(ClampMin="0.1"))
    float FocusTransitionSmoothingSpeed = 6.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera", meta=(ClampMin="0.1"))
    float OverviewTransitionSmoothingSpeed = 5.5f;

    // Selecting a resident transitions into a closer observer framing and
    // follows that resident until the observer manually manipulates the camera.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera|Focus", meta=(ClampMin="500.0"))
    float ObservedResidentFocusDistanceUU = 3200.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera|Focus", meta=(ClampMin="1.0", ClampMax="89.0"))
    float ObservedResidentFocusElevationDegrees = 42.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera|Focus", meta=(ClampMin="-500.0", ClampMax="1000.0"))
    float ObservedResidentFocusHeightOffsetUU = 70.0f;

    // Adult-sized rendered height used to normalize focus distance. Smaller
    // residents automatically frame closer while large residents stay near the
    // established 3200 UU composition.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera|Focus", meta=(ClampMin="10.0"))
    float ObservedResidentReferenceHeightUU = 180.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera|Focus", meta=(ClampMin="0.25", ClampMax="1.0"))
    float ObservedResidentFocusMinDistanceScale = 0.68f;

    // Shift the camera target slightly to screen-right so the resident appears
    // left-of-centre, leaving breathing room for the right-side observer card.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera|Focus", meta=(ClampMin="0.0", ClampMax="0.35"))
    float ObservedResidentFocusLateralFraction = 0.12f;

    // Look slightly ahead of a moving observed resident so travel direction is
    // visible instead of pinning the resident to a static composition point.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera|Focus", meta=(ClampMin="0.0", ClampMax="2.0"))
    float ObservedResidentMovementLeadSeconds = 0.45f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera|Focus", meta=(ClampMin="0.0", ClampMax="1000.0"))
    float ObservedResidentMaxMovementLeadUU = 180.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera|Focus", meta=(ClampMin="0.1", ClampMax="30.0"))
    float ObservedResidentFollowSmoothingSpeed = 6.5f;

    TWeakObjectPtr<ACameraActor> ObserverCamera;
    bool bCameraInitialized = false;

    FVector CurrentOrbitTarget = FVector::ZeroVector;
    FVector DesiredOrbitTarget = FVector::ZeroVector;
    float CurrentOrbitYawDegrees = 0.0f;
    float DesiredOrbitYawDegrees = 0.0f;
    float CurrentOrbitElevationDegrees = 45.0f;
    float DesiredOrbitElevationDegrees = 45.0f;
    float CurrentOrbitDistanceUU = 5000.0f;
    float DesiredOrbitDistanceUU = 5000.0f;

    // Initial overview framing is captured once from the production camera so
    // LEVEL 1 -> LEVEL 0 can smoothly return without inventing a hard-coded
    // world target.
    FVector WorldOverviewTarget = FVector::ZeroVector;
    float WorldOverviewYawDegrees = 0.0f;
    float WorldOverviewElevationDegrees = 45.0f;
    float WorldOverviewDistanceUU = 5000.0f;
    bool bWorldOverviewCaptured = false;

    FGuid FocusedResidentId;
    int64 LastObservedCoreRuntimeGeneration = -1;
    bool bFollowObservedResident = false;
    bool bWorldEventFocusActive = false;
    bool bReturningToWorldOverview = false;

    bool bRightMouseDragging = false;
    bool bMiddleMouseDragging = false;

    bool bTouch1Tracked = false;
    bool bTouchGesture = false;
    bool bTouchHadSecondFinger = false;
    bool bTwoFingerActive = false;
    bool bHUDDetailScrollTouchActive = false;
    FVector2D TouchStart1 = FVector2D::ZeroVector;
    FVector2D LastTouch1 = FVector2D::ZeroVector;
    FVector2D LastTouch2 = FVector2D::ZeroVector;
};
