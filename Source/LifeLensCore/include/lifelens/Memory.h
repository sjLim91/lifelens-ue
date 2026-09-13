#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#include "Ids.h"

namespace lifelens {

enum class MemorySource {
    DirectWitness,
    ToldByOther,
    Inferred
};

struct MemoryRecord {
    CharacterId who = 0;
    CharacterId sourceCharacter = 0;
    std::string what;
    std::string where;
    int minute = 0;

    // Emotional snapshot attached to the event, not the resident's full current state.
    double emotionValence = 0.0; // [-1, 1]
    double emotionIntensity = 0.0; // [0, 1]

    double importance = 0.5; // [0, 1]
    double confidence = 1.0; // [0, 1]
    bool witnessed = true;
    MemorySource source = MemorySource::DirectWitness;

    // Fractional confidence decay per in-world day before importance/emotion protection.
    double decayPerDay = 0.02;
    std::vector<std::string> tags;

    static double clamp01(double value) {
        return std::max(0.0, std::min(1.0, value));
    }

    static double clampSigned(double value) {
        return std::max(-1.0, std::min(1.0, value));
    }

    void normalize() {
        emotionValence = clampSigned(emotionValence);
        emotionIntensity = clamp01(emotionIntensity);
        importance = clamp01(importance);
        confidence = clamp01(confidence);
        decayPerDay = std::max(0.0, decayPerDay);
    }

    double sourceReliability() const {
        switch (source) {
            case MemorySource::DirectWitness: return witnessed ? 1.0 : 0.85;
            case MemorySource::ToldByOther: return 0.72;
            case MemorySource::Inferred: return 0.55;
        }
        return 0.5;
    }

    double ageDays(int currentMinute) const {
        if (currentMinute <= minute) return 0.0;
        return static_cast<double>(currentMinute - minute) / 1440.0;
    }

    double effectiveConfidence(int currentMinute) const {
        const double days = ageDays(currentMinute);
        const double protection = clamp01(0.55 * importance + 0.25 * emotionIntensity);
        const double effectiveDecay = decayPerDay * (1.0 - 0.75 * protection);
        const double ageFactor = std::exp(-effectiveDecay * days);
        return clamp01(confidence * sourceReliability() * ageFactor);
    }

    double recallScore(
        int currentMinute,
        CharacterId relatedWho = 0,
        const std::vector<std::string>& queryTags = {}) const {

        const double days = ageDays(currentMinute);
        const double recency = std::exp(-0.08 * days);
        const double confidenceScore = effectiveConfidence(currentMinute);

        double personMatch = 0.0;
        if (relatedWho != 0 && who == relatedWho) personMatch = 1.0;

        double tagMatch = 0.0;
        if (!queryTags.empty() && !tags.empty()) {
            std::size_t matched = 0;
            for (const auto& query : queryTags) {
                if (std::find(tags.begin(), tags.end(), query) != tags.end()) ++matched;
            }
            tagMatch = static_cast<double>(matched) / static_cast<double>(queryTags.size());
        }

        return clamp01(
            0.28 * importance +
            0.18 * emotionIntensity +
            0.20 * confidenceScore +
            0.14 * recency +
            0.10 * personMatch +
            0.10 * tagMatch);
    }
};

struct MemoryState {
    std::vector<MemoryRecord> entries;

    void add(MemoryRecord memory) {
        memory.normalize();
        entries.push_back(std::move(memory));
    }

    const MemoryRecord* bestRecall(
        int currentMinute,
        CharacterId relatedWho = 0,
        const std::vector<std::string>& queryTags = {}) const {

        const MemoryRecord* best = nullptr;
        double bestScore = -std::numeric_limits<double>::infinity();

        for (const auto& memory : entries) {
            const double score = memory.recallScore(currentMinute, relatedWho, queryTags);
            if (score > bestScore) {
                bestScore = score;
                best = &memory;
            }
        }
        return best;
    }

    std::vector<const MemoryRecord*> recallAbove(
        int currentMinute,
        double threshold,
        CharacterId relatedWho = 0,
        const std::vector<std::string>& queryTags = {}) const {

        std::vector<const MemoryRecord*> result;
        for (const auto& memory : entries) {
            if (memory.recallScore(currentMinute, relatedWho, queryTags) >= threshold) {
                result.push_back(&memory);
            }
        }
        std::sort(result.begin(), result.end(), [=](const MemoryRecord* a, const MemoryRecord* b) {
            return a->recallScore(currentMinute, relatedWho, queryTags) >
                   b->recallScore(currentMinute, relatedWho, queryTags);
        });
        return result;
    }
};

} // namespace lifelens
