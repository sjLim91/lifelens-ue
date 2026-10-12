#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "CognitiveAgent.h"

namespace lifelens {

// COG-3 foundation: an accepted external proposal must be durable BEFORE it
// is ever allowed to influence simulated behavior. This journal does not
// execute or authorize a World action by itself.
inline constexpr std::size_t MaxAcceptedCognitionJournalPerResident=32;
inline constexpr std::size_t MaxCognitionProvenanceFieldLength=128;
inline const std::string& cognitiveAcceptedJournalVersionTag()
{
    static const std::string tag="accepted-cognitive-proposal-v1";
    return tag;
}

struct AcceptedCognitiveEvent {
    CharacterId actor=0;
    int minute=0;
    CognitiveTriggerKind trigger=CognitiveTriggerKind::None;
    CognitiveIntentKind intent=CognitiveIntentKind::None;
    CharacterId target=0;
    double priority=0.0;
    std::string contextFingerprint;
    std::string backendIdentity;
    std::string rationaleDigest;
};

enum class CognitiveJournalWriteResult {
    Rejected,
    AlreadyRecorded,
    Recorded
};

inline std::uint64_t cognitivePriorityBits(double value)
{
    static_assert(sizeof(double)==sizeof(std::uint64_t),"requires IEEE binary64");
    std::uint64_t bits=0;
    std::memcpy(&bits,&value,sizeof(bits));
    return bits;
}

inline double cognitivePriorityFromBits(std::uint64_t bits)
{
    double value=0.0;
    std::memcpy(&value,&bits,sizeof(value));
    return value;
}

inline bool isAcceptedCognitionMemory(const MemoryRecord& memory)
{
    return memory.where=="Core accepted cognition"
        && memory.tags.size()==7
        && memory.tags[0]==cognitiveAcceptedJournalVersionTag();
}

inline bool parseCognitionJournalUnsigned(
    const std::string& text,std::uint64_t& result)
{
    if(text.empty()) return false;
    for(const char c:text){
        if(c<'0' || c>'9') return false;
    }
    try{
        std::size_t consumed=0;
        result=std::stoull(text,&consumed,10);
        return consumed==text.size();
    }catch(...){
        return false;
    }
}

inline bool decodeAcceptedCognitiveEvent(
    const MemoryRecord& memory,AcceptedCognitiveEvent& event)
{
    if(!isAcceptedCognitionMemory(memory)) return false;
    std::uint64_t trigger=0,intent=0,priorityBits=0;
    if(!parseCognitionJournalUnsigned(memory.tags[1],trigger)
       || !parseCognitionJournalUnsigned(memory.tags[2],intent)
       || !parseCognitionJournalUnsigned(memory.tags[5],priorityBits)) return false;
    if(trigger==0 || trigger>static_cast<std::uint64_t>(CognitiveTriggerKind::Reflection)
       || intent==0 || intent>static_cast<std::uint64_t>(CognitiveIntentKind::MigrateHousehold)
       || memory.sourceCharacter==0 || memory.minute<0
       || memory.tags[3].empty() || memory.tags[4].empty()
       || memory.tags[3].size()>MaxCognitionProvenanceFieldLength
       || memory.tags[4].size()>MaxCognitionProvenanceFieldLength
       || memory.tags[6].size()>MaxCognitionProvenanceFieldLength) return false;
    const double priority=cognitivePriorityFromBits(priorityBits);
    if(!std::isfinite(priority)||priority<0.0||priority>1.0) return false;
    event.actor=memory.sourceCharacter;
    event.minute=memory.minute;
    event.trigger=static_cast<CognitiveTriggerKind>(trigger);
    event.intent=static_cast<CognitiveIntentKind>(intent);
    event.target=memory.who;
    event.priority=priority;
    event.contextFingerprint=memory.tags[3];
    event.backendIdentity=memory.tags[4];
    event.rationaleDigest=memory.tags[6];
    return true;
}

inline std::vector<AcceptedCognitiveEvent> observeAcceptedCognitionJournal(
    const Character& actor)
{
    std::vector<AcceptedCognitiveEvent> result;
    for(const MemoryRecord& memory:actor.memory.entries){
        AcceptedCognitiveEvent entry;
        if(decodeAcceptedCognitiveEvent(memory,entry)
            && entry.actor==actor.id) result.push_back(std::move(entry));
    }
    return result;
}

inline CognitiveJournalWriteResult recordAcceptedCognitiveProposal(
    const World& world,Character& actor,
    const CognitiveRequest& request,const CognitiveProposal& proposal,
    const std::string& contextFingerprint,
    const std::string& backendIdentity,
    const std::string& rationaleDigest)
{
    if(actor.id==0 || actor.id!=request.actor
       || request.minute!=world.minute
       || request.minute<0
       || contextFingerprint.empty() || backendIdentity.empty()
       || contextFingerprint.size()>MaxCognitionProvenanceFieldLength
       || backendIdentity.size()>MaxCognitionProvenanceFieldLength
       || rationaleDigest.size()>MaxCognitionProvenanceFieldLength) {
        return CognitiveJournalWriteResult::Rejected;
    }
    const Character* truth=cognitiveCharacter(world,actor.id);
    if(truth==nullptr || truth!=&actor
       || !validateCognitiveProposal(world,request,proposal).accepted){
        return CognitiveJournalWriteResult::Rejected;
    }
    // Identical actor, timestamp, trigger and factual context is one Core
    // accepted decision. Restart/load must not ask the model to replace it.
    std::size_t journalCount=0;
    for(const MemoryRecord& memory:actor.memory.entries){
        if(!isAcceptedCognitionMemory(memory)) continue;
        ++journalCount;
        if(memory.minute==request.minute
           && memory.sourceCharacter==actor.id
           && memory.tags[1]==std::to_string(static_cast<int>(request.trigger))
           && memory.tags[3]==contextFingerprint) {
            return CognitiveJournalWriteResult::AlreadyRecorded;
        }
    }
    // Bound storage without discarding personal non-journal memories.
    while(journalCount>=MaxAcceptedCognitionJournalPerResident){
        const auto it=std::find_if(
            actor.memory.entries.begin(),actor.memory.entries.end(),
            [](const MemoryRecord& memory){
                return isAcceptedCognitionMemory(memory);
            });
        if(it==actor.memory.entries.end()) break;
        actor.memory.entries.erase(it);
        --journalCount;
    }

    MemoryRecord memory;
    memory.sourceCharacter=actor.id;
    memory.who=proposal.targetResident;
    memory.minute=request.minute;
    memory.what="Core accepted cognitive strategic proposal";
    memory.where="Core accepted cognition";
    memory.source=MemorySource::Inferred;
    memory.witnessed=false;
    memory.importance=0.7;
    memory.confidence=1.0;
    memory.decayPerDay=0.0;
    memory.tags={
        cognitiveAcceptedJournalVersionTag(),
        std::to_string(static_cast<int>(request.trigger)),
        std::to_string(static_cast<int>(proposal.intent)),
        contextFingerprint,
        backendIdentity,
        std::to_string(cognitivePriorityBits(proposal.priority)),
        rationaleDigest
    };
    actor.memory.add(std::move(memory));
    return CognitiveJournalWriteResult::Recorded;
}

} // namespace lifelens
