#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/LLSimulationSubsystem.h"
#include "Simulation/LLEnvironmentReadTypes.h"
#include "Simulation/LLTimeReadTypes.h"
#include "LLObserverTimeWeatherOverlay.generated.h"

class UBorder;
class UButton;
class UProgressBar;
class UTextBlock;

/**
 * Compact observer chrome for authoritative calendar/weather readout and
 * presentation-only simulation speed control.
 *
 * Calendar/weather values are read from LifeLensCore DTOs. Speed is runtime
 * control state owned by ULLSimulationSubsystem; this widget never writes Core
 * simulation truth, resident state, weather or calendar values.
 */
UCLASS()
class LIFELENS_API ULLObserverTimeWeatherOverlay : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void RefreshStatus(bool bForce = false);
    void ApplySpeedPreset(ELLSimulationSpeedPreset Preset);
    void RefreshButtonState(ELLSimulationSpeedPreset Preset);

    static FString SeasonLabel(ELLCoreSeasonSummary Season);
    static FString WeatherLabel(ELLCoreWeatherSummary Weather);
    static FString SpeedLabel(ELLSimulationSpeedPreset Preset);

    UFUNCTION() void HandlePauseClicked();
    UFUNCTION() void HandleObserveClicked();
    UFUNCTION() void HandleFastClicked();
    UFUNCTION() void HandleFasterClicked();
    UFUNCTION() void HandleRapidClicked();

    UPROPERTY(Transient) TObjectPtr<UBorder> ControlBorder;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> WeatherText;
    UPROPERTY(Transient) TObjectPtr<UProgressBar> DayProgressBar;
    UPROPERTY(Transient) TObjectPtr<UButton> PauseButton;
    UPROPERTY(Transient) TObjectPtr<UButton> ObserveButton;
    UPROPERTY(Transient) TObjectPtr<UButton> FastButton;
    UPROPERTY(Transient) TObjectPtr<UButton> FasterButton;
    UPROPERTY(Transient) TObjectPtr<UButton> RapidButton;

    int64 LastSimulationMinute = MIN_int64;
    ELLSimulationSpeedPreset LastSpeedPreset = ELLSimulationSpeedPreset::Observe;
    ELLCoreWeatherSummary LastWeatherSummary = ELLCoreWeatherSummary::Clear;
    float LastTemperatureC = 0.0f;
    bool bLastWeatherAvailable = false;
    bool bLastCoreRunning = false;
    bool bHasRenderedStatus = false;
};

/**
 * Auto-created world subsystem that mounts the observer time/weather controls
 * once a local player controller exists. The widget is independent of HUD
 * authority and only consumes existing read/control APIs.
 */
UCLASS()
class LIFELENS_API ULLObserverTimeWeatherPresentationSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual void Deinitialize() override;

private:
    UPROPERTY(Transient)
    TObjectPtr<ULLObserverTimeWeatherOverlay> OverlayWidget;
};
