#pragma once

#include <string>
#include <vector>

#include "Relationship.h"
#include "SocialUtility.h"
#include "World.h"

namespace lifelens {

enum class ObservedActivityKind {
    Idle,
    Physical,
    Social
};

struct RelationshipObservation {
    CharacterId targetId = 0;
    std::string targetName;

    double affection = 0.0;
    double trust = 0.0;
    double respect = 0.0;
    double comfort = 0.0;
    double familiarity = 0.0;
    double attraction = 0.0;
    double romanticInterest = 0.0;
    double sexualAttraction = 0.0;
    double commitment = 0.0;
    double conflict = 0.0;
    double jealousy = 0.0;
    double fear = 0.0;
    double grudge = 0.0;

    double socialBond = 0.0;
    double romancePotential = 0.0;
};

struct ResidentObservation {
    CharacterId id = 0;
    std::string name;
    Sex sex = Sex::Male;

    Needs needs;

    double emotionValence = 0.0;
    double emotionArousal = 0.0;
    double emotionIntensity = 0.0;

    ObservedActivityKind activityKind = ObservedActivityKind::Idle;
    std::string activityLabel = "Idle";
    Goal physicalGoal = Goal::Idle;
    SocialIntent socialIntent = SocialIntent::None;
    CharacterId activityTargetId = 0;
    std::string activityTargetName;

    std::vector<RelationshipObservation> relationships;
};

inline const Character* findObservedCharacter(const World& world, CharacterId id) {
    for (const auto& character : world.characters) {
        if (character.id == id) return &character;
    }
    return nullptr;
}

inline RelationshipObservation makeRelationshipObservation(
    const World& world,
    const Relationship& relationship) {

    RelationshipObservation dto;
    dto.targetId = relationship.to;
    if (const Character* target = findObservedCharacter(world, relationship.to)) {
        dto.targetName = target->name;
    }

    dto.affection = relationship.affection;
    dto.trust = relationship.trust;
    dto.respect = relationship.respect;
    dto.comfort = relationship.comfort;
    dto.familiarity = relationship.familiarity;
    dto.attraction = relationship.attraction;
    dto.romanticInterest = relationship.romanticInterest;
    dto.sexualAttraction = relationship.sexualAttraction;
    dto.commitment = relationship.commitment;
    dto.conflict = relationship.conflict;
    dto.jealousy = relationship.jealousy;
    dto.fear = relationship.fear;
    dto.grudge = relationship.grudge;
    dto.socialBond = relationship.socialBond();
    dto.romancePotential = relationship.romancePotential();
    return dto;
}

inline ResidentObservation buildResidentObservation(
    const World& world,
    const RelationshipBook& relationships,
    const Character& character,
    bool hasPhysicalAction,
    Goal physicalGoal,
    bool socialActive,
    SocialIntent socialIntent,
    CharacterId socialTarget) {

    ResidentObservation dto;
    dto.id = character.id;
    dto.name = character.name;
    dto.sex = character.sex;
    dto.needs = character.needs;
    dto.emotionValence = character.emotion.valence;
    dto.emotionArousal = character.emotion.arousal;
    dto.emotionIntensity = character.emotion.intensity();

    if (socialActive && socialIntent != SocialIntent::None) {
        dto.activityKind = ObservedActivityKind::Social;
        dto.activityLabel = socialIntentName(socialIntent);
        dto.socialIntent = socialIntent;
        dto.activityTargetId = socialTarget;
        if (const Character* target = findObservedCharacter(world, socialTarget)) {
            dto.activityTargetName = target->name;
        }
    } else if (hasPhysicalAction && physicalGoal != Goal::Idle) {
        dto.activityKind = ObservedActivityKind::Physical;
        dto.activityLabel = goalName(physicalGoal);
        dto.physicalGoal = physicalGoal;
    } else {
        dto.activityKind = ObservedActivityKind::Idle;
        dto.activityLabel = "Idle";
    }

    for (const auto& relationship : relationships.all()) {
        if (relationship.from == character.id) {
            dto.relationships.push_back(makeRelationshipObservation(world, relationship));
        }
    }

    return dto;
}

} // namespace lifelens
