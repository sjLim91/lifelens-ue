#pragma once

#include <algorithm>

#include "EnvironmentalExposure.h"
#include "SanitationProblemRecognition.h"

namespace lifelens {

struct PrimitiveSanitationOpportunity {
    bool problemRecognized=false;
    bool siteAvailable=false;
    GridPos suggestedSite{};
    double siteExposure=1.0;
    double problemConfidence=0.0;
};

inline constexpr double PrimitiveSanitationCleanSiteExposureLimit=0.08;

inline double recognizedSanitationProblemConfidence(const Character& character)
{
    const BeliefRecord* belief=character.beliefs.find(
        0,sanitationProblemBeliefProposition());
    return isEstablishedSanitationProblemBelief(belief)
        ? std::max(0.0,std::min(1.0,belief->confidence))
        : 0.0;
}

inline PrimitiveSanitationOpportunity evaluatePrimitiveSanitationOpportunity(
    std::uint64_t worldSeed,
    const Character& character,
    const EnvironmentalResidueField& field,
    int currentMinute)
{
    PrimitiveSanitationOpportunity result;
    result.problemRecognized=hasRecognizedSanitationProblem(character);
    if(!result.problemRecognized) return result;

    result.problemConfidence=recognizedSanitationProblemConfidence(character);
    result.suggestedSite=chooseLowExposureOutdoorReliefPosition(
        worldSeed,character,field,currentMinute);
    result.siteExposure=field.exposureAt(result.suggestedSite);
    result.siteAvailable=result.siteExposure<PrimitiveSanitationCleanSiteExposureLimit;
    return result;
}

} // namespace lifelens
