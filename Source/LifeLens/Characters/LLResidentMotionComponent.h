#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LLResidentMotionComponent.generated.h"

class UAnimSequence;
class UBlendSpace;
class ULLResidentAppearanceComponent;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ELLResidentWorkPresentationMode : uint8
{
    None,
    Interact,
    Gather,
    Build
};

UENUM(BlueprintType)
enum class ELLResidentHeldToolPresentation : uint8
{
    None,
    SharpFlake,
    StoneCuttingTool,
    SimpleContainer
};

// Character Motion Bootstrap: locomotion + lightweight context presentation.
// Core/World keep all action authority; this component only reflects the
// resident actor's actual movement and active authoritative context work.
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

    // Presentation-only signals from WorldDirector. They never start/complete a
    // Core action and are cleared whenever the authoritative context action is
    // not actively being performed at its target.
    UFUNCTION(BlueprintCallable, Category="LifeLens|Motion")
    void SetSocialInteractionActive(bool bActive);

    UFUNCTION(BlueprintCallable, Category="LifeLens|Motion")
    void SetWorkPresentationMode(ELLResidentWorkPresentationMode Mode);

    UFUNCTION(BlueprintCallable, Category="LifeLens|Motion")
    void SetHeldToolPresentation(ELLResidentHeldToolPresentation Tool);

    UFUNCTION(BlueprintPure, Category="LifeLens|Motion")
    bool IsSocialInteractionActive() const { return bSocialInteractionActive; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Motion")
    ELLResidentWorkPresentationMode GetWorkPresentationMode() const { return WorkPresentationMode; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Motion")
    ELLResidentHeldToolPresentation GetHeldToolPresentation() const { return HeldToolPresentation; }

    static constexpr float IdleSpeedThreshold = 5.0f;
    static constexpr float MaxSpeed = 600.0f;
    static constexpr float SpeedWindowSeconds = 0.2f;
    static constexpr float SpeedInterpSpeed = 6.0f;
    static constexpr float YawInterpSpeed = 8.0f;
    static constexpr float TeleportStep = 400.0f;

private:
    void EnsureLocomotionPlaying();
    void UpdateBodyOrientation(float DeltaTime);
    void UpdateContextAnimationState();
    void UpdateHeldToolVisualState();

    UPROPERTY(EditAnywhere, Category="LifeLens|Motion")
    float MeshForwardYawOffsetDegrees = -90.0f;

    UPROPERTY() TObjectPtr<UBlendSpace> LocomotionBlendSpace;
    UPROPERTY() TObjectPtr<UAnimSequence> IdleAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> TalkingAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> InteractAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> BuildAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> ActiveContextAnimation;

    // Tool meshes are intentionally simple presentation proxies. The enum and
    // attachment contract stay stable when proper art assets replace them.
    UPROPERTY() TObjectPtr<UStaticMesh> SharpFlakeMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> StoneCuttingToolMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> SimpleContainerMesh;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> HeldToolMesh;

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
    ELLResidentWorkPresentationMode WorkPresentationMode = ELLResidentWorkPresentationMode::None;
    ELLResidentHeldToolPresentation HeldToolPresentation = ELLResidentHeldToolPresentation::None;
    float DebugLogTimer = 0.0f;
};
