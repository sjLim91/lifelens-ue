#pragma once

#include <cstdint>
#include <cstring>

#include "SimulationRuleset.h"

namespace lifelens {

constexpr char SimulationRulesetSnapshotExtensionMagic[]={'L','L','R','U','L','E','0','1'};
constexpr std::uint32_t SimulationRulesetSnapshotExtensionVersion=1;

template<typename WriterT>
void writeSimulationRulesetSnapshotExtension(WriterT& w,const SimulationRuleset& rules)
{
    w.raw(SimulationRulesetSnapshotExtensionMagic,sizeof(SimulationRulesetSnapshotExtensionMagic));
    w.u32(SimulationRulesetSnapshotExtensionVersion);
    w.u32(rules.version);

    w.real(rules.needs.hungerPerMinute);
    w.real(rules.needs.thirstPerMinute);
    w.real(rules.needs.sleepPerMinute);
    w.real(rules.needs.bladderPerMinute);
    w.real(rules.needs.hygienePerMinute);

    w.real(rules.utilityAI.needExponent);
    w.real(rules.utilityAI.urgentThreshold);
    w.real(rules.utilityAI.urgentSlope);
    w.real(rules.utilityAI.idleScore);
    w.i32(rules.utilityAI.sleepNightStartHour);
    w.i32(rules.utilityAI.sleepNightEndHour);
    w.real(rules.utilityAI.sleepNightMultiplier);
    w.real(rules.utilityAI.washBaseMultiplier);
    w.real(rules.utilityAI.washConscientiousnessMultiplier);
    w.real(rules.utilityAI.sleepBaseMultiplier);
    w.real(rules.utilityAI.sleepIntroversionMultiplier);
    w.real(rules.utilityAI.secondChoiceProbability);
}

template<typename ReaderT>
bool readSimulationRulesetSnapshotExtension(ReaderT& r,SimulationRuleset& rules)
{
    char magic[sizeof(SimulationRulesetSnapshotExtensionMagic)]{};
    std::uint32_t extensionVersion=0;
    if(!r.raw(magic,sizeof(magic))
       || std::memcmp(magic,SimulationRulesetSnapshotExtensionMagic,sizeof(magic))!=0
       || !r.u32(extensionVersion)
       || extensionVersion!=SimulationRulesetSnapshotExtensionVersion
       || !r.u32(rules.version)
       || !r.real(rules.needs.hungerPerMinute)
       || !r.real(rules.needs.thirstPerMinute)
       || !r.real(rules.needs.sleepPerMinute)
       || !r.real(rules.needs.bladderPerMinute)
       || !r.real(rules.needs.hygienePerMinute)
       || !r.real(rules.utilityAI.needExponent)
       || !r.real(rules.utilityAI.urgentThreshold)
       || !r.real(rules.utilityAI.urgentSlope)
       || !r.real(rules.utilityAI.idleScore)
       || !r.i32(rules.utilityAI.sleepNightStartHour)
       || !r.i32(rules.utilityAI.sleepNightEndHour)
       || !r.real(rules.utilityAI.sleepNightMultiplier)
       || !r.real(rules.utilityAI.washBaseMultiplier)
       || !r.real(rules.utilityAI.washConscientiousnessMultiplier)
       || !r.real(rules.utilityAI.sleepBaseMultiplier)
       || !r.real(rules.utilityAI.sleepIntroversionMultiplier)
       || !r.real(rules.utilityAI.secondChoiceProbability)) return false;

    return validSimulationRuleset(rules);
}

} // namespace lifelens
