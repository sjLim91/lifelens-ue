#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <utility>

#include "Character.h"

namespace lifelens {

// Read-only, source-specific feedback for actual material Gather candidates.
// No World lookup, new persistent field, virtual action or permission grant.
inline constexpr int ResidentResourceOutcomeHorizonMinutes=1440;
inline constexpr double ResidentResourceFailurePreferenceLimit=0.08;
inline constexpr double ResidentResourceSuccessPreferenceLimit=0.03;

enum class ResidentResourceOutcomeReason {
    None,
    Success,
    UnspecifiedFailure,
    ContainerMissing,
    AccessUnavailable,
    SourceUnavailable
};

struct ResidentResourceOutcomePreference {
    bool hasEvidence=false;
    bool succeeded=false;
    int minute=-1;
    ResidentResourceOutcomeReason reason=ResidentResourceOutcomeReason::None;
    double utilityAdjustment=0.0;
};

// Ephemeral, per-planning-pass index. Its size depends only on qualifying
// personal source/action memories, never on all materialized World nodes.
// Build ONCE before comparing candidates; lookup never scans memory.
struct ResidentResourceOutcomeIndex {
    CharacterId actor=0;
    int currentMinute=-1;
    std::map<std::pair<std::string,std::uint64_t>,
        ResidentResourceOutcomePreference> entries;

    ResidentResourceOutcomePreference find(
        std::uint64_t sourceNode,const std::string& gatherActionKey) const
    {
        const auto found=entries.find({gatherActionKey,sourceNode});
        return found==entries.end()
            ?ResidentResourceOutcomePreference{}:found->second;
    }
};

namespace resident_resource_outcome_detail {

inline bool parseSourceNode(const std::string& where,std::uint64_t& out)
{
    constexpr std::size_t PrefixSize=14; // "resource-node:"
    if(where.rfind("resource-node:",0)!=0 ||where.size()<=PrefixSize
       ||where[PrefixSize]=='0') return false;
    std::uint64_t node=0;
    for(std::size_t i=PrefixSize;i<where.size();++i){
        if(where[i]<'0' ||where[i]>'9') return false;
        const auto digit=static_cast<std::uint64_t>(where[i]-'0');
        if(node>(std::numeric_limits<std::uint64_t>::max()-digit)/10)
            return false;
        node=node*10+digit;
    }
    if(node==0) return false;
    out=node;
    return true;
}

inline bool newerOutcome(
    const ResidentResourceOutcomePreference& incoming,
    const ResidentResourceOutcomePreference& selected)
{
    // Latest actual evidence wins. A same-minute success overrides a failure
    // independently of vector/restore order. Duplicates never accumulate.
    return !selected.hasEvidence ||incoming.minute>selected.minute
        ||(incoming.minute==selected.minute
           &&incoming.succeeded &&!selected.succeeded)
        ||(incoming.minute==selected.minute
           &&incoming.succeeded==selected.succeeded
           &&(incoming.succeeded
                ?incoming.utilityAdjustment>selected.utilityAdjustment
                :incoming.utilityAdjustment<selected.utilityAdjustment))
        // If different firsthand failures have the SAME minute AND the same
        // decayed utility, actor-memory iteration or snapshot record order
        // must not arbitrarily choose a different hypothesis next turn.
        ||(incoming.minute==selected.minute
           &&incoming.succeeded==selected.succeeded
           &&incoming.utilityAdjustment==selected.utilityAdjustment
           &&static_cast<int>(incoming.reason)>static_cast<int>(selected.reason));
}

} // namespace resident_resource_outcome_detail

// A v2 real navigation episode links the initial failed Core pending token
// to the subsequently issued recovery token. A legacy three-tag episode is
// still valid learning, but has no auditable original token.
inline std::uint64_t residentGatherRouteFailurePendingToken(
    const MemoryRecord& memory)
{
    if(memory.tags.size()!=4) return 0;
    const auto& tag=memory.tags[3];
    constexpr std::size_t prefix=14; // "context-token:"
    if(tag.rfind("context-token:",0)!=0 ||tag.size()<=prefix
       ||tag[prefix]=='0') return 0;
    std::uint64_t token=0;
    for(std::size_t i=prefix;i<tag.size();++i){
        if(tag[i]<'0'||tag[i]>'9') return 0;
        const auto digit=static_cast<std::uint64_t>(tag[i]-'0');
        if(token>(std::numeric_limits<std::uint64_t>::max()-digit)/10)
            return 0;
        token=token*10+digit;
    }
    return token;
}

inline ResidentResourceOutcomeIndex buildResidentResourceOutcomeIndex(
    const Character& actor,int currentMinute)
{
    ResidentResourceOutcomeIndex out;
    if(!actor.alive ||actor.id==0 ||currentMinute<0) return out;
    out.actor=actor.id;
    out.currentMinute=currentMinute;
    // Exactly one personal-memory pass. Do not call the single-source
    // compatibility helper inside a resource-node comparison loop.
    for(const auto& memory:actor.memory.entries){
        // A Core navigation failure happened BEFORE extraction. It is
        // firsthand spatial evidence, but never an executed Gather failure.
        const bool gathered=memory.tags.size()==3
            &&memory.tags[0]=="action_experience";
        const bool routeFailed=(memory.tags.size()==3
                ||(memory.tags.size()==4
                   &&residentGatherRouteFailurePendingToken(memory)!=0))
            &&memory.tags[0]=="navigation_experience"
            &&memory.tags[2]=="route-failed"
            &&memory.what=="Core 이동 경로 실패로 자원 채집을 시작하지 못했다";
        if(memory.source!=MemorySource::DirectWitness ||!memory.witnessed
           ||memory.sourceCharacter!=actor.id ||memory.who!=actor.id
           ||memory.minute<0 ||memory.minute>currentMinute
           ||static_cast<std::int64_t>(currentMinute)-memory.minute
                >ResidentResourceOutcomeHorizonMinutes
           ||(!gathered &&!routeFailed)
           ||memory.tags[1].rfind("Civilization:Gather:",0)!=0
           ||memory.tags[1].size()<=20
           ||!std::isfinite(memory.confidence)
           ||!std::isfinite(memory.importance)
           ||!std::isfinite(memory.emotionIntensity)
           ||!std::isfinite(memory.decayPerDay)) continue;
        std::uint64_t node=0;
        if(!resident_resource_outcome_detail::parseSourceNode(memory.where,node))
            continue;
        const std::string& key=memory.tags[1];
        const std::string success="실제 문명 행동 "+key+" 수행에 성공했다";
        const std::string failure="실제 문명 행동 "+key+" 수행을 시도했지만 실패했다";
        ResidentResourceOutcomePreference incoming;
        double magnitude=0.0;
        if(routeFailed){
            incoming.reason=ResidentResourceOutcomeReason::AccessUnavailable;
            magnitude=-0.06;
        }else if(memory.tags[2]=="success" &&memory.what==success){
            incoming.succeeded=true;
            incoming.reason=ResidentResourceOutcomeReason::Success;
            magnitude=ResidentResourceSuccessPreferenceLimit;
        }else if(memory.tags[2]=="failure"){
            // Exact Core descriptions are written AFTER real attempts.
            // Hearsay/model rationale cannot create direct outcome evidence.
            const std::string causePrefix=failure+" (직접 확인한 원인: ";
            if(memory.what==failure){
                incoming.reason=ResidentResourceOutcomeReason::UnspecifiedFailure;
                magnitude=-0.03;
            }else if(memory.what==causePrefix+"source-exhausted)"
                     ||memory.what==causePrefix+"source-removed)"){
                incoming.reason=ResidentResourceOutcomeReason::SourceUnavailable;
                magnitude=-ResidentResourceFailurePreferenceLimit;
            }else if(memory.what==causePrefix+"access-unavailable)"){
                incoming.reason=ResidentResourceOutcomeReason::AccessUnavailable;
                magnitude=-0.06;
            }else if(memory.what==causePrefix+"container-missing)"){
                // A missing vessel is mainly an inventory prerequisite,
                // not strong evidence that this particular place is unsafe.
                incoming.reason=ResidentResourceOutcomeReason::ContainerMissing;
                magnitude=-0.01;
            }
        }
        if(incoming.reason==ResidentResourceOutcomeReason::None) continue;
        incoming.hasEvidence=true;
        incoming.minute=memory.minute;
        const double age=static_cast<double>(
            static_cast<std::int64_t>(currentMinute)-memory.minute);
        const double recency=std::max(0.0,
            1.0-age/ResidentResourceOutcomeHorizonMinutes);
        incoming.utilityAdjustment=std::clamp(
            magnitude*memory.effectiveConfidence(currentMinute)*recency,
            -ResidentResourceFailurePreferenceLimit,
            ResidentResourceSuccessPreferenceLimit);
        auto& selected=out.entries[{key,node}];
        if(resident_resource_outcome_detail::newerOutcome(incoming,selected))
            selected=incoming;
    }
    return out;
}

// Called ONLY after the production Core navigation solver reports an actual
// failed route for a pending Gather. No resource was harvested and no action
// outcome belief is awarded. Retain one latest failure per resident/source;
// the short-lived location hypothesis is snapshot-backed and can be refuted
// by a later real successful Gather at that very source.
inline bool rememberResidentGatherRouteFailure(
    Character& actor,const std::string& actionKey,std::uint64_t sourceNode,
    std::uint64_t contextToken,int minute)
{
    if(!actor.alive ||actor.id==0 ||minute<0 ||sourceNode==0
       ||contextToken==0 ||actionKey.rfind("Civilization:Gather:",0)!=0
       ||actionKey.size()<=20) return false;
    const std::string source="resource-node:"+std::to_string(sourceNode);
    const std::string outcome="Core 이동 경로 실패로 자원 채집을 시작하지 못했다";
    for(auto& memory:actor.memory.entries){
        if(memory.source!=MemorySource::DirectWitness ||!memory.witnessed
           ||memory.sourceCharacter!=actor.id ||memory.who!=actor.id
           ||memory.where!=source
           ||(memory.tags.size()!=3 &&memory.tags.size()!=4)
           ||memory.tags[0]!="navigation_experience"
           ||memory.tags[1]!=actionKey ||memory.tags[2]!="route-failed"
           ||memory.what!=outcome) continue;
        if(memory.minute>=minute) return false;
        memory.minute=minute;
        memory.confidence=1.0;
        memory.tags={
            "navigation_experience",actionKey,"route-failed",
            "context-token:"+std::to_string(contextToken)
        };
        return true;
    }
    MemoryRecord memory;
    memory.who=actor.id;
    memory.sourceCharacter=actor.id;
    memory.what=outcome;
    memory.where=source;
    memory.minute=minute;
    memory.source=MemorySource::DirectWitness;
    memory.witnessed=true;
    memory.confidence=1.0;
    memory.importance=0.42;
    memory.emotionValence=-0.25;
    memory.emotionIntensity=0.35;
    memory.tags={
        "navigation_experience",actionKey,"route-failed",
        "context-token:"+std::to_string(contextToken)
    };
    actor.memory.add(std::move(memory));
    return true;
}

// Compatibility for isolated single-source callers. Candidate loops should
// buildResidentResourceOutcomeIndex once and call index.find for each source.
inline ResidentResourceOutcomePreference residentResourceOutcomePreference(
    const Character& actor,std::uint64_t sourceNode,
    const std::string& gatherActionKey,int currentMinute)
{
    if(sourceNode==0) return {};
    return buildResidentResourceOutcomeIndex(actor,currentMinute)
        .find(sourceNode,gatherActionKey);
}

} // namespace lifelens
