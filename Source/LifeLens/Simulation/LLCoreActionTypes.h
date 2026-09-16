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

    // Context actions are authoritative decisions whose outcome is still
    // pending. Unreal may move/animate them, but only the matching token ACK
    // allows Core to apply the result.
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Context")
    ELLCoreContextActionKind ContextActionKind = ELLCoreContextActionKind::None;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Context")
    int64 ContextActionToken = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Context")
    int32 ContextActionDurationTicks = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Action|Parenting")
    ELLCoreParentingAction ParentingAction = ELLCoreParentingAction::None;

    // Civilization work is orthogonal to the legacy Idle/Physical/Social
    // observer activity enum. While ContextActionToken is nonzero these fields
    // describe pending work; after completion they may briefly describe the
    // authoritative result for presentation provenance.
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

    // ResourceNode / StorageSite positions are Core-authored. Presentation may
    // use this target only when Core explicitly marks it available.
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
