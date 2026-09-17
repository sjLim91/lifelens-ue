#pragma once

#include <algorithm>
#include <cmath>

#include "Civilization.h"
#include "Needs.h"
#include "SimulationClimate.h"

namespace lifelens {

struct EnvironmentalConsequenceProfile {
    double heatStress01=0.0;
    double coldStress01=0.0;
    double wetStress01=0.0;
    double travelFriction01=0.0;
    double outdoorWorkFriction01=0.0;
    double fireReliability01=1.0;
    double waterReplenishmentMultiplier=1.0;
    double plantFoodRegenerationMultiplier=1.0;
    NeedsDelta perMinuteNeedsDelta{};
};

inline double clampEnvironmentalConsequence01(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

inline EnvironmentalConsequenceProfile deriveEnvironmentalConsequences(
    const DynamicEnvironmentObservation& environment)
{
    EnvironmentalConsequenceProfile result;

    // v1 deliberately models environmental pressure, not full physiology.
    // The comfortable band is broad enough that normal temperate weather has
    // little impact, while sustained heat/cold becomes survival-relevant.
    result.coldStress01=clampEnvironmentalConsequence01((8.0-environment.airTemperatureC)/18.0);
    result.heatStress01=clampEnvironmentalConsequence01((environment.airTemperatureC-28.0)/16.0);
    result.wetStress01=clampEnvironmentalConsequence01(
        0.65*environment.surfaceWetness01+0.35*environment.precipitationIntensity01);

    result.travelFriction01=clampEnvironmentalConsequence01(
        0.45*result.wetStress01+
        0.30*environment.windIntensity01+
        0.25*(1.0-environment.visibility01));
    result.outdoorWorkFriction01=clampEnvironmentalConsequence01(
        0.35*result.wetStress01+
        0.25*environment.windIntensity01+
        0.20*result.coldStress01+
        0.20*result.heatStress01);

    result.fireReliability01=clampEnvironmentalConsequence01(
        1.0-
        0.58*result.wetStress01-
        0.20*environment.windIntensity01-
        0.10*environment.humidity01);

    // Rain/wetness replenish exposed water opportunities, while dry periods
    // suppress them. The bounds keep one weather day from overwhelming the
    // region's authoritative baseline capacity.
    result.waterReplenishmentMultiplier=std::clamp(
        0.55+
        1.15*environment.precipitationIntensity01+
        0.45*environment.surfaceWetness01,
        0.35,2.0);

    // Wild plant-food recovery responds to both thermal comfort and moisture.
    // Crop/agriculture phenology remains a later settlement milestone.
    const double thermalComfort=clampEnvironmentalConsequence01(
        1.0-std::abs(environment.airTemperatureC-19.0)/24.0);
    const double moistureSupport=clampEnvironmentalConsequence01(
        0.35+0.65*environment.surfaceWetness01);
    result.plantFoodRegenerationMultiplier=std::clamp(
        0.20+0.95*thermalComfort*moistureSupport,
        0.15,1.25);

    // Existing Needs remain the authoritative survival-pressure surface.
    // Heat primarily raises thirst; cold raises hunger/fatigue; wet exposure
    // adds fatigue and hygiene pressure. These are intentionally modest/minute.
    result.perMinuteNeedsDelta.thirst=
        0.00045*result.heatStress01+0.00010*result.outdoorWorkFriction01;
    result.perMinuteNeedsDelta.hunger=
        0.00020*result.coldStress01;
    result.perMinuteNeedsDelta.sleep=
        0.00016*(result.coldStress01+result.heatStress01)+
        0.00010*result.wetStress01;
    result.perMinuteNeedsDelta.hygiene=
        0.00012*result.wetStress01;

    return result;
}

inline void applyEnvironmentalNeedPressure(
    Needs& needs,
    const EnvironmentalConsequenceProfile& profile)
{
    needs.apply(profile.perMinuteNeedsDelta);
}

inline int environmentalRegenerationUnits(
    MaterialKind material,
    int baselineUnits,
    const EnvironmentalConsequenceProfile& profile)
{
    if(baselineUnits<=0) return 0;
    double multiplier=1.0;
    if(material==MaterialKind::Water){
        multiplier=profile.waterReplenishmentMultiplier;
    }else if(material==MaterialKind::PlantFood){
        multiplier=profile.plantFoodRegenerationMultiplier;
    }
    return std::max(0,static_cast<int>(std::lround(static_cast<double>(baselineUnits)*multiplier)));
}

} // namespace lifelens
