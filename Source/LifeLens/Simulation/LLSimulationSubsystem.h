#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/LLTypes.h"
#include "LLSimulationSubsystem.generated.h"

class ULLCoreBridgeSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLLSimulationStateChanged);

/**
 * Compatibility/runtime projection used by the current WorldDirector.
 *
 * Production NEW GAME identity/state is authored by LifeLensCore through
 * ULLCoreBridgeSubsystem. This subsystem no longer randomizes a second founder
 * population. Its FLLResidentData/FLLRelationshipData arrays are projections
 * retained while the physical WorldDirector is migrated to Core DTOs.
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

    // Transitional physical-world compatibility. These mutate only the local
    // projection; the next Core tick refreshes authoritative Needs/relationships.
    UFUNCTION(BlueprintCallable, Category="LifeLens|Simulation")
    bool ApplyActionOutcome(FGuid ResidentId, ELLActionIntent Intent, float Strength = 1.0f);

    UFUNCTION(BlueprintCallable, Category="LifeLens|Social")
    bool GetRelationship(FGuid A, FGuid B, FLLRelationshipData& OutRelationship) const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Social")
    bool ApplySocialInteraction(FGuid A, FGuid B, float AffinityDelta, float TrustDelta, float RomanceDelta);

    UPROPERTY(BlueprintAssignable, Category="LifeLens|Simulation")
    FLLSimulationStateChanged OnSimulationStateChanged;

private:
    ULLCoreBridgeSubsystem* GetCoreBridge() const;
    bool RefreshProjectionFromCore();
    FLLResidentData* FindMutableResident(FGuid ResidentId);

    UPROPERTY()
    int32 WorldSeed = 0;

    UPROPERTY()
    int64 SimulationMinute = 0;

    UPROPERTY()
    bool bCoreAuthoritativeRuntime = false;

    UPROPERTY()
    TArray<FLLResidentData> Residents;

    UPROPERTY()
    TArray<FLLRelationshipData> Relationships;
};
