#pragma once

#include <algorithm>
#include <string>
#include "EarthSurfaceCoordinates.h"

namespace lifelens {

// An explicit geographic binding is World state. It does not claim that the
// procedural local terrain/climate is conditioned by real Earth data.
struct EarthScenarioState {
    bool available=false;
    WorldSeed worldSeed=0;
    WorldGenerationVersion generationVersion=0;
    EarthGeoPoint geographicOrigin{};
    GridPos localGridOrigin{};
    double metersPerGridCell=0.0;
    std::string earthDataVersion;
    std::string geographySource;
};

inline bool validEarthScenarioToken(const std::string& value)
{
    return !value.empty() &&value.size()<=128
        &&std::all_of(value.begin(),value.end(),[](unsigned char ch){
            return (ch>='0'&&ch<='9')||(ch>='a'&&ch<='z')
                ||(ch>='A'&&ch<='Z')||ch=='_'||ch=='-'
                ||ch=='.'||ch==':';
        });
}

inline bool validEarthScenarioState(
    const EarthScenarioState& state,const WorldGenesisIdentity& identity)
{
    if(!state.available)
        return state.worldSeed==0 &&state.generationVersion==0
            &&state.geographicOrigin.latitudeDeg==0.0
            &&state.geographicOrigin.longitudeDeg==0.0
            &&state.localGridOrigin.x==0 &&state.localGridOrigin.y==0
            &&state.metersPerGridCell==0.0
            &&state.earthDataVersion.empty() &&state.geographySource.empty();
    return state.worldSeed==identity.worldSeed
        &&state.generationVersion==identity.generationVersion
        &&earthValidGeo(state.geographicOrigin)
        &&std::isfinite(state.metersPerGridCell)
        &&state.metersPerGridCell>0.0
        &&validEarthScenarioToken(state.earthDataVersion)
        &&validEarthScenarioToken(state.geographySource);
}

} // namespace lifelens
