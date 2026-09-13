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
namespace lifelens {
struct Character {
    CharacterId id=0;
    std::string name;
    Needs needs;
    Personality personality;
    EmotionState emotion;
    MemoryState memory;
    BeliefState beliefs;
    GeneticsProfile genetics;
    std::vector<CharacterId> parentIds;
    std::vector<CharacterId> childrenIds;
    int birthMinute=0;
    double metabolism=1.0;
    double sleepTendency=1.0;
};
}
