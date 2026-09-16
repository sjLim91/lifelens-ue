#pragma once

#include "CoreMinimal.h"
#include "LLTypes.generated.h"

UENUM(BlueprintType)
enum class ELLSex : uint8
{
    Male,
    Female
};

UENUM(BlueprintType)
enum class ELLLifeStage : uint8
{
    Infant,
    Child,
    Teen,
    Adult,
    Elder
};

UENUM(BlueprintType)
enum class ELLRelationshipStage : uint8
{
    Stranger,
    Acquaintance,
    Friend,
    Dating,
    Partner,
    Engaged,
    Married,
    Estranged
};

UENUM(BlueprintType)
enum class ELLActionIntent : uint8
{
    // Keep persisted/Blueprint ordinals stable. New intents append only.
    Idle = 0,
    Eat = 1,
    Sleep = 2,
    Socialize = 3,
    Hygiene = 4,
    Toilet = 5,
    HaveFun = 6,
    Drink,
};

USTRUCT(BlueprintType)
struct FLLNeedState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Hunger = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Energy = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Social = 65.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Hygiene = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Bladder = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Fun = 60.0f;

    // Authoritative Core thirst projected onto the legacy observer DTO.
    // Appended after every pre-existing field so any legacy C++ aggregate
    // initializer keeps the meaning of its existing positional values.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Thirst = 80.0f;
};

USTRUCT(BlueprintType)
struct FLLPersonality
{
    GENERATED_BODY()

    // Compatibility summary axis. Core does not own an Extraversion scalar;
    // this remains a derived presentation value for the quick inspector only.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Extraversion = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Agreeableness = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Conscientiousness = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Openness = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float EmotionalStability = 50.0f;

    // Full authoritative Core personality projection. Appended to preserve the
    // original compatibility fields/ordinals while allowing Observer detail to
    // display every Core-owned dimension instead of only five legacy axes.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Introversion = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Empathy = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Impulsiveness = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float RiskTolerance = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Ambition = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Patience = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Sociability = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Curiosity = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Orderliness = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Adaptability = 50.0f;
};

USTRUCT(BlueprintType)
struct FLLResidentData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FGuid ResidentId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    ELLSex Sex = ELLSex::Male;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    ELLLifeStage LifeStage = ELLLifeStage::Adult;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    int32 AgeYears = 25;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FLLPersonality Personality;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FLLNeedState Needs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    TArray<FName> Traits;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    TMap<FName, float> Skills;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    TArray<FName> Preferences;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FString BackgroundTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FGuid PartnerId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    TArray<FGuid> ParentIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    TArray<FGuid> ChildIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    bool bPregnant = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float PregnancyProgressDays = 0.0f;
};

USTRUCT(BlueprintType)
struct FLLRelationshipData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FGuid A;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    FGuid B;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Affinity = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Trust = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    float Romance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
    ELLRelationshipStage Stage = ELLRelationshipStage::Stranger;
};
