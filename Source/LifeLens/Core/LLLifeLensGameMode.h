#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LLLifeLensGameMode.generated.h"

class UStaticMesh;

UCLASS(Config=Game, DefaultConfig)
class LIFELENS_API ALLLifeLensGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ALLLifeLensGameMode();

protected:
    virtual void BeginPlay() override;

private:
    void SpawnRuntimeFloor();
    void SpawnObserverCamera();

    UPROPERTY()
    TObjectPtr<UStaticMesh> RuntimeFloorMesh;

    // Observer framing is presentation tuning. DefaultGame.ini is the normal
    // source of truth; these C++ values are safe fallbacks if config is absent.
    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera", meta=(ClampMin="0.5"))
    float ObserverCameraDistanceChunks = 1.25f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera", meta=(ClampMin="0.5"))
    float ObserverCameraHeightChunks = 2.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera", meta=(ClampMin="0.0"))
    float ObserverCameraTargetHeightUU = 100.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category="LifeLens|Observer|Camera", meta=(ClampMin="30.0", ClampMax="90.0"))
    float ObserverCameraFOVDegrees = 55.0f;
};
