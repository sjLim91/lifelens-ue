#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "MacroWorldGenesis.h"
#include "SimulationCalendar.h"

namespace lifelens {

enum class PrecipitationType : std::uint8_t {
    None,
    Rain,
    Snow
};

enum class WeatherSummary : std::uint8_t {
    Clear,
    Cloudy,
    Rain,
    Snow,
    Fog,
    Storm,
    Heat,
    Cold
};

struct DynamicEnvironmentObservation {
    std::int64_t simulationMinute=0;
    ChunkCoord coord{};

    double baselineTemperature01=0.5;
    double baselineMoisture01=0.5;

    double airTemperatureC=12.0;
    double seasonalTemperatureModifierC=0.0;
    double dailyTemperatureModifierC=0.0;

    double precipitationIntensity01=0.0;
    double cloudCover01=0.0;
    double windIntensity01=0.0;
    double humidity01=0.0;
    double visibility01=1.0;
    double surfaceWetness01=0.0;

    PrecipitationType precipitationType=PrecipitationType::None;
    WeatherSummary summary=WeatherSummary::Clear;
};

inline double climateClamp01(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

inline std::uint64_t climateTimeDomain(std::uint64_t baseDomain,std::int64_t blockIndex)
{
    const std::uint64_t safeIndex=blockIndex<0 ? 0u : static_cast<std::uint64_t>(blockIndex);
    return worldGenesisMix64(baseDomain ^ worldGenesisMix64(safeIndex+0x9E3779B97F4A7C15ULL));
}

inline double climateTemporalSpatialNoise(
    const WorldGenesisIdentity& identity,
    ChunkCoord coord,
    std::int64_t simulationMinute,
    std::uint64_t baseDomain,
    int blockMinutes,
    int spatialSpanChunks)
{
    const std::int64_t safeMinute=std::max<std::int64_t>(0,simulationMinute);
    const int safeBlockMinutes=std::max(1,blockMinutes);
    const std::int64_t blockIndex=safeMinute/safeBlockMinutes;
    const int minuteInBlock=static_cast<int>(safeMinute%safeBlockMinutes);
    const double fraction=static_cast<double>(minuteInBlock)/static_cast<double>(safeBlockMinutes);
    const double smooth=macroSmoothStep(fraction);

    const double a=macroValueNoise(
        identity,coord,climateTimeDomain(baseDomain,blockIndex),spatialSpanChunks);
    const double b=macroValueNoise(
        identity,coord,climateTimeDomain(baseDomain,blockIndex+1),spatialSpanChunks);
    return macroLerp(a,b,smooth);
}

inline DynamicEnvironmentObservation deriveDynamicEnvironment(
    const WorldGenesisIdentity& identity,
    ChunkCoord coord,
    std::int64_t simulationMinute)
{
    constexpr double Pi=3.14159265358979323846;
    constexpr std::uint64_t CloudDomain=0x434C494D4154434CULL; // CLIMATCL
    constexpr std::uint64_t HumidityDomain=0x434C494D41544855ULL; // CLIMATHU
    constexpr std::uint64_t PrecipDomain=0x434C494D41545052ULL; // CLIMATPR
    constexpr std::uint64_t WindDomain=0x434C494D41545749ULL; // CLIMATWI
    constexpr std::uint64_t ThermalDomain=0x434C494D41545448ULL; // CLIMATTH

    DynamicEnvironmentObservation result;
    result.simulationMinute=std::max<std::int64_t>(0,simulationMinute);
    result.coord=coord;

    const MacroRegionFacts region=deriveMacroRegionFacts(identity,coord);
    const SimulationCalendarObservation calendar=deriveSimulationCalendar(result.simulationMinute);
    result.baselineTemperature01=region.temperature;
    result.baselineMoisture01=region.moisture;

    // Broad regional climate remains authoritative; annual and daily cycles are
    // deterministic modifiers layered over that baseline. Summer peaks around
    // the middle of the Observer's Summer quarter and winter is its inverse.
    const double seasonalWave=std::sin(2.0*Pi*(calendar.annualPhase-0.125));
    const double continentality=0.80+0.20*(1.0-region.moisture);
    result.seasonalTemperatureModifierC=10.0*continentality*seasonalWave;

    const double dayFraction=static_cast<double>(calendar.minuteOfDay)
        /static_cast<double>(SimulationMinutesPerDay);
    result.dailyTemperatureModifierC=3.5*std::sin(2.0*Pi*(dayFraction-0.375));

    const double thermalNoise=climateTemporalSpatialNoise(
        identity,coord,result.simulationMinute,ThermalDomain,360,6);
    const double baselineAirTemperatureC=-8.0+40.0*region.temperature;
    result.airTemperatureC=baselineAirTemperatureC
        +result.seasonalTemperatureModifierC
        +result.dailyTemperatureModifierC
        +(thermalNoise-0.5)*4.0;

    // Weather fields evolve smoothly between deterministic six-hour anchors.
    // The same WorldSeed / generation version / chunk / minute always yields
    // the same state, while nearby chunks remain spatially coherent.
    const double humidityNoise=climateTemporalSpatialNoise(
        identity,coord,result.simulationMinute,HumidityDomain,360,5);
    const double cloudNoise=climateTemporalSpatialNoise(
        identity,coord,result.simulationMinute,CloudDomain,360,6);
    const double precipNoise=climateTemporalSpatialNoise(
        identity,coord,result.simulationMinute,PrecipDomain,360,5);
    const double windNoise=climateTemporalSpatialNoise(
        identity,coord,result.simulationMinute,WindDomain,360,7);

    const double seasonalWetness=0.5+0.5*std::sin(2.0*Pi*(calendar.annualPhase+0.05));
    result.humidity01=climateClamp01(
        0.58*region.moisture+0.20*seasonalWetness+0.22*humidityNoise);
    result.cloudCover01=climateClamp01(
        0.10+0.58*result.humidity01+0.42*(cloudNoise-0.5));

    const double precipDriver=0.62*result.humidity01+0.38*precipNoise;
    result.precipitationIntensity01=climateClamp01((precipDriver-0.60)/0.40);
    result.windIntensity01=climateClamp01(
        0.10+0.58*windNoise+0.26*result.precipitationIntensity01);

    const double fog01=climateClamp01((result.humidity01-0.76)*3.0)
        *climateClamp01(1.0-result.windIntensity01*1.6)
        *climateClamp01(1.0-result.precipitationIntensity01*2.0);
    result.visibility01=climateClamp01(
        1.0-0.52*result.precipitationIntensity01-0.62*fog01-0.12*result.windIntensity01);
    result.surfaceWetness01=climateClamp01(
        0.48*result.humidity01+0.68*result.precipitationIntensity01
        -(result.airTemperatureC>30.0 ? 0.12 : 0.0));

    if(result.precipitationIntensity01>=0.05){
        result.precipitationType=result.airTemperatureC<=1.0
            ? PrecipitationType::Snow
            : PrecipitationType::Rain;
    }

    if(result.precipitationIntensity01>=0.68 && result.windIntensity01>=0.55){
        result.summary=WeatherSummary::Storm;
    } else if(result.precipitationType==PrecipitationType::Snow){
        result.summary=WeatherSummary::Snow;
    } else if(result.precipitationType==PrecipitationType::Rain){
        result.summary=WeatherSummary::Rain;
    } else if(fog01>=0.28){
        result.summary=WeatherSummary::Fog;
    } else if(result.airTemperatureC>=32.0){
        result.summary=WeatherSummary::Heat;
    } else if(result.airTemperatureC<=-8.0){
        result.summary=WeatherSummary::Cold;
    } else if(result.cloudCover01>=0.42){
        result.summary=WeatherSummary::Cloudy;
    } else {
        result.summary=WeatherSummary::Clear;
    }

    return result;
}

} // namespace lifelens
