#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Simulation/LLCoreReadTypes.h"
#include "Simulation/LLCoreActionTypes.h"
#include "Simulation/LLCivilizationReadTypes.h"
#include "Simulation/LLEnvironmentReadTypes.h"
#include "Simulation/LLTimeReadTypes.h"
#include "Simulation/LLWorldGenerationReadTypes.h"
#include "LLCoreBridgeSubsystem.generated.h"

namespace lifelens { class Simulation; }

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLLCoreRuntimeStateChanged);

/**
 * Read-only Unreal adapter for the pure C++ LifeLensCore simulation.
 *
 * Character names are display data only. Stable Unreal identity is derived
 * from (WorldSeed, Core CharacterId). The production NEW GAME entry point is
 * StartCoreNewGame(); demo entry points remain available only for development.
 */
UCLASS(Config=Game, DefaultConfig)
class LIFELENS_API ULLCoreBridgeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Runtime")
    void StartCoreNewGame(int32 Seed);

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Observer")
    void StartCoreObserverDemo(int32 Seed = 42, bool bSocialDemo = true);

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Observer")
    void StopCoreObserverDemo();

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Observer")
    void AdvanceCoreMinutes(int32 Minutes = 1);

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Action")
    void SetExternalPhysicalExecutionEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Action")
    int32 GetPhysicalActionDurationTicks(
        ELLCorePhysicalIntent Intent,
        bool bEmergencyFallback,
        bool bDesignatedSanitationSite = false) const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Action")
    bool CompleteResidentPhysicalAction(
        FGuid ResidentId,
        bool bEmergencyFallback,
        int32 ResolvedGridX,
        int32 ResolvedGridY,
        int64 SanitationSiteId = 0);

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Action")
    bool CompleteResidentContextAction(
        FGuid ResidentId,
        int64 ContextActionToken,
        int32 ResolvedGridX,
        int32 ResolvedGridY);

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|World")
    bool GetResidentRuntimeGridPosition(
        FGuid ResidentId,
        int32& OutGridX,
        int32& OutGridY) const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Environment")
    bool GetRecommendedOutdoorReliefGridPosition(
        FGuid ResidentId,
        int32& OutGridX,
        int32& OutGridY) const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Environment")
    bool GetSanitationUseTarget(
        FGuid ResidentId,
        int32& OutGridX,
        int32& OutGridY,
        bool& bOutDesignatedSite,
        int64& OutSanitationSiteId) const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Action")
    bool GetSettlementSleepUseTarget(
        FGuid ResidentId,
        int32& OutGridX,
        int32& OutGridY,
        int64& OutFacilityId) const;

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Observer")
    bool IsCoreRunning() const { return CoreSimulation != nullptr; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Observer")
    int32 GetRuntimeSeed() const { return ActiveSeed; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Observer")
    FLLCoreWorldObservation GetWorldObservation() const;

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Time")
    FLLCoreTimeObservation GetTimeObservation() const;

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Observer")
    TArray<FLLCoreResidentObservation> GetResidentObservations() const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Observer")
    bool GetResidentObservation(FGuid ResidentId, FLLCoreResidentObservation& OutObservation) const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Observer")
    bool GetResidentTraitPreferenceObservation(
        FGuid ResidentId,
        FLLCoreTraitPreferenceObservation& OutObservation) const;

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Social")
    TArray<FLLCoreSocialEventObservation> GetRecentSocialEvents(int32 MaxEvents = 32) const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Action")
    bool GetResidentActionDirective(FGuid ResidentId, FLLCoreActionDirective& OutDirective) const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Action")
    bool GetResidentPendingContextDirective(
        FGuid ResidentId,
        FLLCoreActionDirective& OutDirective) const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Observer")
    bool GetFamilyObservation(FGuid ResidentId, FLLCoreFamilyObservation& OutObservation) const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Civilization")
    bool GetResidentCivilizationObservation(
        FGuid ResidentId,
        FLLCoreResidentCivilizationObservation& OutObservation) const;

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Civilization")
    FLLCoreCivilizationWorldObservation GetCivilizationWorldObservation(int32 MaxRecentDiscoveries = 16) const;

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Environment")
    FLLCoreEnvironmentObservation GetEnvironmentObservation(int32 MaxResidues = 64) const;

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Environment|Weather")
    FLLCoreDynamicEnvironmentObservation GetDynamicEnvironmentObservation(int32 ChunkX, int32 ChunkY) const;

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Environment|Weather")
    FLLCoreDynamicEnvironmentObservation GetInitialRegionDynamicEnvironmentObservation() const;

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Environment|SkyPresentation")
    FLLCoreSkyPresentationObservation GetInitialRegionSkyPresentationObservation() const;

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|WorldGeneration")
    FLLCoreWorldGenerationObservation GetWorldGenerationObservation() const;

    // Authoritative materialized natural chunks in Core registry order.
    // Consumers must use this instead of guessing coordinates from a count/radius.
    UFUNCTION(BlueprintPure, Category="LifeLens|Core|WorldGeneration")
    TArray<FLLCoreNaturalChunkObservation> GetMaterializedNaturalChunkObservations() const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|WorldGeneration")
    bool GetNaturalChunkObservation(
        int32 ChunkX,
        int32 ChunkY,
        FLLCoreNaturalChunkObservation& OutObservation) const;

    // Deterministic Core hydrology projected for authoritative materialized chunks.
    UFUNCTION(BlueprintPure, Category="LifeLens|Core|WorldGeneration|Hydrology")
    TArray<FLLCoreHydrologyObservation> GetMaterializedHydrologyObservations() const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|WorldGeneration|Hydrology")
    bool GetHydrologyObservation(
        int32 ChunkX,
        int32 ChunkY,
        FLLCoreHydrologyObservation& OutObservation) const;

    // Read-only deterministic geometry hints for projecting authoritative
    // materialized hydrology into Unreal Water/mesh presentation.
    UFUNCTION(BlueprintPure, Category="LifeLens|Core|WorldGeneration|Hydrology|Presentation")
    TArray<FLLCoreSurfaceWaterPresentationObservation> GetMaterializedSurfaceWaterPresentationObservations() const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|WorldGeneration|Hydrology|Presentation")
    bool GetSurfaceWaterPresentationObservation(
        int32 ChunkX,
        int32 ChunkY,
        FLLCoreSurfaceWaterPresentationObservation& OutObservation) const;

    // Native persistence bridge.
    // Unreal SaveGame stores these bytes; it never
    // serializes the compatibility resident projection as a second authority.
    bool CaptureCoreSnapshotBytes(TArray<uint8>& OutBytes, FString& OutError) const;
    bool RestoreCoreSnapshotBytes(const TArray<uint8>& Bytes, FString& OutError);

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Observer")
    TArray<FString> GetRecentCoreEvents() const { return RecentEvents; }

    UPROPERTY(BlueprintAssignable, Category="LifeLens|Core|Observer")
    FLLCoreRuntimeStateChanged OnCoreRuntimeStateChanged;

private:
    lifelens::Simulation* CreateConfiguredSimulation(uint64 CoreSeed) const;
    void ResetRuntime();
    void RebuildGuidIndex();
    void PushCoreEvent(const FString& Line);
    FGuid MakeStableResidentGuid(uint64 CoreCharacterId) const;
    bool BuildResidentObservation(uint64 CoreCharacterId, FLLCoreResidentObservation& OutObservation) const;
    bool BuildFamilyObservation(uint64 CoreCharacterId, FLLCoreFamilyObservation& OutObservation) const;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Core|Ruleset|Needs", meta=(ClampMin="0.0"))
    double NeedsHungerPerMinute = 0.0010;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Core|Ruleset|Needs", meta=(ClampMin="0.0"))
    double NeedsThirstPerMinute = 0.0013;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Core|Ruleset|Needs", meta=(ClampMin="0.0"))
    double NeedsSleepPerMinute = 0.0008;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Core|Ruleset|Needs", meta=(ClampMin="0.0"))
    double NeedsBladderPerMinute = 0.0011;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Core|Ruleset|Needs", meta=(ClampMin="0.0"))
    double NeedsHygienePerMinute = 0.0007;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Core|Ruleset|UtilityAI", meta=(ClampMin="0.01"))
    double UtilityNeedExponent = 4.0;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Core|Ruleset|UtilityAI", meta=(ClampMin="0.0", ClampMax="1.0"))
    double UtilityUrgentThreshold = 0.70;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Core|Ruleset|UtilityAI", meta=(ClampMin="0.0"))
    double UtilityUrgentSlope = 1.8;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Core|Ruleset|UtilityAI", meta=(ClampMin="0.0"))
    double UtilityIdleScore = 0.035;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Core|Ruleset|UtilityAI", meta=(ClampMin="0", ClampMax="23"))
    int32 UtilitySleepNightStartHour = 22;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Core|Ruleset|UtilityAI", meta=(ClampMin="0", ClampMax="23"))
    int32 UtilitySleepNightEndHour = 6;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Core|Ruleset|UtilityAI", meta=(ClampMin="0.0"))
    double UtilitySleepNightMultiplier = 1.35;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Core|Ruleset|UtilityAI", meta=(ClampMin="0.0"))
    double UtilityWashBaseMultiplier = 0.85;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Core|Ruleset|UtilityAI", meta=(ClampMin="0.0"))
    double UtilityWashConscientiousnessMultiplier = 0.35;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Core|Ruleset|UtilityAI", meta=(ClampMin="0.0"))
    double UtilitySleepBaseMultiplier = 0.90;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Core|Ruleset|UtilityAI", meta=(ClampMin="0.0"))
    double UtilitySleepIntroversionMultiplier = 0.25;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Core|Ruleset|UtilityAI", meta=(ClampMin="0.0", ClampMax="1.0"))
    double UtilitySecondChoiceProbability = 0.08;

    lifelens::Simulation* CoreSimulation = nullptr;
    int32 ActiveSeed = 0;

    TMap<uint64, FGuid> CoreToGuid;
    TMap<FGuid, uint64> GuidToCore;

    UPROPERTY()
    TArray<FString> RecentEvents;

    static constexpr int32 MaxRecentEvents = 32;
};