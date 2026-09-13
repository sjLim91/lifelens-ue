#pragma once

#include <algorithm>
#include <cmath>

namespace lifelens {

enum class EmotionEventType {
    PositiveSocial,
    Comfort,
    Success,
    RomanticCloseness,
    Rejection,
    Conflict,
    Betrayal,
    Threat,
    Loss
};

struct EmotionDelta {
    double joy = 0.0;
    double sadness = 0.0;
    double anger = 0.0;
    double fear = 0.0;
    double embarrassment = 0.0;
    double pride = 0.0;
    double jealousy = 0.0;
    double affection = 0.0;
    double anxiety = 0.0;
    double relief = 0.0;
    double grief = 0.0;
};

struct EmotionState {
    // MASTER SPEC 25 emotion dimensions. Values remain in [0, 1].
    double joy = 0.0;
    double sadness = 0.0;
    double anger = 0.0;
    double fear = 0.0;
    double embarrassment = 0.0;
    double pride = 0.0;
    double jealousy = 0.0;
    double affection = 0.0;
    double anxiety = 0.0;
    double relief = 0.0;
    double grief = 0.0;

    // Compatibility/summary channels used by existing code and future UI.
    double valence = 0.0; // [-1, 1]
    double arousal = 0.0; // [0, 1]

    static double clamp01(double value) {
        return std::max(0.0, std::min(1.0, value));
    }

    static double clampSigned(double value) {
        return std::max(-1.0, std::min(1.0, value));
    }

    void refreshSummary() {
        const double positive =
            0.32 * joy +
            0.18 * pride +
            0.28 * affection +
            0.22 * relief;

        const double negative =
            0.18 * sadness +
            0.16 * anger +
            0.13 * fear +
            0.08 * embarrassment +
            0.11 * jealousy +
            0.13 * anxiety +
            0.21 * grief;

        valence = clampSigned(positive - negative);

        arousal = clamp01(
            0.10 * joy +
            0.18 * anger +
            0.16 * fear +
            0.08 * embarrassment +
            0.12 * jealousy +
            0.16 * anxiety +
            0.08 * pride +
            0.06 * affection +
            0.06 * grief);
    }

    void apply(const EmotionDelta& d, double scale = 1.0) {
        joy = clamp01(joy + d.joy * scale);
        sadness = clamp01(sadness + d.sadness * scale);
        anger = clamp01(anger + d.anger * scale);
        fear = clamp01(fear + d.fear * scale);
        embarrassment = clamp01(embarrassment + d.embarrassment * scale);
        pride = clamp01(pride + d.pride * scale);
        jealousy = clamp01(jealousy + d.jealousy * scale);
        affection = clamp01(affection + d.affection * scale);
        anxiety = clamp01(anxiety + d.anxiety * scale);
        relief = clamp01(relief + d.relief * scale);
        grief = clamp01(grief + d.grief * scale);
        refreshSummary();
    }

    void decay(double fraction) {
        const double keep = std::max(0.0, 1.0 - std::max(0.0, fraction));
        joy *= keep;
        sadness *= keep;
        anger *= keep;
        fear *= keep;
        embarrassment *= keep;
        pride *= keep;
        jealousy *= keep;
        affection *= keep;
        anxiety *= keep;
        relief *= keep;
        grief *= keep;
        refreshSummary();
    }

    double intensity() const {
        return clamp01(std::max({
            joy, sadness, anger, fear, embarrassment, pride,
            jealousy, affection, anxiety, relief, grief
        }));
    }
};

inline EmotionDelta emotionDeltaForEvent(EmotionEventType event) {
    switch (event) {
        case EmotionEventType::PositiveSocial:
            return {.joy = 0.18, .affection = 0.12, .relief = 0.04};
        case EmotionEventType::Comfort:
            return {.joy = 0.06, .sadness = -0.10, .fear = -0.08,
                    .affection = 0.15, .anxiety = -0.12, .relief = 0.18};
        case EmotionEventType::Success:
            return {.joy = 0.20, .pride = 0.22, .relief = 0.08};
        case EmotionEventType::RomanticCloseness:
            return {.joy = 0.16, .affection = 0.24, .anxiety = -0.03};
        case EmotionEventType::Rejection:
            return {.sadness = 0.18, .embarrassment = 0.16,
                    .affection = -0.06, .anxiety = 0.12};
        case EmotionEventType::Conflict:
            return {.sadness = 0.08, .anger = 0.20, .anxiety = 0.08};
        case EmotionEventType::Betrayal:
            return {.sadness = 0.14, .anger = 0.24, .jealousy = 0.14,
                    .affection = -0.18, .anxiety = 0.10};
        case EmotionEventType::Threat:
            return {.fear = 0.24, .anxiety = 0.20};
        case EmotionEventType::Loss:
            return {.sadness = 0.24, .anxiety = 0.08, .grief = 0.32};
    }
    return {};
}

inline void applyEmotionEvent(
    EmotionState& state,
    EmotionEventType event,
    double sensitivity = 1.0) {
    state.apply(emotionDeltaForEvent(event), std::max(0.0, sensitivity));
}

} // namespace lifelens
