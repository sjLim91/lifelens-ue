#include "Characters/LLResidentMotionComponent.h"
#include "Characters/LLResidentAppearanceComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    // Mirrors ll.DebugTapTargets: off by default, one line per resident per
    // second while enabled.
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

    // The body is built once the resident identity is bound, which happens
    // after BeginPlay, so this keeps retrying until it exists.
    Body = Appearance ? Appearance->GetBodyComponent() : nullptr;
    if (!Body)
    {
        return;
    }

    // A blend space that carries sample data but no runtime triangulation
    // resolves zero samples for every input, and the mesh then shows the
    // reference (T) pose instead of any animation. Leave the idle animation
    // that the appearance component started rather than replacing it with a
    // blend space that cannot produce a pose.
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
        bLocomotionPlaying = true;   // do not retry every frame
        return;
    }

    Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    Body->PlayAnimation(LocomotionBlendSpace, true);
    bLocomotionPlaying = true;

    UE_LOG(LogTemp, Log, TEXT("LLMotion %s locomotion=%s samples=%d resolved=%d"),
        *GetOwner()->GetName(), *LocomotionBlendSpace->GetName(),
        LocomotionBlendSpace->GetBlendSamples().Num(), ResolvedSamples.Num());
}

void ULLResidentMotionComponent::UpdateBodyOrientation(float DeltaTime)
{
    if (!Body)
    {
        return;
    }

    // Presentation-side turning only, and only while the resident is actually
    // travelling. A stationary resident is turned by the world director toward
    // its use point or social target, so the body follows the actor rotation
    // then; holding the last travel heading would make residents interact while
    // visibly facing away.
    const AActor* Owner = GetOwner();
    const float OwnerYaw = Owner ? Owner->GetActorRotation().Yaw : SmoothedYaw;
    const float TargetYaw = SmoothedSpeed > 0.0f ? DesiredYaw : OwnerYaw;

    SmoothedYaw = FMath::FInterpTo(
        SmoothedYaw,
        SmoothedYaw + FMath::FindDeltaAngleDegrees(SmoothedYaw, TargetYaw),
        DeltaTime,
        YawInterpSpeed);

    // Quaternius UBC's authored forward axis imports as local -Y. The actor
    // and simulation continue to use normal Unreal +X forward; only the visual
    // body receives this asset-axis correction.
    Body->SetWorldRotation(FRotator(0.0f, SmoothedYaw + MeshForwardYawOffsetDegrees, 0.0f));
}

void ULLResidentMotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

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

    // Loading a save or restoring a runtime grid position moves the actor in
    // one step; that is not locomotion. The step is dropped entirely so it
    // never reaches the speed window, which would otherwise report a burst of
    // sprint speed when the window closes.
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

    if (Body && LocomotionBlendSpace)
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
            // `facing` is the decisive number: the mesh faces its own local
            // +Y, which is the component right vector, so projecting it onto
            // the travel direction gives +1 when the character walks forwards
            // and -1 when it walks backwards.
            float FacingDot = 0.0f;
            if (Body && SmoothedSpeed > 0.0f)
            {
                const FVector TravelDirection = FRotator(0.0f, DesiredYaw, 0.0f).Vector();
                FacingDot = FVector::DotProduct(Body->GetRightVector(), TravelDirection);
            }

            UE_LOG(LogTemp, Log, TEXT("LLMotion %s speed=%.1f window=%.1f yaw=%.1f visual=%.1f travel=%.1f actor=%.1f facing=%.2f playing=%d loc=%.0f,%.0f"),
                *Owner->GetName(), SmoothedSpeed, WindowedSpeed, SmoothedYaw,
                SmoothedYaw + MeshForwardYawOffsetDegrees, DesiredYaw,
                Owner->GetActorRotation().Yaw, FacingDot,
                bLocomotionPlaying ? 1 : 0, Location.X, Location.Y);
        }
    }
}
