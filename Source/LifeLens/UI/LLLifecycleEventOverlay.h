#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/LLCoreReadTypes.h"
#include "LLLifecycleEventOverlay.generated.h"

class UBorder;
class UTextBlock;
class UVerticalBox;

/**
 * Read-only lifecycle presentation for the observer.
 *
 * v1 deliberately watches authoritative Core observer DTO transitions instead
 * of inventing lifecycle state in Presentation. It surfaces new births,
 * pregnancy starts, life-stage changes and deaths as short-lived notices.
 * The first observed snapshot is baseline only, so loading a save or opening
 * the observer never replays stale events as if they just happened.
 */
UCLASS()
class LIFELENS_API ULLLifecycleEventOverlay : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    struct FResidentVisualState
    {
        FString Name;
        ELLCoreLifeStage LifeStage = ELLCoreLifeStage::Adult;
        bool bAlive = true;
    };

    struct FRomanceVisualState
    {
        FString FirstName;
        FString SecondName;
        FGuid FirstResidentId;
        FGuid SecondResidentId;
        ELLCoreRomanceStage Stage = ELLCoreRomanceStage::None;
        bool bCohabiting = false;
    };

    struct FTransientNotice
    {
        FString Text;
        FGuid SubjectResidentId;
        FGuid RelatedResidentId;
        int64 SimulationMinute = 0;
        double ExpireAtRealSeconds = 0.0;
    };

    void ResetObservationState();
    void RefreshFromCore();
    void RefreshSelectedResidentCard();
    void PushNotice(const FString& Text, FGuid SubjectResidentId = FGuid(),
                    FGuid RelatedResidentId = FGuid(), int64 SimulationMinute = 0);
    void RefreshNoticeWidgets();
    static FString FormatObservedMoment(int64 SimulationMinute);

    static FString LifeStageLabel(ELLCoreLifeStage Stage);
    static FString RomanceStageLabel(ELLCoreRomanceStage Stage);
    static FString RomanceEventPrefix(ELLCoreRomanceStage Stage);
    static FString PregnancyPairKey(const FGuid& First, const FGuid& Second);

    UPROPERTY(Transient)
    TObjectPtr<UBorder> EventBorder;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> EventList;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> RootStack;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> ResidentStatusBorder;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ResidentStatusText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ResidentHistoryText;

    TMap<FGuid, FResidentVisualState> PreviousResidents;
    TSet<FString> PreviousPregnancyPairs;
    TMap<FString, FRomanceVisualState> PreviousRomancePairs;
    TArray<FTransientNotice> Notices;
    TMap<FGuid, TArray<FString>> ObservedLifeHistory;

    int64 LastObservedSimulationMinute = -1;
    bool bBaselineReady = false;

    static constexpr int32 MaxVisibleNotices = 4;
    static constexpr int32 MaxObservedHistoryPerResident = 16;
    static constexpr int32 MaxHistoryLinesOnCard = 4;
    static constexpr double NoticeLifetimeSeconds = 7.5;
};

/**
 * Auto-created world subsystem that mounts the lifecycle overlay once a local
 * observer player controller exists. No GameMode/HUD ownership is replaced.
 */
UCLASS()
class LIFELENS_API ULLLifecyclePresentationSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual void Deinitialize() override;

private:
    UPROPERTY(Transient)
    TObjectPtr<ULLLifecycleEventOverlay> OverlayWidget;
};
