#pragma once
#include <string>
#include "Ids.h"
#include "Needs.h"
#include "Personality.h"
#include "Emotion.h"
#include "Memory.h"
namespace lifelens {
struct Character {
    CharacterId id=0;
    std::string name;
    Needs needs;
    Personality personality;
    EmotionState emotion;
    MemoryState memory;
    double metabolism=1.0;
    double sleepTendency=1.0;
};
}
