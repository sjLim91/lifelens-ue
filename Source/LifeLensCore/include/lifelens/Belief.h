#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "Ids.h"
#include "Memory.h"

namespace lifelens {

struct BeliefRecord {
    CharacterId subject = 0;
    std::string proposition;

    // Positive = believes proposition is true, negative = believes it is false.
    double stance = 0.0; // [-1, 1]
    double confidence = 0.0; // [0, 1]

    double supportWeight = 0.0;
    double contradictionWeight = 0.0;
    std::size_t supportCount = 0;
    std::size_t contradictionCount = 0;
    int lastUpdatedMinute = 0;

    static double clamp01(double value) {
        return std::max(0.0, std::min(1.0, value));
    }

    static double clampSigned(double value) {
        return std::max(-1.0, std::min(1.0, value));
    }

    void refresh() {
        const double total = supportWeight + contradictionWeight;
        if (total <= 1e-12) {
            stance = 0.0;
            confidence = 0.0;
            return;
        }

        stance = clampSigned((supportWeight - contradictionWeight) / total);
        confidence = clamp01(1.0 - std::exp(-total));
    }

    void applyEvidence(bool supports, double strength, int minute) {
        const double safeStrength = std::max(0.0, strength);
        if (supports) {
            supportWeight += safeStrength;
            ++supportCount;
        } else {
            contradictionWeight += safeStrength;
            ++contradictionCount;
        }
        lastUpdatedMinute = std::max(lastUpdatedMinute, minute);
        refresh();
    }
};

struct BeliefState {
    std::vector<BeliefRecord> beliefs;

    BeliefRecord* find(CharacterId subject, const std::string& proposition) {
        for (auto& belief : beliefs) {
            if (belief.subject == subject && belief.proposition == proposition) return &belief;
        }
        return nullptr;
    }

    const BeliefRecord* find(CharacterId subject, const std::string& proposition) const {
        for (const auto& belief : beliefs) {
            if (belief.subject == subject && belief.proposition == proposition) return &belief;
        }
        return nullptr;
    }

    BeliefRecord& getOrCreate(CharacterId subject, std::string proposition) {
        if (BeliefRecord* existing = find(subject, proposition)) return *existing;
        beliefs.push_back(BeliefRecord{});
        BeliefRecord& created = beliefs.back();
        created.subject = subject;
        created.proposition = std::move(proposition);
        return created;
    }

    double evidenceStrength(const MemoryRecord& memory, int currentMinute) const {
        const double reliability = memory.effectiveConfidence(currentMinute);
        const double salience = 0.40 + 0.60 * MemoryRecord::clamp01(memory.importance);
        return std::max(0.0, reliability * salience);
    }

    BeliefRecord& ingestMemory(
        const MemoryRecord& memory,
        const std::string& proposition,
        bool supports,
        int currentMinute) {

        BeliefRecord& belief = getOrCreate(memory.who, proposition);
        belief.applyEvidence(supports, evidenceStrength(memory, currentMinute), currentMinute);
        return belief;
    }
};

} // namespace lifelens
