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

UENUM(BlueprintType)
enum class ELLCoreFacilityBuildAction : uint8
{
    None,
    Plan,
    DeliverMaterial,
    Work,
    Fuel,
    Ignite,
    CollectCharcoal
};

UENUM(BlueprintType)
enum class ELLCoreToolCapability : uint8
{
    None,
    Cut,
    Chop,
    Dig,
    Strike,
    Carry,
    Heat
};

UENUM(BlueprintType)
enum class ELLCoreContextActionKind : uint8
{
    None,
    Social,
    Civilization,
    Parenting
};

UENUM(BlueprintType)
enum class ELLCoreParentingAction : uint8
{
    None,
    Feed,
    PutToSleep,
    Bathe,
    ToiletAssist,
    Hold,
    Play,
    Educate,
    Discipline,
    Comfort,
    HealthCare
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

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Context")
    ELLCoreContextActionKind ContextActionKind = ELLCoreContextActionKind::None;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Context")
    int64 ContextActionToken = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Context")
    int32 ContextActionDurationTicks = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Parenting")
    ELLCoreParentingAction ParentingAction = ELLCoreParentingAction::None;

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
    ELLCoreFacilityBuildAction CivilizationFacilityAction = ELLCoreFacilityBuildAction::None;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization")
    int64 CivilizationFacilityId = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization")
    ELLCoreFacilityKind CivilizationFacilityKind = ELLCoreFacilityKind::PrimitiveStorage;

    // Read-only projection of the exact Core tool candidate that the pending
    // Gather ACK will use. Presentation may display it but cannot grant its
    // efficiency or durability effect.
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization|Tool")
    bool bHasCivilizationTool = false;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization|Tool")
    ELLCoreItemKind CivilizationToolItem = ELLCoreItemKind::RawMaterial;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization|Tool")
    ELLCoreToolCapability CivilizationToolCapability = ELLCoreToolCapability::None;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization|Tool")
    float CivilizationToolQuality = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization|Tool")
    float CivilizationToolDurability = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization|Tool")
    float CivilizationToolQuantityMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Civilization")
    bool bCivilizationActionSucceeded = false;

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
