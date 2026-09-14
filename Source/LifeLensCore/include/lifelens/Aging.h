#pragma once

#include <algorithm>

#include "Character.h"
#include "LifeCycle.h"
#include "LifeCondition.h"

namespace lifelens {

struct AgingSnapshot {
    int ageYears=0;
    LifeStage stage=LifeStage::Adult;
    LifeCondition target;
};

inline double ageRangeProgress(int ageYears,int startAge,int endAge)
{
    if(endAge<=startAge) return ageYears>=endAge ? 1.0 : 0.0;
    return clampLifeCondition(
        static_cast<double>(ageYears-startAge)/static_cast<double>(endAge-startAge));
}

inline LifeCondition lifeConditionForAge(
    int ageYears,
    double geneticHealthPotential=0.5,
    std::size_t childCount=0)
{
    const int age=std::max(0,ageYears);
    const double geneticHealth=0.90+0.20*clampLifeCondition(geneticHealthPotential);
    LifeCondition state;

    if(age<18){
        const double maturity=ageRangeProgress(age,0,18);
        state.physicalHealth=clampLifeCondition(0.97*geneticHealth);
        state.energyCapacity=1.0;
        state.movementCapacity=0.60+0.40*maturity;
        state.reproductivePotential=0.0;
        state.workCapacity=0.0;
        state.appearanceAgeFactor=0.20*maturity;
        state.lifeGoalFamilyFocus=0.10+0.10*maturity;
        state.familyRoleSalience=0.05+0.10*maturity;
    }else if(age<45){
        const double adultProgress=ageRangeProgress(age,18,45);
        state.physicalHealth=clampLifeCondition((0.99-0.05*adultProgress)*geneticHealth);
        state.energyCapacity=1.0-0.10*adultProgress;
        state.movementCapacity=1.0-0.04*adultProgress;
        if(age<30) state.reproductivePotential=1.0;
        else if(age<40) state.reproductivePotential=1.0-0.30*ageRangeProgress(age,30,40);
        else state.reproductivePotential=0.70-0.45*ageRangeProgress(age,40,45);
        state.workCapacity=0.92+0.08*ageRangeProgress(age,18,25);
        state.appearanceAgeFactor=0.20+0.28*adultProgress;
        state.lifeGoalFamilyFocus=0.25+0.30*adultProgress;
        state.familyRoleSalience=0.20+0.35*adultProgress;
    }else if(age<65){
        const double middle=ageRangeProgress(age,45,65);
        state.physicalHealth=clampLifeCondition((0.88-0.18*middle)*geneticHealth);
        state.energyCapacity=0.86-0.18*middle;
        state.movementCapacity=0.94-0.14*middle;
        state.reproductivePotential=0.25*(1.0-middle);
        state.workCapacity=0.96-0.26*middle;
        state.appearanceAgeFactor=0.48+0.24*middle;
        state.lifeGoalFamilyFocus=0.58+0.17*middle;
        state.familyRoleSalience=0.60+0.18*middle;
    }else{
        const double elderly=ageRangeProgress(age,65,90);
        state.physicalHealth=clampLifeCondition((0.66-0.28*elderly)*geneticHealth);
        state.energyCapacity=0.64-0.28*elderly;
        state.movementCapacity=0.76-0.34*elderly;
        state.reproductivePotential=0.0;
        state.workCapacity=0.30*(1.0-elderly);
        state.appearanceAgeFactor=0.72+0.28*elderly;
        state.lifeGoalFamilyFocus=0.78+0.12*elderly;
        state.familyRoleSalience=0.82+0.14*elderly;
    }

    const double familyBoost=std::min(0.16,static_cast<double>(childCount)*0.04);
    state.lifeGoalFamilyFocus+=familyBoost;
    state.familyRoleSalience+=familyBoost;
    state.normalize();
    return state;
}

inline AgingSnapshot observeAging(const Character& character,int currentMinute)
{
    AgingSnapshot snapshot;
    snapshot.stage=character.lifeStage;
    if(character.hasBirthMinute){
        snapshot.ageYears=ageYearsFromMinutes(character.birthMinute,currentMinute);
        snapshot.stage=lifeStageForAgeYears(snapshot.ageYears);
    }
    snapshot.target=lifeConditionForAge(
        snapshot.ageYears,character.genetics.healthPotential,character.childrenIds.size());
    return snapshot;
}

inline AgingSnapshot advanceAging(Character& character,int currentMinute)
{
    if(character.hasBirthMinute) advanceCharacterGrowth(character,currentMinute);
    AgingSnapshot snapshot=observeAging(character,currentMinute);

    // Aging never heals unrelated damage. Capacity ceilings may fall as age advances,
    // while role/life-goal salience can grow with lived experience and family position.
    character.lifeCondition.physicalHealth=std::min(
        character.lifeCondition.physicalHealth,snapshot.target.physicalHealth);
    character.lifeCondition.energyCapacity=std::min(
        character.lifeCondition.energyCapacity,snapshot.target.energyCapacity);
    character.lifeCondition.movementCapacity=std::min(
        character.lifeCondition.movementCapacity,snapshot.target.movementCapacity);
    character.lifeCondition.reproductivePotential=std::min(
        character.lifeCondition.reproductivePotential,snapshot.target.reproductivePotential);
    character.lifeCondition.workCapacity=std::min(
        character.lifeCondition.workCapacity,snapshot.target.workCapacity);
    character.lifeCondition.appearanceAgeFactor=std::max(
        character.lifeCondition.appearanceAgeFactor,snapshot.target.appearanceAgeFactor);
    character.lifeCondition.lifeGoalFamilyFocus=std::max(
        character.lifeCondition.lifeGoalFamilyFocus,snapshot.target.lifeGoalFamilyFocus);
    character.lifeCondition.familyRoleSalience=std::max(
        character.lifeCondition.familyRoleSalience,snapshot.target.familyRoleSalience);
    character.lifeCondition.normalize();

    return snapshot;
}

} // namespace lifelens
