#include "lifelens/WebClientBridge.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

using namespace lifelens;

// Compare with the Core's actual simulation clock (new worlds start at 07:00),
// rather than assuming their absolute initial minute is zero.
static std::int64_t jsonSimulationMinute(const std::string& json)
{
    const std::string key="\"minute\":";
    const auto pos=json.find(key);
    assert(pos!=std::string::npos);
    return std::stoll(json.substr(pos+key.size()));
}

int main()
{
    constexpr const char* Seed="4242001";
    WebClientBridge legacy;
    assert(legacy.newGame(Seed,"",3));
    assert(legacy.earthScenarioJson()=="{\"available\":false}");
    assert(legacy.earthSolarJson().find("false")!=std::string::npos);
    const auto before=legacy.worldOverviewJson();
    const auto people=legacy.residentsJson();
    const auto terrain=legacy.terrainWindowJson(0,0,8);

    // Fail closed: no ocean, invalid latitude, NaN, infinity or wrong model.
    assert(!legacy.newEarthGame(Seed,0.0,-150.0,3)); // south Pacific
    assert(!legacy.newEarthGame(Seed,100.0,129.0,3));
    assert(!legacy.newEarthGame(
        Seed,std::numeric_limits<double>::quiet_NaN(),129.0,3));
    assert(!legacy.newEarthGame(
        Seed,35.0,std::numeric_limits<double>::infinity(),3));
    assert(legacy.worldOverviewJson()==before);
    assert(legacy.earthScenarioJson()=="{\"available\":false}");

    WebClientBridge earth;
    assert(earth.newEarthGame(Seed,37.0,127.0,3));
    const std::string binding=earth.earthScenarioJson();
    const std::string initialSolar=earth.earthSolarJson();
    const auto initialMinute=jsonSimulationMinute(initialSolar);
    assert(initialMinute==jsonSimulationMinute(earth.worldOverviewJson()));
    assert(initialMinute==7*60); // Canonical new-world start is 07:00.
    assert(initialSolar.find("\"available\":true")!=std::string::npos);
    assert(initialSolar.find("\"source\":\"CoreEarthSolar\"")
        !=std::string::npos);
    assert(initialSolar.find("\"worldSeed\":\"4242001\"")
        !=std::string::npos);
    assert(initialSolar.find("\"latitudeDeg\":37")!=std::string::npos);
    assert(initialSolar.find("\"longitudeDeg\":127")!=std::string::npos);
    assert(initialSolar.find("\"regionalWeatherApplied\":false")
        !=std::string::npos);
    assert(initialSolar.find("\"localSolarHour\":")!=std::string::npos);
    assert(initialSolar.find("\"daylightHours\":")!=std::string::npos);

    assert(binding.find("\"available\":true")!=std::string::npos);
    assert(binding.find("\"source\":\"CoreEarthScenario\"")!=std::string::npos);
    assert(binding.find("\"latitudeDeg\":37")!=std::string::npos);
    assert(binding.find("\"longitudeDeg\":127")!=std::string::npos);
    assert(binding.find("\"worldSeed\":\"4242001\"")!=std::string::npos);
    assert(binding.find("\"terrainGeographicallyConditioned\":false")
           !=std::string::npos);
    assert(binding.find("\"snapshotPersistence\":false")
           !=std::string::npos);
    // Explicit Earth observation must NOT change the ordinary LifeLens
    // founder roster, procedural terrain, climate, RNG or daily actions.
    assert(earth.worldOverviewJson()==before);
    assert(earth.residentsJson()==people);
    assert(earth.terrainWindowJson(0,0,8)==terrain);
    legacy.runMinutes(22);
    earth.runMinutes(22);
    assert(earth.earthSolarJson()!=initialSolar);
    // Advancing 22 minutes must preserve the real Core clock at both endpoints.
    const auto progressedMinute=jsonSimulationMinute(earth.earthSolarJson());
    assert(progressedMinute==initialMinute+22);
    assert(progressedMinute==jsonSimulationMinute(earth.worldOverviewJson()));
    assert(progressedMinute==jsonSimulationMinute(legacy.worldOverviewJson()));
    assert(earth.worldOverviewJson()==legacy.worldOverviewJson());
    assert(earth.residentsJson()==legacy.residentsJson());

    // New ordinary world unlinks previous mapped coordinate atomically.
    assert(earth.newGame("874213954","",3));
    assert(earth.earthSolarJson().find("false")!=std::string::npos);
    assert(earth.earthScenarioJson()=="{\"available\":false}");
    // Failed rebind also preserves that existing unrelated world unchanged.
    const auto other=earth.worldOverviewJson();
    assert(!earth.newEarthGame("999",0.0,-150.0,3));
    assert(earth.worldOverviewJson()==other);
    return 0;
}
