#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "EarthSurfaceCoordinates.h"
#include "SimulationCalendar.h"

namespace lifelens {

// EARTH-5 read-only astronomical baseline. Never overrides the existing
// SimulationCalendar/SimulationClimate or Core life state implicitly.
// The 365-day LifeLens simulation year starts at northern vernal equinox.
inline constexpr double EarthSolarAxialTiltDeg=23.439281;

struct EarthSolarObservation {
    std::int64_t simulationMinute=0;
    EarthGeoPoint location{};
    double localSolarHour=0.0;
    double solarDeclinationDeg=0.0;
    double solarElevationDeg=0.0;
    double daylightHours=0.0;
    bool sunlightAboveHorizon=false;
    double sunshinePotential01=0.0;
};

inline double earthSolarFraction01(double fraction)
{
    // fmod avoids catastrophic significance loss of enormous integer minutes.
    double value=std::fmod(fraction,1.0);
    return value<0.0?value+1.0:value;
}

inline bool earthSolarObservationAt(
    EarthGeoPoint location,
    std::int64_t simulationMinute,
    EarthSolarObservation& output)
{
    if(!earthValidGeo(location)) return false;
    const auto calendar=deriveSimulationCalendar(simulationMinute);
    const double lat=earthRadians(location.latitudeDeg);
    const double annual=calendar.annualPhase*2.0*EarthMathPi;
    // Equinox day 0, northern summer solstice 1/4 year, winter at 3/4 year.
    const double declination=earthRadians(EarthSolarAxialTiltDeg)*std::sin(annual);
    const double solarTimeFraction=earthSolarFraction01(
        static_cast<double>(calendar.minuteOfDay)/SimulationMinutesPerDay
        +earthWrapLongitude(location.longitudeDeg)/360.0);
    const double hourAngle=2.0*EarthMathPi*(solarTimeFraction-0.5);
    const double sinElevation=std::clamp(
        std::sin(lat)*std::sin(declination)
        +std::cos(lat)*std::cos(declination)*std::cos(hourAngle),
        -1.0,1.0);
    const double zenithCos=-std::tan(lat)*std::tan(declination);
    // The polar day/night limit is defined by the same sunset equation;
    // never divide by cos(latitude), which is near zero at the poles.
    const double dayHours=zenithCos<=-1.0?24.0
        :zenithCos>=1.0?0.0
        :24.0/EarthMathPi*std::acos(zenithCos);
    output.simulationMinute=calendar.totalMinute;
    output.location=location;
    output.localSolarHour=24.0*solarTimeFraction;
    output.solarDeclinationDeg=earthDegrees(declination);
    output.solarElevationDeg=earthDegrees(std::asin(sinElevation));
    output.daylightHours=dayHours;
    output.sunlightAboveHorizon=sinElevation>0.0;
    output.sunshinePotential01=std::max(0.0,sinElevation);
    return true;
}

} // namespace lifelens
