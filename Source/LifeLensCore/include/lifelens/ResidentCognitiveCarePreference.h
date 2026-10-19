#pragma once

#include "ResidentCognitivePreference.h"

namespace lifelens {

// Core ParentingContext still owns kinship, provisions, distance and self-care.
inline double residentCognitiveCarePreference(
    const World& world,
    const std::vector<AcceptedCognitiveDecision>& journal,
    const Character& caregiver,const Character& child)
{
    if(!child.alive ||!isDependentStage(child.lifeStage)
       ||!lifeStageProfile(caregiver.lifeStage).canParent) return 0.0;
    return residentCognitiveIntentPreference(
        world,journal,caregiver.id,CognitiveIntentKind::CareForDependent,
        child.id);
}

} // namespace lifelens
