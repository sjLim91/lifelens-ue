#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/LLTypes.h"
#include "LLSimulationSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLLSimulationStateChanged);

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

    UFUNCTION(BlueprintCallable, Category="LifeLens|Simulation")
    bool FindResidentById(FGuid ResidentId, FLLResidentData& OutResident) const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Simulation")
    bool ApplyActionOutcome(FGuid ResidentId, ELLActionIntent Intent, float Strength = 1.0f);

    UFUNCTION(BlueprintCallable, Category="LifeLens|Social")
    bool GetRelationship(FGuid A, FGuid B, FLLRelationshipData& OutRelationship) const;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Social")
    bool ApplySocialInteraction(FGuid A, FGuid B, float AffinityDelta, float TrustDelta, float RomanceDelta);

    UPROPERTY(BlueprintAssignable, Category="LifeLens|Simulation")
    FLLSimulationStateChanged OnSimulationStateChanged;

private:
    void GenerateInitialPopulation();
    FLLResidentData GenerateAdult(FRandomStream& Random, ELLSex Sex, TSet<FString>& UsedNames);
    void GenerateInitialRelationships();
    FLLResidentData* FindMutableResident(FGuid ResidentId);
    static FGuid MakeDeterministicGuid(FRandomStream& Random);
    static float RollPercent(FRandomStream& Random, float Min = 15.0f, float Max = 85.0f);

    UPROPERTY()
    int32 WorldSeed = 0;

    UPROPERTY()
    int64 SimulationMinute = 0;

    UPROPERTY()
    TArray<FLLResidentData> Residents;

    UPROPERTY()
    TArray<FLLRelationshipData> Relationships;
};
