#pragma once

#include <algorithm>
#include <cmath>

#include "Character.h"
#include "UtilityAI.h"

namespace lifelens {

inline double maximumNeedPressure(const Needs& needs)
{
    return std::max({needs.hunger,needs.thirst,needs.sleep,needs.bladder,needs.hygiene});
}

inline double needPressureSeverity(const Needs& needs)
{
    constexpr double PressureStart=0.55;
    return EmotionState::clamp01(
        (maximumNeedPressure(needs)-PressureStart)/(1.0-PressureStart));
}

inline void advanceEmotionOneMinute(Character& character)
{
    // Founders stay neutral until a real pressure/event exists. Once emotions
    // exist they fade slowly instead of becoming a permanent personality stat.
    character.emotion.decay(0.00020);

    const double pressure=needPressureSeverity(character.needs);
    if(pressure<=0.0) return;

    const double stability=EmotionState::clamp01(character.personality.emotionalStability);
    const double sensitivity=0.55+0.65*(1.0-stability);
    EmotionDelta delta;
    delta.anxiety=0.00055*pressure;
    delta.fear=0.00020*pressure*pressure;
    delta.sadness=0.00012*pressure;
    delta.relief=-0.00018*pressure;
    character.emotion.apply(delta,sensitivity);
}

inline void applyNeedResolutionEmotion(
    Character& character,
    const Needs& before,
    Goal goal)
{
    double beforeNeed=0.0;
    switch(goal){
        case Goal::Eat: beforeNeed=before.hunger; break;
        case Goal::Drink: beforeNeed=before.thirst; break;
        case Goal::Sleep: beforeNeed=before.sleep; break;
        case Goal::UseToilet: beforeNeed=before.bladder; break;
        case Goal::Wash: beforeNeed=before.hygiene; break;
        case Goal::Idle: default: break;
    }
    const double improvement=std::max(0.0,beforeNeed-needForGoal(character,goal));
    if(improvement<=0.0) return;

    const double urgency=EmotionState::clamp01((beforeNeed-0.35)/0.65);
    EmotionDelta delta;
    delta.relief=0.20*improvement*(0.55+0.45*urgency);
    delta.joy=0.055*improvement;
    delta.anxiety=-0.12*improvement;
    delta.fear=-0.06*improvement;
    character.emotion.apply(delta);
}

inline void applyActionFailureEmotion(Character& character,int consecutiveFailures)
{
    const double repetition=EmotionState::clamp01(
        static_cast<double>(std::max(1,consecutiveFailures))/3.0);
    EmotionDelta delta;
    delta.anger=0.025+0.055*repetition;
    delta.anxiety=0.020+0.050*repetition;
    delta.sadness=0.010*repetition;
    delta.relief=-0.025*repetition;
    character.emotion.apply(delta,0.70+0.60*character.personality.impulsiveness);
}

inline void applyCivilizationOutcomeEmotion(Character& character,bool success,bool discovery=false)
{
    if(success){
        const double ambition=EmotionState::clamp01(character.personality.ambition);
        applyEmotionEvent(
            character.emotion,EmotionEventType::Success,
            (discovery ? 0.80 : 0.45)*(0.75+0.35*ambition));
        return;
    }
    applyActionFailureEmotion(character,1);
}

} // namespace lifelens
