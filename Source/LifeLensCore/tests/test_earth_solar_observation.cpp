#include "lifelens/EarthSolarObservation.h"
#include <cassert>
#include <cmath>
#include <limits>

using namespace lifelens;

static EarthSolarObservation at(double lat,double lon,int day,int hour=12)
{
    EarthSolarObservation out{};
    assert(earthSolarObservationAt({lat,lon},
        static_cast<std::int64_t>(day)*SimulationMinutesPerDay
        +hour*SimulationMinutesPerHour,out));
    return out;
}
static bool near(double a,double b,double epsilon=1e-8)
{return std::abs(a-b)<=epsilon;}

int main()
{
    const auto equinox=at(0,0,0);
    assert(near(equinox.localSolarHour,12));
    assert(near(equinox.daylightHours,12));
    // At noon, annualPhase includes 0.5 simulation day so the sun is
    // about 0.20 degrees past the equinox, not at exact declination zero.
    assert(equinox.solarElevationDeg>89.5 && equinox.solarElevationDeg<90.0);
    assert(equinox.sunlightAboveHorizon);

    // At the same UTC minute, longitude changes local solar time only,
    // not the global simulation minute or the planet-wide solar declination.
    const auto eastern=at(0,90,0);
    const auto western=at(0,-90,0);
    assert(near(eastern.localSolarHour,18));
    assert(near(western.localSolarHour,6));
    assert(near(eastern.solarDeclinationDeg,western.solarDeclinationDeg));
    assert(eastern.simulationMinute==western.simulationMinute);
    assert(near(at(0,-180,0).localSolarHour,0));
    assert(near(at(0,180,0).localSolarHour,0));

    // Northern and southern hemispheres have opposite seasonal sunshine,
    // without modifying the previous 6-to-18 legacy Calendar rule.
    const auto nSummer=at(55,0,91);
    const auto sWinter=at(-55,0,91);
    assert(nSummer.daylightHours>16);
    assert(sWinter.daylightHours<8);
    assert(nSummer.sunshinePotential01>sWinter.sunshinePotential01);
    const auto nWinter=at(55,0,274);
    const auto sSummer=at(-55,0,274);
    assert(nWinter.daylightHours<8);
    assert(sSummer.daylightHours>16);
    assert(nWinter.sunshinePotential01<sSummer.sunshinePotential01);

    assert(at(90,0,91).daylightHours==24);
    assert(at(-90,0,91).daylightHours==0);
    assert(at(90,0,274).daylightHours==0);
    assert(at(-90,0,274).daylightHours==24);
    assert(near(at(0,0,91).daylightHours,12));
    assert(near(at(0,0,274).daylightHours,12));

    const auto a=at(37,127,130,15),b=at(37,127,130,15);
    assert(a.localSolarHour==b.localSolarHour);
    assert(a.solarElevationDeg==b.solarElevationDeg);
    assert(a.daylightHours==b.daylightHours);
    assert(a.sunshinePotential01>=0 && a.sunshinePotential01<=1);
    EarthSolarObservation unchanged{};

    assert(!earthSolarObservationAt({91,0},100,unchanged));
    assert(!earthSolarObservationAt({0,181},100,unchanged));
    assert(!earthSolarObservationAt(
        {std::numeric_limits<double>::quiet_NaN(),0},100,unchanged));
    EarthSolarObservation negative{};
    assert(earthSolarObservationAt({0,0},-999,negative));
    assert(negative.simulationMinute==0);

    // Solar observation cannot mutate population, save/load, resources
    // or climate; it does not hold a pointer/reference to the world.
    static_assert(EarthSolarAxialTiltDeg>23 && EarthSolarAxialTiltDeg<24);
    return 0;
}
