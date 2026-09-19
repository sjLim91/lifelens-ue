#include "Simulation/LLCoreBridgeSubsystem.h"

#include "lifelens/EnvironmentalConsequences.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationCalendar.h"
#include "lifelens/SimulationClimate.h"

namespace
{
int32 SafeEnvironmentCount(std::size_t Count)
{
    return Count > static_cast<std::size_t>(MAX_int32)
        ? MAX_int32
        : static_cast<int32>(Count);
}

ELLCorePrecipitationType ToUnrealPrecipitationType(lifelens::PrecipitationType Type)
{
    switch (Type)
    {
        case lifelens::PrecipitationType::Rain: return ELLCorePrecipitationType::Rain;
        case lifelens::PrecipitationType::Snow: return ELLCorePrecipitationType::Snow;
        case lifelens::PrecipitationType::None:
        default:
            return ELLCorePrecipitationType::None;
    }
}

ELLCoreWeatherSummary ToUnrealWeatherSummary(lifelens::WeatherSummary Summary)
{
    switch (Summary)
    {
        case lifelens::WeatherSummary::Clear: return ELLCoreWeatherSummary::Clear;
        case lifelens::WeatherSummary::Cloudy: return ELLCoreWeatherSummary::Cloudy;
        case lifelens::WeatherSummary::Rain: return ELLCoreWeatherSummary::Rain;
        case lifelens::WeatherSummary::Snow: return ELLCoreWeatherSummary::Snow;
        case lifelens::WeatherSummary::Fog: return ELLCoreWeatherSummary::Fog;
        case lifelens::WeatherSummary::Storm: return ELLCoreWeatherSummary::Storm;
        case lifelens::WeatherSummary::Heat: return ELLCoreWeatherSummary::Heat;
        case lifelens::WeatherSummary::Cold: return ELLCoreWeatherSummary::Cold;
    }
    return ELLCoreWeatherSummary::Clear;
}

FLLCoreDynamicEnvironmentObservation ToUnrealDynamicEnvironment(
    const lifelens::DynamicEnvironmentObservation& CoreEnvironment)
{
    const lifelens::EnvironmentalConsequenceProfile Consequences =
        lifelens::deriveEnvironmentalConsequences(CoreEnvironment);

    FLLCoreDynamicEnvironmentObservation Result;
    Result.bAvailable = true;
    Result.SimulationMinute = static_cast<int64>(CoreEnvironment.simulationMinute);
    Result.ChunkX = CoreEnvironment.coord.x;
    Result.ChunkY = CoreEnvironment.coord.y;
    Result.BaselineTemperature01 = static_cast<float>(CoreEnvironment.baselineTemperature01);
    Result.BaselineMoisture01 = static_cast<float>(CoreEnvironment.baselineMoisture01);
    Result.AirTemperatureC = static_cast<float>(CoreEnvironment.airTemperatureC);
    Result.SeasonalTemperatureModifierC = static_cast<float>(CoreEnvironment.seasonalTemperatureModifierC);
    Result.DailyTemperatureModifierC = static_cast<float>(CoreEnvironment.dailyTemperatureModifierC);
    Result.PrecipitationIntensity01 = static_cast<float>(CoreEnvironment.precipitationIntensity01);
    Result.CloudCover01 = static_cast<float>(CoreEnvironment.cloudCover01);
    Result.WindIntensity01 = static_cast<float>(CoreEnvironment.windIntensity01);
    Result.Humidity01 = static_cast<float>(CoreEnvironment.humidity01);
    Result.Visibility01 = static_cast<float>(CoreEnvironment.visibility01);
    Result.SurfaceWetness01 = static_cast<float>(CoreEnvironment.surfaceWetness01);
    Result.HeatStress01 = static_cast<float>(Consequences.heatStress01);
    Result.ColdStress01 = static_cast<float>(Consequences.coldStress01);
    Result.WetStress01 = static_cast<float>(Consequences.wetStress01);
    Result.TravelFriction01 = static_cast<float>(Consequences.travelFriction01);
    Result.OutdoorWorkFriction01 = static_cast<float>(Consequences.outdoorWorkFriction01);
    Result.FireReliability01 = static_cast<float>(Consequences.fireReliability01);
    Result.WaterReplenishmentMultiplier = static_cast<float>(Consequences.waterReplenishmentMultiplier);
    Result.PlantFoodRegenerationMultiplier = static_cast<float>(Consequences.plantFoodRegenerationMultiplier);
    Result.PrecipitationType = ToUnrealPrecipitationType(CoreEnvironment.precipitationType);
    Result.WeatherSummary = ToUnrealWeatherSummary(CoreEnvironment.summary);
    return Result;
}
}

FLLCoreEnvironmentObservation ULLCoreBridgeSubsystem::GetEnvironmentObservation(int32 MaxResidues) const
{
    FLLCoreEnvironmentObservation Result;
    if (!CoreSimulation)
    {
        return Result;
    }

    const std::size_t Limit = MaxResidues <= 0
        ? 0u
        : static_cast<std::size_t>(MaxResidues);
    const lifelens::EnvironmentObservation CoreEnvironment =
        CoreSimulation->observeEnvironment(Limit);

    Result.SimulationMinute = static_cast<int64>(CoreEnvironment.minute);
    Result.TotalResidues = SafeEnvironmentCount(CoreEnvironment.totalResidues);
    Result.HumanWasteResidues = SafeEnvironmentCount(CoreEnvironment.humanWasteResidues);
    Result.AggregateAmount = static_cast<float>(CoreEnvironment.aggregateAmount);
    Result.PeakIntensity = static_cast<float>(CoreEnvironment.peakIntensity);
    Result.Residues.Reserve(SafeEnvironmentCount(CoreEnvironment.residues.size()));

    for (const lifelens::EnvironmentalResidueObservation& CoreResidue : CoreEnvironment.residues)
    {
        FLLCoreEnvironmentalResidueObservation Residue;
        Residue.ResidueId = CoreResidue.id > static_cast<uint64>(MAX_int64)
            ? MAX_int64
            : static_cast<int64>(CoreResidue.id);
        Residue.Kind = ELLCoreEnvironmentalResidueKind::HumanWaste;
        Residue.GridX = CoreResidue.pos.x;
        Residue.GridY = CoreResidue.pos.y;
        Residue.SourceResidentId = MakeStableResidentGuid(static_cast<uint64>(CoreResidue.sourceCharacter));
        Residue.AgeMinutes = CoreResidue.ageMinutes;
        Residue.Amount = static_cast<float>(CoreResidue.amount);
        Residue.Intensity = static_cast<float>(CoreResidue.intensity);
        Residue.RadiusTiles = CoreResidue.radiusTiles;
        Result.Residues.Add(MoveTemp(Residue));
    }

    return Result;
}

FLLCoreDynamicEnvironmentObservation ULLCoreBridgeSubsystem::GetDynamicEnvironmentObservation(
    int32 ChunkX,
    int32 ChunkY) const
{
    if (!CoreSimulation)
    {
        return FLLCoreDynamicEnvironmentObservation{};
    }

    const lifelens::World& CoreWorld = CoreSimulation->world();
    const lifelens::DynamicEnvironmentObservation CoreEnvironment =
        lifelens::deriveDynamicEnvironment(
            CoreWorld.genesisIdentity(),
            {ChunkX, ChunkY},
            static_cast<std::int64_t>(CoreWorld.minute));
    return ToUnrealDynamicEnvironment(CoreEnvironment);
}

FLLCoreDynamicEnvironmentObservation ULLCoreBridgeSubsystem::GetInitialRegionDynamicEnvironmentObservation() const
{
    if (!CoreSimulation)
    {
        return FLLCoreDynamicEnvironmentObservation{};
    }

    const lifelens::World& CoreWorld = CoreSimulation->world();
    const lifelens::ChunkCoord Coord = CoreWorld.hasInitialStartRegionSelection
        ? CoreWorld.initialStartRegionCoord
        : CoreWorld.initialStartRegion().region.coord;
    return GetDynamicEnvironmentObservation(Coord.x, Coord.y);
}


FLLCoreSkyPresentationObservation ULLCoreBridgeSubsystem::GetInitialRegionSkyPresentationObservation() const
{
    FLLCoreSkyPresentationObservation Result;
    if (!CoreSimulation)
    {
        return Result;
    }

    const lifelens::World& CoreWorld = CoreSimulation->world();
    const lifelens::ChunkCoord Coord = CoreWorld.hasInitialStartRegionSelection
        ? CoreWorld.initialStartRegionCoord
        : CoreWorld.initialStartRegion().region.coord;
    const lifelens::SimulationCalendarObservation CoreTime =
        lifelens::deriveSimulationCalendar(static_cast<std::int64_t>(CoreWorld.minute));
    const lifelens::DynamicEnvironmentObservation CoreEnvironment =
        lifelens::deriveDynamicEnvironment(
            CoreWorld.genesisIdentity(),
            Coord,
            static_cast<std::int64_t>(CoreWorld.minute));

    constexpr float Pi = 3.14159265358979323846f;
    constexpr float MaxSunElevationDegrees = 70.0f;
    constexpr float MaxNightDepressionDegrees = 18.0f;

    const float SolarDayProgress = FMath::Clamp(
        static_cast<float>(CoreTime.minuteOfDay)
            / static_cast<float>(lifelens::SimulationMinutesPerDay),
        0.0f,
        1.0f);

    float SunElevation = 0.0f;
    if (CoreTime.isDay)
    {
        const float DayProgress = FMath::Clamp(
            static_cast<float>(CoreTime.minuteOfDay - lifelens::SimulationSunriseMinute)
                / static_cast<float>(
                    lifelens::SimulationSunsetMinute - lifelens::SimulationSunriseMinute),
            0.0f,
            1.0f);
        SunElevation = FMath::Sin(Pi * DayProgress) * MaxSunElevationDegrees;
    }
    else
    {
        const int32 ExtendedMinute = CoreTime.minuteOfDay < lifelens::SimulationSunriseMinute
            ? CoreTime.minuteOfDay + lifelens::SimulationMinutesPerDay
            : CoreTime.minuteOfDay;
        const float NightProgress = FMath::Clamp(
            static_cast<float>(ExtendedMinute - lifelens::SimulationSunsetMinute)
                / static_cast<float>(
                    lifelens::SimulationMinutesPerDay
                    - lifelens::SimulationSunsetMinute
                    + lifelens::SimulationSunriseMinute),
            0.0f,
            1.0f);
        SunElevation = -FMath::Sin(Pi * NightProgress) * MaxNightDepressionDegrees;
    }

    const float CloudCover = FMath::Clamp(
        static_cast<float>(CoreEnvironment.cloudCover01), 0.0f, 1.0f);
    const float Visibility = FMath::Clamp(
        static_cast<float>(CoreEnvironment.visibility01), 0.0f, 1.0f);
    const float Humidity = FMath::Clamp(
        static_cast<float>(CoreEnvironment.humidity01), 0.0f, 1.0f);
    const float Precipitation = FMath::Clamp(
        static_cast<float>(CoreEnvironment.precipitationIntensity01), 0.0f, 1.0f);
    const float Daylight = FMath::Clamp(
        static_cast<float>(CoreTime.daylight01), 0.0f, 1.0f);

    Result.bAvailable = true;
    Result.SimulationMinute = CoreTime.totalMinute;
    Result.MinuteOfDay = CoreTime.minuteOfDay;
    Result.SolarDayProgress01 = SolarDayProgress;
    Result.bSunAboveHorizon = CoreTime.isDay;
    Result.SunElevationDegrees = SunElevation;
    Result.SunAzimuthDegrees = FMath::Fmod(SolarDayProgress * 360.0f, 360.0f);
    Result.SunIntensity01 = FMath::Clamp(
        Daylight
        * (1.0f - 0.55f * CloudCover)
        * (0.35f + 0.65f * Visibility),
        0.0f,
        1.0f);
    Result.SkyBrightness01 = FMath::Clamp(
        0.04f + 0.96f * Daylight - 0.18f * CloudCover,
        0.02f,
        1.0f);
    Result.CloudCover01 = CloudCover;
    Result.FogAmount01 = FMath::Clamp(
        0.75f * (1.0f - Visibility)
        + 0.15f * Humidity
        + 0.10f * Precipitation,
        0.0f,
        1.0f);
    Result.WindIntensity01 = FMath::Clamp(
        static_cast<float>(CoreEnvironment.windIntensity01), 0.0f, 1.0f);
    Result.SurfaceWetness01 = FMath::Clamp(
        static_cast<float>(CoreEnvironment.surfaceWetness01), 0.0f, 1.0f);
    Result.WeatherSummary = ToUnrealWeatherSummary(CoreEnvironment.summary);
    return Result;
}
