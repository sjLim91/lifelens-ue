#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LLResidentMotionComponent.generated.h"

class UBlendSpace;
class ULLResidentAppearanceComponent;
class USkeletalMeshComponent;

// Character Motion Bootstrap: the first locomotion slice of Motion & Context v1.
//
// Presentation only. Core/World keep every movement and action decision; this
// component measures how fast the resident actor is actually moving and
// reflects it in the body animation, replacing the "Idle pose while sliding"
// presentation. It never sets a movement target, never picks an action and
// never writes simulation state.
UCLASS(ClassGroup=(LifeLens), meta=(BlueprintSpawnableComponent))
class LIFELENS_API ULLResidentMotionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULLResidentMotionComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Smoothed ground speed in cm/s, as fed to the locomotion blend space.
    UFUNCTION(BlueprintPure, Category="LifeLens|Motion")
    float GetGroundSpeed() const { return SmoothedSpeed; }

    // ---- Tuning ---------------------------------------------------------------
    // Below this the resident is treated as standing still.
    static constexpr float IdleSpeedThreshold = 5.0f;
    // Blend space axis range; matches BS_ResidentLocomotion.
    static constexpr float MaxSpeed = 600.0f;
    // Speed is measured over a short window: the resident actor is moved by
    // the world director, which does not produce a step on every frame, so a
    // per-frame sample alternates between full speed and zero.
    static constexpr float SpeedWindowSeconds = 0.2f;
    // Smoothing of the windowed speed, in 1/seconds.
    static constexpr float SpeedInterpSpeed = 6.0f;
    // Orientation smoothing, in degrees per second of interpolation strength.
    static constexpr float YawInterpSpeed = 8.0f;
    // A single frame step longer than this is a teleport (load, grid restore),
    // not locomotion.
    static constexpr float TeleportStep = 400.0f;

private:
    void EnsureLocomotionPlaying();
    void UpdateBodyOrientation(float DeltaTime);

    // Quaternius UBC imports with visual forward along local -Y, while LifeLens
    // actor/world forward is Unreal +X. Keep the asset-axis correction in the
    // presentation component; actor yaw and Core/World movement stay untouched.
    UPROPERTY(EditAnywhere, Category="LifeLens|Motion")
    float MeshForwardYawOffsetDegrees = 90.0f;

    // Catalogue (referenced in the constructor so it is cooked).
    UPROPERTY() TObjectPtr<UBlendSpace> LocomotionBlendSpace;

    UPROPERTY() TObjectPtr<ULLResidentAppearanceComponent> Appearance;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Body;

    FVector PreviousLocation = FVector::ZeroVector;
    float WindowDistance = 0.0f;
    float WindowSeconds = 0.0f;
    float WindowedSpeed = 0.0f;
    float SmoothedSpeed = 0.0f;
    // Last direction the resident actually travelled in; kept across frames
    // without movement so the turn keeps easing instead of stalling.
    float DesiredYaw = 0.0f;
    float SmoothedYaw = 0.0f;
    bool bHasPreviousLocation = false;
    bool bLocomotionPlaying = false;
    float DebugLogTimer = 0.0f;
};
