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
            preference=std::min(0.12,preference*
                learnedActionDisposition(*resident,experienceKey));
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
