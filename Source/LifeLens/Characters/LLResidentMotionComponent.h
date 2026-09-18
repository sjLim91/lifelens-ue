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
    SimpleContainer,
    DiggingStick,
    StoneHammer
};

// Character Context Motion v2 presentation vocabulary.
//
// This is a *presentation* classification, not a second action authority. Each
// entry is chosen from the authoritative `FLLCoreActionDirective` and maps to
// one already-imported Quaternius UAL clip. Core decides what a resident does;
// this enum only decides what that already-decided work looks like.
UENUM(BlueprintType)
enum class ELLResidentContextMotion : uint8
{
    None,
    Talk,           // Idle_Talking_Loop  - social exchange, teaching, instruction
    Learn,          // Interact           - learner side of a teaching exchange
    GatherPick,     // GatherAnimation    - gathering, storing, carrying into a store
    StrikeSwing,    // StrikeAnimation    - Chop/Cut/Strike tool capability
    DigWork,        // DigAnimation       - Dig capability
    CraftWork,      // BuildAnimation     - craft, experiment, facility work, smelt charge
    HaulPush,       // Push_Loop          - material delivery
    FireTend,       // Idle_Torch_Loop    - ignite, fuel, charcoal collection
    CrouchLow,      // Crouch_Idle_Loop   - sanitation site use, hygiene
    SeatedCare      // Sitting_* sequence - parenting care and comfort
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
    void UpdateContextAnimationState(float DeltaTime);
    void UpdateHeldToolVisualState();

    // PR #143's WorldDirector-signal clip selection, kept intact. It decides
    // whether a resident is presenting context work at all, and stays the
    // result whenever the authoritative directive cannot refine it.
    UAnimSequence* LegacyContextAnimation() const;
    // Reads the authoritative directive for this resident and classifies it.
    // Read-only: nothing here starts, completes or mutates a Core action.
    ELLResidentContextMotion ResolveContextMotion() const;
    UAnimSequence* ClipForContextMotion(ELLResidentContextMotion Motion) const;
    // ll.DebugMotion diagnostics: reports which bridge read succeeded and what
    // the authoritative directives actually contain, so a wrong classification
    // can be told apart from an empty read.
    void LogDirectiveDiagnostics() const;

    UPROPERTY(EditAnywhere, Category="LifeLens|Motion")
    float MeshForwardYawOffsetDegrees = -90.0f;

    UPROPERTY() TObjectPtr<UBlendSpace> LocomotionBlendSpace;
    UPROPERTY() TObjectPtr<UAnimSequence> IdleAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> TalkingAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> InteractAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> GatherAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> DigAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> StrikeAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> BuildAnimation;

    // Context Motion v2 clips. All of them are already-imported Quaternius UAL
    // sequences; no new animation asset is introduced by this milestone.
    // Gather/Dig/Strike are the clips #143 already selected and are reused
    // as-is rather than duplicated under new names.
    UPROPERTY() TObjectPtr<UAnimSequence> HaulAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> FireAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> CrouchAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> SeatedEnterAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> SeatedCareAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> SeatedExitAnimation;

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

    // Seated care is the only motion with authored enter/exit clips, so it is
    // the only one that needs a transition state machine. Everything else is a
    // single looping clip swap.
    ELLResidentContextMotion ActiveContextMotion = ELLResidentContextMotion::None;
    float SeatedTransitionRemaining = 0.0f;
    bool bSeatedEntered = false;
    float DebugLogTimer = 0.0f;
};
