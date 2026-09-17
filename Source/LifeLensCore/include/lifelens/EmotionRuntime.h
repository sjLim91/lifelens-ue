#pragma once

#include <algorithm>

#include "Character.h"

namespace lifelens {

inline double emotionRuntimeSensitivity(const Character& character)
{
    return std::clamp(
        1.05 - 0.35 * character.personality.emotionalStability,
        0.70,
        1.05);
}

inline double residentNeedPressure(const Character& character)
{
    const double pressures[] = {
        character.needs.hunger,
        character.needs.thirst,
        character.needs.sleep,
        character.needs.bladder,
        character.needs.hygiene
    };

    double total = 0.0;
    for (double need : pressures) {
        total += std::max(0.0, need - 0.55) / 0.45;
    }
    return std::clamp(total / 5.0, 0.0, 1.0);
}

inline double maximumResidentNeedPressure(const Character& character)
{
    return std::max({
        character.needs.hunger,
        character.needs.thirst,
        character.needs.sleep,
        character.needs.bladder,
        character.needs.hygiene
    });
}

inline void advanceEmotionRuntime(Character& character, int currentMinute)
{
    if (!character.alive) return;

    // About a day-scale emotional half-life. Significant events can persist,
    // while ordinary pressure does not permanently saturate the resident.
    character.emotion.decay(0.00045);

    // Need pressure is sampled every 15 simulation minutes so severe unmet
    // survival needs create bounded anxiety/fear without per-minute runaway.
    if (currentMinute < 0 || currentMinute % 15 != 0) return;

    const double pressure = residentNeedPressure(character);
    const double maximumNeed = maximumResidentNeedPressure(character);
    if (pressure <= 0.0 && maximumNeed < 0.82) return;

    EmotionDelta delta;
    delta.anxiety = 0.012 * pressure;
    delta.sadness = 0.004 * pressure;
    if (maximumNeed >= 0.82) {
        delta.fear = 0.010 * std::clamp((maximumNeed - 0.82) / 0.18, 0.0, 1.0);
    }
    character.emotion.apply(delta, emotionRuntimeSensitivity(character));
}

inline void applyNeedResolutionEmotion(
    Character& character,
    double needBefore,
    double needAfter)
{
    const double improvement = std::clamp(needBefore - needAfter, 0.0, 1.0);
    if (improvement < 0.015) return;

    EmotionDelta delta;
    delta.joy = 0.020 + 0.050 * improvement;
    delta.relief = 0.050 + 0.140 * improvement;
    delta.anxiety = -0.060 * improvement;
    delta.fear = -0.035 * improvement;
    character.emotion.apply(delta, emotionRuntimeSensitivity(character));
}

inline void applyActionSuccessEmotion(Character& character, double intensity = 1.0)
{
    applyEmotionEvent(
        character.emotion,
        EmotionEventType::Success,
        std::clamp(intensity, 0.0, 1.0) * emotionRuntimeSensitivity(character));
}

inline void applyActionFailureEmotion(
    Character& character,
    int consecutiveFailures,
    double intensity = 1.0)
{
    const double streak = std::clamp(static_cast<double>(consecutiveFailures), 1.0, 4.0);
    EmotionDelta delta;
    delta.sadness = 0.008 + 0.004 * streak;
    delta.anger = 0.018 + 0.012 * streak;
    delta.anxiety = 0.024 + 0.010 * streak;
    delta.relief = -0.018;
    character.emotion.apply(
        delta,
        std::clamp(intensity, 0.0, 1.0) * emotionRuntimeSensitivity(character));
}

inline double emotionSleepUtilityMultiplier(const Character& character)
{
    const double emotionalPressure = std::clamp(
        0.45 * character.emotion.anxiety +
        0.35 * character.emotion.fear +
        0.20 * character.emotion.grief,
        0.0,
        1.0);
    return 1.0 + 0.08 * emotionalPressure;
}

} // namespace lifelens
