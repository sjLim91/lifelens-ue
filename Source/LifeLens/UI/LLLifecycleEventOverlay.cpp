#include "UI/LLLifecycleEventOverlay.h"

#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Stats/Stats.h"

void ULLLifecycleEventOverlay::NativeConstruct()
{
    Super::NativeConstruct();

    if (!WidgetTree)
    {
        return;
    }

    EventBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LifecycleEventBorder"));
    EventList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LifecycleEventList"));

    EventBorder->SetContent(EventList);
    EventBorder->SetPadding(FMargin(12.0f, 8.0f));
    EventBorder->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.03f, 0.72f));
    EventBorder->SetVisibility(ESlateVisibility::Collapsed);
    WidgetTree->RootWidget = EventBorder;

    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ULLLifecycleEventOverlay::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    RefreshFromCore();

    const UWorld* World = GetWorld();
    const double Now = World ? static_cast<double>(World->GetRealTimeSeconds()) : 0.0;
    const int32 Removed = Notices.RemoveAll([Now](const FTransientNotice& Notice)
    {
        return Notice.ExpireAtRealSeconds <= Now;
    });
    if (Removed > 0)
    {
        RefreshNoticeWidgets();
    }
}

void ULLLifecycleEventOverlay::ResetObservationState()
{
    PreviousResidents.Reset();
    PreviousPregnancyPairs.Reset();
    Notices.Reset();
    LastObservedSimulationMinute = -1;
    bBaselineReady = false;
    RefreshNoticeWidgets();
}

FString ULLLifecycleEventOverlay::LifeStageLabel(ELLCoreLifeStage Stage)
{
    switch (Stage)
    {
        case ELLCoreLifeStage::Baby: return TEXT("영아");
        case ELLCoreLifeStage::Toddler: return TEXT("유아");
        case ELLCoreLifeStage::Child: return TEXT("아동");
        case ELLCoreLifeStage::Teen: return TEXT("청소년");
        case ELLCoreLifeStage::YoungAdult: return TEXT("청년");
        case ELLCoreLifeStage::Adult: return TEXT("성인");
        case ELLCoreLifeStage::MiddleAge: return TEXT("중년");
        case ELLCoreLifeStage::Elderly: return TEXT("노년");
    }
    return TEXT("미상");
}

FString ULLLifecycleEventOverlay::PregnancyPairKey(const FGuid& First, const FGuid& Second)
{
    const FString A = First.ToString(EGuidFormats::Digits);
    const FString B = Second.ToString(EGuidFormats::Digits);
    return A < B ? A + TEXT(":") + B : B + TEXT(":") + A;
}

void ULLLifecycleEventOverlay::RefreshFromCore()
{
    UGameInstance* GameInstance = GetGameInstance();
    ULLCoreBridgeSubsystem* Bridge = GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
    if (!Bridge || !Bridge->IsCoreRunning())
    {
        if (bBaselineReady || Notices.Num() > 0)
        {
            ResetObservationState();
        }
        return;
    }

    const FLLCoreWorldObservation WorldObservation = Bridge->GetWorldObservation();
    if (WorldObservation.SimulationMinute == LastObservedSimulationMinute)
    {
        return;
    }

    TMap<FGuid, FResidentVisualState> CurrentResidents;
    TSet<FString> CurrentPregnancyPairs;
    TMap<FString, FString> PregnancyLabels;

    const TArray<FLLCoreResidentObservation> Residents = Bridge->GetResidentObservations();
    CurrentResidents.Reserve(Residents.Num());

    for (const FLLCoreResidentObservation& Resident : Residents)
    {
        FResidentVisualState State;
        State.Name = Resident.DisplayName;
        State.LifeStage = Resident.LifeStage;
        State.bAlive = Resident.bAlive;
        CurrentResidents.Add(Resident.ResidentId, MoveTemp(State));

        if (!Resident.bAlive)
        {
            continue;
        }

        FLLCoreFamilyObservation Family;
        if (!Bridge->GetFamilyObservation(Resident.ResidentId, Family)
            || !Family.bExpectingChild
            || !Family.PregnancyPartnerResidentId.IsValid())
        {
            continue;
        }

        const FString PairKey = PregnancyPairKey(Resident.ResidentId, Family.PregnancyPartnerResidentId);
        CurrentPregnancyPairs.Add(PairKey);
        if (!PregnancyLabels.Contains(PairKey))
        {
            const FString PartnerName = Family.PregnancyPartnerName.IsEmpty()
                ? TEXT("상대 주민")
                : Family.PregnancyPartnerName;
            PregnancyLabels.Add(PairKey, Resident.DisplayName + TEXT(" · ") + PartnerName);
        }
    }

    if (!bBaselineReady)
    {
        PreviousResidents = MoveTemp(CurrentResidents);
        PreviousPregnancyPairs = MoveTemp(CurrentPregnancyPairs);
        LastObservedSimulationMinute = WorldObservation.SimulationMinute;
        bBaselineReady = true;
        return;
    }

    for (const TPair<FGuid, FResidentVisualState>& Pair : CurrentResidents)
    {
        const FResidentVisualState* Previous = PreviousResidents.Find(Pair.Key);
        const FResidentVisualState& Current = Pair.Value;

        if (!Previous)
        {
            if (Current.bAlive && Current.LifeStage == ELLCoreLifeStage::Baby)
            {
                PushNotice(FString::Printf(TEXT("[탄생] %s"), *Current.Name));
            }
            else
            {
                PushNotice(FString::Printf(TEXT("[새 주민] %s"), *Current.Name));
            }
            continue;
        }

        if (Previous->bAlive && !Current.bAlive)
        {
            PushNotice(FString::Printf(TEXT("[사망] %s"), *Current.Name));
            continue;
        }

        if (Current.bAlive && Previous->LifeStage != Current.LifeStage)
        {
            PushNotice(FString::Printf(
                TEXT("[성장] %s · %s → %s"),
                *Current.Name,
                *LifeStageLabel(Previous->LifeStage),
                *LifeStageLabel(Current.LifeStage)));
        }
    }

    for (const FString& PairKey : CurrentPregnancyPairs)
    {
        if (!PreviousPregnancyPairs.Contains(PairKey))
        {
            const FString* Label = PregnancyLabels.Find(PairKey);
            PushNotice(FString::Printf(TEXT("[임신] %s"), Label ? **Label : TEXT("새 가족")));
        }
    }

    PreviousResidents = MoveTemp(CurrentResidents);
    PreviousPregnancyPairs = MoveTemp(CurrentPregnancyPairs);
    LastObservedSimulationMinute = WorldObservation.SimulationMinute;
}

void ULLLifecycleEventOverlay::PushNotice(const FString& Text)
{
    if (Text.IsEmpty())
    {
        return;
    }

    for (const FTransientNotice& Existing : Notices)
    {
        if (Existing.Text == Text)
        {
            return;
        }
    }

    const UWorld* World = GetWorld();
    const double Now = World ? static_cast<double>(World->GetRealTimeSeconds()) : 0.0;

    FTransientNotice Notice;
    Notice.Text = Text;
    Notice.ExpireAtRealSeconds = Now + NoticeLifetimeSeconds;
    Notices.Insert(MoveTemp(Notice), 0);

    if (Notices.Num() > MaxVisibleNotices)
    {
        Notices.SetNum(MaxVisibleNotices, EAllowShrinking::No);
    }
    RefreshNoticeWidgets();
}

void ULLLifecycleEventOverlay::RefreshNoticeWidgets()
{
    if (!EventList || !EventBorder)
    {
        return;
    }

    EventList->ClearChildren();
    if (Notices.Num() == 0)
    {
        EventBorder->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    EventBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
    for (const FTransientNotice& Notice : Notices)
    {
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Label->SetText(FText::FromString(Notice.Text));
        Label->SetAutoWrapText(true);
        Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.96f, 1.0f, 1.0f)));
        Label->SetVisibility(ESlateVisibility::HitTestInvisible);

        if (UVerticalBoxSlot* Slot = EventList->AddChildToVerticalBox(Label))
        {
            Slot->SetPadding(FMargin(0.0f, 2.0f));
        }
    }
}

void ULLLifecyclePresentationSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld() || World->GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (OverlayWidget && OverlayWidget->IsInViewport())
    {
        return;
    }

    APlayerController* PlayerController = World->GetFirstPlayerController();
    if (!PlayerController || !PlayerController->IsLocalController())
    {
        return;
    }

    OverlayWidget = CreateWidget<ULLLifecycleEventOverlay>(PlayerController, ULLLifecycleEventOverlay::StaticClass());
    if (!OverlayWidget)
    {
        return;
    }

    OverlayWidget->AddToViewport(60);
    OverlayWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
    OverlayWidget->SetPositionInViewport(FVector2D(18.0f, 108.0f), false);
    OverlayWidget->SetDesiredSizeInViewport(FVector2D(560.0f, 220.0f));
}

TStatId ULLLifecyclePresentationSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(ULLLifecyclePresentationSubsystem, STATGROUP_Tickables);
}

void ULLLifecyclePresentationSubsystem::Deinitialize()
{
    if (OverlayWidget)
    {
        OverlayWidget->RemoveFromParent();
        OverlayWidget = nullptr;
    }
    Super::Deinitialize();
}
