#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
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

// Context identity belongs to Core, never to the external language model.
// Use explicit typed fields (not padding bytes/locale strings) and stable
// FNV-1a ordering so replay on another platform receives the same identity.
inline std::string cognitiveRequestFingerprint(const CognitiveRequest& request)
{
    std::uint64_t hash=14695981039346656037ULL;
    const auto u64=[&hash](std::uint64_t value){
        for(int i=0;i<8;++i){
            hash^=static_cast<unsigned char>((value>>(i*8))&0xffU);
            hash*=1099511628211ULL;
        }
    };
    const auto number=[&](double value){
        u64(cognitivePriorityBits(value));
    };
    const auto string=[&](const std::string& value){
        u64(value.size());
        for(unsigned char c:value){
            hash^=c;hash*=1099511628211ULL;
        }
    };
    u64(request.actor);
    u64(static_cast<std::uint64_t>(request.minute));
    u64(static_cast<std::uint64_t>(request.trigger));
    for(double value:{
        request.needs.hunger,request.needs.thirst,request.needs.sleep,
        request.needs.bladder,request.needs.hygiene,
        request.personality.introversion,request.personality.conscientiousness,
        request.personality.openness,request.personality.agreeableness,
        request.personality.emotionalStability,request.personality.empathy,
        request.personality.impulsiveness,request.personality.riskTolerance,
        request.personality.ambition,request.personality.patience,
        request.personality.sociability,request.personality.curiosity,
        request.personality.orderliness,request.personality.adaptability,
        request.emotion.joy,request.emotion.sadness,request.emotion.anger,
        request.emotion.fear,request.emotion.embarrassment,
        request.emotion.pride,request.emotion.jealousy,request.emotion.affection,
        request.emotion.anxiety,request.emotion.relief,request.emotion.grief,
        request.emotion.valence,request.emotion.arousal
    }) number(value);
    u64(request.allowedIntents.size());
    for(const auto intent:request.allowedIntents){
        u64(static_cast<std::uint64_t>(intent));
    }
    u64(request.memories.size());
    for(const auto& memory:request.memories){
        u64(memory.who);u64(static_cast<std::uint64_t>(memory.minute));
        number(memory.recallScore);number(memory.confidence);
        number(memory.importance);number(memory.emotionValence);
        number(memory.emotionIntensity);string(memory.what);
        string(memory.where);u64(memory.tags.size());
        for(const auto& tag:memory.tags) string(tag);
    }
    u64(request.beliefs.size());
    for(const auto& belief:request.beliefs){
        u64(belief.subject);string(belief.proposition);
        number(belief.stance);number(belief.confidence);
        u64(static_cast<std::uint64_t>(belief.lastUpdatedMinute));
    }
    u64(request.relationships.size());
    for(const auto& relation:request.relationships){
        u64(relation.target);
        number(relation.socialBond);number(relation.affection);
        number(relation.trust);number(relation.respect);
        number(relation.conflict);number(relation.fear);
        number(relation.grudge);
    }
    std::ostringstream out;
    out<<"ctx-v1-"<<std::hex<<std::setw(16)<<std::setfill('0')<<hash;
    return out.str();
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
       || contextFingerprint!=cognitiveRequestFingerprint(request)
       || backendIdentity.empty()
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
