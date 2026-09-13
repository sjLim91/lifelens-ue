#pragma once
#include <optional>
#include "Ids.h"
#include "Needs.h"
namespace lifelens {
struct GridPos { int x=0; int y=0; };
enum class ObjectKind { Bed, Toilet, Sink, Fridge, Chair, Table, Sofa };
struct SmartObject {
    ObjectId id=0;
    ObjectKind kind=ObjectKind::Chair;
    GridPos pos{};
    std::optional<CharacterId> reservedBy;
    NeedsDelta effectPerTick{};
    int useDurationTicks=1;
};
inline int manhattan(GridPos a, GridPos b) { return std::abs(a.x-b.x)+std::abs(a.y-b.y); }
}
