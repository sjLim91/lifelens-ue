#pragma once

#include <algorithm>

#include "Genetics.h"
#include "Personality.h"

namespace lifelens {

inline double clampDisposition(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

// Named resident traits are an authoritative Core read model derived from
// persistent Core personality/genetic state. They are deliberately not stored
// as a second mutable authority, so Save/Load cannot desynchronize them from
// the underlying resident state.
struct TraitProfile {
    double resilience=0.5;
    double creativity=0.5;
    double discipline=0.5;
    double compassion=0.5;
    double adaptability=0.5;
    double boldness=0.5;
    double perseverance=0.5;
    double resourcefulness=0.5;
};

// Preferences use the same rule: explicit named Core semantics, deterministic
// from persistent resident state, and presentation only consumes the result.
struct PreferenceProfile {
    double socializing=0.5;
    double solitude=0.5;
    double exploration=0.5;
    double crafting=0.5;
    double gathering=0.5;
    double comfort=0.5;
    double novelty=0.5;
    double order=0.5;
};

inline TraitProfile deriveTraitProfile(
    const Personality& personality,
    const GeneticsProfile& genetics)
{
    TraitProfile profile;
    profile.resilience=clampDisposition(
        personality.emotionalStability*0.42+
        personality.adaptability*0.33+
        genetics.healthPotential*0.25);
    profile.creativity=clampDisposition(
        personality.openness*0.40+
        personality.curiosity*0.35+
        genetics.learningPotential*0.25);
    profile.discipline=clampDisposition(
        personality.conscientiousness*0.45+
        personality.orderliness*0.30+
        personality.patience*0.25);
    profile.compassion=clampDisposition(
        personality.empathy*0.55+
        personality.agreeableness*0.30+
        (1.0-personality.impulsiveness)*0.15);
    profile.adaptability=clampDisposition(
        personality.adaptability*0.55+
        personality.openness*0.25+
        personality.emotionalStability*0.20);
    profile.boldness=clampDisposition(
        personality.riskTolerance*0.50+
        (1.0-personality.introversion)*0.30+
        personality.ambition*0.20);
    profile.perseverance=clampDisposition(
        personality.patience*0.45+
        personality.conscientiousness*0.35+
        personality.ambition*0.20);
    profile.resourcefulness=clampDisposition(
        personality.adaptability*0.35+
        personality.curiosity*0.30+
        personality.openness*0.20+
        genetics.learningPotential*0.15);
    return profile;
}

inline PreferenceProfile derivePreferenceProfile(
    const Personality& personality,
    const GeneticsProfile& genetics)
{
    PreferenceProfile profile;
    profile.socializing=clampDisposition(
        personality.sociability*0.45+
        personality.agreeableness*0.25+
        personality.empathy*0.20+
        (1.0-personality.introversion)*0.10);
    profile.solitude=clampDisposition(
        personality.introversion*0.55+
        personality.emotionalStability*0.25+
        personality.patience*0.20);
    profile.exploration=clampDisposition(
        personality.curiosity*0.35+
        personality.openness*0.30+
        personality.riskTolerance*0.20+
        personality.adaptability*0.15);
    profile.crafting=clampDisposition(
        personality.conscientiousness*0.35+
        personality.openness*0.25+
        personality.patience*0.25+
        personality.orderliness*0.15);
    profile.gathering=clampDisposition(
        personality.patience*0.30+
        personality.adaptability*0.25+
        personality.conscientiousness*0.25+
        genetics.healthPotential*0.20);
    profile.comfort=clampDisposition(
        (1.0-personality.riskTolerance)*0.40+
        personality.orderliness*0.30+
        personality.emotionalStability*0.30);
    profile.novelty=clampDisposition(
        personality.openness*0.50+
        personality.curiosity*0.35+
        personality.adaptability*0.15);
    profile.order=clampDisposition(
        personality.orderliness*0.50+
        personality.conscientiousness*0.30+
        personality.patience*0.20);
    return profile;
}

inline bool validTraitProfile(const TraitProfile& profile)
{
    const auto valid=[](double value){ return value>=0.0 && value<=1.0; };
    return valid(profile.resilience)
        && valid(profile.creativity)
        && valid(profile.discipline)
        && valid(profile.compassion)
        && valid(profile.adaptability)
        && valid(profile.boldness)
        && valid(profile.perseverance)
        && valid(profile.resourcefulness);
}

inline bool validPreferenceProfile(const PreferenceProfile& profile)
{
    const auto valid=[](double value){ return value>=0.0 && value<=1.0; };
    return valid(profile.socializing)
        && valid(profile.solitude)
        && valid(profile.exploration)
        && valid(profile.crafting)
        && valid(profile.gathering)
        && valid(profile.comfort)
        && valid(profile.novelty)
        && valid(profile.order);
}

} // namespace lifelens
