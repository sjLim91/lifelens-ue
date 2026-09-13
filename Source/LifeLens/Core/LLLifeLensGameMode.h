#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LLLifeLensGameMode.generated.h"

class UStaticMesh;

UCLASS()
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
};
