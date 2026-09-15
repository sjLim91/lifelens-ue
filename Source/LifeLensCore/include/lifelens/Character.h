#pragma once
#include <string>
#include <vector>
#include "Ids.h"
#include "Needs.h"
#include "Personality.h"
#include "Emotion.h"
#include "Memory.h"
#include "Belief.h"
#include "Genetics.h"
#include "LifeHistory.h"
#include "LifeStage.h"
#include "Development.h"
#include "LifeCondition.h"
#include "Civilization.h"
namespace lifelens {

enum class Sex {
    Male,
    Female
};

inline const char* sexName(Sex sex)
{
    return sex==Sex::Male ? "Male" : "Female";
}

struct Character {
    CharacterId id=0;
    std::string name;
    Sex sex=Sex::Male;
    Needs needs;
    Personality personality;
    EmotionState emotion;
    MemoryState memory;
    BeliefState beliefs;
    GeneticsProfile genetics;
    ChildDevelopment development;
    LifeCondition lifeCondition;
    IndividualCivilizationState civilization;
    std::vector<CharacterId> parentIds;
    std::vector<CharacterId> childrenIds;
    std::vector<LifeHistoryEntry> lifeHistory;
    bool hasBirthMinute=false;
    int birthMinute=0;
    LifeStage lifeStage=LifeStage::Adult;
    bool alive=true;
    int deathMinute=-1;
    double baseMetabolism=1.0;
    double baseSleepTendency=1.0;
    double metabolism=1.0;
    double sleepTendency=1.0;
};
}
