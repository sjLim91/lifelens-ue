#include "Characters/LLResidentMotionComponent.h"
#include "Characters/LLResidentAppearanceComponent.h"
#include "Characters/LLResidentCharacter.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ReferenceSkeleton.h"
#include "Simulation/LLCoreActionTypes.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "World/LLWorldDirector.h"
#include "EngineUtils.h"
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

    // Context Motion v2 adds the clips the tool-based route above has no entry
    // for: hauling, fire tending, low crouched work and seated care. Gather,
    // Dig and Strike stay on the clips #143 already chose.
    //
    // Survey evidence (Content/Characters/Quaternius/Import/survey_animations.py):
    // none of the 43 imported UAL sequences carries root motion, so looping a
    // work clip never fights the authoritative Core/World movement.
    static ConstructorHelpers::FObjectFinder<UAnimSequence> HaulFinder(
        TEXT("/Game/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Push_Loop.Push_Loop"));
    HaulAnimation = HaulFinder.Succeeded() ? HaulFinder.Object : nullptr;

    static ConstructorHelpers::FObjectFinder<UAnimSequence> FireFinder(
        TEXT("/Game/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Idle_Torch_Loop.Idle_Torch_Loop"));
    FireAnimation = FireFinder.Succeeded() ? FireFinder.Object : nullptr;

    static ConstructorHelpers::FObjectFinder<UAnimSequence> CrouchFinder(
        TEXT("/Game/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Crouch_Idle_Loop.Crouch_Idle_Loop"));
    CrouchAnimation = CrouchFinder.Succeeded() ? CrouchFinder.Object : nullptr;

    static ConstructorHelpers::FObjectFinder<UAnimSequence> SeatedEnterFinder(
        TEXT("/Game/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Sitting_Enter.Sitting_Enter"));
    SeatedEnterAnimation = SeatedEnterFinder.Succeeded() ? SeatedEnterFinder.Object : nullptr;

    static ConstructorHelpers::FObjectFinder<UAnimSequence> SeatedCareFinder(
        TEXT("/Game/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Sitting_Talking_Loop.Sitting_Talking_Loop"));
    SeatedCareAnimation = SeatedCareFinder.Succeeded() ? SeatedCareFinder.Object : nullptr;

    static ConstructorHelpers::FObjectFinder<UAnimSequence> SeatedExitFinder(
        TEXT("/Game/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Sitting_Exit.Sitting_Exit"));
    SeatedExitAnimation = SeatedExitFinder.Succeeded() ? SeatedExitFinder.Object : nullptr;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> FlakeFinder(
        TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CuttingToolFinder(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ContainerFinder(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> FoodFinder(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> DrinkFinder(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> HeldPropMaterialFinder(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    SharpFlakeMesh = FlakeFinder.Succeeded() ? FlakeFinder.Object : nullptr;
    StoneCuttingToolMesh = CuttingToolFinder.Succeeded() ? CuttingToolFinder.Object : nullptr;
    SimpleContainerMesh = ContainerFinder.Succeeded() ? ContainerFinder.Object : nullptr;
    FoodProxyMesh = FoodFinder.Succeeded() ? FoodFinder.Object : nullptr;
    DrinkProxyMesh = DrinkFinder.Succeeded() ? DrinkFinder.Object : nullptr;
    HeldPropMaterialBase = HeldPropMaterialFinder.Succeeded() ? HeldPropMaterialFinder.Object : nullptr;
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

UAnimSequence* ULLResidentMotionComponent::LegacyContextAnimation() const
{
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

    return DesiredAnimation;
}

ELLResidentContextMotion ULLResidentMotionComponent::ResolveDirectPhysicalMotion() const
{
    const ALLResidentCharacter* Resident = Cast<ALLResidentCharacter>(GetOwner());
    const UWorld* World = GetWorld();
    if (!Resident || !World || !Resident->GetResidentId().IsValid())
    {
        return ELLResidentContextMotion::None;
    }

    const ALLWorldDirector* WorldDirector = nullptr;
    for (TActorIterator<ALLWorldDirector> It(World); It; ++It)
    {
        WorldDirector = *It;
        break;
    }

    // Do not infer "performing" from a cleared movement target. WorldDirector
    // owns the physical-use window and exposes its actual runtime state.
    if (!WorldDirector
        || !WorldDirector->IsResidentPerformingPhysicalAction(Resident->GetResidentId()))
    {
        return ELLResidentContextMotion::None;
    }

    switch (Resident->GetCurrentIntent())
    {
        case ELLActionIntent::Eat:     return ELLResidentContextMotion::Eat;
        case ELLActionIntent::Drink:   return ELLResidentContextMotion::Drink;
        case ELLActionIntent::Sleep:   return ELLResidentContextMotion::SleepRest;
        case ELLActionIntent::Toilet:
        case ELLActionIntent::Hygiene: return ELLResidentContextMotion::CrouchLow;
        default:                       return ELLResidentContextMotion::None;
    }
}

ELLResidentContextMotion ULLResidentMotionComponent::ResolveTravelMotion() const
{
    const ALLResidentCharacter* Resident = Cast<ALLResidentCharacter>(GetOwner());
    const UWorld* World = GetWorld();
    const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    const ULLCoreBridgeSubsystem* Bridge =
        GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
    if (!Resident || !Bridge || !Resident->GetResidentId().IsValid()
        || Resident->HasReachedMovementTarget())
    {
        return ELLResidentContextMotion::None;
    }

    FLLCoreActionDirective Directive;
    if (!Bridge->GetResidentPendingContextDirective(Resident->GetResidentId(), Directive))
    {
        return ELLResidentContextMotion::None;
    }

    if (Directive.ContextActionKind == ELLCoreContextActionKind::Civilization
        && Directive.CivilizationFacilityAction == ELLCoreFacilityBuildAction::DeliverMaterial)
    {
        return ELLResidentContextMotion::HaulPush;
    }
    return ELLResidentContextMotion::None;
}

ELLResidentContextMotion ULLResidentMotionComponent::ResolveContextMotion() const
{
    const ALLResidentCharacter* Resident = Cast<ALLResidentCharacter>(GetOwner());
    const UWorld* World = GetWorld();
    const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    const ULLCoreBridgeSubsystem* Bridge =
        GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
    if (!Resident || !Bridge || !Resident->GetResidentId().IsValid())
    {
        return ELLResidentContextMotion::None;
    }

    // The *pending context directive* is the one WorldDirector itself consumed
    // to open this work window (ALLWorldDirector::ApplyPendingContextDirective),
    // and it is the only read that carries ContextActionKind, the facility
    // action, the parenting action and the equipped tool capability.
    //
    // GetResidentActionDirective is a different read: it reports the observed
    // activity plus the physical/social intent, and it fills the civilization
    // fields only while the resident is Idle. Measured over a headless run,
    // 112/112 samples came back with ctxKind=0 facility=0 tool=0 cap=0 from
    // that read while the pending read carried ctxKind=1/2/4 — which is why
    // every refinement used to fall through to the legacy clip.
    FLLCoreActionDirective Directive;
    const bool bHasPending =
        Bridge->GetResidentPendingContextDirective(Resident->GetResidentId(), Directive);
    if (!bHasPending && !Bridge->GetResidentActionDirective(Resident->GetResidentId(), Directive))
    {
        return ELLResidentContextMotion::None;
    }

    if (!Directive.bAlive)
    {
        // Death presentation belongs to the lifecycle lane, not to work motion.
        return ELLResidentContextMotion::None;
    }

    // Sanitation is classified first: Core marks it both on the civilization
    // action and on the physical intent, and a squat reads correctly for both.
    if (Directive.CivilizationSanitationSiteId != 0
        || Directive.PhysicalIntent == ELLCorePhysicalIntent::Toilet
        || Directive.PhysicalIntent == ELLCorePhysicalIntent::Hygiene)
    {
        return ELLResidentContextMotion::CrouchLow;
    }

    switch (Directive.ContextActionKind)
    {
        case ELLCoreContextActionKind::KnowledgeTeaching:
            // The directive carries a teaching target only on the teacher side.
            return Directive.TargetResidentId.IsValid()
                ? ELLResidentContextMotion::Talk
                : ELLResidentContextMotion::Learn;

        case ELLCoreContextActionKind::Parenting:
            switch (Directive.ParentingAction)
            {
                case ELLCoreParentingAction::Feed:
                case ELLCoreParentingAction::Hold:
                case ELLCoreParentingAction::Comfort:
                case ELLCoreParentingAction::HealthCare:
                    return ELLResidentContextMotion::SeatedCare;
                case ELLCoreParentingAction::Educate:
                case ELLCoreParentingAction::Discipline:
                    return ELLResidentContextMotion::Talk;
                case ELLCoreParentingAction::Play:
                    return ELLResidentContextMotion::Learn;
                case ELLCoreParentingAction::Bathe:
                case ELLCoreParentingAction::ToiletAssist:
                    return ELLResidentContextMotion::CrouchLow;
                case ELLCoreParentingAction::PutToSleep:
                    // No authored lying/settling clip exists in the imported
                    // pack, so this stays a seated care pose and is recorded as
                    // a Context Motion v2 asset gap rather than dressed up with
                    // an unrelated clip.
                    return ELLResidentContextMotion::SeatedCare;
                default:
                    return ELLResidentContextMotion::Talk;
            }

        case ELLCoreContextActionKind::Social:
            return Directive.SocialIntent == ELLCoreSocialIntent::Comfort
                ? ELLResidentContextMotion::SeatedCare
                : ELLResidentContextMotion::Talk;

        default:
            break;
    }

    if (Directive.CivilizationFacilityAction != ELLCoreFacilityBuildAction::None)
    {
        switch (Directive.CivilizationFacilityAction)
        {
            case ELLCoreFacilityBuildAction::DeliverMaterial:
                return ELLResidentContextMotion::HaulPush;
            case ELLCoreFacilityBuildAction::Fuel:
            case ELLCoreFacilityBuildAction::Ignite:
            case ELLCoreFacilityBuildAction::CollectCharcoal:
                return ELLResidentContextMotion::FireTend;
            default:
                return ELLResidentContextMotion::CraftWork;
        }
    }

    // Tool capability describes the physical gesture better than the action
    // verb does, so it wins whenever Core reports an equipped tool. The clips
    // are the same ones the held-tool route in LegacyContextAnimation() uses,
    // so a refined and an unrefined resident never disagree on screen.
    if (Directive.bHasCivilizationTool)
    {
        switch (Directive.CivilizationToolCapability)
        {
            case ELLCoreToolCapability::Chop:
            case ELLCoreToolCapability::Cut:
            case ELLCoreToolCapability::Strike:
                return ELLResidentContextMotion::StrikeSwing;
            case ELLCoreToolCapability::Dig:
                return ELLResidentContextMotion::DigWork;
            case ELLCoreToolCapability::Heat:
                return ELLResidentContextMotion::FireTend;
            case ELLCoreToolCapability::Carry:
                return ELLResidentContextMotion::GatherPick;
            default:
                break;
        }
    }

    switch (Directive.CivilizationAction)
    {
        case ELLCoreCivilizationAction::Gather:
        case ELLCoreCivilizationAction::Store:
            return ELLResidentContextMotion::GatherPick;
        case ELLCoreCivilizationAction::Craft:
        case ELLCoreCivilizationAction::Experiment:
            return ELLResidentContextMotion::CraftWork;
        default:
            break;
    }

    // Observation-read residue: a social exchange with no pending context
    // action still reads as a conversation.
    if (Directive.ActivityKind == ELLCoreObservedActivityKind::Social
        && Directive.SocialIntent != ELLCoreSocialIntent::None
        && Directive.SocialIntent != ELLCoreSocialIntent::Avoid)
    {
        return ELLResidentContextMotion::Talk;
    }

    return ELLResidentContextMotion::None;
}

UAnimSequence* ULLResidentMotionComponent::ClipForContextMotion(ELLResidentContextMotion Motion) const
{
    switch (Motion)
    {
        case ELLResidentContextMotion::Eat:         return InteractAnimation;
        case ELLResidentContextMotion::Drink:       return InteractAnimation;
        case ELLResidentContextMotion::SleepRest:   return IdleAnimation;
        case ELLResidentContextMotion::Talk:        return TalkingAnimation;
        case ELLResidentContextMotion::Learn:       return InteractAnimation;
        case ELLResidentContextMotion::GatherPick:  return GatherAnimation;
        case ELLResidentContextMotion::StrikeSwing: return StrikeAnimation;
        case ELLResidentContextMotion::DigWork:     return DigAnimation;
        case ELLResidentContextMotion::CraftWork:   return BuildAnimation;
        case ELLResidentContextMotion::HaulPush:
            // Travel hauling should read as walking while carrying the visible
            // container, not as a stationary push loop sliding across terrain.
            // Returning no context clip releases animation ownership to the
            // locomotion BlendSpace while ActiveContextMotion still keeps the
            // carry prop visible from the authoritative delivery directive.
            return nullptr;
        case ELLResidentContextMotion::FireTend:    return FireAnimation;
        case ELLResidentContextMotion::CrouchLow:   return CrouchAnimation;
        case ELLResidentContextMotion::SeatedCare:  return SeatedCareAnimation;
        case ELLResidentContextMotion::None:
        default:                                    return nullptr;
    }
}

void ULLResidentMotionComponent::LogDirectiveDiagnostics() const
{
    const AActor* Owner = GetOwner();
    const ALLResidentCharacter* Resident = Cast<ALLResidentCharacter>(Owner);
    const UWorld* World = GetWorld();
    const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    const ULLCoreBridgeSubsystem* Bridge =
        GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;

    const bool bResidentCast = Resident != nullptr;
    const bool bIdValid = Resident && Resident->GetResidentId().IsValid();

    FLLCoreActionDirective ActionDirective;
    bool bActionRead = false;
    FLLCoreActionDirective PendingDirective;
    bool bPendingRead = false;
    if (Bridge && bIdValid)
    {
        bActionRead = Bridge->GetResidentActionDirective(Resident->GetResidentId(), ActionDirective);
        bPendingRead = Bridge->GetResidentPendingContextDirective(Resident->GetResidentId(), PendingDirective);
    }

    UE_LOG(LogTemp, Log,
        TEXT("LLMotionDiag %s cast=%d id=%d bridge=%d | action=%d activity=%d phys=%d social=%d ctxKind=%d civ=%d facility=%d tool=%d cap=%d sanit=%lld")
        TEXT(" | pending=%d ctxKind=%d civ=%d facility=%d parenting=%d tool=%d cap=%d sanit=%lld"),
        Owner ? *Owner->GetName() : TEXT("none"),
        bResidentCast ? 1 : 0, bIdValid ? 1 : 0, Bridge ? 1 : 0,
        bActionRead ? 1 : 0,
        static_cast<int32>(ActionDirective.ActivityKind),
        static_cast<int32>(ActionDirective.PhysicalIntent),
        static_cast<int32>(ActionDirective.SocialIntent),
        static_cast<int32>(ActionDirective.ContextActionKind),
        static_cast<int32>(ActionDirective.CivilizationAction),
        static_cast<int32>(ActionDirective.CivilizationFacilityAction),
        ActionDirective.bHasCivilizationTool ? 1 : 0,
        static_cast<int32>(ActionDirective.CivilizationToolCapability),
        static_cast<long long>(ActionDirective.CivilizationSanitationSiteId),
        bPendingRead ? 1 : 0,
        static_cast<int32>(PendingDirective.ContextActionKind),
        static_cast<int32>(PendingDirective.CivilizationAction),
        static_cast<int32>(PendingDirective.CivilizationFacilityAction),
        static_cast<int32>(PendingDirective.ParentingAction),
        PendingDirective.bHasCivilizationTool ? 1 : 0,
        static_cast<int32>(PendingDirective.CivilizationToolCapability),
        static_cast<long long>(PendingDirective.CivilizationSanitationSiteId));
}

void ULLResidentMotionComponent::UpdateContextAnimationState(float DeltaTime)
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

    auto PlayClip = [this](UAnimSequence* Clip, bool bLoop)
    {
        if (!Clip || ActiveContextAnimation == Clip)
        {
            return;
        }
        Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Body->PlayAnimation(Clip, bLoop);
        ActiveContextAnimation = Clip;
        bLocomotionPlaying = false;
    };

    // The WorldDirector signal stays the authority on *whether* a resident is
    // presenting context work at its target. Refining the clip never widens
    // that window, so a resident walking toward a tree is still walking.
    UAnimSequence* LegacyAnimation = LegacyContextAnimation();

    ELLResidentContextMotion DesiredMotion = ResolveDirectPhysicalMotion();
    UAnimSequence* DesiredAnimation = ClipForContextMotion(DesiredMotion);

    if (DesiredMotion == ELLResidentContextMotion::None)
    {
        DesiredMotion = ResolveTravelMotion();
        DesiredAnimation = ClipForContextMotion(DesiredMotion);
    }

    if (DesiredMotion == ELLResidentContextMotion::None && LegacyAnimation)
    {
        DesiredMotion = ResolveContextMotion();
        DesiredAnimation = ClipForContextMotion(DesiredMotion);
        if (!DesiredAnimation)
        {
            // No bridge, unbound resident, or a directive this milestone does
            // not classify: keep exactly what the held-tool route decided.
            DesiredMotion = ELLResidentContextMotion::None;
            DesiredAnimation = LegacyAnimation;
        }
    }

    // Seated care owns authored enter/exit clips, so it is the only motion that
    // needs a transition state. Everything else is a plain looping clip swap.
    if (SeatedTransitionRemaining > 0.0f)
    {
        SeatedTransitionRemaining = FMath::Max(0.0f, SeatedTransitionRemaining - DeltaTime);
        if (SeatedTransitionRemaining > 0.0f)
        {
            return;
        }

        if (bSeatedEntered)
        {
            PlayClip(SeatedCareAnimation, true);
            ActiveContextMotion = ELLResidentContextMotion::SeatedCare;
            return;
        }

        // The stand-up clip finished. Release the body explicitly, otherwise
        // the one-shot exit pose would hold and locomotion could never take
        // ownership again.
        ActiveContextAnimation = nullptr;
        ActiveContextMotion = ELLResidentContextMotion::None;
        bLocomotionPlaying = false;
    }

    const bool bWasSeated = bSeatedEntered;
    const bool bWantsSeated = DesiredMotion == ELLResidentContextMotion::SeatedCare;

    if (bWantsSeated && !bWasSeated && SeatedEnterAnimation)
    {
        PlayClip(SeatedEnterAnimation, false);
        SeatedTransitionRemaining = SeatedEnterAnimation->GetPlayLength();
        bSeatedEntered = true;
        ActiveContextMotion = ELLResidentContextMotion::SeatedCare;
        return;
    }

    if (!bWantsSeated && bWasSeated && SeatedExitAnimation)
    {
        bSeatedEntered = false;
        PlayClip(SeatedExitAnimation, false);
        SeatedTransitionRemaining = SeatedExitAnimation->GetPlayLength();
        ActiveContextMotion = ELLResidentContextMotion::None;
        return;
    }

    ActiveContextMotion = DesiredMotion;

    if (DesiredAnimation)
    {
        PlayClip(DesiredAnimation, true);
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
    const bool bDailyLifeProp =
        ActiveContextMotion == ELLResidentContextMotion::Eat
        || ActiveContextMotion == ELLResidentContextMotion::Drink
        || ActiveContextMotion == ELLResidentContextMotion::HaulPush;

    if (HeldToolPresentation == ELLResidentHeldToolPresentation::None && !bDailyLifeProp)
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
        if (HeldPropMaterialBase)
        {
            HeldPropMaterial = UMaterialInstanceDynamic::Create(HeldPropMaterialBase, HeldToolMesh);
            if (HeldPropMaterial)
            {
                HeldToolMesh->SetMaterial(0, HeldPropMaterial);
            }
        }
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
    FLinearColor PropColor(0.38f, 0.34f, 0.30f, 1.0f);

    if (HeldToolPresentation == ELLResidentHeldToolPresentation::None)
    {
        if (ActiveContextMotion == ELLResidentContextMotion::Eat)
        {
            DesiredMesh = FoodProxyMesh.Get();
            RelativeScale = FVector(0.10f);
            RelativeLocation = FVector(8.0f, 2.0f, 0.0f);
            PropColor = FLinearColor(0.78f, 0.48f, 0.20f, 1.0f);
        }
        else if (ActiveContextMotion == ELLResidentContextMotion::Drink)
        {
            DesiredMesh = DrinkProxyMesh.Get();
            RelativeScale = FVector(0.075f, 0.075f, 0.16f);
            RelativeRotation = FRotator(0.0f, 0.0f, 90.0f);
            RelativeLocation = FVector(7.0f, 1.0f, 0.0f);
            PropColor = FLinearColor(0.32f, 0.60f, 0.86f, 1.0f);
        }
        else if (ActiveContextMotion == ELLResidentContextMotion::HaulPush)
        {
            DesiredMesh = SimpleContainerMesh.Get();
            RelativeScale = FVector(0.20f, 0.20f, 0.17f);
            RelativeRotation = FRotator(4.0f, -8.0f, 12.0f);
            RelativeLocation = FVector(12.0f, 1.0f, -8.0f);
            PropColor = FLinearColor(0.48f, 0.30f, 0.14f, 1.0f);
        }
    }

    switch (HeldToolPresentation)
    {
        case ELLResidentHeldToolPresentation::SharpFlake:
            DesiredMesh = SharpFlakeMesh.Get();
            RelativeScale = FVector(0.035f, 0.055f, 0.025f);
            RelativeRotation = FRotator(0.0f, 90.0f, 90.0f);
            PropColor = FLinearColor(0.46f, 0.50f, 0.54f, 1.0f);
            break;
        case ELLResidentHeldToolPresentation::StoneCuttingTool:
            DesiredMesh = StoneCuttingToolMesh.Get();
            RelativeScale = FVector(0.035f, 0.035f, 0.20f);
            RelativeRotation = FRotator(0.0f, 15.0f, 70.0f);
            PropColor = FLinearColor(0.40f, 0.42f, 0.44f, 1.0f);
            break;
        case ELLResidentHeldToolPresentation::SimpleContainer:
            DesiredMesh = SimpleContainerMesh.Get();
            RelativeScale = FVector(0.09f, 0.09f, 0.12f);
            RelativeLocation = FVector(5.0f, 0.0f, -3.0f);
            PropColor = FLinearColor(0.50f, 0.31f, 0.15f, 1.0f);
            break;
        case ELLResidentHeldToolPresentation::DiggingStick:
            DesiredMesh = StoneCuttingToolMesh.Get();
            RelativeScale = FVector(0.022f, 0.022f, 0.28f);
            RelativeRotation = FRotator(0.0f, 8.0f, 82.0f);
            RelativeLocation = FVector(4.0f, 0.0f, -4.0f);
            PropColor = FLinearColor(0.43f, 0.25f, 0.10f, 1.0f);
            break;
        case ELLResidentHeldToolPresentation::StoneHammer:
            DesiredMesh = StoneCuttingToolMesh.Get();
            RelativeScale = FVector(0.065f, 0.045f, 0.16f);
            RelativeRotation = FRotator(0.0f, 24.0f, 68.0f);
            RelativeLocation = FVector(3.0f, 0.0f, 1.0f);
            PropColor = FLinearColor(0.34f, 0.36f, 0.40f, 1.0f);
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
    if (HeldPropMaterial)
    {
        HeldPropMaterial->SetVectorParameterValue(FName(TEXT("Color")), PropColor);
        HeldToolMesh->SetMaterial(0, HeldPropMaterial);
    }
    HeldToolMesh->SetRelativeScale3D(RelativeScale);
    HeldToolMesh->SetRelativeRotation(RelativeRotation);
    if (!HeldToolMesh->GetAttachSocketName().IsNone())
    {
        HeldToolMesh->SetRelativeLocation(RelativeLocation);
    }
    HeldToolMesh->SetVisibility(true, true);
}

bool ULLResidentMotionComponent::ResolveInteractionTargetYaw(float& OutYawDegrees) const
{
    const ALLResidentCharacter* Resident = Cast<ALLResidentCharacter>(GetOwner());
    const UWorld* World = GetWorld();
    const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    const ULLCoreBridgeSubsystem* Bridge =
        GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
    if (!Resident || !World || !Bridge || !Resident->GetResidentId().IsValid())
    {
        return false;
    }

    FLLCoreActionDirective Directive;
    bool bHasDirective =
        Bridge->GetResidentPendingContextDirective(Resident->GetResidentId(), Directive);
    if (!bHasDirective)
    {
        bHasDirective =
            Bridge->GetResidentActionDirective(Resident->GetResidentId(), Directive);
    }
    if (!bHasDirective || !Directive.TargetResidentId.IsValid())
    {
        return false;
    }

    const bool bInterpersonalDirective =
        Directive.ContextActionKind == ELLCoreContextActionKind::Social
        || Directive.ContextActionKind == ELLCoreContextActionKind::KnowledgeTeaching
        || Directive.ContextActionKind == ELLCoreContextActionKind::Parenting
        || Directive.ActivityKind == ELLCoreObservedActivityKind::Social;
    if (!bInterpersonalDirective)
    {
        return false;
    }

    for (TActorIterator<ALLResidentCharacter> It(World); It; ++It)
    {
        const ALLResidentCharacter* Target = *It;
        if (!Target || Target->GetResidentId() != Directive.TargetResidentId)
        {
            continue;
        }

        FVector ToTarget = Target->GetActorLocation() - Resident->GetActorLocation();
        ToTarget.Z = 0.0f;
        if (ToTarget.IsNearlyZero())
        {
            return false;
        }

        OutYawDegrees = ToTarget.Rotation().Yaw;
        return true;
    }

    return false;
}

void ULLResidentMotionComponent::UpdateBodyOrientation(float DeltaTime)
{
    if (!Body)
    {
        return;
    }

    const AActor* Owner = GetOwner();
    const float OwnerYaw = Owner ? Owner->GetActorRotation().Yaw : SmoothedYaw;

    if (ActiveContextMotion == ELLResidentContextMotion::SleepRest)
    {
        // The current imported standard pack has no dedicated lying clip.
        // Rotate the complete appearance hierarchy only while WorldDirector has
        // actually reached the authoritative sleep use point. The capsule,
        // navigation position and Core state remain upright/unchanged.
        Body->SetWorldRotation(FRotator(
            0.0f,
            OwnerYaw + MeshForwardYawOffsetDegrees,
            90.0f));
        return;
    }

    float TargetYaw = SmoothedSpeed > 0.0f ? DesiredYaw : OwnerYaw;

    // Social/teaching/parenting work is much easier to read when participants
    // face one another. Only stationary interpersonal presentation may override
    // facing; movement remains owned by the actual travel direction.
    const bool bCanFaceInteractionTarget =
        SmoothedSpeed <= IdleSpeedThreshold
        && (bSocialInteractionActive
            || ActiveContextMotion == ELLResidentContextMotion::Talk
            || ActiveContextMotion == ELLResidentContextMotion::Learn
            || ActiveContextMotion == ELLResidentContextMotion::SeatedCare);
    float InteractionYaw = TargetYaw;
    if (bCanFaceInteractionTarget && ResolveInteractionTargetYaw(InteractionYaw))
    {
        TargetYaw = InteractionYaw;
    }

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

    UpdateContextAnimationState(DeltaTime);
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
            LogDirectiveDiagnostics();
            float FacingDot = 0.0f;
            if (Body && SmoothedSpeed > 0.0f)
            {
                const FVector TravelDirection = FRotator(0.0f, DesiredYaw, 0.0f).Vector();
                FacingDot = FVector::DotProduct(Body->GetRightVector(), TravelDirection);
            }

            UE_LOG(LogTemp, Log, TEXT("LLMotion %s speed=%.1f window=%.1f yaw=%.1f visual=%.1f travel=%.1f actor=%.1f facing=%.2f locomotion=%d talking=%d work=%d tool=%d motion=%d context=%s loc=%.0f,%.0f"),
                *Owner->GetName(), SmoothedSpeed, WindowedSpeed, SmoothedYaw,
                SmoothedYaw + MeshForwardYawOffsetDegrees, DesiredYaw,
                Owner->GetActorRotation().Yaw, FacingDot,
                bLocomotionPlaying ? 1 : 0,
                bSocialInteractionActive ? 1 : 0,
                static_cast<int32>(WorkPresentationMode),
                static_cast<int32>(HeldToolPresentation),
                static_cast<int32>(ActiveContextMotion),
                ActiveContextAnimation ? *ActiveContextAnimation->GetName() : TEXT("none"),
                Location.X, Location.Y);
        }
    }
}
