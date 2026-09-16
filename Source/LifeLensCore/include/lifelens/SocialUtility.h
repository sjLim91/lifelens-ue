#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

#include "CivilizationDecision.h"
#include "Relationship.h"
#include "TraitsPreferences.h"
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
    Social,
    Civilization
};

struct UnifiedUtilityDecision {
    UnifiedDecisionKind kind = UnifiedDecisionKind::Physical;
    Goal physicalGoal = Goal::Idle;
    SocialUtilityDecision social;
    CivilizationUtilityDecision civilization;
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

    const TraitProfile traits = deriveTraitProfile(self.personality, self.genetics);
    const PreferenceProfile preferences = derivePreferenceProfile(self.personality, self.genetics);
    const double positiveMemory = taggedMemoryScore(self, world.minute, target, "positive");
    const double negativeMemory = taggedMemoryScore(self, world.minute, target, "negative");
    const double friendlyBelief = std::max(0.0, beliefSignal(self, target, "is_friendly"));
    const double trustworthyBelief = beliefSignal(self, target, "is_trustworthy");

    double score =
        0.02 +
        0.12 * preferences.socializing +
        0.04 * preferences.exploration +
        0.02 * (1.0 - preferences.solitude) +
        0.01 * traits.compassion +
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
    const TraitProfile traits = deriveTraitProfile(self.personality, self.genetics);
    const PreferenceProfile preferences = derivePreferenceProfile(self.personality, self.genetics);

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
        0.14 * preferences.comfort +
        0.08 * preferences.solitude +
        0.13 * (1.0 - traits.boldness);

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

    const TraitProfile traits = deriveTraitProfile(self.personality, self.genetics);
    const PreferenceProfile preferences = derivePreferenceProfile(self.personality, self.genetics);
    const double score =
        0.25 * relation->conflict +
        0.12 * relation->affection +
        0.10 * relation->trust +
        0.08 * relation->comfort +
        0.27 * traits.compassion +
        0.09 * traits.perseverance +
        0.04 * preferences.socializing -
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
    const TraitProfile traits = deriveTraitProfile(self.personality, self.genetics);
    const PreferenceProfile preferences = derivePreferenceProfile(self.personality, self.genetics);

    const double willingness =
        0.43 * traits.compassion +
        0.16 * bond +
        0.10 * affection +
        0.10 * preferences.socializing +
        0.05 * traits.resourcefulness -
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

inline double civilizationDispositionAffinity(
    CivilizationIntent intent,
    const TraitProfile& traits,
    const PreferenceProfile& preferences) {

    switch (intent) {
        case CivilizationIntent::Gather:
            return socialClamp01(
                0.50 * preferences.gathering +
                0.25 * traits.resourcefulness +
                0.25 * traits.perseverance);
        case CivilizationIntent::Store:
            return socialClamp01(
                0.45 * preferences.order +
                0.20 * preferences.comfort +
                0.35 * traits.discipline);
        case CivilizationIntent::Experiment:
            return socialClamp01(
                0.30 * preferences.exploration +
                0.30 * preferences.novelty +
                0.25 * traits.creativity +
                0.15 * traits.boldness);
        case CivilizationIntent::Craft:
            return socialClamp01(
                0.40 * preferences.crafting +
                0.20 * preferences.order +
                0.20 * traits.creativity +
                0.20 * traits.discipline);
        case CivilizationIntent::None:
        default:
            return 0.5;
    }
}

inline CivilizationUtilityDecision applyCivilizationDispositionBias(
    const Character& self,
    CivilizationUtilityDecision candidate) {

    if (candidate.intent == CivilizationIntent::None || candidate.utility <= 0.0) return candidate;
    const TraitProfile traits = deriveTraitProfile(self.personality, self.genetics);
    const PreferenceProfile preferences = derivePreferenceProfile(self.personality, self.genetics);
    const double affinity = civilizationDispositionAffinity(candidate.intent, traits, preferences);
    const double multiplier = 0.90 + 0.20 * affinity;
    candidate.utility = socialClamp01(candidate.utility * multiplier);
    return candidate;
}

inline CivilizationUtilityDecision chooseDispositionAwareCivilizationDecision(
    const World& world,
    const Character& self) {

    CivilizationUtilityDecision best;
    if (self.id == 0 || self.civilization.character != self.id) return best;

    considerCivilizationDecision(best, applyCivilizationDispositionBias(self, bestExperimentDecision(world, self)));
    considerCivilizationDecision(best, applyCivilizationDispositionBias(self, bestCraftDecision(world, self)));
    considerCivilizationDecision(best, applyCivilizationDispositionBias(self, bestStoreDecision(world, self)));
    considerCivilizationDecision(best, applyCivilizationDispositionBias(self, bestGatherDecision(world, self)));
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

inline double maximumResidentNeed(const Character& self)
{
    return std::max({
        self.needs.hunger,
        self.needs.thirst,
        self.needs.sleep,
        self.needs.bladder,
        self.needs.hygiene
    });
}

// Ordinary civilization yields to urgent survival pressure. The one exception
// is acquiring a missing provision required to satisfy that survival pressure:
// if hunger/thirst is already critical and there is no carried PlantFood/Water,
// a real matching ResourceNode may be gathered. This is still an authoritative
// Civilization::Gather action; it does not synthesize food/water or unlock the
// rest of civilization while the resident is in crisis.
inline CivilizationUtilityDecision urgentSurvivalProvisionGatherDecision(
    const World& world,
    const Character& self)
{
    constexpr double SurvivalProvisionThreshold = 0.74;
    CivilizationUtilityDecision best;

    const auto considerProvision = [&](Goal goal, MaterialKind material, double need) {
        if (need < SurvivalProvisionThreshold) return;
        if (objectAvailableFor(world, goal, self.id)) return;
        if (self.civilization.inventory.count(ItemKind::RawMaterial, material) > 0) return;

        for (const auto& node : world.resourceNodes) {
            if (node.id == 0 || node.quantity <= 0 || node.material != material) continue;

            CivilizationUtilityDecision candidate;
            candidate.intent = CivilizationIntent::Gather;
            candidate.utility = socialClamp01(0.80 + 0.20 * need);
            candidate.resourceNode = node.id;
            candidate.material = material;
            candidate.item = ItemKind::RawMaterial;
            candidate.quantity = 2 + static_cast<int>(2.0 * clampCivilization01(self.civilization.gatheringSkill));
            considerCivilizationDecision(best, candidate);
        }
    };

    // Thirst wins exact ties because the production decay rate is higher.
    considerProvision(Goal::Eat, MaterialKind::PlantFood, self.needs.hunger);
    const double hungerUtility = best.utility;
    CivilizationUtilityDecision thirstCandidate;
    if (self.needs.thirst >= SurvivalProvisionThreshold
        && !objectAvailableFor(world, Goal::Drink, self.id)
        && self.civilization.inventory.count(ItemKind::RawMaterial, MaterialKind::Water) == 0) {
        for (const auto& node : world.resourceNodes) {
            if (node.id == 0 || node.quantity <= 0 || node.material != MaterialKind::Water) continue;
            thirstCandidate.intent = CivilizationIntent::Gather;
            thirstCandidate.utility = socialClamp01(0.80 + 0.20 * self.needs.thirst);
            thirstCandidate.resourceNode = node.id;
            thirstCandidate.material = MaterialKind::Water;
            thirstCandidate.item = ItemKind::RawMaterial;
            thirstCandidate.quantity = 2 + static_cast<int>(2.0 * clampCivilization01(self.civilization.gatheringSkill));
            break;
        }
    }
    if (thirstCandidate.intent != CivilizationIntent::None
        && (thirstCandidate.utility > hungerUtility + 1e-12
            || std::abs(thirstCandidate.utility - hungerUtility) <= 1e-12)) {
        best = thirstCandidate;
    }

    return best;
}

inline UnifiedUtilityDecision chooseUnifiedUtilityDecision(
    const World& world,
    const Character& self,
    const RelationshipBook& relationships,
    double minimumSocialUtility = 0.18,
    double minimumCivilizationUtility = 0.14) {

    const auto physical = bestPhysicalUtility(world, self);
    const SocialUtilityDecision social = chooseSocialUtilityDecision(world, self, relationships);
    const CivilizationUtilityDecision civilization = chooseDispositionAwareCivilizationDecision(world, self);
    const CivilizationUtilityDecision survivalProvision = urgentSurvivalProvisionGatherDecision(world, self);

    UnifiedUtilityDecision decision;
    decision.physicalGoal = physical.first;
    decision.social = social;
    decision.civilization = civilization;

    // A missing critical provision is part of survival, not optional progress.
    // Promote only the matching Gather action before applying the normal rule
    // that urgent Needs suppress civilization. Trait/preference bias is
    // intentionally not applied to this emergency path.
    if (survivalProvision.intent != CivilizationIntent::None) {
        decision.kind = UnifiedDecisionKind::Civilization;
        decision.civilization = survivalProvision;
        decision.utility = survivalProvision.utility;
        return decision;
    }

    // Preserve the pre-civilization Physical/Social winner first so existing
    // behavior remains stable unless civilization is clearly more valuable.
    if (social.intent != SocialIntent::None &&
        social.utility >= minimumSocialUtility &&
        social.utility > physical.second * 1.05) {
        decision.kind = UnifiedDecisionKind::Social;
        decision.utility = social.utility;
    } else {
        decision.kind = UnifiedDecisionKind::Physical;
        decision.utility = physical.second;
    }

    // Survival is still dominant. Civilization competes only while all Needs
    // are below the urgent threshold, and must beat the existing winner by a
    // margin rather than constantly interrupting life/social behavior.
    if (maximumResidentNeed(self) < 0.74 &&
        civilization.intent != CivilizationIntent::None &&
        civilization.utility >= minimumCivilizationUtility &&
        civilization.utility > decision.utility * 1.08) {
        decision.kind = UnifiedDecisionKind::Civilization;
        decision.utility = civilization.utility;
    }

    return decision;
}

} // namespace lifelens
