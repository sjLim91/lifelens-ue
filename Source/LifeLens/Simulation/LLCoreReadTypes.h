#pragma once

#include "CoreMinimal.h"
#include "LLCoreReadTypes.generated.h"

UENUM(BlueprintType)
enum class ELLCoreObservedActivityKind : uint8
{
    Idle,
    Physical,
    Social
};

UENUM(BlueprintType)
enum class ELLCoreRomanceStage : uint8
{
    None,
    Dating,
    Engaged,
    Married,
    Separated,
    Divorced,
    Widowed,
    FormerPartners
};

USTRUCT(BlueprintType)
struct FLLCoreNeedSnapshot
{
    GENERATED_BODY()

    // LifeLensCore Needs are deficit/pressure values: 0 = satisfied, 1 = urgent.
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Hunger = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Thirst = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Sleep = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Bladder = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Hygiene = 0.0f;
};

USTRUCT(BlueprintType)
struct FLLCoreEmotionSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Joy = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Sadness = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Anger = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Fear = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Embarrassment = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Pride = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Jealousy = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Affection = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Anxiety = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Relief = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Grief = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Valence = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Arousal = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Intensity = 0.0f;
};

USTRUCT(BlueprintType)
struct FLLCoreRelationshipSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FGuid TargetResidentId;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FString TargetName;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Affection = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Trust = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Respect = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Comfort = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Familiarity = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Attraction = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float RomanticInterest = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float SexualAttraction = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Commitment = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Conflict = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Jealousy = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Fear = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float Grudge = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float SocialBond = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") float RomancePotential = 0.0f;
};

USTRUCT(BlueprintType)
struct FLLCoreFamilyMemberSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FGuid ResidentId;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FString DisplayName;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FString KinshipLabel;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FString LifeStageLabel;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") bool bAlive = true;
};

USTRUCT(BlueprintType)
struct FLLCoreFamilyObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FGuid SubjectResidentId;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int64 HouseholdId = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") bool bHasRomanceHistory = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") bool bHasActivePartner = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FGuid PartnerResidentId;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FString PartnerName;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") ELLCoreRomanceStage PartnerStage = ELLCoreRomanceStage::None;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") bool bCohabitingWithPartner = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") bool bGestationalParent = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") bool bExpectingChild = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FGuid PregnancyPartnerResidentId;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FString PregnancyPartnerName;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") TArray<FLLCoreFamilyMemberSnapshot> Parents;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") TArray<FLLCoreFamilyMemberSnapshot> Children;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") TArray<FLLCoreFamilyMemberSnapshot> Siblings;
};

USTRUCT(BlueprintType)
struct FLLCoreResidentObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FGuid ResidentId;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FString DisplayName;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") bool bAlive = true;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FLLCoreNeedSnapshot Needs;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FLLCoreEmotionSnapshot Emotion;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") ELLCoreObservedActivityKind ActivityKind = ELLCoreObservedActivityKind::Idle;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FString ActivityLabel = TEXT("Idle");
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FGuid ActivityTargetResidentId;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") FString ActivityTargetName;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") TArray<FLLCoreRelationshipSnapshot> Relationships;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 MemoryCount = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 BeliefCount = 0;
};

USTRUCT(BlueprintType)
struct FLLCoreWorldObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int64 SimulationMinute = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 TotalResidents = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 LivingResidents = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 DeceasedResidents = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 BabyResidents = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 ToddlerResidents = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 ChildResidents = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 TeenResidents = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 YoungAdultResidents = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 AdultResidents = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 MiddleAgeResidents = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 ElderlyResidents = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 Households = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 ActiveCouples = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 DatingCouples = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 EngagedCouples = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 MarriedCouples = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 SeparatedCouples = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 ActivePregnancies = 0;

    // Counts per-character LifeHistory records classified as major observer events.
    // It is not a deduplicated count of unique world-level events.
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core") int32 MajorLifeEventRecords = 0;
};
