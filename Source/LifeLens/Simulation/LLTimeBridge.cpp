#include "Simulation/LLCoreBridgeSubsystem.h"

#include "lifelens/Simulation.h"
#include "lifelens/SimulationCalendar.h"

namespace
{
ELLCoreSeasonSummary ToUnrealSeason(lifelens::SeasonSummary Season)
{
    switch (Season)
    {
        case lifelens::SeasonSummary::Spring: return ELLCoreSeasonSummary::Spring;
        case lifelens::SeasonSummary::Summer: return ELLCoreSeasonSummary::Summer;
        case lifelens::SeasonSummary::Autumn: return ELLCoreSeasonSummary::Autumn;
        case lifelens::SeasonSummary::Winter: return ELLCoreSeasonSummary::Winter;
    }
    return ELLCoreSeasonSummary::Spring;
}
}

FLLCoreTimeObservation ULLCoreBridgeSubsystem::GetTimeObservation() const
{
    FLLCoreTimeObservation Result;
    if (!CoreSimulation)
    {
        return Result;
    }

    const lifelens::SimulationCalendarObservation CoreTime =
        lifelens::deriveSimulationCalendar(CoreSimulation->world().minute);

    Result.SimulationMinute = CoreTime.totalMinute;
    Result.MinuteOfDay = CoreTime.minuteOfDay;
    Result.HourOfDay = CoreTime.hourOfDay;
    Result.MinuteOfHour = CoreTime.minuteOfHour;
    Result.DayIndex = CoreTime.dayIndex;
    Result.DayOfYear = CoreTime.dayOfYear;
    Result.YearIndex = CoreTime.yearIndex;
    Result.AnnualPhase = static_cast<float>(CoreTime.annualPhase);
    Result.Season = ToUnrealSeason(CoreTime.season);
    Result.bIsDay = CoreTime.isDay;
    Result.bIsNight = CoreTime.isNight;
    Result.Daylight01 = static_cast<float>(CoreTime.daylight01);
    return Result;
}
