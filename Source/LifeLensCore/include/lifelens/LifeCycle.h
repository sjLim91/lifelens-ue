#pragma once

#include <algorithm>

#include "Character.h"
#include "LifeStage.h"

namespace lifelens {

struct GrowthSnapshot {
    int ageYears=0;
    LifeStage stage=LifeStage::Adult;
    LifeStageProfile profile;
    bool changed=false;
};

inline int lifeStageStartAgeYears(LifeStage stage)
{
    switch(stage){
        case LifeStage::Baby: return 0;
        case LifeStage::Toddler: return 2;
        case LifeStage::Child: return 5;
        case LifeStage::Teen: return 13;
        case LifeStage::YoungAdult: return 18;
        case LifeStage::Adult: return 25;
        case LifeStage::MiddleAge: return 45;
        case LifeStage::Elderly: return 65;
    }
    return 0;
}

inline LifeStage nextLifeStage(LifeStage stage)
{
    switch(stage){
        case LifeStage::Baby: return LifeStage::Toddler;
        case LifeStage::Toddler: return LifeStage::Child;
        case LifeStage::Child: return LifeStage::Teen;
        case LifeStage::Teen: return LifeStage::YoungAdult;
        case LifeStage::YoungAdult: return LifeStage::Adult;
        case LifeStage::Adult: return LifeStage::MiddleAge;
        case LifeStage::MiddleAge: return LifeStage::Elderly;
        case LifeStage::Elderly: return LifeStage::Elderly;
    }
    return LifeStage::Elderly;
}

inline void applyLifeStageProfile(Character& character,LifeStage stage)
{
    const LifeStageProfile profile=lifeStageProfile(stage);
    character.metabolism=character.baseMetabolism*profile.metabolismMultiplier;
    character.sleepTendency=character.baseSleepTendency*profile.sleepTendencyMultiplier;
}

inline GrowthSnapshot observeGrowth(const Character& character,int currentMinute)
{
    GrowthSnapshot snapshot;
    snapshot.stage=character.lifeStage;
    snapshot.profile=lifeStageProfile(character.lifeStage);
    if(character.hasBirthMinute){
        snapshot.ageYears=ageYearsFromMinutes(character.birthMinute,currentMinute);
        snapshot.stage=lifeStageForAgeYears(snapshot.ageYears);
        snapshot.profile=lifeStageProfile(snapshot.stage);
    }
    return snapshot;
}

inline GrowthSnapshot advanceCharacterGrowth(Character& character,int currentMinute)
{
    GrowthSnapshot result=observeGrowth(character,currentMinute);
    if(!character.hasBirthMinute) return result;

    const LifeStage target=result.stage;
    LifeStage cursor=character.lifeStage;

    // World time should be monotonic. Never regress a character if an old snapshot is inspected.
    if(static_cast<int>(target)<=static_cast<int>(cursor)){
        result.stage=character.lifeStage;
        result.profile=lifeStageProfile(character.lifeStage);
        return result;
    }

    while(cursor!=target){
        const LifeStage next=nextLifeStage(cursor);
        if(next==cursor) break;
        const int transitionMinute=character.birthMinute+
            lifeStageStartAgeYears(next)*LifeMinutesPerYear;
        character.lifeHistory.push_back(LifeHistoryEntry{
            LifeEventType::LifeStageChanged,
            std::min(currentMinute,transitionMinute),
            {},
            static_cast<int>(next)});
        cursor=next;
    }

    character.lifeStage=target;
    applyLifeStageProfile(character,target);
    result.stage=target;
    result.profile=lifeStageProfile(target);
    result.changed=true;
    return result;
}

} // namespace lifelens
