#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

#include "Relationship.h"
#include "UtilityAI.h"

namespace lifelens {

enum class SocialIntent {
    None,
    Approach,
    Avoid,
    Repair,
    Comfort
};

inline const char* socialIntentName(SocialIntent intent) {
    switch (intent) {
        case SocialIntent::Approach: return "Approach";
        case SocialIntent::Avoid: return "Avoid";
        case SocialIntent::Repair: return "Repair";
        case SocialIntent::Comfort: return "Comfort";
        default: return "None";
    }
}

struct SocialUtilityDecision {
    SocialIntent intent = SocialIntent::None;
    CharacterId target = 0;
    double utility = 0.0;
};

enum class UnifiedDecisionKind {
    Physical,
    Social
};

struct UnifiedUtilityDecision {
    UnifiedDecisionKind kind = UnifiedDecisionKind::Physical;
    Goal physicalGoal = Goal::Idle;
    SocialUtilityDecision social;
    double utility = 0.0;
};

inline double socialClamp01(double value) {
    return std::max(0.0, std::min(1.0, value));
}

inline double beliefSignal(
    const Character& character,
    CharacterId target,
    const std::string& proposition) {

    const BeliefRecord* belief = character.beliefs.find(target, proposition);
    if (!belief) return 0.0;
    return belief->stance * belief->confidence;
}

inline double taggedMemoryScore(
    const Character& character,
    int currentMinute,
    CharacterId target,
    const std::string& tag) {

    double best = 0.0;
    for (const auto& memory : character.memory.entries) {
        if (memory.who != target) continue;
        if (std::find(memory.tags.begin(), memory.tags.end(), tag) == memory.tags.end()) continue;
        best = std::max(best, memory.recallScore(currentMinute, target, {tag}));
    }
    return best;
}

inline const Character* findCharacter(const World& world, CharacterId id) {
    for (const auto& character : world.characters) {
        if (character.id == id) return &character;
    }
    return nullptr;
}

inline double scoreApproachIntent(
    const World& world,
    const Character& self,
    CharacterId target,
    const RelationshipBook& relationships) {

    const Relationship* relation = relationships.find(self.id, target);
    const double bond = relation ? relation->socialBond() : 0.0;
    const double trust = relation ? relation->trust : 0.0;
    const double affection = relation ? relation->affection : 0.0;
    const double fear = relation ? relation->fear : 0.0;
    const double conflict = relation ? relation->conflict : 0.0;
    const double grudge = relation ? relation->grudge : 0.0;

    const double positiveMemory = taggedMemoryScore(self, world.minute, target, "positive");
    const double negativeMemory = taggedMemoryScore(self, world.minute, target, "negative");
    const double friendlyBelief = std::max(0.0, beliefSignal(self, target, "is_friendly"));
    const double trustworthyBelief = beliefSignal(self, target, "is_trustworthy");

    double score =
        0.02 +
        0.10 * self.personality.sociability +
        0.05 * self.personality.curiosity +
        0.04 * (1.0 - self.personality.introversion) +
        0.22 * bond +
        0.10 * trust +
        0.08 * affection +
        0.08 * positiveMemory +
        0.07 * friendlyBelief +
        0.05 * std::max(0.0, trustworthyBelief) -
        0.24 * fear -
        0.18 * conflict -
        0.15 * grudge -
        0.08 * negativeMemory -
        0.08 * std::max(0.0, -trustworthyBelief);

    return socialClamp01(score);
}

inline double scoreAvoidIntent(
    const World& world,
    const Character& self,
    CharacterId target,
    const RelationshipBook& relationships) {

    const Relationship* relation = relationships.find(self.id, target);
    const double fear = relation ? relation->fear : 0.0;
    const double conflict = relation ? relation->conflict : 0.0;
    const double grudge = relation ? relation->grudge : 0.0;
    const double negativeMemory = taggedMemoryScore(self, world.minute, target, "negative");
    const double trustworthyBelief = beliefSignal(self, target, "is_trustworthy");
    const double safeBelief = beliefSignal(self, target, "is_safe");

    double score =
        0.30 * fear +
        0.22 * conflict +
        0.18 * grudge +
        0.12 * negativeMemory +
        0.13 * std::max(0.0, -trustworthyBelief) +
        0.10 * std::max(0.0, -safeBelief) +
        0.08 * self.emotion.fear +
        0.07 * self.emotion.anxiety +
        0.05 * self.emotion.anger;

    const double caution =
        0.85 +
        0.25 * (1.0 - self.personality.riskTolerance) +
        0.10 * self.personality.introversion;

    return socialClamp01(score * caution);
}

inline double scoreRepairIntent(
    const Character& self,
    CharacterId target,
    const RelationshipBook& relationships) {

    const Relationship* relation = relationships.find(self.id, target);
    if (!relation || relation->conflict < 0.08) return 0.0;

    const double remainingBond =
        (relation->affection + relation->trust + relation->comfort) / 3.0;
    if (remainingBond < 0.08) return 0.0;

    const double score =
        0.25 * relation->conflict +
        0.12 * relation->affection +
        0.10 * relation->trust +
        0.08 * relation->comfort +
        0.15 * self.personality.agreeableness +
        0.15 * self.personality.empathy +
        0.10 * self.personality.patience -
        0.12 * relation->grudge -
        0.12 * relation->fear -
        0.08 * self.emotion.anger;

    return socialClamp01(score);
}

inline double scoreComfortIntent(
    const Character& self,
    const Character& target,
    const RelationshipBook& relationships) {

    const double distress = std::max({
        target.emotion.sadness,
        target.emotion.grief,
        target.emotion.anxiety,
        target.emotion.fear
    });
    if (distress < 0.15) return 0.0;

    const Relationship* relation = relationships.find(self.id, target.id);
    const double bond = relation ? relation->socialBond() : 0.0;
    const double affection = relation ? relation->affection : 0.0;
    const double conflict = relation ? relation->conflict : 0.0;
    const double fear = relation ? relation->fear : 0.0;

    const double willingness =
        0.34 * self.personality.empathy +
        0.18 * self.personality.agreeableness +
        0.16 * bond +
        0.10 * affection +
        0.06 * self.personality.sociability -
        0.10 * conflict -
        0.08 * fear;

    return socialClamp01(distress * std::max(0.0, willingness));
}

inline SocialUtilityDecision chooseSocialUtilityDecision(
    const World& world,
    const Character& self,
    const RelationshipBook& relationships) {

    SocialUtilityDecision best;

    for (const auto& candidate : world.characters) {
        if (candidate.id == self.id) continue;

        const std::array<std::pair<SocialIntent, double>, 4> scores = {{
            {SocialIntent::Approach, scoreApproachIntent(world, self, candidate.id, relationships)},
            {SocialIntent::Avoid, scoreAvoidIntent(world, self, candidate.id, relationships)},
            {SocialIntent::Repair, scoreRepairIntent(self, candidate.id, relationships)},
            {SocialIntent::Comfort, scoreComfortIntent(self, candidate, relationships)}
        }};

        for (const auto& entry : scores) {
            if (entry.second > best.utility) {
                best.intent = entry.first;
                best.target = candidate.id;
                best.utility = entry.second;
            }
        }
    }

    return best;
}

inline std::pair<Goal, double> bestPhysicalUtility(
    const World& world,
    const Character& self) {

    const std::array<Goal, 6> goals = {
        Goal::Eat,
        Goal::Drink,
        Goal::Sleep,
        Goal::UseToilet,
        Goal::Wash,
        Goal::Idle
    };

    Goal bestGoal = Goal::Idle;
    double bestScore = -1.0;
    for (const Goal goal : goals) {
        const double score = scoreGoal(world, self, goal);
        if (score > bestScore) {
            bestGoal = goal;
            bestScore = score;
        }
    }
    return {bestGoal, bestScore};
}

inline UnifiedUtilityDecision chooseUnifiedUtilityDecision(
    const World& world,
    const Character& self,
    const RelationshipBook& relationships,
    double minimumSocialUtility = 0.18) {

    const auto physical = bestPhysicalUtility(world, self);
    const SocialUtilityDecision social = chooseSocialUtilityDecision(world, self, relationships);

    UnifiedUtilityDecision decision;
    decision.physicalGoal = physical.first;
    decision.social = social;

    if (social.intent != SocialIntent::None &&
        social.utility >= minimumSocialUtility &&
        social.utility > physical.second * 1.05) {
        decision.kind = UnifiedDecisionKind::Social;
        decision.utility = social.utility;
    } else {
        decision.kind = UnifiedDecisionKind::Physical;
        decision.utility = physical.second;
    }

    return decision;
}

} // namespace lifelens
