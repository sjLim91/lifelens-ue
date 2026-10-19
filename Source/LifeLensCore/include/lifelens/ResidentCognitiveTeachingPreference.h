#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "CognitiveDecisionJournal.h"
#include "ResidentAIExistingAffordance.h"

namespace lifelens {

// Cognition can rank WHO to teach, not authorize a new teaching action.
// This preference is considered only after all current Core eligibility,
// distance, network, actual knowledge gap and documented fact checks pass.
// Zero means the ordinary Core candidate's ordering is unchanged.
inline double residentCognitiveTeachingPreference(
    const World& world,
    const std::vector<AcceptedCognitiveDecision>& journal,
    CharacterId teacher,CharacterId learner)
{
    if(teacher==0 ||learner==0 ||teacher==learner) return 0.0;
    for(auto it=journal.rbegin();it!=journal.rend();++it){
        if(it->actor!=teacher) continue;
        // The latest accepted resident thought owns this one decision slot.
        // An older TeachKnowledge cannot override their later chosen goal.
        if(it->minute>world.minute
           ||world.minute-it->minute>ResidentAgencyGoalHorizonMinutes
           ||it->intent!=CognitiveIntentKind::TeachKnowledge
           ||it->target!=learner
           ||!std::isfinite(it->priority)
           ||it->priority<0.55
           ||!validAcceptedCognitiveDecision(*it,world))
            return 0.0;
        return std::min(0.16,0.12*it->priority);
    }
    return 0.0;
}

} // namespace lifelens
