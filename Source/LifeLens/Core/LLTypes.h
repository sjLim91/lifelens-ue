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
    Idle,
    Eat,
    Drink,
    Sleep,
    Socialize,
    Hygiene,
    Toilet,
    HaveFun
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
};

USTRUCT(BlueprintType)
struct FLLPersonality
{
    GENERATED_BODY()

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
