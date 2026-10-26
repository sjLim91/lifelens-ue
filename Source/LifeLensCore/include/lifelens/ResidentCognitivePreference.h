#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
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
    constexpr const char* localFactPrefix="core-local-exchange-fact:";
    const std::string key(experienceKey);
    // Local barter is a distinct, fact-backed Core execution lane. It has no
    // pending context token and may ONLY provide successful trade evidence.
    const auto provenance=[&](const MemoryRecord& memory)->bool{
        const bool token=memory.where.rfind(tokenPrefix,0)==0;
        const bool localFact=key=="CoreContext:Trade"
            &&memory.tags[2]=="success"
            &&memory.where.rfind(localFactPrefix,0)==0;
        if(!token &&!localFact) return false;
        const char* prefix=token?tokenPrefix:localFactPrefix;
        const std::string id=memory.where.substr(
            std::char_traits<char>::length(prefix));
        if(id.empty() ||id.front()=='0' ||id.size()>20) return false;
        std::uint64_t parsed=0;
        for(const char ch:id){
            if(ch<'0' ||ch>'9') return false;
            const auto digit=static_cast<std::uint64_t>(ch-'0');
            if(parsed>(std::numeric_limits<std::uint64_t>::max()-digit)/10)
                return false;
            parsed=parsed*10+digit;
        }
        if(parsed==0) return false;
        if(localFact)
            return memory.what=="실제 지역 교역 대상 "
                +std::to_string(partner)+" 물품 교환 완료 (Core exchange fact)";
        const std::string outcomePrefix="실제 행동 "+key+" 대상 "
            +std::to_string(partner)+" 결과: ";
        if(key=="CoreContext:Teaching")
            return memory.tags[2]=="success"
                ?memory.what==outcomePrefix+"teaching-advanced"
                :(memory.what==outcomePrefix+"comprehension-failed"
                  ||memory.what==outcomePrefix+"clarity-or-trust-failed"
                  ||memory.what==outcomePrefix+"teaching-core-rejected");
        if(key=="CoreContext:Trade")
            return memory.tags[2]=="success"
                ?memory.what==outcomePrefix+"trade-exchanged"
                :memory.what==outcomePrefix+"trade-core-rejected";
        return false;
    };
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
           ||!provenance(memory)) continue;
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
    if(latestSucceeded) return 1.04;

    // Two DISTINCT recent, firsthand failures with the SAME partner are
    // stronger evidence than one rejected exchange. Stop reinforcing that
    // stale optional model target for this attempt; let ordinary Core rank
    // independently eligible partners instead. Nothing is banned and no
    // social action or exchange is executed by this preference.
    int priorMinute=-1;
    bool priorSucceeded=false;
    for(const auto& memory:resident.memory.entries){
        if(memory.minute<0 ||memory.minute>=latestMinute
           ||memory.minute>nowMinute
           ||nowMinute-memory.minute>ResidentAgencyGoalHorizonMinutes
           ||memory.source!=MemorySource::DirectWitness
           ||!memory.witnessed ||memory.sourceCharacter!=resident.id
           ||memory.who!=partner ||memory.tags.size()!=3
           ||memory.tags[0]!="action_experience"
           ||memory.tags[1]!=key
           ||(memory.tags[2]!="success" &&memory.tags[2]!="failure")
           ||!provenance(memory)) continue;
        const bool success=memory.tags[2]=="success";
        if(memory.minute>priorMinute
           ||(memory.minute==priorMinute &&success)){
            priorMinute=memory.minute;
            priorSucceeded=success;
        }
    }
    return priorMinute>=0 &&!priorSucceeded?0.0:0.92;
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
