#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LLCoreBridgeSubsystem.generated.h"

namespace lifelens { class Simulation; }

UCLASS()
class LIFELENS_API ULLCoreBridgeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core")
    void StartFastTest(int32 Seed = 42);

    UFUNCTION(BlueprintCallable, Category="LifeLens|Core")
    void AdvanceCoreMinutes(int32 Minutes = 1);

    UFUNCTION(BlueprintPure, Category="LifeLens|Core")
    TArray<FString> GetRecentCoreEvents() const { return RecentEvents; }

    UFUNCTION(BlueprintPure, Category="LifeLens|Core")
    bool IsCoreRunning() const { return CoreSimulation != nullptr; }

private:
    void PushCoreEvent(const FString& Line);

    lifelens::Simulation* CoreSimulation = nullptr;

    UPROPERTY()
    TArray<FString> RecentEvents;

    static constexpr int32 MaxRecentEvents = 10;
};
