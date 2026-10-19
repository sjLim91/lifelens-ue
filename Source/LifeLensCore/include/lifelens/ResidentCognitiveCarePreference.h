#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "CognitiveDecisionJournal.h"
#include "ResidentAIExistingAffordance.h"

namespace lifelens {

// The accepted *single resident* CareForDependent choice is a bounded
// preference for a real Core caregiver. It cannot select an unrelated child,
// create resources, authorize remote care, override critical self-care or
// replace the established ParentingContext decision and completion token.
inline double residentCognitiveCarePreference(
    const World& world,
    const std::vector<AcceptedCognitiveDecision>& journal,
    const Character& caregiver,const Character& child)
{
    if(!caregiver.alive ||!child.alive
       ||!isDependentStage(child.lifeStage)
       ||!lifeStageProfile(caregiver.lifeStage).canParent)
        return 0.0;
    for(auto it=journal.rbegin();it!=journal.rend();++it){
        if(it->actor!=caregiver.id) continue;
        if(it->minute>world.minute
           ||world.minute-it->minute>ResidentAgencyGoalHorizonMinutes
           ||it->intent!=CognitiveIntentKind::CareForDependent
           ||it->target!=child.id
           ||!std::isfinite(it->priority)
           ||it->priority<0.55
           ||!validAcceptedCognitiveDecision(*it,world))
            return 0.0;
        return std::min(0.12,0.12*it->priority);
    }
    return 0.0;
}
} // namespace lifelens
