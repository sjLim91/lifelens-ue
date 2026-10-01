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
    double primitiveDrinkThirstRelief=0.45;
    double primitiveToiletBladderReliefPerMinute=0.13;
    double primitiveToiletHygieneBurdenPerMinute=0.012;
    double primitiveWashHygieneReliefPerMinute=0.18;

    double outdoorToiletCompletionHygieneBurden=0.025;
    double outdoorToiletResidueIntensity=0.42;
    int outdoorToiletResidueRadiusTiles=3;

    // Gross recovery. Normal Need decay still runs while asleep.
    double outdoorSleepRecoveryPerMinute=0.00215;
    double shelterSleepRecoveryBasePerMinute=0.00220;
    double shelterSleepRecoveryEffectivenessBonus=0.00010;
    double sleepingPlaceRecoveryBasePerMinute=0.00240;
    double sleepingPlaceRecoveryEffectivenessBonus=0.00015;
    double smartObjectSleepRecoveryPerMinute=0.00245;

    // Legacy SmartObject facilities consume one carried provision at action
    // start, so their full action remains more efficient than primitive use.
    double smartObjectEatReliefPerMinute=0.075;
    double smartObjectDrinkReliefPerMinute=0.085;
    double smartObjectToiletReliefPerMinute=0.12;
    double smartObjectWashReliefPerMinute=0.10;
};

inline constexpr PhysiologyBalance DefaultPhysiologyBalance{};

} // namespace lifelens
