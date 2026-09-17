#include "UI/LLObserverTimeWeatherOverlay.h"

#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Stats/Stats.h"

namespace
{
    constexpr float ButtonWidth = 54.0f;
    constexpr float ButtonHeight = 34.0f;

    UButton* BuildSpeedButton(
        UWidgetTree* WidgetTree,
        UHorizontalBox* Row,
        const FName ButtonName,
        const FString& Label)
    {
        if (!WidgetTree || !Row)
        {
            return nullptr;
        }

        USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            *FString::Printf(TEXT("%sSize"), *ButtonName.ToString()));
        Size->SetWidthOverride(ButtonWidth);
        Size->SetHeightOverride(ButtonHeight);

        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            *FString::Printf(TEXT("%sLabel"), *ButtonName.ToString()));
        Text->SetText(FText::FromString(Label));
        Text->SetJustification(ETextJustify::Center);
        Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        Text->SetVisibility(ESlateVisibility::HitTestInvisible);

        Button->SetContent(Text);
        Size->SetContent(Button);

        if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Size))
        {
            Slot->SetPadding(FMargin(2.0f, 0.0f));
            Slot->SetVerticalAlignment(VAlign_Center);
        }
        return Button;
    }
}

void ULLObserverTimeWeatherOverlay::NativeConstruct()
{
    Super::NativeConstruct();

    if (!WidgetTree)
    {
        return;
    }

    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("ObserverTimeWeatherRoot"));
    ControlBorder = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("ObserverTimeWeatherBorder"));
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ObserverTimeWeatherContent"));
    StatusText = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("ObserverTimeWeatherStatus"));
    UHorizontalBox* SpeedRow = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("ObserverSpeedRow"));

    WidgetTree->RootWidget = RootCanvas;
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    ControlBorder->SetPadding(FMargin(10.0f, 8.0f));
    ControlBorder->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.03f, 0.78f));
    ControlBorder->SetContent(Content);

    if (UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(ControlBorder))
    {
        BorderSlot->SetAnchors(FAnchors(1.0f, 0.0f));
        BorderSlot->SetAlignment(FVector2D(1.0f, 0.0f));
        BorderSlot->SetPosition(FVector2D(-16.0f, 16.0f));
        BorderSlot->SetAutoSize(true);
    }

    StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.96f, 1.0f, 1.0f)));
    StatusText->SetAutoWrapText(false);
    StatusText->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (UVerticalBoxSlot* StatusSlot = Content->AddChildToVerticalBox(StatusText))
    {
        StatusSlot->SetPadding(FMargin(4.0f, 0.0f, 4.0f, 6.0f));
    }

    if (UVerticalBoxSlot* SpeedSlot = Content->AddChildToVerticalBox(SpeedRow))
    {
        SpeedSlot->SetHorizontalAlignment(HAlign_Right);
    }

    PauseButton = BuildSpeedButton(WidgetTree, SpeedRow, TEXT("PauseButton"), TEXT("정지"));
    ObserveButton = BuildSpeedButton(WidgetTree, SpeedRow, TEXT("ObserveButton"), TEXT("1x"));
    FastButton = BuildSpeedButton(WidgetTree, SpeedRow, TEXT("FastButton"), TEXT("4x"));
    FasterButton = BuildSpeedButton(WidgetTree, SpeedRow, TEXT("FasterButton"), TEXT("16x"));
    RapidButton = BuildSpeedButton(WidgetTree, SpeedRow, TEXT("RapidButton"), TEXT("64x"));

    if (PauseButton) PauseButton->OnClicked.AddDynamic(this, &ULLObserverTimeWeatherOverlay::HandlePauseClicked);
    if (ObserveButton) ObserveButton->OnClicked.AddDynamic(this, &ULLObserverTimeWeatherOverlay::HandleObserveClicked);
    if (FastButton) FastButton->OnClicked.AddDynamic(this, &ULLObserverTimeWeatherOverlay::HandleFastClicked);
    if (FasterButton) FasterButton->OnClicked.AddDynamic(this, &ULLObserverTimeWeatherOverlay::HandleFasterClicked);
    if (RapidButton) RapidButton->OnClicked.AddDynamic(this, &ULLObserverTimeWeatherOverlay::HandleRapidClicked);

    RefreshStatus(true);
}

void ULLObserverTimeWeatherOverlay::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    RefreshStatus(false);
}

FString ULLObserverTimeWeatherOverlay::SeasonLabel(ELLCoreSeasonSummary Season)
{
    switch (Season)
    {
        case ELLCoreSeasonSummary::Spring: return TEXT("봄");
        case ELLCoreSeasonSummary::Summer: return TEXT("여름");
        case ELLCoreSeasonSummary::Autumn: return TEXT("가을");
        case ELLCoreSeasonSummary::Winter: return TEXT("겨울");
    }
    return TEXT("계절 미상");
}

FString ULLObserverTimeWeatherOverlay::WeatherLabel(ELLCoreWeatherSummary Weather)
{
    switch (Weather)
    {
        case ELLCoreWeatherSummary::Clear: return TEXT("맑음");
        case ELLCoreWeatherSummary::Cloudy: return TEXT("흐림");
        case ELLCoreWeatherSummary::Rain: return TEXT("비");
        case ELLCoreWeatherSummary::Snow: return TEXT("눈");
        case ELLCoreWeatherSummary::Fog: return TEXT("안개");
        case ELLCoreWeatherSummary::Storm: return TEXT("폭풍");
        case ELLCoreWeatherSummary::Heat: return TEXT("폭염");
        case ELLCoreWeatherSummary::Cold: return TEXT("한파");
    }
    return TEXT("날씨 미상");
}

FString ULLObserverTimeWeatherOverlay::SpeedLabel(ELLSimulationSpeedPreset Preset)
{
    switch (Preset)
    {
        case ELLSimulationSpeedPreset::Paused: return TEXT("정지");
        case ELLSimulationSpeedPreset::Observe: return TEXT("1x");
        case ELLSimulationSpeedPreset::Fast: return TEXT("4x");
        case ELLSimulationSpeedPreset::Faster: return TEXT("16x");
        case ELLSimulationSpeedPreset::Rapid: return TEXT("64x");
    }
    return TEXT("1x");
}

void ULLObserverTimeWeatherOverlay::RefreshStatus(bool bForce)
{
    UGameInstance* GameInstance = GetGameInstance();
    ULLSimulationSubsystem* Simulation = GameInstance
        ? GameInstance->GetSubsystem<ULLSimulationSubsystem>()
        : nullptr;
    ULLCoreBridgeSubsystem* Bridge = GameInstance
        ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>()
        : nullptr;

    const ELLSimulationSpeedPreset SpeedPreset = Simulation
        ? Simulation->GetSimulationSpeedPreset()
        : ELLSimulationSpeedPreset::Observe;

    if (!Bridge || !Bridge->IsCoreRunning())
    {
        if (StatusText && (bForce || !bHasRenderedStatus || LastSpeedPreset != SpeedPreset))
        {
            StatusText->SetText(FText::FromString(
                FString::Printf(TEXT("LifeLens 준비 중 · %s"), *SpeedLabel(SpeedPreset))));
            RefreshButtonState(SpeedPreset);
            LastSpeedPreset = SpeedPreset;
            bHasRenderedStatus = true;
        }
        return;
    }

    const FLLCoreTimeObservation Time = Bridge->GetTimeObservation();
    const FLLCoreDynamicEnvironmentObservation Weather =
        Bridge->GetInitialRegionDynamicEnvironmentObservation();

    const bool bChanged = bForce
        || !bHasRenderedStatus
        || Time.SimulationMinute != LastSimulationMinute
        || SpeedPreset != LastSpeedPreset
        || Weather.bAvailable != bLastWeatherAvailable
        || (Weather.bAvailable && Weather.WeatherSummary != LastWeatherSummary)
        || (Weather.bAvailable && !FMath::IsNearlyEqual(Weather.AirTemperatureC, LastTemperatureC, 0.05f));
    if (!bChanged)
    {
        return;
    }

    const FString WeatherText = Weather.bAvailable
        ? FString::Printf(TEXT("%s %.1f°C"), *WeatherLabel(Weather.WeatherSummary), Weather.AirTemperatureC)
        : FString(TEXT("날씨 준비 중"));
    const FString DayPhase = Time.bIsNight ? TEXT("밤") : TEXT("낮");
    const int64 DisplayYear = FMath::Max<int64>(0, Time.YearIndex) + 1;
    const int32 DisplayDay = FMath::Max(0, Time.DayOfYear) + 1;

    if (StatusText)
    {
        StatusText->SetText(FText::FromString(FString::Printf(
            TEXT("%lld년 %d일 · %02d:%02d · %s · %s · %s · %s"),
            static_cast<long long>(DisplayYear),
            DisplayDay,
            Time.HourOfDay,
            Time.MinuteOfHour,
            *SeasonLabel(Time.Season),
            *DayPhase,
            *WeatherText,
            *SpeedLabel(SpeedPreset))));
    }

    RefreshButtonState(SpeedPreset);
    LastSimulationMinute = Time.SimulationMinute;
    LastSpeedPreset = SpeedPreset;
    bLastWeatherAvailable = Weather.bAvailable;
    LastWeatherSummary = Weather.WeatherSummary;
    LastTemperatureC = Weather.AirTemperatureC;
    bHasRenderedStatus = true;
}

void ULLObserverTimeWeatherOverlay::RefreshButtonState(ELLSimulationSpeedPreset Preset)
{
    const FLinearColor Active(0.12f, 0.42f, 0.72f, 0.95f);
    const FLinearColor Inactive(0.12f, 0.14f, 0.18f, 0.92f);

    const auto Apply = [Preset, &Active, &Inactive](UButton* Button, ELLSimulationSpeedPreset ButtonPreset)
    {
        if (Button)
        {
            Button->SetBackgroundColor(Preset == ButtonPreset ? Active : Inactive);
        }
    };

    Apply(PauseButton, ELLSimulationSpeedPreset::Paused);
    Apply(ObserveButton, ELLSimulationSpeedPreset::Observe);
    Apply(FastButton, ELLSimulationSpeedPreset::Fast);
    Apply(FasterButton, ELLSimulationSpeedPreset::Faster);
    Apply(RapidButton, ELLSimulationSpeedPreset::Rapid);
}

void ULLObserverTimeWeatherOverlay::ApplySpeedPreset(ELLSimulationSpeedPreset Preset)
{
    UGameInstance* GameInstance = GetGameInstance();
    ULLSimulationSubsystem* Simulation = GameInstance
        ? GameInstance->GetSubsystem<ULLSimulationSubsystem>()
        : nullptr;
    if (!Simulation)
    {
        return;
    }

    Simulation->SetSimulationSpeedPreset(Preset);
    RefreshStatus(true);
}

void ULLObserverTimeWeatherOverlay::HandlePauseClicked()
{
    ApplySpeedPreset(ELLSimulationSpeedPreset::Paused);
}

void ULLObserverTimeWeatherOverlay::HandleObserveClicked()
{
    ApplySpeedPreset(ELLSimulationSpeedPreset::Observe);
}

void ULLObserverTimeWeatherOverlay::HandleFastClicked()
{
    ApplySpeedPreset(ELLSimulationSpeedPreset::Fast);
}

void ULLObserverTimeWeatherOverlay::HandleFasterClicked()
{
    ApplySpeedPreset(ELLSimulationSpeedPreset::Faster);
}

void ULLObserverTimeWeatherOverlay::HandleRapidClicked()
{
    ApplySpeedPreset(ELLSimulationSpeedPreset::Rapid);
}

void ULLObserverTimeWeatherPresentationSubsystem::Tick(float DeltaTime)
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

    OverlayWidget = CreateWidget<ULLObserverTimeWeatherOverlay>(
        PlayerController,
        ULLObserverTimeWeatherOverlay::StaticClass());
    if (!OverlayWidget)
    {
        return;
    }

    OverlayWidget->AddToViewport(65);
}

TStatId ULLObserverTimeWeatherPresentationSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(ULLObserverTimeWeatherPresentationSubsystem, STATGROUP_Tickables);
}

void ULLObserverTimeWeatherPresentationSubsystem::Deinitialize()
{
    if (OverlayWidget)
    {
        OverlayWidget->RemoveFromParent();
        OverlayWidget = nullptr;
    }
    Super::Deinitialize();
}
