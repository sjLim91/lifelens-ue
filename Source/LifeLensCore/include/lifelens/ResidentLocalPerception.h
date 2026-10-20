#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "World.h"

namespace lifelens {

// M1-01: bounded read-only spatial awareness, not a memory, path permission,
// line-of-sight proof or objective ability to infer remote resources.
// A resource must already be materialized in Core at a reachable *distance
// to inspect*; actual movement, occlusion and pickup remain independent Core
// contracts. In particular this view must NEVER expose all World resources.
inline constexpr int ResidentLocalPerceptionMaxRadius=8;
inline constexpr std::size_t ResidentLocalPerceptionMaxFacts=12;

enum class ObservedResourceAbundance { Scarce, Present };

struct ObservedLocalResource {
    ResourceNodeId nodeId=0;
    MaterialKind material=MaterialKind::Unknown;
    GridPos position{};
    int distanceCells=0;
    ObservedResourceAbundance abundance=ObservedResourceAbundance::Scarce;
};

struct ResidentLocalPerception {
    CharacterId actor=0;
    GridPos observerPosition{};
    int minute=-1;
    int radiusCells=0;
    std::vector<ObservedLocalResource> nearbyResources;
    // True only as a statement about a local radius-limited scan, NOT a
    // claim that the world has no other resources or that LOS is solved.
    bool proximityOnly=true;
};

inline std::int64_t perceptionGridDistance(GridPos a,GridPos b)
{
    const auto dx=static_cast<std::int64_t>(a.x)-b.x;
    const auto dy=static_cast<std::int64_t>(a.y)-b.y;
    return (dx<0?-dx:dx)+(dy<0?-dy:dy);
}

inline ResidentLocalPerception observeLocalResourceProximity(
    const World& world,CharacterId actor,GridPos actualPosition,
    int requestedRadius=6,std::size_t maxFacts=ResidentLocalPerceptionMaxFacts)
{
    ResidentLocalPerception view;
    if(actor==0 ||world.minute<0 ||requestedRadius<1 ||maxFacts==0)
        return view;
    const auto living=std::find_if(
        world.characters.begin(),world.characters.end(),
        [&](const Character& c){return c.id==actor &&c.alive;});
    if(living==world.characters.end()) return view;
    view.actor=actor;
    view.minute=world.minute;
    view.observerPosition=actualPosition;
    view.radiusCells=std::min(
        requestedRadius,ResidentLocalPerceptionMaxRadius);
    for(const auto& node:world.resourceNodes){
        if(node.id==0 ||node.material==MaterialKind::Unknown
           ||node.quantity<=0) continue;
        const auto distance=perceptionGridDistance(
            actualPosition,node.pos);
        if(distance>view.radiusCells) continue;
        view.nearbyResources.push_back({
            node.id,node.material,node.pos,static_cast<int>(distance),
            node.quantity<3
                ?ObservedResourceAbundance::Scarce
                :ObservedResourceAbundance::Present
        });
    }
    std::sort(view.nearbyResources.begin(),view.nearbyResources.end(),
        [](const auto& a,const auto& b){
            if(a.distanceCells!=b.distanceCells)
                return a.distanceCells<b.distanceCells;
            return a.nodeId<b.nodeId;
        });
    if(view.nearbyResources.size()>maxFacts)
        view.nearbyResources.resize(maxFacts);
    return view;
}

} // namespace lifelens
