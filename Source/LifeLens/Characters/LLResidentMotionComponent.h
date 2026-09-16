#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LLResidentMotionComponent.generated.h"

class UAnimSequence;
class UBlendSpace;
class ULLResidentAppearanceComponent;
class USkeletalMeshComponent;

// Character Motion Bootstrap: locomotion + lightweight context presentation.
// Core/World keep all action authority; this component only reflects the
// resident actor's actual movement and active social interaction visually.
UCLASS(ClassGroup=(LifeLens), meta=(BlueprintSpawnableComponent))
class LIFELENS_API ULLResidentMotionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULLResidentMotionComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintPure, Category="LifeLens|Motion")
    float GetGroundSpeed() const { return SmoothedSpeed; }

    // Presentation-only signal from WorldDirector. It never starts/completes a
    // Core action and is cleared whenever the authoritative context action is
    // no longer being performed at its target.
    UFUNCTION(BlueprintCallable, Category="LifeLens|Motion")
    void SetSocialInteractionActive(bool bActive);

    UFUNCTION(BlueprintPure, Category="LifeLens|Motion")
    bool IsSocialInteractionActive() const { return bSocialInteractionActive; }

    static constexpr float IdleSpeedThreshold = 5.0f;
    static constexpr float MaxSpeed = 600.0f;
    static constexpr float SpeedWindowSeconds = 0.2f;
    static constexpr float SpeedInterpSpeed = 6.0f;
    static constexpr float YawInterpSpeed = 8.0f;
    static constexpr float TeleportStep = 400.0f;

private:
    void EnsureLocomotionPlaying();
    void UpdateBodyOrientation(float DeltaTime);
    void UpdateSocialAnimationState();

    UPROPERTY(EditAnywhere, Category="LifeLens|Motion")
    float MeshForwardYawOffsetDegrees = -90.0f;

    UPROPERTY() TObjectPtr<UBlendSpace> LocomotionBlendSpace;
    UPROPERTY() TObjectPtr<UAnimSequence> IdleAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> TalkingAnimation;

    UPROPERTY() TObjectPtr<ULLResidentAppearanceComponent> Appearance;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Body;

    FVector PreviousLocation = FVector::ZeroVector;
    float WindowDistance = 0.0f;
    float WindowSeconds = 0.0f;
    float WindowedSpeed = 0.0f;
    float SmoothedSpeed = 0.0f;
    float DesiredYaw = 0.0f;
    float SmoothedYaw = 0.0f;
    bool bHasPreviousLocation = false;
    bool bLocomotionPlaying = false;
    bool bSocialInteractionActive = false;
    bool bSocialAnimationPlaying = false;
    float DebugLogTimer = 0.0f;
};
