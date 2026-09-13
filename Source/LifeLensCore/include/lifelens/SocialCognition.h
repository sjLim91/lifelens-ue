#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "Belief.h"
#include "Character.h"
#include "Emotion.h"
#include "Memory.h"
#include "Relationship.h"

namespace lifelens {

enum class SocialEventType {
    PositiveInteraction,
    Help,
    Comfort,
    Conflict,
    Betrayal,
    Rejection,
    Apology,
    Intimacy,
    Commitment
};

struct SocialEvent {
    CharacterId actor = 0;
    CharacterId recipient = 0;
    SocialEventType type = SocialEventType::PositiveInteraction;
    double intensity = 1.0;
    int minute = 0;
    std::string where;

    // Perception channel used by the observer processing this event.
    MemorySource source = MemorySource::DirectWitness;
    CharacterId sourceCharacter = 0;
    bool witnessed = true;
};

struct SocialBeliefEffect {
    std::string proposition;
    bool supports = true;
};

struct SocialCognitionResult {
    double perspectiveScale = 0.0;
    double beliefConfidence = 0.0;
    double beliefStance = 0.0;
    double relationshipScale = 0.0;
    std::string beliefProposition;
    bool beliefSupports = true;
    std::size_t memoryIndex = 0;
};

inline double clampSocial01(double value) {
    return std::max(0.0, std::min(1.0, value));
}

inline const char* socialEventTag(SocialEventType type) {
    switch (type) {
        case SocialEventType::PositiveInteraction: return "positive_interaction";
        case SocialEventType::Help: return "help";
        case SocialEventType::Comfort: return "comfort";
        case SocialEventType::Conflict: return "conflict";
        case SocialEventType::Betrayal: return "betrayal";
        case SocialEventType::Rejection: return "rejection";
        case SocialEventType::Apology: return "apology";
        case SocialEventType::Intimacy: return "intimacy";
        case SocialEventType::Commitment: return "commitment";
    }
    return "social_event";
}

inline bool socialEventIsNegative(SocialEventType type) {
    return type == SocialEventType::Conflict ||
           type == SocialEventType::Betrayal ||
           type == SocialEventType::Rejection;
}

inline EmotionEventType emotionEventForSocial(SocialEventType type) {
    switch (type) {
        case SocialEventType::PositiveInteraction: return EmotionEventType::PositiveSocial;
        case SocialEventType::Help: return EmotionEventType::PositiveSocial;
        case SocialEventType::Comfort: return EmotionEventType::Comfort;
        case SocialEventType::Conflict: return EmotionEventType::Conflict;
        case SocialEventType::Betrayal: return EmotionEventType::Betrayal;
        case SocialEventType::Rejection: return EmotionEventType::Rejection;
        case SocialEventType::Apology: return EmotionEventType::Comfort;
        case SocialEventType::Intimacy: return EmotionEventType::RomanticCloseness;
        case SocialEventType::Commitment: return EmotionEventType::RomanticCloseness;
    }
    return EmotionEventType::PositiveSocial;
}

inline RelationshipEvent relationshipEventForSocial(SocialEventType type) {
    switch (type) {
        case SocialEventType::PositiveInteraction: return RelationshipEvent::SharedPositiveExperience;
        case SocialEventType::Help: return RelationshipEvent::Helped;
        case SocialEventType::Comfort: return RelationshipEvent::Comforted;
        case SocialEventType::Conflict: return RelationshipEvent::Conflict;
        case SocialEventType::Betrayal: return RelationshipEvent::Betrayal;
        case SocialEventType::Rejection: return RelationshipEvent::Rejection;
        case SocialEventType::Apology: return RelationshipEvent::Apology;
        case SocialEventType::Intimacy: return RelationshipEvent::Intimacy;
        case SocialEventType::Commitment: return RelationshipEvent::CommitmentMade;
    }
    return RelationshipEvent::SharedPositiveExperience;
}

inline SocialBeliefEffect beliefEffectForSocial(SocialEventType type) {
    SocialBeliefEffect effect;
    switch (type) {
        case SocialEventType::PositiveInteraction:
            effect.proposition = "is_friendly";
            effect.supports = true;
            break;
        case SocialEventType::Help:
            effect.proposition = "is_reliable";
            effect.supports = true;
            break;
        case SocialEventType::Comfort:
            effect.proposition = "is_caring";
            effect.supports = true;
            break;
        case SocialEventType::Conflict:
            effect.proposition = "is_safe";
            effect.supports = false;
            break;
        case SocialEventType::Betrayal:
            effect.proposition = "is_trustworthy";
            effect.supports = false;
            break;
        case SocialEventType::Rejection:
            effect.proposition = "romantically_interested";
            effect.supports = false;
            break;
        case SocialEventType::Apology:
            effect.proposition = "wants_repair";
            effect.supports = true;
            break;
        case SocialEventType::Intimacy:
            effect.proposition = "romantically_interested";
            effect.supports = true;
            break;
        case SocialEventType::Commitment:
            effect.proposition = "is_committed";
            effect.supports = true;
            break;
    }
    return effect;
}

inline double socialEventImportance(SocialEventType type) {
    switch (type) {
        case SocialEventType::PositiveInteraction: return 0.42;
        case SocialEventType::Help: return 0.60;
        case SocialEventType::Comfort: return 0.64;
        case SocialEventType::Conflict: return 0.70;
        case SocialEventType::Betrayal: return 0.96;
        case SocialEventType::Rejection: return 0.76;
        case SocialEventType::Apology: return 0.56;
        case SocialEventType::Intimacy: return 0.82;
        case SocialEventType::Commitment: return 0.92;
    }
    return 0.5;
}

inline double socialPerspectiveScale(const Character& observer, const SocialEvent& event) {
    if (observer.id == event.recipient) return 1.0;
    if (observer.id == event.actor) return 0.55;

    switch (event.source) {
        case MemorySource::DirectWitness:
            return event.witnessed ? 0.35 : 0.24;
        case MemorySource::ToldByOther:
            return 0.20;
        case MemorySource::Inferred:
            return 0.12;
    }
    return 0.12;
}

inline RelationshipDelta observedRelationshipDelta(SocialEventType type, double intensity) {
    const double k = clampSocial01(intensity);
    RelationshipDelta d;

    switch (type) {
        case SocialEventType::PositiveInteraction:
            d.familiarity = 0.025 * k;
            d.comfort = 0.012 * k;
            break;
        case SocialEventType::Help:
            d.trust = 0.022 * k;
            d.respect = 0.038 * k;
            d.affection = 0.012 * k;
            break;
        case SocialEventType::Comfort:
            d.trust = 0.018 * k;
            d.respect = 0.025 * k;
            d.affection = 0.015 * k;
            break;
        case SocialEventType::Conflict:
            d.trust = -0.020 * k;
            d.respect = -0.018 * k;
            d.conflict = 0.030 * k;
            d.fear = 0.012 * k;
            break;
        case SocialEventType::Betrayal:
            d.trust = -0.055 * k;
            d.respect = -0.040 * k;
            d.conflict = 0.035 * k;
            d.grudge = 0.020 * k;
            break;
        case SocialEventType::Rejection:
            d.familiarity = 0.010 * k;
            break;
        case SocialEventType::Apology:
            d.respect = 0.018 * k;
            d.trust = 0.010 * k;
            break;
        case SocialEventType::Intimacy:
            d.familiarity = 0.018 * k;
            break;
        case SocialEventType::Commitment:
            d.respect = 0.012 * k;
            d.familiarity = 0.016 * k;
            break;
    }
    return d;
}

inline SocialCognitionResult processSocialEvent(
    Character& observer,
    RelationshipBook& relationships,
    const SocialEvent& event) {

    SocialCognitionResult result;
    result.perspectiveScale = socialPerspectiveScale(observer, event);
    const double eventIntensity = clampSocial01(event.intensity);
    const double perceptionStrength = eventIntensity * result.perspectiveScale;

    // 1) Immediate emotional response.
    applyEmotionEvent(
        observer.emotion,
        emotionEventForSocial(event.type),
        perceptionStrength);

    // 2) Persist a memory snapshot of what this observer believes they perceived.
    MemoryRecord memory;
    memory.who = event.actor;
    memory.sourceCharacter = event.sourceCharacter;
    memory.what = socialEventTag(event.type);
    memory.where = event.where;
    memory.minute = event.minute;
    memory.emotionValence = observer.emotion.valence;
    memory.emotionIntensity = observer.emotion.intensity();
    memory.importance = clampSocial01(socialEventImportance(event.type) * (0.65 + 0.35 * eventIntensity));
    memory.confidence = 1.0;
    memory.witnessed = event.witnessed;
    memory.source = event.source;
    memory.decayPerDay = socialEventIsNegative(event.type) ? 0.014 : 0.022;
    memory.tags.push_back("social");
    memory.tags.push_back(socialEventTag(event.type));
    memory.tags.push_back(socialEventIsNegative(event.type) ? "negative" : "positive");
    observer.memory.add(std::move(memory));
    result.memoryIndex = observer.memory.entries.empty() ? 0 : observer.memory.entries.size() - 1;

    // 3) Turn remembered evidence into a belief instead of granting omniscient knowledge.
    const SocialBeliefEffect beliefEffect = beliefEffectForSocial(event.type);
    BeliefRecord& belief = observer.beliefs.ingestMemory(
        observer.memory.entries[result.memoryIndex],
        beliefEffect.proposition,
        beliefEffect.supports,
        event.minute);

    result.beliefProposition = beliefEffect.proposition;
    result.beliefSupports = beliefEffect.supports;
    result.beliefConfidence = belief.confidence;
    result.beliefStance = belief.stance;

    // 4) Relationship changes are filtered by perception and belief confidence.
    if (event.actor != 0 && event.actor != observer.id) {
        Relationship& relation = relationships.getOrCreate(observer.id, event.actor);
        const double beliefGate = 0.55 + 0.45 * belief.confidence;
        result.relationshipScale = clampSocial01(perceptionStrength * beliefGate);

        if (observer.id == event.recipient) {
            relation.apply(relationshipDeltaFor(
                relationshipEventForSocial(event.type),
                result.relationshipScale));
        } else {
            RelationshipDelta observed = observedRelationshipDelta(event.type, result.relationshipScale);
            relation.apply(observed);
        }
    }

    return result;
}

} // namespace lifelens
