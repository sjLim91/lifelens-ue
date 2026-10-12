#pragma once
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include "EnvironmentalExposure.h"

namespace lifelens {

// Selecting one of several actually available sources is a strategy choice;
// a resident's need for water must not be weakened by previous bad experiences.
// The existing EnvironmentalExposure system produces firsthand, persisted
// grid-specific memories. This module reads those memories only.
struct ExperiencedWaterAccess {
    ResourceNodeId resource=0;
    GridPos position{};
};

inline double directlyRememberedWaterHazard(
    const Character& resident,GridPos destination,int minute)
{
    const std::string location=environmentalGridLabel(destination);
    double hazard=0.0;
    for(const MemoryRecord& memory:resident.memory.entries){
        if(memory.where!=location
           || memory.minute>minute
           || memory.source!=MemorySource::DirectWitness
           || !memory.witnessed
           || !hasMemoryTag(memory,"contamination")) continue;
        // Do not assume witnessing waste proves a specific infection.
        // Confidence fades with time; unseen alternatives are neutral.
        const double evidence=memory.effectiveConfidence(minute)
            *MemoryRecord::clamp01(memory.importance);
        hazard=std::max(hazard,evidence);
    }
    return hazard;
}

inline bool chooseExperiencedWaterAccess(
    const Character& resident,GridPos origin,int minute,
    const std::vector<ExperiencedWaterAccess>& candidates,
    GridPos& chosen)
{
    bool found=false;
    ResourceNodeId bestId=0;
    double bestCost=std::numeric_limits<double>::infinity();
    // A remembered problem justifies a *short* detour, not an indefinitely
    // long trek that would endanger an already thirsty resident.
    constexpr double RememberedWaterHazardDetourLimitTiles=3.0;
    for(const ExperiencedWaterAccess& source:candidates){
        if(source.resource==0) continue;
        const double travel=static_cast<double>(
            std::abs(source.position.x-origin.x)
            +std::abs(source.position.y-origin.y));
        const double cost=travel
            +RememberedWaterHazardDetourLimitTiles
               *directlyRememberedWaterHazard(
                   resident,source.position,minute);
        if(!found || cost<bestCost
           || (cost==bestCost && source.resource<bestId)){
            found=true;
            bestId=source.resource;
            bestCost=cost;
            chosen=source.position;
        }
    }
    return found;
}

} // namespace lifelens
