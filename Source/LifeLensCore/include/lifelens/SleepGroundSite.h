#pragma once

#include <algorithm>
#include <cmath>
#include "ContinuousTerrain.h"
#include "Facility.h"
#include "World.h"

namespace lifelens {
/** A bed or shelter needs actual continuous terrain support across its
 * footprint, not just a flat-looking center sample. The site planner already
 * rewards flatness but could previously build on steep ground if proximity to
 * water/residents compensated the soft score. This is a hard physical gate.
 *
 * Sample the true generation-versioned 3D heightfield, rather than sculpting
 * presentation-only terraces or changing the whole terrain for one building.
 * Core elevations are normalized [0,1]; the Web maps 1.0 to 48 world units.
 */
inline bool settlementSleepFoundationGroundIsLevel(
    const World& world, GridPos pos, FacilityKind kind)
{
    if(kind!=FacilityKind::SleepingPlace && kind!=FacilityKind::Shelter)
        return true;
    // This is the real occupied foundation radius, not a point on the
    // visual mesh. A bedding sheet spans several Core grid cells (32 cells
    // per 8 scene units), so a 1-cell sample would miss the hill underneath.
    const int radius=facilityFootprintRadiusGrid(kind);
    const double maximumRange=kind==FacilityKind::SleepingPlace ? 0.012 : 0.018;
    const WorldGenesisIdentity identity=world.genesisIdentity();
    const double center=deriveContinuousSurfaceElevation01(identity,pos);
    double minimum=center,maximum=center;
    for(const int dy:{-radius,0,radius}){
        for(const int dx:{-radius,0,radius}){
            const double h=deriveContinuousSurfaceElevation01(
                identity,{pos.x+dx,pos.y+dy});
            minimum=std::min(minimum,h);
            maximum=std::max(maximum,h);
            if(maximum-minimum>maximumRange) return false;
        }
    }
    return true;
}


/** Structures from older saves keep their original coordinates and identity.
 * A steep pre-existing bed remains physical, but is no longer usable as an
 * autonomous sleep destination, protection, or settlement sleep capacity.
 * Residents must construct new bedding at a genuinely safe location. */
inline bool settlementSleepFacilityGroundUsable(
    const World& world,const ConstructedFacility& facility)
{
    return settlementSleepFoundationGroundIsLevel(world,facility.pos,facility.kind);
}

} // namespace lifelens
