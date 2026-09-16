#pragma once

#include <string>

#include "SocialCognition.h"
#include "SocialUtility.h"

namespace lifelens {

struct DecisionExecutionResult {
    UnifiedUtilityDecision decision;
    bool socialExecuted = false;
    bool generatedSocialEvent = false;
    CharacterId actor = 0;
    CharacterId target = 0;
    SocialEventType eventType = SocialEventType::PositiveInteraction;
    SocialEvent socialEvent{};
};

inline Character* findMutableCharacter(World& world, CharacterId id) {
    for (auto& character : world.characters) {
        if (character.id == id) return &character;
    }
    return nullptr;
}

inline void applyAvoidRelief(Character& actor, double utility) {
    const double scale = socialClamp01(utility);
    EmotionDelta d;
    d.fear = -0.07;
    d.anxiety = -0.08;
    d.anger = -0.02;
    d.relief = 0.08;
    actor.emotion.apply(d, 0.45 + 0.55 * scale);
}

inline bool socialEventForIntent(
    SocialIntent intent,
    SocialEventType& eventType,
    double& intensity) {

    switch (intent) {
        case SocialIntent::Approach:
            eventType = SocialEventType::PositiveInteraction;
            intensity = 0.55;
            return true;
        case SocialIntent::Repair:
            eventType = SocialEventType::Apology;
            intensity = 0.82;
            return true;
        case SocialIntent::Comfort:
            eventType = SocialEventType::Comfort;
            intensity = 0.88;
            return true;
        case SocialIntent::Avoid:
        case SocialIntent::None:
            return false;
    }
    return false;
}

inline DecisionExecutionResult executeSocialDecision(
    World& world,
    RelationshipBook& relationships,
    CharacterId actorId,
    const SocialUtilityDecision& social,
    const std::string& where = "") {

    DecisionExecutionResult result;
    result.actor = actorId;
    result.target = social.target;
    result.decision.kind = UnifiedDecisionKind::Social;
    result.decision.social = social;
    result.decision.utility = social.utility;

    Character* actor = findMutableCharacter(world, actorId);
    Character* target = findMutableCharacter(world, social.target);
    if (!actor || !target || !actor->alive || !target->alive ||
        actorId == social.target || social.intent == SocialIntent::None) {
        return result;
    }

    if (social.intent == SocialIntent::Avoid) {
        applyAvoidRelief(*actor, social.utility);
        result.socialExecuted = true;
        return result;
    }

    SocialEventType eventType = SocialEventType::PositiveInteraction;
    double baseIntensity = 0.0;
    if (!socialEventForIntent(social.intent, eventType, baseIntensity)) {
        return result;
    }

    SocialEvent event;
    event.actor = actorId;
    event.recipient = social.target;
    event.type = eventType;
    event.intensity = socialClamp01(baseIntensity * (0.70 + 0.30 * social.utility));
    event.minute = world.minute;
    event.where = where;
    event.source = MemorySource::DirectWitness;
    event.witnessed = true;

    processSocialEvent(*target, relationships, event);

    // Acting positively also affects the initiator's immediate emotion, without
    // manufacturing omniscient beliefs about the target.
    if (social.intent == SocialIntent::Approach) {
        applyEmotionEvent(actor->emotion, EmotionEventType::PositiveSocial, 0.30 * event.intensity);
    } else if (social.intent == SocialIntent::Repair) {
        EmotionDelta d;
        d.anxiety = -0.04;
        d.relief = 0.08;
        actor->emotion.apply(d, event.intensity);
    } else if (social.intent == SocialIntent::Comfort) {
        EmotionDelta d;
        d.affection = 0.05;
        d.relief = 0.04;
        actor->emotion.apply(d, event.intensity);
    }

    result.socialExecuted = true;
    result.generatedSocialEvent = true;
    result.eventType = eventType;
    result.socialEvent = event;
    return result;
}

inline DecisionExecutionResult executeUnifiedDecision(
    World& world,
    RelationshipBook& relationships,
    CharacterId actorId,
    double minimumSocialUtility = 0.18,
    const std::string& where = "") {

    DecisionExecutionResult result;
    result.actor = actorId;

    Character* actor = findMutableCharacter(world, actorId);
    if (!actor || !actor->alive) return result;

    result.decision = chooseUnifiedUtilityDecision(
        world,
        *actor,
        relationships,
        minimumSocialUtility);

    if (result.decision.kind == UnifiedDecisionKind::Physical) {
        return result;
    }

    return executeSocialDecision(
        world,
        relationships,
        actorId,
        result.decision.social,
        where);
}

} // namespace lifelens
