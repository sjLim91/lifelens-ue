#pragma once

#include <algorithm>

#include "Character.h"

namespace lifelens {

// Shared resident-level food preferences. These are derived from persistent
// personality/state instead of being stored as a second mutable authority.
inline double foodReserveDiscipline01(const Character& character)
{
    return std::clamp(
        0.45*character.personality.conscientiousness
        +0.30*character.personality.orderliness
        +0.25*character.personality.patience,
        0.0,1.0);
}

inline double immediateEatingDrive01(const Character& character)
{
    return std::clamp(
        0.50*character.personality.impulsiveness
        +0.25*(1.0-character.personality.patience)
        +0.25*(1.0-character.personality.conscientiousness),
        0.0,1.0);
}

inline double carriedPlantFoodFreshness01(const Character& character)
{
    const int carried=character.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood);
    return carried>0
        ? std::clamp(
            character.civilization.inventory.averagePlantFoodFreshness(),
            0.0,1.0)
        : 0.0;
}

// How strongly this resident values keeping the currently carried PlantFood
// available for near-term personal consumption. This is not an Eat command;
// it is an opportunity-cost signal used when another action would irreversibly
// consume the same provision (for example, planting the final edible unit).
inline double plantFoodRetentionClaim01(const Character& character)
{
    const int carried=character.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood);
    if(carried<=0) return 0.0;

    const double hunger=std::clamp(character.needs.hunger,0.0,1.0);
    const double discipline=foodReserveDiscipline01(character);
    const double immediateDrive=immediateEatingDrive01(character);
    const double freshness=carriedPlantFoodFreshness01(character);
    const double spoilagePressure=1.0-freshness;

    return std::clamp(
        0.58*hunger
        +0.18*immediateDrive
        +0.16*discipline*(1.0-hunger)
        +0.08*freshness
        -0.18*spoilagePressure,
        0.0,1.0);
}

} // namespace lifelens
