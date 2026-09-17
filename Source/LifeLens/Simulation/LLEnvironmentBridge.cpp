#include "Simulation/LLCoreBridgeSubsystem.h"

#include "lifelens/Simulation.h"
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
