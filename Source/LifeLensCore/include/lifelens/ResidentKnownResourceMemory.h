#pragma once

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include "ResidentLocalPerception.h"
#include "ResidentResourceOutcomePreference.h"

namespace lifelens {

// A dated personal hypothesis, never a live view of a distant source. The
// existing Core proximity sensor is not line-of-sight, so these episodes stay
// Inferred. Neither identity nor stock is discovered by the recall operation.
struct ResidentKnownResource {
    ResourceNodeId nodeId=0;
    MaterialKind material=MaterialKind::Unknown;
    GridPos position{};
    int observedQuantity=0;
    int minute=-1;
    double confidence=0.0;
};

inline constexpr std::size_t ResidentKnownResourceMemoryLimit=12;
inline constexpr const char* ResidentKnownResourceMemoryTag="resource_observation_v1";
inline constexpr const char* ResidentKnownResourceMemoryDescription=
    "근처 자원의 위치와 당시 수량을 감지했다. 현재 잔량과 접근 가능성은 다시 확인해야 한다.";

namespace resident_known_resource_detail {

template<class Integer>
inline bool parseInteger(const std::string& value,Integer& result)
{
    if(value.empty()) return false;
    const auto parsed=std::from_chars(value.data(),value.data()+value.size(),result);
    return parsed.ec==std::errc{} &&parsed.ptr==value.data()+value.size()
        &&std::to_string(result)==value;
}

template<class Integer>
inline bool parseTag(const std::string& tag,const char* prefix,Integer& result)
{
    const std::string start(prefix);
    return tag.rfind(start,0)==0 &&parseInteger(tag.substr(start.size()),result);
}

inline bool belongsToActor(const MemoryRecord& memory,CharacterId actor)
{
    return actor!=0 &&memory.who==actor &&memory.sourceCharacter==actor
        &&memory.source==MemorySource::Inferred &&!memory.witnessed
        &&!memory.tags.empty() &&memory.tags.front()==ResidentKnownResourceMemoryTag;
}

} // namespace resident_known_resource_detail

inline bool readResidentKnownResource(
    const MemoryRecord& memory,CharacterId actor,int currentMinute,
    ResidentKnownResource& result)
{
    using namespace resident_known_resource_detail;
    if(!belongsToActor(memory,actor) ||memory.tags.size()!=4
       ||memory.what!=ResidentKnownResourceMemoryDescription
       ||memory.minute<0 ||currentMinute<memory.minute
       ||static_cast<std::int64_t>(currentMinute)-memory.minute
            >ResidentResourceOutcomeHorizonMinutes
       ||!std::isfinite(memory.confidence) ||memory.confidence<=0.0
       ||memory.confidence>1.0 ||!std::isfinite(memory.importance)
       ||memory.importance<0.0 ||memory.importance>1.0
       ||!std::isfinite(memory.emotionIntensity)
       ||memory.emotionIntensity<0.0 ||memory.emotionIntensity>1.0
       ||!std::isfinite(memory.decayPerDay) ||memory.decayPerDay<0.0)
        return false;
    ResidentKnownResource parsed;
    int material=0;
    if(!parseTag(memory.tags[1],"node:",parsed.nodeId) ||parsed.nodeId==0
       ||!parseTag(memory.tags[2],"material:",material)
       ||material<static_cast<int>(MaterialKind::Stone)
       ||material>static_cast<int>(MaterialKind::Bronze)
       ||!parseTag(memory.tags[3],"quantity:",parsed.observedQuantity)
       ||parsed.observedQuantity<=0 ||memory.where.rfind("grid:",0)!=0)
        return false;
    const auto separator=memory.where.find(',',5);
    if(separator==std::string::npos
       ||!parseInteger(memory.where.substr(5,separator-5),parsed.position.x)
       ||!parseInteger(memory.where.substr(separator+1),parsed.position.y))
        return false;
    parsed.material=static_cast<MaterialKind>(material);
    parsed.minute=memory.minute;
    parsed.confidence=memory.effectiveConfidence(currentMinute);
    if(!std::isfinite(parsed.confidence) ||parsed.confidence<=0.0) return false;
    result=parsed;
    return true;
}

// Deterministic bounded recall from this resident only. Quantity deliberately
// remains what was sensed then, even if a distant node has since disappeared.
inline std::vector<ResidentKnownResource> residentKnownResources(
    const Character& actor,int currentMinute)
{
    std::vector<ResidentKnownResource> result;
    if(!actor.alive ||actor.id==0 ||currentMinute<0) return result;
    result.reserve(ResidentKnownResourceMemoryLimit);
    for(const auto& memory:actor.memory.entries){
        ResidentKnownResource known;
        if(!readResidentKnownResource(memory,actor.id,currentMinute,known)) continue;
        auto same=std::find_if(result.begin(),result.end(),[&](const auto& previous){
            return previous.nodeId==known.nodeId;
        });
        if(same!=result.end()){
            // Same-minute contradictory duplicated snapshots retain the more
            // conservative amount; vector order must not change a plan.
            if(known.minute>same->minute
               ||(known.minute==same->minute
                   &&known.observedQuantity<same->observedQuantity)) *same=known;
        }else result.push_back(known);
        std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){
            return a.minute!=b.minute ? a.minute>b.minute : a.nodeId<b.nodeId;
        });
        if(result.size()>ResidentKnownResourceMemoryLimit) result.pop_back();
    }
    return result;
}

// Call only from the resident's authoritative runtime position. The existing
// memory sensor's radius 6 / four nearest facts is unchanged. Reading stock
// is restricted to those currently sensed IDs, never remembered remote IDs.
inline std::size_t rememberResidentKnownLocalResources(
    const World& world,Character& actor,GridPos actualPosition)
{
    using namespace resident_known_resource_detail;
    const auto view=observeLocalResourceProximity(world,actor.id,actualPosition,6,4);
    if(!actor.alive ||view.actor!=actor.id ||actor.id==0) return 0;
    std::size_t newlyRemembered=0;
    for(const auto& sensed:view.nearbyResources){
        const auto live=std::find_if(world.resourceNodes.begin(),world.resourceNodes.end(),
            [&](const ResourceNode& node){return node.id==sensed.nodeId;});
        if(live==world.resourceNodes.end() ||live->material!=sensed.material
           ||live->pos.x!=sensed.position.x ||live->pos.y!=sensed.position.y
           ||live->quantity<=0) continue;
        const std::string nodeTag="node:"+std::to_string(live->id);
        MemoryRecord observation;
        observation.who=observation.sourceCharacter=actor.id;
        observation.what=ResidentKnownResourceMemoryDescription;
        observation.where="grid:"+std::to_string(live->pos.x)+","+std::to_string(live->pos.y);
        observation.minute=world.minute;
        observation.source=MemorySource::Inferred;
        observation.witnessed=false;
        observation.importance=0.32;
        observation.confidence=0.60;
        observation.decayPerDay=0.025;
        observation.tags={ResidentKnownResourceMemoryTag,nodeTag,
            "material:"+std::to_string(static_cast<int>(live->material)),
            "quantity:"+std::to_string(live->quantity)};
        auto previous=std::find_if(actor.memory.entries.begin(),actor.memory.entries.end(),
            [&](const MemoryRecord& memory){
                return belongsToActor(memory,actor.id) &&memory.tags.size()>1
                    &&memory.tags[1]==nodeTag;
            });
        if(previous!=actor.memory.entries.end()){
            if(previous->minute<=world.minute) *previous=std::move(observation);
        }else{
            actor.memory.add(std::move(observation));
            ++newlyRemembered;
        }
        // Bound only this schema; real failures/successes and all other
        // private experiences remain untouched.
        for(;;){
            std::size_t count=0;
            auto oldest=actor.memory.entries.end();
            for(auto it=actor.memory.entries.begin();it!=actor.memory.entries.end();++it){
                if(!belongsToActor(*it,actor.id)) continue;
                ++count;
                if(oldest==actor.memory.entries.end() ||it->minute<oldest->minute)
                    oldest=it;
            }
            if(count<=ResidentKnownResourceMemoryLimit) break;
            actor.memory.entries.erase(oldest);
        }
    }
    return newlyRemembered;
}

} // namespace lifelens
