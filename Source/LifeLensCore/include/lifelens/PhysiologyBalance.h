#pragma once

namespace lifelens {

// P0 daily physiology budget.
//
// Values are calibrated against one authoritative simulation minute so a
// founder can satisfy basic body maintenance without spending essentially the
// entire day in survival actions. Primitive life remains costly, while
// settlement facilities retain a real time/comfort advantage.
struct PhysiologyBalance {
    int primitiveEatMinutes=1;
    int primitiveDrinkMinutes=1;
    int primitiveToiletMinutes=2;
    int primitiveWashMinutes=4;

    double primitiveEatHungerRelief=0.42;
    double primitiveDrinkThirstRelief=0.46;
    double primitiveToiletBladderReliefPerMinute=0.20;
    double primitiveToiletHygieneBurdenPerMinute=0.012;
    double primitiveWashHygieneReliefPerMinute=0.16;

    double outdoorToiletCompletionHygieneBurden=0.025;
    double outdoorToiletResidueIntensity=0.42;
    int outdoorToiletResidueRadiusTiles=3;

    // A dug pit must remain an actual progression improvement after primitive
    // toilet relief is recalibrated.
    double dugPitBladderReliefPerMinute=0.22;
    double dugPitHygieneBurdenPerMinute=0.004;
    double dugPitCompletionHygieneBurden=0.008;
    double dugPitResidueIntensity=0.16;
    int dugPitResidueRadiusTiles=1;

    // Gross recovery. Normal Need decay still runs while asleep.
    double outdoorSleepRecoveryPerMinute=0.00230;
    double shelterSleepRecoveryBasePerMinute=0.00255;
    double shelterSleepRecoveryEffectivenessBonus=0.00020;
    double sleepingPlaceRecoveryBasePerMinute=0.00300;
    double sleepingPlaceRecoveryEffectivenessBonus=0.00020;
    double smartObjectSleepRecoveryPerMinute=0.00300;

    // Legacy SmartObject facilities consume one carried provision at action
    // start, so their full action remains more efficient than primitive use.
    double smartObjectEatReliefPerMinute=0.075;
    double smartObjectDrinkReliefPerMinute=0.085;
    double smartObjectToiletReliefPerMinute=0.12;
    double smartObjectWashReliefPerMinute=0.10;
};

inline constexpr PhysiologyBalance DefaultPhysiologyBalance{};

} // namespace lifelens
