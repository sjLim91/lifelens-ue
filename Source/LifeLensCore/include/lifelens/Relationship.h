#pragma once
#include "Ids.h"
namespace lifelens {
struct Relationship {
    CharacterId a=0, b=0;
    double affinity=0.0, trust=0.0, romance=0.0, respect=0.0, tension=0.0, familiarity=0.0;
};
}
