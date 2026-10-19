#pragma once

#include "ResidentCognitivePreference.h"

namespace lifelens {

// Called only for an existing Core-approved, spatial teaching opportunity.
inline double residentCognitiveTeachingPreference(
    const World& world,
    const std::vector<AcceptedCognitiveDecision>& journal,
    CharacterId teacher,CharacterId learner)
{
    return residentCognitiveIntentPreference(
        world,journal,teacher,CognitiveIntentKind::TeachKnowledge,learner);
}

} // namespace lifelens
