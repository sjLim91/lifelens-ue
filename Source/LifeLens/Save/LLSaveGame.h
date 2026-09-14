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
    int32 SaveVersion = 2;

    UPROPERTY(BlueprintReadWrite, SaveGame)
    int32 WorldSeed = 0;

    UPROPERTY(BlueprintReadWrite, SaveGame)
    int64 SimulationMinute = 0;

    /** Authoritative LifeLensCore snapshot. Required for SaveVersion >= 2. */
    UPROPERTY(SaveGame)
    TArray<uint8> CoreSnapshotBytes;

    // Legacy v1 compatibility only. New saves do not populate these arrays and
    // LoadGame never treats them as authoritative simulation state.
    UPROPERTY(BlueprintReadWrite, SaveGame)
    TArray<FLLResidentData> Residents;

    UPROPERTY(BlueprintReadWrite, SaveGame)
    TArray<FLLRelationshipData> Relationships;
};
