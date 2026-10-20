#pragma once
#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <utility>

#include "ResidentLocalPerception.h"

namespace lifelens {

// The Core *proximity* sensor is not yet line-of-sight. A resident may infer
// a resource opportunity from proximity, but must never store it as a
// firsthand VISUAL sighting or a promise of future supply/reachability.
inline constexpr std::size_t ResidentSensedResourceMemoryLimit=12;
inline constexpr int ResidentSensedResourceMemoryCadenceMinutes=720;

inline bool isResidentSensedResourceMemory(
    const MemoryRecord& memory,CharacterId actor)
{
    return memory.source==MemorySource::Inferred
        &&!memory.witnessed &&memory.sourceCharacter==actor
        &&memory.tags.size()==3
        &&memory.tags.front()=="proximity_resource_inference"
        &&memory.tags[1].rfind("node:",0)==0;
}

// Real Core caller must provide a current proximity scan from actor's actual
// runtime position. Returns NEW episodes only; refreshes are not discoveries.
// Mutates only snapshot-backed personal memories, never the World resource
// ledger, paths, RNG, physiology or another resident's hidden knowledge.
inline std::size_t rememberResidentSensedLocalResources(
    Character& actor,const ResidentLocalPerception& view)
{
    if(!actor.alive ||actor.id==0 ||view.actor!=actor.id
       ||view.minute<0 ||!view.proximityOnly
       ||view.radiusCells<1
       ||view.radiusCells>ResidentLocalPerceptionMaxRadius)
        return 0;
    std::size_t newlyRemembered=0;
    // Capture known node IDs BEFORE inserting or evicting anything. A test
    // fixture (or future observer) may hand us more than the 12 normal sensor
    // facts. Evicting an early record must not make that SAME fact look like
    // a new discovery later in this scan.
    std::set<std::string> seenInThisScan;
    for(const auto& memory:actor.memory.entries){
        if(isResidentSensedResourceMemory(memory,actor.id))
            seenInThisScan.insert(memory.tags[1]);
    }
    for(const auto& source:view.nearbyResources){
        if(source.nodeId==0 ||source.material==MaterialKind::Unknown
           ||source.distanceCells<0
           ||source.distanceCells>view.radiusCells) continue;
        const std::string nodeTag="node:"+std::to_string(source.nodeId);
        const std::string kindTag="material:"
            +std::to_string(static_cast<int>(source.material));
        const std::string where="grid:"
            +std::to_string(source.position.x)+","
            +std::to_string(source.position.y);
        const std::string what="근처에서 자원 존재 신호를 감지했다. "
            "현재도 남아 있거나 접근 가능한지는 알 수 없다.";
        auto it=std::find_if(
            actor.memory.entries.begin(),actor.memory.entries.end(),
            [&](const MemoryRecord& memory){
                return isResidentSensedResourceMemory(memory,actor.id)
                    &&memory.tags[1]==nodeTag;
            });
        if(it!=actor.memory.entries.end()){
            if(it->minute<=view.minute){
                it->minute=view.minute;
                it->where=where;
                it->what=what;
                it->tags[2]=kindTag;
                it->confidence=0.60;
            }
            continue;
        }
        if(seenInThisScan.count(nodeTag)) continue;
        seenInThisScan.insert(nodeTag);
        // Only this specific inference-memory class is subject to eviction;
        // never erase real executed action successes, loss or private bonds.
        std::size_t count=0;
        auto oldest=actor.memory.entries.end();
        for(auto existing=actor.memory.entries.begin();
            existing!=actor.memory.entries.end();++existing){
            if(!isResidentSensedResourceMemory(*existing,actor.id))
                continue;
            ++count;
            if(oldest==actor.memory.entries.end()
               ||existing->minute<oldest->minute)
                oldest=existing;
        }
        if(count>=ResidentSensedResourceMemoryLimit
           &&oldest!=actor.memory.entries.end())
            actor.memory.entries.erase(oldest);
        MemoryRecord memory;
        memory.who=actor.id;
        memory.sourceCharacter=actor.id;
        memory.what=what;
        memory.where=where;
        memory.minute=view.minute;
        memory.source=MemorySource::Inferred;
        memory.witnessed=false;
        memory.importance=0.32;
        memory.confidence=0.60;
        memory.decayPerDay=0.025;
        memory.tags={"proximity_resource_inference",nodeTag,kindTag};
        actor.memory.add(std::move(memory));
        ++newlyRemembered;
    }
    return newlyRemembered;
}

} // namespace lifelens
