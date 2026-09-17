#pragma once

#include "CoreMinimal.h"
#include "LLTimeReadTypes.generated.h"

UENUM(BlueprintType)
enum class ELLCoreSeasonSummary : uint8
{
    Spring,
    Summer,
    Autumn,
    Winter
};

USTRUCT(BlueprintType)
struct FLLCoreTimeObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Time") int64 SimulationMinute = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Time") int32 MinuteOfDay = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Time") int32 HourOfDay = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Time") int32 MinuteOfHour = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Time") int64 DayIndex = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Time") int32 DayOfYear = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Time") int64 YearIndex = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Time") float AnnualPhase = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Time") ELLCoreSeasonSummary Season = ELLCoreSeasonSummary::Spring;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Time") bool bIsDay = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Time") bool bIsNight = true;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Time") float Daylight01 = 0.0f;
};
