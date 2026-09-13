#pragma once

#include <vector>
#include "Ids.h"

namespace lifelens {

enum class LifeEventType {
    Birth
};

struct LifeHistoryEntry {
    LifeEventType type=LifeEventType::Birth;
    int minute=0;
    std::vector<CharacterId> relatedCharacters;
};

} // namespace lifelens
