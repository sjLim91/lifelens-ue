#pragma once

#include "CoreMinimal.h"
#include "LLCivilizationReadTypes.generated.h"

UENUM(BlueprintType)
enum class ELLCoreMaterialKind : uint8
{
    Unknown,
    Stone,
    Flint,
    Wood,
    Fiber,
    Clay,
    Water,
    PlantFood,
    Bone,
    Hide,
    CopperOre,
    TinOre,
    IronOre,
    Charcoal
};

UENUM(BlueprintType)
enum class ELLCoreItemKind : uint8
{
    RawMaterial,
    SharpFlake,
    StoneCuttingTool,
    Cordage,
    SimpleContainer,
    FuelBundle
};

UENUM(BlueprintType)
enum class ELLCoreTechniqueId : uint8
{
    None,
    SharpFlake,
    ChippedStoneTool,
    FireMaking,
    FiberCordage,
    SimpleContainer
};

UENUM(BlueprintType)
enum class ELLCoreKnowledgeLevel : uint8
{
    Unknown,
    Observed,
    Hypothesized,
    Understood,
    Reproducible,
    Practiced,
    Mastered
};

UENUM(BlueprintType)
enum class ELLCoreKnowledgeSource : uint8
{
    Unknown,
    SelfDiscovery,
    DirectWitness,
    Teaching
};

USTRUCT(BlueprintType)
struct FLLCoreCivilizationItemStack
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") ELLCoreItemKind Item = ELLCoreItemKind::RawMaterial;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") ELLCoreMaterialKind Material = ELLCoreMaterialKind::Unknown;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 Quantity = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") float Quality = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") float Durability = 0.0f;
};

USTRUCT(BlueprintType)
struct FLLCoreTechniqueKnowledgeObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") ELLCoreTechniqueId Technique = ELLCoreTechniqueId::None;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") ELLCoreKnowledgeLevel Level = ELLCoreKnowledgeLevel::Unknown;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") float Confidence = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 SuccessfulUses = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") bool bHasProvenance = false;
    // SocialFactId is uint64 in Core. String preserves its full unsigned value for Blueprint/UI.
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") FString ProvenanceFactId;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") FGuid OriginResidentId;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") FGuid ImmediateSourceResidentId;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") ELLCoreKnowledgeSource Source = ELLCoreKnowledgeSource::Unknown;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int64 LearnedMinute = -1;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 HopCount = 0;
};

USTRUCT(BlueprintType)
struct FLLCoreResidentCivilizationObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") FGuid ResidentId;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") FString DisplayName;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 TotalInventoryUnits = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") float GatheringSkill = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") float CraftingSkill = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") float LearningSkill = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 KnownTechniqueCount = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 ReproducibleTechniqueCount = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int64 LatestKnowledgeMinute = -1;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") ELLCoreTechniqueId LatestTechnique = ELLCoreTechniqueId::None;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") TArray<FLLCoreCivilizationItemStack> Inventory;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") TArray<FLLCoreTechniqueKnowledgeObservation> Techniques;
};

USTRUCT(BlueprintType)
struct FLLCoreCivilizationResourceObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int64 ResourceNodeId = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") ELLCoreMaterialKind Material = ELLCoreMaterialKind::Unknown;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 Quantity = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 MaxQuantity = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") bool bRenewable = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 RegenerationPerDay = 0;
};

USTRUCT(BlueprintType)
struct FLLCoreCivilizationStorageObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int64 StorageId = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 TotalUnits = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") TArray<FLLCoreCivilizationItemStack> Inventory;
};

USTRUCT(BlueprintType)
struct FLLCoreCivilizationDiscoveryObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") FString FactId;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") ELLCoreTechniqueId Technique = ELLCoreTechniqueId::None;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") FGuid DiscovererResidentId;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") FString DiscovererName;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int64 Minute = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 RecipientCount = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 LivingKnowerCount = 0;
};

USTRUCT(BlueprintType)
struct FLLCoreCivilizationWorldObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int64 SimulationMinute = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 ResourceNodeCount = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 DepletedResourceNodeCount = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 TotalResourceUnits = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 StorageSiteCount = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 TotalStoredUnits = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 TechniqueFactCount = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 TransmissionReceiptCount = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 UniqueKnownTechniqueTypes = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 UniqueReproducibleTechniqueTypes = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 KnownTechniqueOwners = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") int32 ReproducibleTechniqueOwners = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") TArray<FLLCoreCivilizationResourceObservation> Resources;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") TArray<FLLCoreCivilizationStorageObservation> Storages;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Civilization") TArray<FLLCoreCivilizationDiscoveryObservation> RecentDiscoveries;
};
