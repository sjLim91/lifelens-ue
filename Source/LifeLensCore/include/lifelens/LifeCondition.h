#pragma once

#include <algorithm>

namespace lifelens {

inline double clampLifeCondition(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

struct LifeCondition {
    // Lifelong physical/social condition. These are not one-off Needs.
    double physicalHealth=1.0;
    double energyCapacity=1.0;
    double movementCapacity=1.0;
    double reproductivePotential=1.0;
    double workCapacity=1.0;
    double appearanceAgeFactor=0.0;
    double lifeGoalFamilyFocus=0.30;
    double familyRoleSalience=0.20;

    void normalize()
    {
        physicalHealth=clampLifeCondition(physicalHealth);
        energyCapacity=clampLifeCondition(energyCapacity);
        movementCapacity=clampLifeCondition(movementCapacity);
        reproductivePotential=clampLifeCondition(reproductivePotential);
        workCapacity=clampLifeCondition(workCapacity);
        appearanceAgeFactor=clampLifeCondition(appearanceAgeFactor);
        lifeGoalFamilyFocus=clampLifeCondition(lifeGoalFamilyFocus);
        familyRoleSalience=clampLifeCondition(familyRoleSalience);
    }
};

} // namespace lifelens
