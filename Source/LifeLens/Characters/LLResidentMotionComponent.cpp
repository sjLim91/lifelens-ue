#include "Characters/LLResidentMotionComponent.h"
#include "Characters/LLResidentAppearanceComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    static TAutoConsoleVariable<int32> CVarDebugMotion(
        TEXT("ll.DebugMotion"),
        0,
        TEXT("Log resident locomotion speed and blend space state once per second."),
        ECVF_Default);

    constexpr float DebugLogInterval = 1.0f;
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

void ULLResidentMotionComponent::EnsureLocomotionPlaying()
{
    if (bLocomotionPlaying || !LocomotionBlendSpace)
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

void ULLResidentMotionComponent::UpdateSocialAnimationState()
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

    if (bSocialInteractionActive)
    {
        if (!bSocialAnimationPlaying && TalkingAnimation)
        {
            Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
            Body->PlayAnimation(TalkingAnimation, true);
            bSocialAnimationPlaying = true;
        }
        return;
    }

    if (!bSocialAnimationPlaying)
    {
        return;
    }

    bSocialAnimationPlaying = false;
    bLocomotionPlaying = false;

    // Restore a truthful stationary pose immediately. If the authored
    // locomotion blend space is valid it will take ownership again on the next
    // tick; if it was intentionally disabled as a fallback, this idle remains.
    if (IdleAnimation)
    {
        Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Body->PlayAnimation(IdleAnimation, true);
    }
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

    EnsureLocomotionPlaying();
    UpdateSocialAnimationState();

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

    if (!bSocialAnimationPlaying && Body && LocomotionBlendSpace)
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

            UE_LOG(LogTemp, Log, TEXT("LLMotion %s speed=%.1f window=%.1f yaw=%.1f visual=%.1f travel=%.1f actor=%.1f facing=%.2f locomotion=%d talking=%d loc=%.0f,%.0f"),
                *Owner->GetName(), SmoothedSpeed, WindowedSpeed, SmoothedYaw,
                SmoothedYaw + MeshForwardYawOffsetDegrees, DesiredYaw,
                Owner->GetActorRotation().Yaw, FacingDot,
                bLocomotionPlaying ? 1 : 0, bSocialAnimationPlaying ? 1 : 0,
                Location.X, Location.Y);
        }
    }
}
