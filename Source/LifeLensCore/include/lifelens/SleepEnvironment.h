#pragma once

#include <algorithm>
#include "EnvironmentalConsequences.h"

namespace lifelens {

// Central sleep policy. Uses the existing climate/consequence authority only.
struct SleepEnvironmentContract {
    static constexpr double ProtectionPreferredExposure = 0.35;
    static constexpr double EmergencyExposure = 0.60;
    static constexpr double MaximumRecoveryPenalty = 0.25;
    static constexpr double ProtectedResidualExposure = 0.15;
    static constexpr double ExposureTravelCost = 24.0;
    static constexpr int ReplanIntervalMinutes = 15;
    static constexpr double ReplanCostImprovement = 4.0;
    static constexpr int MinimumTravelBudgetCells = 8;
    static constexpr int MaximumTravelBudgetCells = 32;
};

enum class SleepContext { None, Protected, Exposed, ExposedEmergency };

inline const char* sleepContextName(SleepContext context)
{
    switch(context){
        case SleepContext::Protected: return "Protected";
        case SleepContext::Exposed: return "Exposed";
        case SleepContext::ExposedEmergency: return "ExposedEmergency";
        default: return "None";
    }
}

struct SleepEnvironmentEvaluation {
    double exposure01=0.0;
    double recoveryMultiplier01=1.0;
    bool weatherProtectionPreferred=false;
    bool exposedEmergencyOnly=false;
};

inline SleepEnvironmentEvaluation evaluateSleepEnvironment(
    const DynamicEnvironmentObservation& weather,
    const EnvironmentalConsequenceProfile& consequence,
    bool weatherProtected)
{
    SleepEnvironmentEvaluation result;
    // A strong individual hazard must not be averaged away (rain OR snow,
    // wind, wet ground, cold or heat). Snow uses existing precipitation truth.
    result.exposure01=std::clamp(std::max({
        weather.precipitationIntensity01,
        0.80*consequence.wetStress01,
        0.75*weather.windIntensity01,
        consequence.coldStress01,
        consequence.heatStress01}),0.0,1.0);
    result.weatherProtectionPreferred=
        result.exposure01>=SleepEnvironmentContract::ProtectionPreferredExposure;
    result.exposedEmergencyOnly=!weatherProtected
        && result.exposure01>=SleepEnvironmentContract::EmergencyExposure;
    const double residual=weatherProtected
        ? SleepEnvironmentContract::ProtectedResidualExposure : 1.0;
    result.recoveryMultiplier01=1.0
        -SleepEnvironmentContract::MaximumRecoveryPenalty*result.exposure01*residual;
    return result;
}

inline int sleepTravelBudgetCells(double sleepNeed)
{
    const double urgency=std::clamp((sleepNeed-0.60)/0.40,0.0,1.0);
    return static_cast<int>(SleepEnvironmentContract::MaximumTravelBudgetCells
        -urgency*(SleepEnvironmentContract::MaximumTravelBudgetCells
            -SleepEnvironmentContract::MinimumTravelBudgetCells));
}

inline double sleepCandidateCost(
    const SleepEnvironmentEvaluation& environment,
    bool weatherProtected,
    int travelDistance,
    double sleepNeed,
    double effectiveness01)
{
    const double exposedPenalty=environment.weatherProtectionPreferred
        && !weatherProtected
        ? SleepEnvironmentContract::ExposureTravelCost*environment.exposure01 : 0.0;
    return std::max(0,travelDistance)*(1.0+std::clamp(sleepNeed,0.0,1.0))
        +exposedPenalty+2.0*(1.0-std::clamp(effectiveness01,0.0,1.0));
}

} // namespace lifelens
