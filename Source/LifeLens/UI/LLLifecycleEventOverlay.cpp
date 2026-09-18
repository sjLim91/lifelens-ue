#include "UI/LLLifecycleEventOverlay.h"

#include "UI/LLObservationSubsystem.h"
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

    RootStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LifecycleRootStack"));
    EventBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LifecycleEventBorder"));
    EventList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LifecycleEventList"));
    ResidentStatusBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResidentLifecycleStatusBorder"));
    UVerticalBox* StatusStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResidentLifecycleStatusStack"));
    ResidentStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResidentLifecycleStatusText"));
    ResidentHistoryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResidentLifecycleHistoryText"));

    EventBorder->SetContent(EventList);
    EventBorder->SetPadding(FMargin(12.0f, 8.0f));
    EventBorder->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.03f, 0.72f));
    EventBorder->SetVisibility(ESlateVisibility::Collapsed);

    ResidentStatusText->SetAutoWrapText(true);
    ResidentStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.97f, 1.0f, 1.0f)));
    ResidentHistoryText->SetAutoWrapText(true);
    ResidentHistoryText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.84f, 0.92f, 0.95f)));
    if (UVerticalBoxSlot* StatusSlot = StatusStack->AddChildToVerticalBox(ResidentStatusText))
    {
        StatusSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
    }
    StatusStack->AddChildToVerticalBox(ResidentHistoryText);
    ResidentStatusBorder->SetContent(StatusStack);
    ResidentStatusBorder->SetPadding(FMargin(12.0f, 9.0f));
    ResidentStatusBorder->SetBrushColor(FLinearColor(0.02f, 0.025f, 0.04f, 0.70f));
    ResidentStatusBorder->SetVisibility(ESlateVisibility::Collapsed);

    if (UVerticalBoxSlot* EventSlot = RootStack->AddChildToVerticalBox(EventBorder))
    {
        EventSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
    }
    RootStack->AddChildToVerticalBox(ResidentStatusBorder);
    WidgetTree->RootWidget = RootStack;

    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ULLLifecycleEventOverlay::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    RefreshFromCore();
    RefreshSelectedResidentCard();

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
    PreviousRomancePairs.Reset();
    Notices.Reset();
    ObservedLifeHistory.Reset();
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

FString ULLLifecycleEventOverlay::RomanceStageLabel(ELLCoreRomanceStage Stage)
{
    switch (Stage)
    {
        case ELLCoreRomanceStage::Dating: return TEXT("연애 중");
        case ELLCoreRomanceStage::Engaged: return TEXT("약혼");
        case ELLCoreRomanceStage::Married: return TEXT("결혼");
        case ELLCoreRomanceStage::Separated: return TEXT("별거");
        case ELLCoreRomanceStage::Divorced: return TEXT("이혼");
        case ELLCoreRomanceStage::Widowed: return TEXT("사별");
        case ELLCoreRomanceStage::FormerPartners: return TEXT("이전 연인");
        case ELLCoreRomanceStage::None:
        default: return TEXT("관계 없음");
    }
}

FString ULLLifecycleEventOverlay::RomanceEventPrefix(ELLCoreRomanceStage Stage)
{
    switch (Stage)
    {
        case ELLCoreRomanceStage::Dating: return TEXT("[연애]");
        case ELLCoreRomanceStage::Engaged: return TEXT("[약혼]");
        case ELLCoreRomanceStage::Married: return TEXT("[결혼]");
        case ELLCoreRomanceStage::Separated: return TEXT("[별거]");
        case ELLCoreRomanceStage::Divorced: return TEXT("[이혼]");
        case ELLCoreRomanceStage::Widowed: return TEXT("[사별]");
        case ELLCoreRomanceStage::FormerPartners: return TEXT("[관계 종료]");
        case ELLCoreRomanceStage::None:
        default: return TEXT("[관계]");
    }
}

FString ULLLifecycleEventOverlay::FormatObservedMoment(int64 SimulationMinute)
{
    const int64 SafeMinute = FMath::Max<int64>(0, SimulationMinute);
    const int64 Day = SafeMinute / 1440 + 1;
    const int32 MinuteOfDay = static_cast<int32>(SafeMinute % 1440);
    return FString::Printf(TEXT("%lld일 %02d:%02d"),
        static_cast<long long>(Day), MinuteOfDay / 60, MinuteOfDay % 60);
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
    TMap<FString, FRomanceVisualState> CurrentRomancePairs;
    TMap<FString, FString> PregnancyLabels;
    TMap<FString, FGuid> PregnancySubjects;
    TMap<FString, FGuid> PregnancyPartners;

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
        if (!Bridge->GetFamilyObservation(Resident.ResidentId, Family))
        {
            continue;
        }

        if (Family.bHasRomanceHistory && Family.PartnerResidentId.IsValid())
        {
            const FString RomanceKey =
                PregnancyPairKey(Resident.ResidentId, Family.PartnerResidentId);
            if (!CurrentRomancePairs.Contains(RomanceKey))
            {
                const FString ResidentKey =
                    Resident.ResidentId.ToString(EGuidFormats::Digits);
                const FString PartnerKey =
                    Family.PartnerResidentId.ToString(EGuidFormats::Digits);
                const FString PartnerName = Family.PartnerName.IsEmpty()
                    ? TEXT("상대 주민")
                    : Family.PartnerName;

                FRomanceVisualState Romance;
                if (ResidentKey < PartnerKey)
                {
                    Romance.FirstResidentId = Resident.ResidentId;
                    Romance.FirstName = Resident.DisplayName;
                    Romance.SecondResidentId = Family.PartnerResidentId;
                    Romance.SecondName = PartnerName;
                }
                else
                {
                    Romance.FirstResidentId = Family.PartnerResidentId;
                    Romance.FirstName = PartnerName;
                    Romance.SecondResidentId = Resident.ResidentId;
                    Romance.SecondName = Resident.DisplayName;
                }
                Romance.Stage = Family.PartnerStage;
                Romance.bCohabiting = Family.bCohabitingWithPartner;
                CurrentRomancePairs.Add(RomanceKey, MoveTemp(Romance));
            }
        }

        if (Family.bExpectingChild && Family.PregnancyPartnerResidentId.IsValid())
        {
            const FString PairKey =
                PregnancyPairKey(Resident.ResidentId, Family.PregnancyPartnerResidentId);
            CurrentPregnancyPairs.Add(PairKey);
            if (!PregnancyLabels.Contains(PairKey))
            {
                const FString PartnerName = Family.PregnancyPartnerName.IsEmpty()
                    ? TEXT("상대 주민")
                    : Family.PregnancyPartnerName;
                PregnancyLabels.Add(
                    PairKey,
                    Resident.DisplayName + TEXT(" · ") + PartnerName);
                PregnancySubjects.Add(PairKey, Resident.ResidentId);
                PregnancyPartners.Add(PairKey, Family.PregnancyPartnerResidentId);
            }
        }
    }

    if (!bBaselineReady)
    {
        PreviousResidents = MoveTemp(CurrentResidents);
        PreviousPregnancyPairs = MoveTemp(CurrentPregnancyPairs);
        PreviousRomancePairs = MoveTemp(CurrentRomancePairs);
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
                PushNotice(FString::Printf(TEXT("[탄생] %s"), *Current.Name), Pair.Key, FGuid(), WorldObservation.SimulationMinute);
            }
            else
            {
                PushNotice(FString::Printf(TEXT("[새 주민] %s"), *Current.Name), Pair.Key, FGuid(), WorldObservation.SimulationMinute);
            }
            continue;
        }

        if (Previous->bAlive && !Current.bAlive)
        {
            PushNotice(FString::Printf(TEXT("[사망] %s"), *Current.Name), Pair.Key, FGuid(), WorldObservation.SimulationMinute);
            continue;
        }

        if (Current.bAlive && Previous->LifeStage != Current.LifeStage)
        {
            PushNotice(FString::Printf(
                TEXT("[성장] %s · %s → %s"),
                *Current.Name,
                *LifeStageLabel(Previous->LifeStage),
                *LifeStageLabel(Current.LifeStage)), Pair.Key, FGuid(), WorldObservation.SimulationMinute);
        }
    }

    for (const TPair<FString, FRomanceVisualState>& Pair : CurrentRomancePairs)
    {
        const FRomanceVisualState& Current = Pair.Value;
        const FRomanceVisualState* Previous = PreviousRomancePairs.Find(Pair.Key);
        const FString Names = Current.FirstName + TEXT(" · ") + Current.SecondName;

        if (!Previous)
        {
            if (Current.Stage != ELLCoreRomanceStage::None)
            {
                PushNotice(
                    RomanceEventPrefix(Current.Stage) + TEXT(" ") + Names,
                    Current.FirstResidentId,
                    Current.SecondResidentId,
                    WorldObservation.SimulationMinute);
            }
            if (Current.bCohabiting)
            {
                PushNotice(
                    TEXT("[동거] ") + Names,
                    Current.FirstResidentId,
                    Current.SecondResidentId,
                    WorldObservation.SimulationMinute);
            }
            continue;
        }

        if (Previous->Stage != Current.Stage)
        {
            PushNotice(
                RomanceEventPrefix(Current.Stage) + TEXT(" ") + Names,
                Current.FirstResidentId,
                Current.SecondResidentId,
                WorldObservation.SimulationMinute);
        }

        if (Previous->bCohabiting != Current.bCohabiting)
        {
            PushNotice(
                (Current.bCohabiting ? TEXT("[동거] ") : TEXT("[동거 종료] ")) + Names,
                Current.FirstResidentId,
                Current.SecondResidentId,
                WorldObservation.SimulationMinute);
        }
    }

    for (const FString& PairKey : CurrentPregnancyPairs)
    {
        if (!PreviousPregnancyPairs.Contains(PairKey))
        {
            const FString* Label = PregnancyLabels.Find(PairKey);
            const FGuid* Subject = PregnancySubjects.Find(PairKey);
            const FGuid* Partner = PregnancyPartners.Find(PairKey);
            PushNotice(FString::Printf(TEXT("[임신] %s"), Label ? **Label : TEXT("새 가족")),
                Subject ? *Subject : FGuid(), Partner ? *Partner : FGuid(), WorldObservation.SimulationMinute);
        }
    }

    PreviousResidents = MoveTemp(CurrentResidents);
    PreviousPregnancyPairs = MoveTemp(CurrentPregnancyPairs);
    PreviousRomancePairs = MoveTemp(CurrentRomancePairs);
    LastObservedSimulationMinute = WorldObservation.SimulationMinute;
}

void ULLLifecycleEventOverlay::RefreshSelectedResidentCard()
{
    if (!ResidentStatusBorder || !ResidentStatusText || !ResidentHistoryText)
    {
        return;
    }

    UGameInstance* GameInstance = GetGameInstance();
    ULLObservationSubsystem* Observation = GameInstance ? GameInstance->GetSubsystem<ULLObservationSubsystem>() : nullptr;
    ULLCoreBridgeSubsystem* Bridge = GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
    if (!Observation || !Bridge || !Bridge->IsCoreRunning() || !Observation->HasObservedResident())
    {
        ResidentStatusBorder->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    const FGuid ResidentId = Observation->GetObservedResidentId();
    FLLCoreResidentObservation Resident;
    if (!Bridge->GetResidentObservation(ResidentId, Resident))
    {
        ResidentStatusBorder->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    FLLCoreFamilyObservation Family;
    const bool bHasFamily = Bridge->GetFamilyObservation(ResidentId, Family);
    FString Status = FString::Printf(TEXT("인생 · %s · %d세 · %s%s"),
        *Resident.DisplayName,
        Resident.AgeYears,
        *LifeStageLabel(Resident.LifeStage),
        Resident.bAlive ? TEXT("") : TEXT(" · 사망"));

    if (bHasFamily)
    {
        if (Family.bHasActivePartner)
        {
            Status += FString::Printf(TEXT("\n파트너 · %s · %s%s"),
                Family.PartnerName.IsEmpty() ? TEXT("이름 미상") : *Family.PartnerName,
                *RomanceStageLabel(Family.PartnerStage),
                Family.bCohabitingWithPartner ? TEXT(" · 동거") : TEXT(""));
        }
        else if (Family.bHasRomanceHistory && Family.PartnerResidentId.IsValid())
        {
            Status += FString::Printf(TEXT("\n관계 이력 · %s · %s"),
                Family.PartnerName.IsEmpty() ? TEXT("이름 미상") : *Family.PartnerName,
                *RomanceStageLabel(Family.PartnerStage));
        }
        if (Family.bExpectingChild)
        {
            Status += FString::Printf(TEXT("\n임신 진행 중 · %s"),
                Family.PregnancyPartnerName.IsEmpty() ? TEXT("상대 주민") : *Family.PregnancyPartnerName);
        }
        Status += FString::Printf(TEXT("\n가족 · 자녀 %d · 부모 %d · 형제 %d · 가구 %lld"),
            Family.Children.Num(), Family.Parents.Num(), Family.Siblings.Num(), static_cast<long long>(Family.HouseholdId));
    }
    ResidentStatusText->SetText(FText::FromString(Status));

    FString History = TEXT("관찰된 인생 사건");
    const TArray<FString>* Entries = ObservedLifeHistory.Find(ResidentId);
    if (!Entries || Entries->Num() == 0)
    {
        History += TEXT("\n· 이번 관찰 세션에서 새 사건 없음");
    }
    else
    {
        const int32 Start = FMath::Max(0, Entries->Num() - MaxHistoryLinesOnCard);
        for (int32 Index = Start; Index < Entries->Num(); ++Index)
        {
            History += TEXT("\n· ") + (*Entries)[Index];
        }
    }
    ResidentHistoryText->SetText(FText::FromString(History));
    ResidentStatusBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ULLLifecycleEventOverlay::PushNotice(const FString& Text, FGuid SubjectResidentId,
    FGuid RelatedResidentId, int64 SimulationMinute)
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
    Notice.SubjectResidentId = SubjectResidentId;
    Notice.RelatedResidentId = RelatedResidentId;
    Notice.SimulationMinute = SimulationMinute;
    Notice.ExpireAtRealSeconds = Now + NoticeLifetimeSeconds;
    Notices.Insert(MoveTemp(Notice), 0);

    const FString HistoryLine = FormatObservedMoment(SimulationMinute) + TEXT(" · ") + Text;
    auto AppendHistory = [this, &HistoryLine](FGuid ResidentId)
    {
        if (!ResidentId.IsValid())
        {
            return;
        }
        TArray<FString>& Entries = ObservedLifeHistory.FindOrAdd(ResidentId);
        Entries.Add(HistoryLine);
        if (Entries.Num() > MaxObservedHistoryPerResident)
        {
            Entries.RemoveAt(0, Entries.Num() - MaxObservedHistoryPerResident, EAllowShrinking::No);
        }
    };
    AppendHistory(SubjectResidentId);
    if (RelatedResidentId != SubjectResidentId)
    {
        AppendHistory(RelatedResidentId);
    }

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
    OverlayWidget->SetDesiredSizeInViewport(FVector2D(560.0f, 360.0f));
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
