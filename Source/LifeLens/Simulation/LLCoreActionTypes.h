#pragma once

#include "CoreMinimal.h"
#include "Simulation/LLCoreReadTypes.h"
#include "LLCoreActionTypes.generated.h"

UENUM(BlueprintType)
enum class ELLCorePhysicalIntent : uint8
{
    None,
    Eat,
    Drink,
    Sleep,
    Toilet,
    Hygiene
};

UENUM(BlueprintType)
enum class ELLCoreSocialIntent : uint8
{
    None,
    Approach,
    Avoid,
    Repair,
    Comfort
};

/**
 * Typed execution directive projected from the authoritative LifeLensCore
 * runtime. Display labels are deliberately excluded from decision authority;
 * Unreal presentation consumes these enums and the stable resident target.
 */
USTRUCT(BlueprintType)
struct FLLCoreActionDirective
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action")
    ELLCoreObservedActivityKind ActivityKind = ELLCoreObservedActivityKind::Idle;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action")
    ELLCorePhysicalIntent PhysicalIntent = ELLCorePhysicalIntent::None;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action")
    ELLCoreSocialIntent SocialIntent = ELLCoreSocialIntent::None;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action")
    FGuid TargetResidentId;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action")
    bool bAlive = true;
};
