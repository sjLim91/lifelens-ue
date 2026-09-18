#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/LLTypes.h"
#include "Simulation/LLCoreActionTypes.h"
#include "World/LLWorldAffordanceTypes.h"
#include "World/LLWorldSpatialContract.h"
#include "LLWorldDirector.generated.h"

class ALLActivityAnchor;
class ALLResidentCharacter;
class ULLSimulationSubsystem;
class ULLCoreBridgeSubsystem;
class USceneComponent;
class ULLEnvironmentalResidueVisualizerComponent;

struct FLLResidentRuntimeState
{
    bool bInitialized = false;
    bool bPerformingAction = false;
    float PhysicalUseElapsedSeconds = 0.0f;
    int64 ActiveContextActionToken = 0;
    float ContextUseElapsedSeconds = 0.0f;
    ELLCoreObservedActivityKind LastActivityKind = ELLCoreObservedActivityKind::Idle;
    ELLCorePhysicalIntent LastPhysicalIntent = ELLCorePhysicalIntent::None;
    ELLCoreSocialIntent LastSocialIntent = ELLCoreSocialIntent::None;
    FGuid LastTargetId;
    TWeakObjectPtr<ALLActivityAnchor> ReservedAnchor;
    ELLActionIntent ReservedIntent = ELLActionIntent::Idle;
    ELLWorldAffordanceTier ActiveAffordanceTier = ELLWorldAffordanceTier::Unavailable;
    bool bUsingEmergencyFallback = false;
    bool bUsingDesignatedSanitationSite = false;
    int64 CoreSanitationSiteId = 0;
    FTransform EmergencyUseTransform = FTransform::Identity;
};

UCLASS(Config=Game, DefaultConfig)
class LIFELENS_API ALLWorldDirector : public AActor
{
    GENERATED_BODY()

public:
    ALLWorldDirector();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category="LifeLens|World")
    ALLResidentCharacter* FindResidentActor(FGuid ResidentId) const;

    UFUNCTION(BlueprintPure, Category="LifeLens|World")
    ELLActionIntent GetResidentIntent(FGuid ResidentId) const;

    UFUNCTION(BlueprintPure, Category="LifeLens|World")
    ELLWorldAffordanceTier GetResidentAffordanceTier(FGuid ResidentId) const;

    UFUNCTION(BlueprintPure, Category="LifeLens|World")
    bool IsResidentUsingEmergencyFallback(FGuid ResidentId) const;

    // Presentation truth signal: true only while WorldDirector is actually
    // executing a Core Physical activity at its resolved use point.
    UFUNCTION(BlueprintPure, Category="LifeLens|World")
    bool IsResidentPerformingPhysicalAction(FGuid ResidentId) const;

    UFUNCTION(BlueprintPure, Category="LifeLens|World|Environment")
    int32 GetEnvironmentalResidueVisualCount() const;

protected:
    virtual void BeginPlay() override;

private:
    void RefreshCorePresentationOrigin();
    void CollectActivityAnchors();
    void SpawnResidents();
    void UpdateResident(ALLResidentCharacter& Character, float DeltaSeconds);
    void ApplyPendingContextDirective(
        ALLResidentCharacter& Character,
        FLLResidentRuntimeState& Runtime,
        const FLLCoreActionDirective& Directive,
        float DeltaSeconds);
    void ApplyCoreDirective(
        ALLResidentCharacter& Character,
        FLLResidentRuntimeState& Runtime,
        const FLLCoreActionDirective& Directive,
        float DeltaSeconds);
    ELLActionIntent ToPresentationIntent(ELLCorePhysicalIntent Intent) const;
    ALLActivityAnchor* FindBestUsableAnchor(
        const ALLResidentCharacter& Character,
        ELLActionIntent Intent,
        ELLWorldAffordanceTier& OutTier) const;
    ALLActivityAnchor* EnsurePhysicalReservation(
        ALLResidentCharacter& Character,
        FLLResidentRuntimeState& Runtime,
        ELLActionIntent Intent);
    bool EnsureEmergencyFallback(
        const ALLResidentCharacter& Character,
        FLLResidentRuntimeState& Runtime,
        ELLActionIntent Intent,
        FTransform& OutUseTransform) const;
    bool SupportsEmergencyFallback(ELLActionIntent Intent) const;
    FTransform ResolveEmergencyFallbackTransform(
        const ALLResidentCharacter& Character,
        ELLActionIntent Intent) const;
    FIntPoint WorldLocationToCoreGrid(const FVector& WorldLocation) const;
    FVector CoreGridToWorldSpawnLocation(int32 GridX, int32 GridY, int32 PresentationSlot) const;
    void ReleasePhysicalReservation(FGuid ResidentId, FLLResidentRuntimeState& Runtime);
    FVector ResolveSocialTargetLocation(
        const ALLResidentCharacter& Character,
        const ALLResidentCharacter& Target,
        ELLCoreSocialIntent SocialIntent) const;

    UPROPERTY(VisibleAnywhere, Category="LifeLens|World")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category="LifeLens|World|Environment")
    TObjectPtr<ULLEnvironmentalResidueVisualizerComponent> EnvironmentalResidueVisualizer;

    UPROPERTY()
    TObjectPtr<ULLSimulationSubsystem> Simulation;

    UPROPERTY()
    TObjectPtr<ULLCoreBridgeSubsystem> CoreBridge;

    UPROPERTY()
    TArray<TObjectPtr<ALLResidentCharacter>> SpawnedResidents;

    UPROPERTY()
    TArray<TObjectPtr<ALLActivityAnchor>> ActivityAnchors;

    TMap<FGuid, FLLResidentRuntimeState> RuntimeStates;

    float SimulationClockAccumulator = 0.0f;
    float EnvironmentalVisualRefreshAccumulator = 0.0f;
    FIntPoint CorePresentationOriginGrid = FIntPoint::ZeroValue;

    // Runtime tuning belongs in DefaultGame.ini so normal iteration does not
    // require recompiling the WorldDirector. C++ defaults are safety fallbacks
    // when a config key cannot be resolved; loaded config values override them.
    UPROPERTY(Config, EditAnywhere, Category="LifeLens|Environment|Visual", meta=(ClampMin="0.05"))
    float EnvironmentalVisualRefreshIntervalSeconds = 0.25f;

    // Canonical Observe speed: 480 real seconds / 1440 simulation minutes.
    UPROPERTY(Config, EditAnywhere, Category="LifeLens|Time", meta=(ClampMin="0.01"))
    float RealSecondsPerSimulationMinute = 0.333333f;

    // Prevent long hitches / high speed from turning one render frame into an
    // unbounded Core catch-up loop. Backlog remains queued in the accumulator.
    UPROPERTY(Config, EditAnywhere, Category="LifeLens|Time", meta=(ClampMin="1", ClampMax="512"))
    int32 MaxSimulationMinutesPerFrame = 32;

    UPROPERTY(Config, EditAnywhere, Category="LifeLens|Action|Context", meta=(ClampMin="1.0"))
    float ContextWorldTargetArrivalRadiusUU = 165.0f;

    UPROPERTY(Config, EditAnywhere, Category="LifeLens|Action|Context", meta=(ClampMin="1.0"))
    float ContextResidentArrivalRadiusUU = 130.0f;

    // Spatial contract between Unreal presentation and Core GridPos. The
    // default is shared with bootstrap ground and WorldPresentation so the
    // same authoritative Core target maps to the same Unreal distance.
    UPROPERTY(EditAnywhere, Category="LifeLens|World", meta=(ClampMin="1.0"))
    float CoreGridCellSizeUU = LLWorldSpatialContract::GridCellSizeUU;
};
