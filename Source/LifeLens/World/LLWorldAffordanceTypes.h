#pragma once

#include "CoreMinimal.h"
#include "LLWorldAffordanceTypes.generated.h"

/**
 * How sophisticated the currently selected world affordance is.
 * Lower values are preferred. Emergency is a no-tool fallback and never
 * creates a world object implicitly.
 */
UENUM(BlueprintType)
enum class ELLWorldAffordanceTier : uint8
{
    Preferred = 0,
    Primitive = 1,
    Natural = 2,
    Emergency = 3,
    Unavailable = 255
};
