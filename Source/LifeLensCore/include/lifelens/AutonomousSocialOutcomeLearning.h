#pragma once

#include <algorithm>
#include <cmath>
#include <string>

#include "Character.h"
#include "SocialUtility.h"

namespace lifelens {

// B4c-1: A resident can directly witness THEIR OWN mood after a completed
// social action. Another resident's private trust, intent or thoughts are
// not observable evidence. No result is written unless Core actually
// executed the action and the caller measured an emotional change.
inline std::string socialMoodExperienceProposition(SocialIntent intent)
{
    return std::string("experienced-social-self-mood:") + socialIntentName(intent);
}

inline double learnedSocialMoodDisposition(
    const Character& actor, CharacterId target, SocialIntent intent)
{
    if(target==0 || target==actor.id
       || (intent!=SocialIntent::Approach && intent!=SocialIntent::Repair))
        return 0.0;
    const BeliefRecord* record=actor.beliefs.find(
        target,socialMoodExperienceProposition(intent));
    if(record==nullptr) return 0.0;
    return std::clamp(record->stance*record->confidence,-1.0,1.0);
}

// Only the Core completion site may call this after a confirmed execution.
// A zero delta is not evidence of failure: valence can be saturated, and
// an interaction can be meaningful without an immediate emotional payoff.
// Memories are compacted by partner/intent/outcome to bound growth and are
// stored in the existing snapshot-backed Character memory and belief fields.
inline bool rememberExperiencedSocialMoodOutcome(
    Character& actor,CharacterId target,SocialIntent intent,
    int minute,double beforeValence,double afterValence)
{
    if(!actor.alive || actor.id==0 || target==0 || target==actor.id
       ||minute<0
       ||(intent!=SocialIntent::Approach && intent!=SocialIntent::Repair)
       ||!std::isfinite(beforeValence) ||!std::isfinite(afterValence)
       ||beforeValence< -1.0 ||beforeValence>1.0
       ||afterValence< -1.0 ||afterValence>1.0)
        return false;

    const double change=afterValence-beforeValence;
    if(std::abs(change)<=1e-8) return false;
    const bool improved=change>0.0;
    const std::string proposition=socialMoodExperienceProposition(intent);
    BeliefRecord* existing=actor.beliefs.find(target,proposition);
    if(existing!=nullptr &&existing->lastUpdatedMinute==minute
       &&existing->supportCount+existing->contradictionCount>0)
        return false; // one actual outcome per partner/intent/minute

    BeliefRecord& learned=actor.beliefs.getOrCreate(target,proposition);
    if(learned.supportWeight+learned.contradictionWeight>=12.0){
        learned.supportWeight*=0.8;
        learned.contradictionWeight*=0.8;
    }
    // This is an estimate of personally felt usefulness, not the
    // probability that the relationship objective was achieved.
    learned.applyEvidence(improved,
        std::clamp(std::abs(change)*12.0,0.20,1.0),minute);

    const std::string outcome=improved ? "improved" : "worsened";
    const std::string action=socialIntentName(intent);
    for(MemoryRecord& memory:actor.memory.entries){
        if(memory.who!=target ||memory.tags.size()!=4
           ||memory.tags[0]!="social_self_outcome"
           ||memory.tags[1]!=action
           ||memory.tags[2]!=outcome
           ||memory.tags[3]!="direct") continue;
        memory.minute=minute;
        memory.confidence=1.0;
        memory.source=MemorySource::DirectWitness;
        memory.witnessed=true;
        memory.importance=improved ? 0.32 : 0.48;
        memory.emotionValence=afterValence;
        memory.emotionIntensity=actor.emotion.intensity();
        return true;
    }

    MemoryRecord memory;
    memory.who=target;
    memory.sourceCharacter=actor.id;
    memory.what=improved
        ? "이 사람과의 행동 후 내 기분이 실제로 나아졌다"
        : "이 사람과의 행동 후 내 기분이 실제로 나빠졌다";
    memory.where="personal social experience";
    memory.minute=minute;
    memory.emotionValence=afterValence;
    memory.emotionIntensity=actor.emotion.intensity();
    memory.importance=improved ? 0.32 : 0.48;
    memory.confidence=1.0;
    memory.source=MemorySource::DirectWitness;
    memory.witnessed=true;
    memory.tags={"social_self_outcome",action,outcome,"direct"};
    actor.memory.add(std::move(memory));
    return true;
}

} // namespace lifelens
