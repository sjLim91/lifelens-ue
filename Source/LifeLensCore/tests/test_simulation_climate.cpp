#include <cassert>
#include <cmath>

#include "lifelens/SimulationCalendar.h"
#include "lifelens/SimulationClimate.h"
#include "lifelens/WorldGenesis.h"

using namespace lifelens;

int main()
{
    const WorldGenesisIdentity identity=makeWorldGenesisIdentity(
        424242,7,CurrentWorldGenerationVersion);
    const ChunkCoord coord{3,-2};
    const std::int64_t minute=17LL*SimulationMinutesPerDay+13LL*SimulationMinutesPerHour+25;

    const DynamicEnvironmentObservation a=deriveDynamicEnvironment(identity,coord,minute);
    const DynamicEnvironmentObservation b=deriveDynamicEnvironment(identity,coord,minute);

    assert(a.simulationMinute==b.simulationMinute);
    assert(a.coord.x==b.coord.x && a.coord.y==b.coord.y);
    assert(a.airTemperatureC==b.airTemperatureC);
    assert(a.precipitationIntensity01==b.precipitationIntensity01);
    assert(a.cloudCover01==b.cloudCover01);
    assert(a.windIntensity01==b.windIntensity01);
    assert(a.humidity01==b.humidity01);
    assert(a.visibility01==b.visibility01);
    assert(a.surfaceWetness01==b.surfaceWetness01);
    assert(a.precipitationType==b.precipitationType);
    assert(a.summary==b.summary);

    // PopulationSeed must not alter natural climate truth.
    const WorldGenesisIdentity otherPopulation=makeWorldGenesisIdentity(
        424242,9999,CurrentWorldGenerationVersion);
    const DynamicEnvironmentObservation sameWorldDifferentPopulation=
        deriveDynamicEnvironment(otherPopulation,coord,minute);
    assert(a.airTemperatureC==sameWorldDifferentPopulation.airTemperatureC);
    assert(a.precipitationIntensity01==sameWorldDifferentPopulation.precipitationIntensity01);
    assert(a.cloudCover01==sameWorldDifferentPopulation.cloudCover01);

    assert(a.baselineTemperature01>=0.0 && a.baselineTemperature01<=1.0);
    assert(a.baselineMoisture01>=0.0 && a.baselineMoisture01<=1.0);
    assert(a.precipitationIntensity01>=0.0 && a.precipitationIntensity01<=1.0);
    assert(a.cloudCover01>=0.0 && a.cloudCover01<=1.0);
    assert(a.windIntensity01>=0.0 && a.windIntensity01<=1.0);
    assert(a.humidity01>=0.0 && a.humidity01<=1.0);
    assert(a.visibility01>=0.0 && a.visibility01<=1.0);
    assert(a.surfaceWetness01>=0.0 && a.surfaceWetness01<=1.0);

    // Annual forcing is explicit and must remain directionally sane.
    const std::int64_t summerMinute=137LL*SimulationMinutesPerDay+12LL*SimulationMinutesPerHour;
    const std::int64_t winterMinute=319LL*SimulationMinutesPerDay+12LL*SimulationMinutesPerHour;
    const DynamicEnvironmentObservation summer=deriveDynamicEnvironment(identity,coord,summerMinute);
    const DynamicEnvironmentObservation winter=deriveDynamicEnvironment(identity,coord,winterMinute);
    assert(summer.seasonalTemperatureModifierC>winter.seasonalTemperatureModifierC);

    // Smooth time interpolation should not jump wildly from one minute to the next.
    const DynamicEnvironmentObservation nextMinute=deriveDynamicEnvironment(identity,coord,minute+1);
    assert(std::abs(nextMinute.airTemperatureC-a.airTemperatureC)<1.0);
    assert(std::abs(nextMinute.cloudCover01-a.cloudCover01)<0.05);
    assert(std::abs(nextMinute.humidity01-a.humidity01)<0.05);

    if(a.precipitationIntensity01<0.05){
        assert(a.precipitationType==PrecipitationType::None);
    } else if(a.airTemperatureC<=1.0){
        assert(a.precipitationType==PrecipitationType::Snow);
    } else {
        assert(a.precipitationType==PrecipitationType::Rain);
    }

    const DynamicEnvironmentObservation negative=deriveDynamicEnvironment(identity,coord,-100);
    assert(negative.simulationMinute==0);

    return 0;
}
