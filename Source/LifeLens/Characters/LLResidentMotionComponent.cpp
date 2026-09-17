#include "Characters/LLResidentMotionComponent.h"
#include "Characters/LLResidentAppearanceComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "ReferenceSkeleton.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    static TAutoConsoleVariable<int32> CVarDebugMotion(
        TEXT("ll.DebugMotion"),
        0,
        TEXT("Log resident locomotion speed and context presentation state once per second."),
        ECVF_Default);

    constexpr float DebugLogInterval = 1.0f;

    FName ResolveRightHandBone(const USkeletalMeshComponent* Body)
    {
        if (!Body || !Body->GetSkeletalMeshAsset())
        {
            return NAME_None;
        }

        const FReferenceSkeleton& RefSkeleton = Body->GetSkeletalMeshAsset()->GetRefSkeleton();
        static const FName Candidates[] = {
            FName(TEXT("RightHand")),
            FName(TEXT("Hand_R")),
            FName(TEXT("hand_r")),
            FName(TEXT("R_Hand")),
            FName(TEXT("Hand.R"))
        };
        for (const FName Candidate : Candidates)
        {
            if (RefSkeleton.FindBoneIndex(Candidate) != INDEX_NONE)
            {
                return Candidate;
            }
        }
        return NAME_None;
    }
}

ULLResidentMotionComponent::ULLResidentMotionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    static ConstructorHelpers::FObjectFinder<UBlendSpace> LocomotionFinder(
        TEXT("/Game/Characters/Quaternius/UAL/BS_ResidentLocomotion.BS_ResidentLocomotion"));
    LocomotionBlendSpace = LocomotionFinder.Succeeded() ? LocomotionFinder.Object : nullptr;

    static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleFinder(
        TEXT("/Game/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Idle_Loop.Idle_Loop"));
    IdleAnimation = IdleFinder.Succeeded() ? IdleFinder.Object : nullptr;

    static ConstructorHelpers::FObjectFinder<UAnimSequence> TalkingFinder(
        TEXT("/Game/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Idle_Talking_Loop.Idle_Talking_Loop"));
    TalkingAnimation = TalkingFinder.Succeeded() ? TalkingFinder.Object : nullptr;

    static ConstructorHelpers::FObjectFinder<UAnimSequence> InteractFinder(
        TEXT("/Game/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Interact.Interact"));
    InteractAnimation = InteractFinder.Succeeded() ? InteractFinder.Object : nullptr;

    static ConstructorHelpers::FObjectFinder<UAnimSequence> GatherFinder(
        TEXT("/Game/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/PickUp_Table.PickUp_Table"));
    GatherAnimation = GatherFinder.Succeeded() ? GatherFinder.Object : nullptr;

    static ConstructorHelpers::FObjectFinder<UAnimSequence> DigFinder(
        TEXT("/Game/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Fixing_Kneeling.Fixing_Kneeling"));
    DigAnimation = DigFinder.Succeeded() ? DigFinder.Object : nullptr;

    // Quaternius has no dedicated primitive hammer clip. Sword_Attack is used
    // only as a repeated arm-swing fallback; no sword is spawned, and the
    // authoritative held StoneHammer presentation remains the visible tool.
    static ConstructorHelpers::FObjectFinder<UAnimSequence> StrikeFinder(
        TEXT("/Game/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Sword_Attack.Sword_Attack"));
    StrikeAnimation = StrikeFinder.Succeeded() ? StrikeFinder.Object : nullptr;

    static ConstructorHelpers::FObjectFinder<UAnimSequence> BuildFinder(
        TEXT("/Game/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Fixing_Kneeling.Fixing_Kneeling"));
    BuildAnimation = BuildFinder.Succeeded() ? BuildFinder.Object : nullptr;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> FlakeFinder(
        TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CuttingToolFinder(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ContainerFinder(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    SharpFlakeMesh = FlakeFinder.Succeeded() ? FlakeFinder.Object : nullptr;
    StoneCuttingToolMesh = CuttingToolFinder.Succeeded() ? CuttingToolFinder.Object : nullptr;
    SimpleContainerMesh = ContainerFinder.Succeeded() ? ContainerFinder.Object : nullptr;
}

void ULLResidentMotionComponent::BeginPlay()
{
    Super::BeginPlay();

    if (const AActor* Owner = GetOwner())
    {
        PreviousLocation = Owner->GetActorLocation();
        bHasPreviousLocation = true;
        SmoothedYaw = Owner->GetActorRotation().Yaw;
        DesiredYaw = SmoothedYaw;
    }
}

void ULLResidentMotionComponent::SetSocialInteractionActive(bool bActive)
{
    bSocialInteractionActive = bActive;
}

void ULLResidentMotionComponent::SetWorkPresentationMode(ELLResidentWorkPresentationMode Mode)
{
    WorkPresentationMode = Mode;
}

void ULLResidentMotionComponent::SetHeldToolPresentation(ELLResidentHeldToolPresentation Tool)
{
    HeldToolPresentation = Tool;
}

void ULLResidentMotionComponent::EnsureLocomotionPlaying()
{
    if (bSocialInteractionActive
        || WorkPresentationMode != ELLResidentWorkPresentationMode::None
        || ActiveContextAnimation
        || bLocomotionPlaying
        || !LocomotionBlendSpace)
    {
        return;
    }

    if (!Appearance)
    {
        if (AActor* Owner = GetOwner())
        {
            Appearance = Owner->FindComponentByClass<ULLResidentAppearanceComponent>();
        }
    }

    Body = Appearance ? Appearance->GetBodyComponent() : nullptr;
    if (!Body)
    {
        return;
    }

    TArray<FBlendSampleData> ResolvedSamples;
    int32 CachedTriangulationIndex = INDEX_NONE;
    const bool bResolved = LocomotionBlendSpace->GetSamplesFromBlendInput(
        FVector(0.0f, 0.0f, 0.0f), ResolvedSamples, CachedTriangulationIndex, true);
    if (!bResolved || ResolvedSamples.Num() == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("LLMotion %s locomotion=%s resolves no samples (%d authored); keeping the idle animation"),
            *GetOwner()->GetName(), *LocomotionBlendSpace->GetName(),
            LocomotionBlendSpace->GetBlendSamples().Num());
        LocomotionBlendSpace = nullptr;
        bLocomotionPlaying = true;
        return;
    }

    Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    Body->PlayAnimation(LocomotionBlendSpace, true);
    bLocomotionPlaying = true;

    UE_LOG(LogTemp, Log, TEXT("LLMotion %s locomotion=%s samples=%d resolved=%d"),
        *GetOwner()->GetName(), *LocomotionBlendSpace->GetName(),
        LocomotionBlendSpace->GetBlendSamples().Num(), ResolvedSamples.Num());
}

void ULLResidentMotionComponent::UpdateContextAnimationState()
{
    if (!Appearance)
    {
        if (AActor* Owner = GetOwner())
        {
            Appearance = Owner->FindComponentByClass<ULLResidentAppearanceComponent>();
        }
    }
    Body = Appearance ? Appearance->GetBodyComponent() : nullptr;
    if (!Body)
    {
        return;
    }

    UAnimSequence* DesiredAnimation = nullptr;
    if (bSocialInteractionActive)
    {
        DesiredAnimation = TalkingAnimation;
    }
    else
    {
        switch (WorkPresentationMode)
        {
            case ELLResidentWorkPresentationMode::Interact:
                DesiredAnimation = InteractAnimation;
                break;

            case ELLResidentWorkPresentationMode::Gather:
                // The Core/World contract already tells Presentation which real
                // tool is being used. Reuse that signal to select a closer body
                // motion without inventing a second action state.
                switch (HeldToolPresentation)
                {
                    case ELLResidentHeldToolPresentation::DiggingStick:
                        DesiredAnimation = DigAnimation ? DigAnimation : InteractAnimation;
                        break;
                    case ELLResidentHeldToolPresentation::StoneHammer:
                        DesiredAnimation = StrikeAnimation ? StrikeAnimation : InteractAnimation;
                        break;
                    case ELLResidentHeldToolPresentation::SimpleContainer:
                    case ELLResidentHeldToolPresentation::None:
                        DesiredAnimation = GatherAnimation ? GatherAnimation : InteractAnimation;
                        break;
                    case ELLResidentHeldToolPresentation::SharpFlake:
                    case ELLResidentHeldToolPresentation::StoneCuttingTool:
                    default:
                        DesiredAnimation = InteractAnimation;
                        break;
                }
                break;

            case ELLResidentWorkPresentationMode::Build:
                DesiredAnimation = BuildAnimation;
                break;

            case ELLResidentWorkPresentationMode::None:
            default:
                break;
        }
    }

    if (DesiredAnimation)
    {
        if (ActiveContextAnimation != DesiredAnimation)
        {
            Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
            Body->PlayAnimation(DesiredAnimation, true);
            ActiveContextAnimation = DesiredAnimation;
            bLocomotionPlaying = false;
        }
        return;
    }

    if (!ActiveContextAnimation)
    {
        return;
    }

    ActiveContextAnimation = nullptr;
    bLocomotionPlaying = false;

    // Restore a truthful stationary pose immediately. Locomotion may take
    // ownership again later in this same tick when no context presentation is active.
    if (IdleAnimation)
    {
        Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Body->PlayAnimation(IdleAnimation, true);
    }
}

void ULLResidentMotionComponent::UpdateHeldToolVisualState()
{
    if (HeldToolPresentation == ELLResidentHeldToolPresentation::None)
    {
        if (HeldToolMesh)
        {
            HeldToolMesh->SetVisibility(false, true);
        }
        return;
    }

    if (!Body)
    {
        return;
    }

    if (!HeldToolMesh)
    {
        AActor* Owner = GetOwner();
        if (!Owner)
        {
            return;
        }

        HeldToolMesh = NewObject<UStaticMeshComponent>(Owner, TEXT("HeldToolPresentationMesh"));
        HeldToolMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        HeldToolMesh->SetCastShadow(true);
        const FName HandBone = ResolveRightHandBone(Body);
        HeldToolMesh->SetupAttachment(Body, HandBone);
        HeldToolMesh->RegisterComponent();
        if (HandBone.IsNone())
        {
            // Safe visual fallback for an unexpected vendor skeleton. It keeps
            // the tool near the upper body instead of inventing another socket.
            HeldToolMesh->SetRelativeLocation(FVector(20.0f, -28.0f, 92.0f));
        }
    }

    UStaticMesh* DesiredMesh = nullptr;
    FVector RelativeScale(0.05f, 0.05f, 0.05f);
    FRotator RelativeRotation = FRotator::ZeroRotator;
    FVector RelativeLocation(2.0f, 0.0f, 0.0f);

    switch (HeldToolPresentation)
    {
        case ELLResidentHeldToolPresentation::SharpFlake:
            DesiredMesh = SharpFlakeMesh.Get();
            RelativeScale = FVector(0.035f, 0.055f, 0.025f);
            RelativeRotation = FRotator(0.0f, 90.0f, 90.0f);
            break;
        case ELLResidentHeldToolPresentation::StoneCuttingTool:
            DesiredMesh = StoneCuttingToolMesh.Get();
            RelativeScale = FVector(0.035f, 0.035f, 0.20f);
            RelativeRotation = FRotator(0.0f, 15.0f, 70.0f);
            break;
        case ELLResidentHeldToolPresentation::SimpleContainer:
            DesiredMesh = SimpleContainerMesh.Get();
            RelativeScale = FVector(0.09f, 0.09f, 0.12f);
            RelativeLocation = FVector(5.0f, 0.0f, -3.0f);
            break;
        case ELLResidentHeldToolPresentation::DiggingStick:
            DesiredMesh = StoneCuttingToolMesh.Get();
            RelativeScale = FVector(0.022f, 0.022f, 0.28f);
            RelativeRotation = FRotator(0.0f, 8.0f, 82.0f);
            RelativeLocation = FVector(4.0f, 0.0f, -4.0f);
            break;
        case ELLResidentHeldToolPresentation::StoneHammer:
            DesiredMesh = StoneCuttingToolMesh.Get();
            RelativeScale = FVector(0.065f, 0.045f, 0.16f);
            RelativeRotation = FRotator(0.0f, 24.0f, 68.0f);
            RelativeLocation = FVector(3.0f, 0.0f, 1.0f);
            break;
        case ELLResidentHeldToolPresentation::None:
        default:
            break;
    }

    if (!DesiredMesh)
    {
        HeldToolMesh->SetVisibility(false, true);
        return;
    }

    HeldToolMesh->SetStaticMesh(DesiredMesh);
    HeldToolMesh->SetRelativeScale3D(RelativeScale);
    HeldToolMesh->SetRelativeRotation(RelativeRotation);
    if (!HeldToolMesh->GetAttachSocketName().IsNone())
    {
        HeldToolMesh->SetRelativeLocation(RelativeLocation);
    }
    HeldToolMesh->SetVisibility(true, true);
}

void ULLResidentMotionComponent::UpdateBodyOrientation(float DeltaTime)
{
    if (!Body)
    {
        return;
    }

    const AActor* Owner = GetOwner();
    const float OwnerYaw = Owner ? Owner->GetActorRotation().Yaw : SmoothedYaw;
    const float TargetYaw = SmoothedSpeed > 0.0f ? DesiredYaw : OwnerYaw;

    SmoothedYaw = FMath::FInterpTo(
        SmoothedYaw,
        SmoothedYaw + FMath::FindDeltaAngleDegrees(SmoothedYaw, TargetYaw),
        DeltaTime,
        YawInterpSpeed);

    Body->SetWorldRotation(FRotator(0.0f, SmoothedYaw + MeshForwardYawOffsetDegrees, 0.0f));
}

void ULLResidentMotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UpdateContextAnimationState();
    UpdateHeldToolVisualState();
    EnsureLocomotionPlaying();

    const AActor* Owner = GetOwner();
    if (!Owner || DeltaTime <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const FVector Location = Owner->GetActorLocation();
    FVector Delta = Location - PreviousLocation;
    Delta.Z = 0.0f;
    PreviousLocation = Location;

    if (!bHasPreviousLocation)
    {
        bHasPreviousLocation = true;
        return;
    }

    const float Step = Delta.Size2D();
    if (Step > TeleportStep)
    {
        WindowDistance = 0.0f;
        WindowSeconds = 0.0f;
        WindowedSpeed = 0.0f;
        SmoothedSpeed = 0.0f;
    }
    else
    {
        if (Step > KINDA_SMALL_NUMBER)
        {
            DesiredYaw = Delta.Rotation().Yaw;
        }

        WindowDistance += Step;
        WindowSeconds += DeltaTime;
        if (WindowSeconds >= SpeedWindowSeconds)
        {
            WindowedSpeed = WindowDistance / WindowSeconds;
            WindowDistance = 0.0f;
            WindowSeconds = 0.0f;
        }
    }

    SmoothedSpeed = FMath::FInterpTo(SmoothedSpeed, WindowedSpeed, DeltaTime, SpeedInterpSpeed);
    if (SmoothedSpeed < IdleSpeedThreshold)
    {
        SmoothedSpeed = 0.0f;
    }

    UpdateBodyOrientation(DeltaTime);

    if (!ActiveContextAnimation && Body && LocomotionBlendSpace)
    {
        if (UAnimSingleNodeInstance* SingleNode = Body->GetSingleNodeInstance())
        {
            SingleNode->SetBlendSpacePosition(
                FVector(FMath::Clamp(SmoothedSpeed, 0.0f, MaxSpeed), 0.0f, 0.0f));
        }
    }

    if (CVarDebugMotion.GetValueOnGameThread() != 0)
    {
        DebugLogTimer -= DeltaTime;
        if (DebugLogTimer <= 0.0f)
        {
            DebugLogTimer = DebugLogInterval;
            float FacingDot = 0.0f;
            if (Body && SmoothedSpeed > 0.0f)
            {
                const FVector TravelDirection = FRotator(0.0f, DesiredYaw, 0.0f).Vector();
                FacingDot = FVector::DotProduct(Body->GetRightVector(), TravelDirection);
            }

            UE_LOG(LogTemp, Log, TEXT("LLMotion %s speed=%.1f window=%.1f yaw=%.1f visual=%.1f travel=%.1f actor=%.1f facing=%.2f locomotion=%d talking=%d work=%d tool=%d context=%s loc=%.0f,%.0f"),
                *Owner->GetName(), SmoothedSpeed, WindowedSpeed, SmoothedYaw,
                SmoothedYaw + MeshForwardYawOffsetDegrees, DesiredYaw,
                Owner->GetActorRotation().Yaw, FacingDot,
                bLocomotionPlaying ? 1 : 0,
                bSocialInteractionActive ? 1 : 0,
                static_cast<int32>(WorkPresentationMode),
                static_cast<int32>(HeldToolPresentation),
                ActiveContextAnimation ? *ActiveContextAnimation->GetName() : TEXT("none"),
                Location.X, Location.Y);
        }
    }
}
