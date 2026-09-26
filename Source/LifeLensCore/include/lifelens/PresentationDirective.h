#pragma once

#include <cstdint>

#include "ContextAction.h"
#include "Planner.h"
#include "SmartObject.h"

namespace lifelens {

enum class PresentationActionKind {
    None,
    Physical,
    Social,
    Civilization,
    Parenting,
    KnowledgeTeaching
};

enum class PresentationActionPhase {
    Idle,
    Moving,
    Interacting
};

struct ResidentPresentationObservation {
    CharacterId residentId=0;
    bool active=false;
    PresentationActionKind kind=PresentationActionKind::None;
    PresentationActionPhase phase=PresentationActionPhase::Idle;

    Goal physicalGoal=Goal::Idle;
    SocialIntent socialIntent=SocialIntent::None;
    CivilizationIntent civilizationIntent=CivilizationIntent::None;
    ParentingAction parentingAction=ParentingAction::Comfort;

    CharacterId targetResidentId=0;
    bool hasTargetGrid=false;
    GridPos targetGrid{};

    bool hasObjectTarget=false;
    ObjectId objectId=0;
    ObjectKind objectKind=ObjectKind::Chair;

    bool emergencyFallback=false;
    bool designatedSanitationSite=false;
    SanitationSiteId sanitationSiteId=0;

    std::uint64_t contextActionToken=0;
    int durationTicks=0;
};

} // namespace lifelens
