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
    Character& actor,const char* action,int minute,bool success)
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

    // One durable recollection per outcome/intent pair. This is an actual
    // attempt, not an invented memory of observing the whole world.
    const std::string resultTag=success ? "success" : "failure";
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
    actor.memory.add(std::move(memory));
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
