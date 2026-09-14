#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Simulation/LLCoreReadTypes.h"
#include "LLCoreBridgeSubsystem.generated.h"

namespace lifelens { class Simulation; }

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLLCoreRuntimeStateChanged);

/**
 * Read-only Unreal adapter for the pure C++ LifeLensCore simulation.
 *
 * The bridge intentionally does not expose Core implementation types to UHT or
 * Blueprint. Character names are display data only; a deterministic Unreal
 * FGuid is derived from (world seed, Core CharacterId) and remains stable for
 * that simulated world.
 *
 * The Core simulation owns its relationship and family-state books. This
 * adapter only projects those authoritative states into read-only Unreal DTOs;
 * UI code never mutates Core state through the observer API.
 * Family/world reads remain observer-only so UI consumers cannot become state owners.
 */
UCLASS()
class LIFELENS_API ULLCoreBridgeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Observer")
    void StartCoreObserverDemo(int32 Seed = 42, bool bSocialDemo = true);

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Observer")
    void StopCoreObserverDemo();

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Observer")
    void AdvanceCoreMinutes(int32 Minutes = 1);

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

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core|Observer")
    bool GetFamilyObservation(FGuid ResidentId, FLLCoreFamilyObservation& OutObservation) const;

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

    // Adapter-only identity index. Core remains independent from Unreal types.
    TMap<uint64, FGuid> CoreToGuid;
    TMap<FGuid, uint64> GuidToCore;

    UPROPERTY()
    TArray<FString> RecentEvents;

    static constexpr int32 MaxRecentEvents = 32;
};
