#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "CognitiveDecisionJournal.h"
#include "ActionExperienceLearning.h"

namespace lifelens {

// A thought may guide the next ordinary Core opportunity for at most one day.
// It cannot authorize an opportunity, alter its success score or skip travel.
inline constexpr int ResidentAgencyGoalHorizonMinutes=1440;

// The same resident can learn different outcomes with different partners.
// Only real Core-owned firsthand execution memories qualify. A target-specific
// signal never grants an action, changes a Core result, or invents an outcome;
// it merely nudges an ALREADY eligible optional teaching/trade preference.
// This uses the existing bounded 4-per-action memory window and saved truth.
inline double residentCognitivePartnerExperienceFactor(
    const Character& resident,CharacterId partner,
    const char* experienceKey,int nowMinute)
{
    if(!resident.alive ||resident.id==0 ||partner==0
       ||partner==resident.id ||experienceKey==nullptr
       ||nowMinute<0) return 1.0;
    constexpr const char* tokenPrefix="core-context-token:";
    const std::string key(experienceKey);
    int latestMinute=-1;
    bool latestSucceeded=false;
    for(const auto& memory:resident.memory.entries){
        if(memory.source!=MemorySource::DirectWitness
           ||!memory.witnessed ||memory.sourceCharacter!=resident.id
           ||memory.who!=partner ||memory.minute<0
           ||memory.minute>nowMinute
           ||nowMinute-memory.minute>ResidentAgencyGoalHorizonMinutes
           ||memory.tags.size()!=3
           ||memory.tags[0]!="action_experience"
           ||memory.tags[1]!=key
           ||(memory.tags[2]!="success" &&memory.tags[2]!="failure")
           ||memory.where.rfind(tokenPrefix,0)!=0) continue;
        const std::string token=memory.where.substr(
            std::char_traits<char>::length(tokenPrefix));
        if(token.empty() ||token=="0"
           ||!std::all_of(token.begin(),token.end(),
               [](unsigned char ch){return ch>='0' &&ch<='9';}))
            continue;
        const bool success=memory.tags[2]=="success";
        // Insertion order and snapshot ordering do not change the answer;
        // a verified success is decisive if records share a minute.
        if(memory.minute>latestMinute
           ||(memory.minute==latestMinute &&success)){
            latestMinute=memory.minute;
            latestSucceeded=success;
        }
    }
    if(latestMinute<0) return 1.0;
    return latestSucceeded?1.04:0.92;
}

inline double residentCognitiveIntentPreference(
    const World& world,
    const std::vector<AcceptedCognitiveDecision>& journal,
    CharacterId actor,CognitiveIntentKind intent,CharacterId target=0)
{
    const Character* resident=cognitiveCharacter(world,actor);
    if(resident==nullptr ||!resident->alive
       ||requiresDirectCare(resident->lifeStage)) return 0.0;
    if(cognitiveIntentTargetsResident(intent)){
        const Character* other=cognitiveCharacter(world,target);
        if(other==nullptr ||!other->alive ||target==actor) return 0.0;
    }
    for(auto it=journal.rbegin();it!=journal.rend();++it){
        if(it->actor!=actor) continue;
        // A later thought, even one with a different goal, replaces the old one.
        if(it->intent!=intent ||it->target!=target
           ||!validAcceptedCognitiveDecision(*it,world)
           ||world.minute-it->minute>ResidentAgencyGoalHorizonMinutes
           ||it->priority<0.55) return 0.0;
        double preference=0.12*it->priority;
        // ONLY optional social/economic choices adapt to firsthand Core
        // success/failure. Critical childcare and survival remain protected.
        const char* experienceKey=nullptr;
        if(intent==CognitiveIntentKind::TeachKnowledge)
            experienceKey="CoreContext:Teaching";
        else if(intent==CognitiveIntentKind::TradeWithResident)
            experienceKey="CoreContext:Trade";
        if(experienceKey!=nullptr)
            preference=std::min(0.12,preference
                *learnedActionDisposition(*resident,experienceKey)
                *residentCognitivePartnerExperienceFactor(
                    *resident,target,experienceKey,world.minute));
        return preference;
    }
    return 0.0;
}

inline double residentCognitiveTradePreference(
    const World& world,
    const std::vector<AcceptedCognitiveDecision>& journal,
    CharacterId first,CharacterId second)
{
    // Mutual preference is not two votes or a higher economic success score.
    return std::max(
        residentCognitiveIntentPreference(world,journal,first,
            CognitiveIntentKind::TradeWithResident,second),
        residentCognitiveIntentPreference(world,journal,second,
            CognitiveIntentKind::TradeWithResident,first));
}

} // namespace lifelens
