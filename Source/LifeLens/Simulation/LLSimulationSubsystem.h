#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/LLTypes.h"
#include "LLSimulationSubsystem.generated.h"

class ULLCoreBridgeSubsystem;

UENUM(BlueprintType)
enum class ELLSimulationSpeedPreset : uint8
{
    Paused UMETA(DisplayName="Paused"),
    Observe UMETA(DisplayName="1x Observe"),
    Fast UMETA(DisplayName="4x Fast"),
    Faster UMETA(DisplayName="16x Faster"),
    Rapid UMETA(DisplayName="64x Rapid")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLLSimulationStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FLLSimulationSpeedChanged,
    ELLSimulationSpeedPreset,
    SpeedPreset);

/**
 * Compatibility/runtime projection used by the current WorldDirector.
 *
 * Production NEW GAME identity/state is authored by LifeLensCore through
 * ULLCoreBridgeSubsystem. This subsystem does not own decision or outcome
 * authority; FLLResidentData/FLLRelationshipData remain read-only projections
 * while presentation consumers migrate to direct Core DTOs.
 *
 * Observer time speed is intentionally runtime control state rather than Core
 * simulation truth. The Core minute remains authoritative and Save/Load never
 * fabricates or rewrites simulation time from the selected presentation speed.
 */
UCLASS()
class LIFELENS_API ULLSimulationSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Simulation")
    void NewGame(int32 OptionalSeed = 0);

    UFUNCTION(BlueprintCallable, Category="LifeLens|Simulation")
    bool SaveGame(const FString& SlotName = TEXT("LifeLens_Autosave"));

    UFUNCTION(BlueprintCallable, Category="LifeLens|Simulation")
    bool LoadGame(const FString& SlotName = TEXT("LifeLens_Autosave"));

    UFUNCTION(BlueprintCallable, Category="LifeLens|Simulation")
    void AdvanceSimulationMinutes(int32 Minutes);

    // Rebuild the compatibility/physical projection from the currently
    // installed Core runtime. Used when Core is replaced outside the normal
    // SimulationSubsystem NewGame/LoadGame entry points.
    bool SynchronizeProjectionFromCore();

    UFUNCTION(BlueprintCallable, Category="LifeLens|Simulation|Time")
    void SetSimulationSpeedPreset(ELLSimulationSpeedPreset NewPreset);

    UFUNCTION(BlueprintPure, Category="LifeLens|Simulation|Time")
    ELLSimulationSpeedPreset GetSimulationSpeedPreset() const { return SimulationSpeedPreset; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Simulation|Time")
    float GetSimulationSpeedMultiplier() const;

    UFUNCTION(BlueprintPure, Category="LifeLens|Simulation|Time")
    bool IsSimulationPaused() const { return SimulationSpeedPreset == ELLSimulationSpeedPreset::Paused; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Simulation")
    TArray<FLLResidentData> GetResidents() const { return Residents; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Simulation")
    TArray<FLLRelationshipData> GetRelationships() const { return Relationships; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Simulation")
    int32 GetWorldSeed() const { return WorldSeed; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Simulation")
    int64 GetSimulationMinute() const { return SimulationMinute; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Simulation")
    bool IsCoreAuthoritativeRuntime() const { return bCoreAuthoritativeRuntime; }

    UFUNCTION(BlueprintCallable, Category="LifeLens|Simulation")
    bool FindResidentById(FGuid ResidentId, FLLResidentData& OutResident) const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Social")
    bool GetRelationship(FGuid A, FGuid B, FLLRelationshipData& OutRelationship) const;

    UPROPERTY(BlueprintAssignable, Category="LifeLens|Simulation")
    FLLSimulationStateChanged OnSimulationStateChanged;

    UPROPERTY(BlueprintAssignable, Category="LifeLens|Simulation|Time")
    FLLSimulationSpeedChanged OnSimulationSpeedChanged;

private:
    static float SpeedMultiplierForPreset(ELLSimulationSpeedPreset Preset);

    ULLCoreBridgeSubsystem* GetCoreBridge() const;
    bool RefreshProjectionFromCore();

    UPROPERTY()
    int32 WorldSeed = 0;

    UPROPERTY()
    int64 SimulationMinute = 0;

    UPROPERTY()
    bool bCoreAuthoritativeRuntime = false;

    UPROPERTY()
    ELLSimulationSpeedPreset SimulationSpeedPreset = ELLSimulationSpeedPreset::Observe;

    UPROPERTY()
    TArray<FLLResidentData> Residents;

    UPROPERTY()
    TArray<FLLRelationshipData> Relationships;
};
