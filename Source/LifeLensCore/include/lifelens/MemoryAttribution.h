#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include "Memory.h"

namespace lifelens {
struct MemoryAttributionCategory {
    std::uint64_t entries=0;
    std::uint64_t textBytes=0;
};
// Read-only breakdown of action memories for long-run #669 audits. This
// classifies tags, not actual execution results: provenance text alone never
// proves that an action was performed or that a model earned credit.
struct MemoryActionAttribution {
    std::uint64_t entries=0;
    std::uint64_t successful=0;
    std::uint64_t failed=0;
    std::uint64_t witnessedDirect=0;
    std::uint64_t indirectOrUnverified=0;
    std::uint64_t claimedContextTokens=0;
    std::uint64_t claimedLocalExchangeFacts=0;
    std::uint64_t utf8TextBytes=0;
};
struct MemoryAttribution {
    std::uint64_t entries=0;
    std::uint64_t textBytes=0;
    std::uint64_t exactDuplicates=0;
    std::uint64_t witnessedDirect=0;
    std::uint64_t futureEntries=0;
    std::array<std::uint64_t,3> sources{};
    std::array<std::uint64_t,5> ageBuckets{};
    std::map<std::string,MemoryAttributionCategory> primaryTags;
    std::map<std::string,MemoryActionAttribution> actionKinds;
};

inline std::uint64_t memoryNumberBits(double number)
{
    std::uint64_t bits=0;
    std::memcpy(&bits,&number,sizeof(bits));
    return bits;
}
inline bool sameMemoryRecord(const MemoryRecord& a,const MemoryRecord& b)
{
    return a.who==b.who &&a.sourceCharacter==b.sourceCharacter
        &&a.what==b.what &&a.where==b.where &&a.minute==b.minute
        &&a.source==b.source &&a.witnessed==b.witnessed &&a.tags==b.tags
        &&memoryNumberBits(a.emotionValence)==memoryNumberBits(b.emotionValence)
        &&memoryNumberBits(a.emotionIntensity)==memoryNumberBits(b.emotionIntensity)
        &&memoryNumberBits(a.importance)==memoryNumberBits(b.importance)
        &&memoryNumberBits(a.confidence)==memoryNumberBits(b.confidence)
        &&memoryNumberBits(a.decayPerDay)==memoryNumberBits(b.decayPerDay);
}

inline std::uint64_t memoryRecordFingerprint(const MemoryRecord& m)
{
    std::uint64_t hash=14695981039346656037ULL;
    const auto byte=[&](unsigned char value){hash^=value;hash*=1099511628211ULL;};
    const auto integer=[&](std::uint64_t value){
        for(int i=0;i<8;++i)byte(static_cast<unsigned char>(value>>(i*8)));
    };
    const auto string=[&](const std::string& value){
        integer(value.size());
        for(unsigned char ch:value)byte(ch);
    };
    integer(m.who);integer(m.sourceCharacter);
    integer(static_cast<std::uint64_t>(m.minute));
    integer(static_cast<std::uint64_t>(m.source));integer(m.witnessed);
    for(double value:{m.emotionValence,m.emotionIntensity,m.importance,
                      m.confidence,m.decayPerDay})integer(memoryNumberBits(value));
    string(m.what);string(m.where);integer(m.tags.size());
    for(const auto& tag:m.tags)string(tag);
    return hash;
}

// Read-only checkpoint instrumentation. Text bytes are UTF-8 payload bytes,
// not a fabricated exact sizeof/vector capacity or full snapshot-size claim.
// Hash collisions are resolved by complete record comparison.
inline MemoryAttribution observeMemoryAttribution(
    const MemoryState& memory,int currentMinute)
{
    MemoryAttribution out;
    std::map<std::uint64_t,std::vector<std::size_t>> seen;
    for(std::size_t index=0;index<memory.entries.size();++index){
        const auto& m=memory.entries[index];
        ++out.entries;
        std::uint64_t text=m.what.size()+m.where.size();
        for(const auto& tag:m.tags)text+=tag.size();
        out.textBytes+=text;
        const auto source=static_cast<std::size_t>(m.source);
        if(source<out.sources.size())++out.sources[source];
        if(m.source==MemorySource::DirectWitness &&m.witnessed)
            ++out.witnessedDirect;
        const auto age=static_cast<std::int64_t>(currentMinute)-m.minute;
        if(age<0)++out.futureEntries;
        else ++out.ageBuckets[age<1440?0:age<7*1440?1:
                             age<30*1440?2:age<365*1440?3:4];
        std::string category=m.tags.empty()?"(untagged)":m.tags.front();
        if(out.primaryTags.size()>=64 &&out.primaryTags.count(category)==0)
            category="(other tags)";
        auto& group=out.primaryTags[category];
        ++group.entries;group.textBytes+=text;

        // Split the huge "action_experience" group by actual recorded action
        // key and source, without changing or pruning authoritative memory.
        // Do not label missing provenance as verified direct experience.
        if(m.tags.size()==3 &&m.tags[0]=="action_experience"
           &&(m.tags[2]=="success" ||m.tags[2]=="failure")){
            std::string action=m.tags[1].empty()?"(missing key)":m.tags[1];
            if(out.actionKinds.size()>=64
               &&out.actionKinds.count(action)==0)
                action="(other action kinds)";
            auto& entry=out.actionKinds[action];
            ++entry.entries;
            entry.utf8TextBytes+=text;
            if(m.tags[2]=="success")++entry.successful;
            else ++entry.failed;
            if(m.source==MemorySource::DirectWitness
               &&m.witnessed &&m.sourceCharacter!=0)
                ++entry.witnessedDirect;
            else ++entry.indirectOrUnverified;
            if(m.where.rfind("core-context-token:",0)==0)
                ++entry.claimedContextTokens;
            if(m.where.rfind("core-local-exchange-fact:",0)==0)
                ++entry.claimedLocalExchangeFacts;
        }

        auto& bucket=seen[memoryRecordFingerprint(m)];
        bool duplicate=false;
        for(const auto prior:bucket)
            if(sameMemoryRecord(m,memory.entries[prior])){
                duplicate=true;break;
            }
        if(duplicate)++out.exactDuplicates;
        else bucket.push_back(index);
    }
    return out;
}
} // namespace lifelens
