#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Simulation/LLCoreReadTypes.h"
#include "Simulation/LLCoreActionTypes.h"
#include "Simulation/LLCivilizationReadTypes.h"
#include "Simulation/LLEnvironmentReadTypes.h"
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
UCLASS()
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

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Action")
    bool CompleteResidentPhysicalAction(
        FGuid ResidentId,
        bool bEmergencyFallback,
        int32 ResolvedGridX,
        int32 ResolvedGridY);

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Observer")
    bool IsCoreRunning() const { return CoreSimulation != nullptr; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Observer")
    int32 GetRuntimeSeed() const { return ActiveSeed; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Observer")
    FLLCoreWorldObservation GetWorldObservation() const;

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Observer")
    TArray<FLLCoreResidentObservation> GetResidentObservations() const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Observer")
    bool GetResidentObservation(FGuid ResidentId, FLLCoreResidentObservation& OutObservation) const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Action")
    bool GetResidentActionDirective(FGuid ResidentId, FLLCoreActionDirective& OutDirective) const;

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

    // Native persistence bridge. Unreal SaveGame stores these bytes; it never
    // serializes the compatibility resident projection as a second authority.
    bool CaptureCoreSnapshotBytes(TArray<uint8>& OutBytes, FString& OutError) const;
    bool RestoreCoreSnapshotBytes(const TArray<uint8>& Bytes, FString& OutError);

    UFUNCTION(BlueprintPure, Category="LifeLens|Core|Observer")
    TArray<FString> GetRecentCoreEvents() const { return RecentEvents; }

    UPROPERTY(BlueprintAssignable, Category="LifeLens|Core|Observer")
    FLLCoreRuntimeStateChanged OnCoreRuntimeStateChanged;

private:
    void ResetRuntime();
    void RebuildGuidIndex();
    void PushCoreEvent(const FString& Line);
    FGuid MakeStableResidentGuid(uint64 CoreCharacterId) const;
    bool BuildResidentObservation(uint64 CoreCharacterId, FLLCoreResidentObservation& OutObservation) const;
    bool BuildFamilyObservation(uint64 CoreCharacterId, FLLCoreFamilyObservation& OutObservation) const;

    lifelens::Simulation* CoreSimulation = nullptr;
    int32 ActiveSeed = 0;

    TMap<uint64, FGuid> CoreToGuid;
    TMap<FGuid, uint64> GuidToCore;

    UPROPERTY()
    TArray<FString> RecentEvents;

    static constexpr int32 MaxRecentEvents = 32;
};