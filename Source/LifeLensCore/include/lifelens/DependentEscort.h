#pragma once
#include "ContextAction.h"
namespace lifelens {
// A physical obligation survives cancellation of the current care action.
struct DependentEscortState {
    CharacterId child=0;
    GridPos origin{};
    SanitationSiteId site=0;
    int startedMinute=0;
    bool returning=false;
    bool active() const { return child!=0; }
    void clear(){*this=DependentEscortState{};}
};
}
