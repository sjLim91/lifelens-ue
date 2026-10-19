#pragma once
#include <algorithm>
#include <string>
#include "Character.h"

namespace lifelens {

// The same bounded feedback channel can later cover civilization and social
// actions. Only actually attempted physical goals are wired in this first step.
// Character beliefs and memories are already part of the authoritative save.
inline std::string actionExperienceProposition(const char* action)
{
    return std::string("experienced-action-outcome:") + action;
}

inline void rememberActionOutcome(
    Character& actor,const char* action,int minute,bool success,
    bool retainRecentPersonalAttempts=false)
{
    if(action==nullptr || *action=='\0' || minute<0) return;
    const std::string proposition=actionExperienceProposition(action);
    BeliefRecord* existing=actor.beliefs.find(0,proposition);
    if(existing!=nullptr && existing->lastUpdatedMinute==minute
       && existing->supportCount+existing->contradictionCount>0){
        return; // one completed attempt per action per authoritative minute
    }

    BeliefRecord& experience=actor.beliefs.getOrCreate(0,proposition);
    // A bounded moving evidence window, not permanent reinforcement of the
    // first hundred successes. No random rolls or hidden skill unlocks.
    const double weight=experience.supportWeight+experience.contradictionWeight;
    if(weight>=12.0){
        experience.supportWeight*=0.80;
        experience.contradictionWeight*=0.80;
    }
    experience.applyEvidence(success,1.0,minute);

    // Generic reflex feedback keeps its compact latest success/failure.
    // Physical civilization attempts instead need a SHORT chronological
    // history, or repeated failure reflection can never see two failures:
    // overwriting the first record on every retry erases the evidence.
    const std::string resultTag=success ? "success" : "failure";
    if(!retainRecentPersonalAttempts){
        for(MemoryRecord& memory:actor.memory.entries){
            if(memory.tags.size()!=3
               || memory.tags[0]!="action_experience"
               || memory.tags[1]!=action
               || memory.tags[2]!=resultTag) continue;
            memory.minute=minute;
            memory.confidence=1.0;
            memory.witnessed=true;
            memory.source=MemorySource::DirectWitness;
            memory.importance=success ? 0.28 : 0.42;
            memory.emotionValence=success ? 0.20 : -0.28;
            memory.emotionIntensity=success ? 0.18 : 0.38;
            return;
        }
    }

    MemoryRecord memory;
    memory.what=success ? "시도한 행동에 성공했다" : "시도한 행동에 실패했다";
    memory.where="personal experience";
    memory.minute=minute;
    memory.source=MemorySource::DirectWitness;
    memory.witnessed=true;
    memory.importance=success ? 0.28 : 0.42;
    memory.emotionValence=success ? 0.20 : -0.28;
    memory.emotionIntensity=success ? 0.18 : 0.38;
    memory.tags={"action_experience",action,resultTag};
    if(retainRecentPersonalAttempts){
        memory.who=actor.id;
        memory.sourceCharacter=actor.id;
    }
    actor.memory.add(std::move(memory));

    if(retainRecentPersonalAttempts){
        // Bound growth per Core action key (both successes AND failures).
        // Keep actual most-recent minutes, not insertion order: snapshots
        // and observer views may present memories in different orders.
        constexpr std::size_t MaxPersonalAttemptsPerAction=4;
        const auto belongsToActor=[&](const MemoryRecord& entry){
            return entry.source==MemorySource::DirectWitness
                &&entry.witnessed &&entry.sourceCharacter==actor.id
                &&entry.tags.size()==3
                &&entry.tags[0]=="action_experience"
                &&entry.tags[1]==action
                &&(entry.tags[2]=="success" ||entry.tags[2]=="failure");
        };
        std::size_t count=0;
        for(const auto& entry:actor.memory.entries)
            if(belongsToActor(entry)) ++count;
        while(count>MaxPersonalAttemptsPerAction){
            std::size_t oldest=actor.memory.entries.size();
            for(std::size_t i=0;i<actor.memory.entries.size();++i){
                if(belongsToActor(actor.memory.entries[i])
                   &&(oldest==actor.memory.entries.size()
                       ||actor.memory.entries[i].minute
                           <actor.memory.entries[oldest].minute))
                    oldest=i;
            }
            if(oldest==actor.memory.entries.size()) break;
            actor.memory.entries.erase(actor.memory.entries.begin()+oldest);
            --count;
        }
    }
}

inline double learnedActionDisposition(
    const Character& actor,const char* action)
{
    if(action==nullptr || *action=='\0') return 1.0;
    const BeliefRecord* belief=actor.beliefs.find(
        0,actionExperienceProposition(action));
    if(belief==nullptr) return 1.0;
    const double good=std::max(0.0,belief->supportWeight);
    const double bad=std::max(0.0,belief->contradictionWeight);
    const double evidence=good+bad;
    if(evidence<=0.0) return 1.0;
    // Both positive and negative attempts matter, but an action never becomes
    // inaccessible. Novel/untried alternatives retain an honest neutral prior.
    const double preference=(good-bad)/(evidence+3.0);
    return std::clamp(1.0+0.10*preference,0.90,1.10);
}

} // namespace lifelens
