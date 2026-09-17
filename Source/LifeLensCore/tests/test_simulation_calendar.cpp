#include <cassert>
#include <cmath>

#include "lifelens/SimulationCalendar.h"

using namespace lifelens;

int main()
{
    {
        const auto midnight=deriveSimulationCalendar(0);
        assert(midnight.totalMinute==0);
        assert(midnight.dayIndex==0);
        assert(midnight.yearIndex==0);
        assert(midnight.dayOfYear==0);
        assert(midnight.hourOfDay==0);
        assert(midnight.minuteOfHour==0);
        assert(midnight.isNight && !midnight.isDay);
        assert(midnight.daylight01==0.0);
        assert(midnight.season==SeasonSummary::Spring);
    }

    {
        const auto beforeDawn=deriveSimulationCalendar(5*60+59);
        const auto dawn=deriveSimulationCalendar(6*60);
        const auto noon=deriveSimulationCalendar(12*60);
        const auto beforeSunset=deriveSimulationCalendar(18*60-1);
        const auto sunset=deriveSimulationCalendar(18*60);
        assert(beforeDawn.isNight);
        assert(dawn.isDay);
        assert(dawn.daylight01==0.0);
        assert(noon.isDay);
        assert(std::abs(noon.daylight01-1.0)<1e-9);
        assert(beforeSunset.isDay);
        assert(sunset.isNight);
        assert(sunset.daylight01==0.0);
    }

    {
        const auto endDay=deriveSimulationCalendar(1439);
        const auto nextDay=deriveSimulationCalendar(1440);
        assert(endDay.dayIndex==0 && endDay.minuteOfDay==1439);
        assert(nextDay.dayIndex==1 && nextDay.minuteOfDay==0);
    }

    {
        const auto yearBoundary=deriveSimulationCalendar(
            static_cast<std::int64_t>(SimulationDaysPerYear)*SimulationMinutesPerDay);
        assert(yearBoundary.yearIndex==1);
        assert(yearBoundary.dayOfYear==0);
        assert(yearBoundary.annualPhase==0.0);
        assert(yearBoundary.season==SeasonSummary::Spring);
    }

    {
        const auto summer=deriveSimulationCalendar(
            static_cast<std::int64_t>(92)*SimulationMinutesPerDay);
        const auto autumn=deriveSimulationCalendar(
            static_cast<std::int64_t>(183)*SimulationMinutesPerDay);
        const auto winter=deriveSimulationCalendar(
            static_cast<std::int64_t>(274)*SimulationMinutesPerDay);
        assert(summer.season==SeasonSummary::Summer);
        assert(autumn.season==SeasonSummary::Autumn);
        assert(winter.season==SeasonSummary::Winter);
        assert(summer.annualPhase>=0.0 && summer.annualPhase<1.0);
        assert(autumn.annualPhase>=0.0 && autumn.annualPhase<1.0);
        assert(winter.annualPhase>=0.0 && winter.annualPhase<1.0);
    }

    {
        const auto clamped=deriveSimulationCalendar(-100);
        assert(clamped.totalMinute==0);
        assert(clamped.dayIndex==0);
    }

    return 0;
}
