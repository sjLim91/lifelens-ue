#include "UI/LLRuntimeObserverHUD.h"

#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLEnvironmentReadTypes.h"
#include "Simulation/LLTimeReadTypes.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"

namespace
{
    constexpr ELLSimulationSpeedPreset RuntimeSpeedPresets[] =
    {
        ELLSimulationSpeedPreset::Paused,
        ELLSimulationSpeedPreset::Observe,
        ELLSimulationSpeedPreset::Fast,
        ELLSimulationSpeedPreset::Faster,
        ELLSimulationSpeedPreset::Rapid
    };

    const TCHAR* RuntimeSpeedLabel(ELLSimulationSpeedPreset Preset)
    {
        switch (Preset)
        {
            case ELLSimulationSpeedPreset::Paused: return TEXT("0x");
            case ELLSimulationSpeedPreset::Observe: return TEXT("1x");
            case ELLSimulationSpeedPreset::Fast: return TEXT("4x");
            case ELLSimulationSpeedPreset::Faster: return TEXT("16x");
            case ELLSimulationSpeedPreset::Rapid: return TEXT("64x");
            default: return TEXT("?");
        }
    }

    const TCHAR* RuntimeSeasonLabel(ELLCoreSeasonSummary Season)
    {
        switch (Season)
        {
            case ELLCoreSeasonSummary::Spring: return TEXT("봄");
            case ELLCoreSeasonSummary::Summer: return TEXT("여름");
            case ELLCoreSeasonSummary::Autumn: return TEXT("가을");
            case ELLCoreSeasonSummary::Winter: return TEXT("겨울");
            default: return TEXT("계절 미상");
        }
    }

    const TCHAR* RuntimeWeatherLabel(ELLCoreWeatherSummary Weather)
    {
        switch (Weather)
        {
            case ELLCoreWeatherSummary::Clear: return TEXT("맑음");
            case ELLCoreWeatherSummary::Cloudy: return TEXT("흐림");
            case ELLCoreWeatherSummary::Rain: return TEXT("비");
            case ELLCoreWeatherSummary::Snow: return TEXT("눈");
            case ELLCoreWeatherSummary::Fog: return TEXT("안개");
            case ELLCoreWeatherSummary::Storm: return TEXT("폭풍");
            case ELLCoreWeatherSummary::Heat: return TEXT("더위");
            case ELLCoreWeatherSummary::Cold: return TEXT("추위");
            default: return TEXT("날씨 미상");
        }
    }

    bool RuntimeRectContains(const FBox2D& Rect, const FVector2D& Point)
    {
        return Rect.bIsValid && Rect.IsInside(Point);
    }

    UFont* RuntimeHUDFont()
    {
        return GEngine ? GEngine->GetSmallFont() : nullptr;
    }
}

void ALLRuntimeObserverHUD::DrawHUD()
{
    Super::DrawHUD();

    for (FBox2D& Rect : SpeedButtonRects)
    {
        Rect = FBox2D(ForceInit);
    }

    if (!Canvas || !GetWorld())
    {
        return;
    }

    DrawRuntimeChrome();
}

bool ALLRuntimeObserverHUD::HandleTap(const FVector2D& ScreenPosition, const FVector2D& ViewportSize)
{
    const FVector2D CanvasPosition = ViewportToCanvas(ScreenPosition, ViewportSize);
    for (int32 Index = 0; Index < SpeedButtonCount; ++Index)
    {
        if (!RuntimeRectContains(SpeedButtonRects[Index], CanvasPosition))
        {
            continue;
        }

        UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
        ULLSimulationSubsystem* Simulation = GameInstance
            ? GameInstance->GetSubsystem<ULLSimulationSubsystem>()
            : nullptr;
        if (Simulation)
        {
            Simulation->SetSimulationSpeedPreset(RuntimeSpeedPresets[Index]);
        }
        return true;
    }

    return Super::HandleTap(ScreenPosition, ViewportSize);
}

void ALLRuntimeObserverHUD::DrawRuntimeChrome()
{
    UFont* Font = RuntimeHUDFont();
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    ULLSimulationSubsystem* Simulation = GameInstance
        ? GameInstance->GetSubsystem<ULLSimulationSubsystem>()
        : nullptr;
    ULLCoreBridgeSubsystem* Bridge = GameInstance
        ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>()
        : nullptr;
    if (!Font || !Simulation || !Bridge || !Bridge->IsCoreRunning())
    {
        return;
    }

    const float UIScale = FMath::Clamp(
        FMath::Min(Canvas->ClipX, Canvas->ClipY) / 540.0f,
        1.0f,
        2.5f);
    const FSafeInsets Insets = SafeInsets(UIScale);

    const FLLCoreTimeObservation Time = Bridge->GetTimeObservation();
    const FLLCoreDynamicEnvironmentObservation Environment =
        Bridge->GetInitialRegionDynamicEnvironmentObservation();

    const int64 DisplayYear = FMath::Max<int64>(1, Time.YearIndex + 1);
    const int32 DisplayDay = FMath::Max(1, Time.DayOfYear + 1);
    FString StatusLine;
    if (Environment.bAvailable)
    {
        StatusLine = FString::Printf(
            TEXT("%lld년 %d일  %02d:%02d · %s · %s · %.1f°C"),
            static_cast<long long>(DisplayYear),
            DisplayDay,
            Time.HourOfDay,
            Time.MinuteOfHour,
            RuntimeSeasonLabel(Time.Season),
            RuntimeWeatherLabel(Environment.WeatherSummary),
            Environment.AirTemperatureC);
    }
    else
    {
        StatusLine = FString::Printf(
            TEXT("%lld년 %d일  %02d:%02d · %s"),
            static_cast<long long>(DisplayYear),
            DisplayDay,
            Time.HourOfDay,
            Time.MinuteOfHour,
            RuntimeSeasonLabel(Time.Season));
    }

    const float TextScale = 0.82f * UIScale;
    const float ButtonTextScale = 0.78f * UIScale;
    const float PadX = 10.0f * UIScale;
    const float PadY = 7.0f * UIScale;
    const float Gap = 6.0f * UIScale;
    const float ButtonGap = 4.0f * UIScale;
    const float ButtonWidth = 48.0f * UIScale;
    const float ButtonHeight = 32.0f * UIScale;

    float StatusW = 0.0f;
    float StatusH = 0.0f;
    GetTextSize(StatusLine, StatusW, StatusH, Font, TextScale);

    const float ButtonsWidth = ButtonWidth * SpeedButtonCount
        + ButtonGap * (SpeedButtonCount - 1);
    const float ContentWidth = FMath::Max(StatusW, ButtonsWidth);
    const float PanelWidth = FMath::Min(
        ContentWidth + PadX * 2.0f,
        FMath::Max(0.0f, Canvas->ClipX - Insets.Left - Insets.Right - 8.0f * UIScale));
    const float PanelHeight = PadY + StatusH + Gap + ButtonHeight + PadY;
    const float PanelX = FMath::Max(
        Insets.Left,
        Canvas->ClipX - Insets.Right - PanelWidth - 8.0f * UIScale);
    const float PanelY = FMath::Max(Insets.Top, 8.0f * UIScale);

    DrawRect(
        FLinearColor(0.015f, 0.02f, 0.03f, 0.84f),
        PanelX,
        PanelY,
        PanelWidth,
        PanelHeight);

    DrawText(
        StatusLine,
        FLinearColor(0.94f, 0.96f, 1.0f, 0.96f),
        PanelX + PadX,
        PanelY + PadY,
        Font,
        TextScale,
        false);

    const ELLSimulationSpeedPreset ActivePreset = Simulation->GetSimulationSpeedPreset();
    const float ButtonsY = PanelY + PadY + StatusH + Gap;
    const float ButtonsX = PanelX + PadX;

    for (int32 Index = 0; Index < SpeedButtonCount; ++Index)
    {
        const ELLSimulationSpeedPreset Preset = RuntimeSpeedPresets[Index];
        const float X = ButtonsX + Index * (ButtonWidth + ButtonGap);
        const bool bActive = Preset == ActivePreset;
        const FLinearColor Background = bActive
            ? FLinearColor(0.15f, 0.36f, 0.56f, 0.96f)
            : FLinearColor(0.07f, 0.09f, 0.13f, 0.92f);
        const FLinearColor Foreground = bActive
            ? FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)
            : FLinearColor(0.78f, 0.84f, 0.92f, 0.92f);

        DrawRect(Background, X, ButtonsY, ButtonWidth, ButtonHeight);
        SpeedButtonRects[Index] = FBox2D(
            FVector2D(X, ButtonsY),
            FVector2D(X + ButtonWidth, ButtonsY + ButtonHeight));

        const FString Label(RuntimeSpeedLabel(Preset));
        float LabelW = 0.0f;
        float LabelH = 0.0f;
        GetTextSize(Label, LabelW, LabelH, Font, ButtonTextScale);
        DrawText(
            Label,
            Foreground,
            X + (ButtonWidth - LabelW) * 0.5f,
            ButtonsY + (ButtonHeight - LabelH) * 0.5f,
            Font,
            ButtonTextScale,
            false);
    }
}
