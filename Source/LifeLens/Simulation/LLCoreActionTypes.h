#pragma once

#include "CoreMinimal.h"
#include "Simulation/LLCivilizationReadTypes.h"
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

UENUM(BlueprintType)
enum class ELLCoreCivilizationAction : uint8
{
    None,
    Gather,
    Store,
    Experiment,
    Craft
};

/**
 * Typed execution directive projected from the authoritative LifeLensCore
 * runtime. Display labels are deliberately excluded from decision authority;
 * Unreal presentation consumes these enums and stable target/provenance data.
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

    // Civilization work is orthogonal to the legacy Idle/Physical/Social
    // observer activity enum. A non-None value means a real Core civilization
    // action executed and its short-lived presentation context is still active.
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization")
    ELLCoreCivilizationAction CivilizationAction = ELLCoreCivilizationAction::None;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization")
    ELLCoreMaterialKind CivilizationMaterial = ELLCoreMaterialKind::Unknown;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization")
    ELLCoreItemKind CivilizationItem = ELLCoreItemKind::RawMaterial;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization")
    ELLCoreTechniqueId CivilizationTechnique = ELLCoreTechniqueId::None;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization")
    int32 CivilizationQuantity = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization")
    int64 CivilizationActionMinute = -1;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization")
    int64 CivilizationResourceNodeId = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization")
    int64 CivilizationStorageId = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization")
    bool bCivilizationActionSucceeded = false;

    // Ordinary ResourceNode/Storage records do not yet own world-grid positions,
    // so Presentation must not invent scenery targets. This is true only when
    // Core actually authored a position (currently sanitation-site work).
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization")
    bool bHasCivilizationSpatialTarget = false;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization")
    int32 CivilizationTargetGridX = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization")
    int32 CivilizationTargetGridY = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization")
    int64 CivilizationSanitationSiteId = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action")
    bool bAlive = true;
};
