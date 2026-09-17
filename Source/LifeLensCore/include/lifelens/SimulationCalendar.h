#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace lifelens {

enum class SeasonSummary : std::uint8_t {
    Spring,
    Summer,
    Autumn,
    Winter
};

struct SimulationCalendarObservation {
    std::int64_t totalMinute=0;
    int minuteOfDay=0;
    int hourOfDay=0;
    int minuteOfHour=0;
    std::int64_t dayIndex=0;
    int dayOfYear=0;
    std::int64_t yearIndex=0;
    double annualPhase=0.0;
    SeasonSummary season=SeasonSummary::Spring;
    bool isDay=false;
    bool isNight=true;
    double daylight01=0.0;
};

inline constexpr int SimulationMinutesPerHour=60;
inline constexpr int SimulationHoursPerDay=24;
inline constexpr int SimulationMinutesPerDay=SimulationMinutesPerHour*SimulationHoursPerDay;
inline constexpr int SimulationDaysPerYear=365;
inline constexpr int SimulationSunriseMinute=6*SimulationMinutesPerHour;
inline constexpr int SimulationSunsetMinute=18*SimulationMinutesPerHour;

inline SimulationCalendarObservation deriveSimulationCalendar(std::int64_t simulationMinute)
{
    SimulationCalendarObservation result;
    const std::int64_t safeMinute=std::max<std::int64_t>(0,simulationMinute);
    result.totalMinute=safeMinute;
    result.dayIndex=safeMinute/SimulationMinutesPerDay;
    result.minuteOfDay=static_cast<int>(safeMinute%SimulationMinutesPerDay);
    result.hourOfDay=result.minuteOfDay/SimulationMinutesPerHour;
    result.minuteOfHour=result.minuteOfDay%SimulationMinutesPerHour;
    result.dayOfYear=static_cast<int>(result.dayIndex%SimulationDaysPerYear);
    result.yearIndex=result.dayIndex/SimulationDaysPerYear;

    const double fractionOfDay=static_cast<double>(result.minuteOfDay)/SimulationMinutesPerDay;
    result.annualPhase=(static_cast<double>(result.dayOfYear)+fractionOfDay)/SimulationDaysPerYear;

    if(result.annualPhase<0.25){ result.season=SeasonSummary::Spring; }
    else if(result.annualPhase<0.50){ result.season=SeasonSummary::Summer; }
    else if(result.annualPhase<0.75){ result.season=SeasonSummary::Autumn; }
    else { result.season=SeasonSummary::Winter; }

    result.isDay=result.minuteOfDay>=SimulationSunriseMinute
        && result.minuteOfDay<SimulationSunsetMinute;
    result.isNight=!result.isDay;
    if(result.isDay){
        const double daylightProgress=static_cast<double>(result.minuteOfDay-SimulationSunriseMinute)
            /static_cast<double>(SimulationSunsetMinute-SimulationSunriseMinute);
        constexpr double Pi=3.14159265358979323846;
        result.daylight01=std::clamp(std::sin(Pi*daylightProgress),0.0,1.0);
    }
    return result;
}

} // namespace lifelens
