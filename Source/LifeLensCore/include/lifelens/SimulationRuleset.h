#pragma once

#include <cmath>
#include <cstdint>

namespace lifelens {

using SimulationRulesetVersion = std::uint32_t;
constexpr SimulationRulesetVersion CurrentSimulationRulesetVersion = 1;

struct NeedsRuleset {
    double hungerPerMinute = 0.0010;
    double thirstPerMinute = 0.0013;
    double sleepPerMinute = 0.0008;
    double bladderPerMinute = 0.0011;
    double hygienePerMinute = 0.0007;
};

struct UtilityAIRuleset {
    double needExponent = 4.0;
    double urgentThreshold = 0.70;
    double urgentSlope = 1.8;
    double idleScore = 0.035;

    int sleepNightStartHour = 22;
    int sleepNightEndHour = 6;
    double sleepNightMultiplier = 1.35;

    double washBaseMultiplier = 0.85;
    double washConscientiousnessMultiplier = 0.35;
    double sleepBaseMultiplier = 0.90;
    double sleepIntroversionMultiplier = 0.25;

    double secondChoiceProbability = 0.08;
};

struct SimulationRuleset {
    SimulationRulesetVersion version = CurrentSimulationRulesetVersion;
    NeedsRuleset needs{};
    UtilityAIRuleset utilityAI{};
};

inline constexpr SimulationRuleset DefaultSimulationRuleset{};

inline bool validSimulationRuleset(const SimulationRuleset& rules)
{
    const auto finiteNonNegative=[](double value){
        return std::isfinite(value) && value >= 0.0;
    };
    const auto probability=[](double value){
        return std::isfinite(value) && value >= 0.0 && value <= 1.0;
    };

    if(rules.version != CurrentSimulationRulesetVersion) return false;
    if(!finiteNonNegative(rules.needs.hungerPerMinute)
       || !finiteNonNegative(rules.needs.thirstPerMinute)
       || !finiteNonNegative(rules.needs.sleepPerMinute)
       || !finiteNonNegative(rules.needs.bladderPerMinute)
       || !finiteNonNegative(rules.needs.hygienePerMinute)) return false;

    const UtilityAIRuleset& utility = rules.utilityAI;
    return std::isfinite(utility.needExponent) && utility.needExponent > 0.0
        && probability(utility.urgentThreshold)
        && finiteNonNegative(utility.urgentSlope)
        && finiteNonNegative(utility.idleScore)
        && utility.sleepNightStartHour >= 0 && utility.sleepNightStartHour < 24
        && utility.sleepNightEndHour >= 0 && utility.sleepNightEndHour < 24
        && finiteNonNegative(utility.sleepNightMultiplier)
        && finiteNonNegative(utility.washBaseMultiplier)
        && finiteNonNegative(utility.washConscientiousnessMultiplier)
        && finiteNonNegative(utility.sleepBaseMultiplier)
        && finiteNonNegative(utility.sleepIntroversionMultiplier)
        && probability(utility.secondChoiceProbability);
}

inline bool sameSimulationRuleset(const SimulationRuleset& a,const SimulationRuleset& b)
{
    return a.version == b.version
        && a.needs.hungerPerMinute == b.needs.hungerPerMinute
        && a.needs.thirstPerMinute == b.needs.thirstPerMinute
        && a.needs.sleepPerMinute == b.needs.sleepPerMinute
        && a.needs.bladderPerMinute == b.needs.bladderPerMinute
        && a.needs.hygienePerMinute == b.needs.hygienePerMinute
        && a.utilityAI.needExponent == b.utilityAI.needExponent
        && a.utilityAI.urgentThreshold == b.utilityAI.urgentThreshold
        && a.utilityAI.urgentSlope == b.utilityAI.urgentSlope
        && a.utilityAI.idleScore == b.utilityAI.idleScore
        && a.utilityAI.sleepNightStartHour == b.utilityAI.sleepNightStartHour
        && a.utilityAI.sleepNightEndHour == b.utilityAI.sleepNightEndHour
        && a.utilityAI.sleepNightMultiplier == b.utilityAI.sleepNightMultiplier
        && a.utilityAI.washBaseMultiplier == b.utilityAI.washBaseMultiplier
        && a.utilityAI.washConscientiousnessMultiplier == b.utilityAI.washConscientiousnessMultiplier
        && a.utilityAI.sleepBaseMultiplier == b.utilityAI.sleepBaseMultiplier
        && a.utilityAI.sleepIntroversionMultiplier == b.utilityAI.sleepIntroversionMultiplier
        && a.utilityAI.secondChoiceProbability == b.utilityAI.secondChoiceProbability;
}

} // namespace lifelens
