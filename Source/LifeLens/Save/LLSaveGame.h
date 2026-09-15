#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "LLSaveGame.generated.h"

UCLASS()
class LIFELENS_API ULLSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 CurrentSaveVersion = 3;

    UPROPERTY(SaveGame)
    int32 SaveVersion = CurrentSaveVersion;

    /** Complete authoritative LifeLensCore snapshot. */
    UPROPERTY(SaveGame)
    TArray<uint8> CoreSnapshotBytes;
};
