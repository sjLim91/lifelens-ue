#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LLObserverHUD.generated.h"

UCLASS()
class LIFELENS_API ALLObserverHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};
