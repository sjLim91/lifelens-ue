#pragma once

#include "CoreMinimal.h"
#include "LLEnvironmentReadTypes.generated.h"

UENUM(BlueprintType)
enum class ELLCoreEnvironmentalResidueKind : uint8
{
    HumanWaste
};

UENUM(BlueprintType)
enum class ELLCorePrecipitationType : uint8
{
    None,
    Rain,
    Snow
};

UENUM(BlueprintType)
enum class ELLCoreWeatherSummary : uint8
{
    Clear,
    Cloudy,
    Rain,
    Snow,
    Fog,
    Storm,
    Heat,
    Cold
};

USTRUCT(BlueprintType)
struct FLLCoreEnvironmentalResidueObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") int64 ResidueId = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") ELLCoreEnvironmentalResidueKind Kind = ELLCoreEnvironmentalResidueKind::HumanWaste;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") int32 GridX = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") int32 GridY = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") FGuid SourceResidentId;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") int32 AgeMinutes = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") float Amount = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") float Intensity = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") int32 RadiusTiles = 1;
};

USTRUCT(BlueprintType)
struct FLLCoreEnvironmentObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") int64 SimulationMinute = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") int32 TotalResidues = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") int32 HumanWasteResidues = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") float AggregateAmount = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") float PeakIntensity = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment") TArray<FLLCoreEnvironmentalResidueObservation> Residues;
};

/**
 * Read-only climate/weather projection for one authoritative world chunk.
 * Values are derived from Core WorldSeed + generation version + chunk + time;
 * Presentation/physical execution must consume this DTO rather than create a
 * second weather truth.
 */
/**
 * Read-only rendering hints for Unreal SkyAtmosphere / directional light /
 * fog consumers. These values are deterministic projections of authoritative
 * Core time + initial-region weather; they are not a second astronomy/weather
 * simulation.
 */
USTRUCT(BlueprintType)
struct FLLCoreSkyPresentationObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|SkyPresentation") bool bAvailable = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|SkyPresentation") int64 SimulationMinute = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|SkyPresentation") int32 MinuteOfDay = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|SkyPresentation") float SolarDayProgress01 = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|SkyPresentation") bool bSunAboveHorizon = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|SkyPresentation") float SunElevationDegrees = -18.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|SkyPresentation") float SunAzimuthDegrees = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|SkyPresentation") float SunIntensity01 = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|SkyPresentation") float SkyBrightness01 = 0.04f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|SkyPresentation") float CloudCover01 = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|SkyPresentation") float FogAmount01 = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|SkyPresentation") float WindIntensity01 = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|SkyPresentation") float SurfaceWetness01 = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|SkyPresentation") ELLCoreWeatherSummary WeatherSummary = ELLCoreWeatherSummary::Clear;
};

USTRUCT(BlueprintType)
struct FLLCoreDynamicEnvironmentObservation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Weather") bool bAvailable = false;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Weather") int64 SimulationMinute = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Weather") int32 ChunkX = 0;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Weather") int32 ChunkY = 0;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Weather") float BaselineTemperature01 = 0.5f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Weather") float BaselineMoisture01 = 0.5f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Weather") float AirTemperatureC = 12.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Weather") float SeasonalTemperatureModifierC = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Weather") float DailyTemperatureModifierC = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Weather") float PrecipitationIntensity01 = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Weather") float CloudCover01 = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Weather") float WindIntensity01 = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Weather") float Humidity01 = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Weather") float Visibility01 = 1.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Weather") float SurfaceWetness01 = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Consequences") float HeatStress01 = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Consequences") float ColdStress01 = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Consequences") float WetStress01 = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Consequences") float TravelFriction01 = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Consequences") float OutdoorWorkFriction01 = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Consequences") float FireReliability01 = 1.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Consequences") float WaterReplenishmentMultiplier = 1.0f;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Consequences") float PlantFoodRegenerationMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Weather") ELLCorePrecipitationType PrecipitationType = ELLCorePrecipitationType::None;
    UPROPERTY(BlueprintReadOnly, Category="LifeLens|Core|Environment|Weather") ELLCoreWeatherSummary WeatherSummary = ELLCoreWeatherSummary::Clear;
};
