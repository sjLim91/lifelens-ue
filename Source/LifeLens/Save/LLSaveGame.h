#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/LLTypes.h"
#include "LLSaveGame.generated.h"

UCLASS()
class LIFELENS_API ULLSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, SaveGame)
    int32 SaveVersion = 1;

    UPROPERTY(BlueprintReadWrite, SaveGame)
    int32 WorldSeed = 0;

    UPROPERTY(BlueprintReadWrite, SaveGame)
    int64 SimulationMinute = 0;

    UPROPERTY(BlueprintReadWrite, SaveGame)
    TArray<FLLResidentData> Residents;

    UPROPERTY(BlueprintReadWrite, SaveGame)
    TArray<FLLRelationshipData> Relationships;
};
