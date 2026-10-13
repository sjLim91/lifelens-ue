#pragma once
#include "World.h"
#include "SimulationClimate.h"

namespace lifelens {
inline void advanceSnowpack(SnowpackState& pack, const DynamicEnvironmentObservation& weather)
{
    if(weather.simulationMinute<=pack.minute) return;
    const double minutes=static_cast<double>(weather.simulationMinute-pack.minute);
    const double snow=weather.precipitationType==PrecipitationType::Snow
        ? weather.precipitationIntensity01*0.00008 : 0.0;
    const double rain=weather.precipitationType==PrecipitationType::Rain
        ? weather.precipitationIntensity01*0.00002 : 0.0;
    const double melt=std::max(0.0,weather.airTemperatureC)*0.000012+rain;
    pack.depthMeters=std::clamp(pack.depthMeters+(snow-melt)*minutes,0.0,2.0);
    pack.minute=static_cast<int>(weather.simulationMinute);
}
inline void advanceNaturalEcology(World& world)
{
    for(auto& node:world.resourceNodes){node.ecologyMinute=world.minute; node.ensureTrees();}
    // Five simulation minutes is the authoritative ecology integration quantum;
    // it bounds climate work during fast-forward and survives any save boundary.
    if(world.minute%5!=0) return;
    for(const auto& chunk:world.generatedNaturalChunks){
        auto it=std::lower_bound(world.snowpacks.begin(),world.snowpacks.end(),chunk.coord,
            [](const SnowpackState& a,ChunkCoord b){return a.coord<b;});
        if(it==world.snowpacks.end() || !(it->coord==chunk.coord))
            it=world.snowpacks.insert(it,{chunk.coord,0.0,std::min(world.minute,std::max(chunk.materializedMinute,std::max(0,world.minute-5)))});
        advanceSnowpack(*it,deriveDynamicEnvironment(world.genesisIdentity(),chunk.coord,world.minute));
    }
    const auto calendar=deriveSimulationCalendar(world.minute);
    for(auto& node:world.resourceNodes){
        node.ecologyMinute=world.minute;
        node.ensureTrees();
        if(node.material!=MaterialKind::PlantFood && node.material!=MaterialKind::Fiber) continue;
        const auto coord=chunkCoordForGrid(node.pos);
        const auto weather=deriveDynamicEnvironment(world.genesisIdentity(),coord,world.minute);
        const double stock=node.maxQuantity>0 ? static_cast<double>(node.quantity)/node.maxQuantity : 0.0;
        const double comfort=climateClamp01((weather.airTemperatureC-3.0)/12.0);
        const double phase=calendar.annualPhase;
        node.flower01=stock*comfort*climateClamp01(1.0-std::abs(phase-0.30)*9.0);
        node.fruit01=node.material==MaterialKind::PlantFood
            ? stock*comfort*climateClamp01(1.0-std::abs(phase-0.60)*7.0) : 0.0;
    }
}
inline double snowDepthAt(const World& world,ChunkCoord coord)
{
    const auto it=std::lower_bound(world.snowpacks.begin(),world.snowpacks.end(),coord,
        [](const SnowpackState& a,ChunkCoord b){return a.coord<b;});
    return it!=world.snowpacks.end() && it->coord==coord ? it->depthMeters : 0.0;
}
}
