#pragma once
#include "World.h"
namespace lifelens {
inline bool validNaturalEcology(const World& world)
{
    for(const auto& n:world.resourceNodes){
        if(n.ecologyMinute<0 || n.ecologyMinute>world.minute || !std::isfinite(n.flower01) || !std::isfinite(n.fruit01) || n.flower01<0 || n.flower01>1 || n.fruit01<0 || n.fruit01>1) return false;
        if(n.trees.empty()) continue;
        if(n.material!=MaterialKind::Wood || n.trees.size()!=8) return false;
        std::int64_t wood=0,capacity=0;
        for(std::size_t i=0;i<n.trees.size();++i){
            const auto& t=n.trees[i];
            if(t.ordinal!=static_cast<int>(i) || t.bornMinute>world.minute || t.felledMinute<-1 || t.felledMinute>world.minute || t.generation<0 || t.remainingWood<0 || t.capacity<0 || t.remainingWood>t.capacity) return false;
            wood+=t.remainingWood; capacity+=t.capacity;
        }
        if(wood!=n.quantity || capacity!=n.maxQuantity) return false;
    }
    for(std::size_t i=0;i<world.snowpacks.size();++i){
        const auto& s=world.snowpacks[i];
        if(!std::isfinite(s.depthMeters) || s.depthMeters<0 || s.depthMeters>2 || s.minute<0 || s.minute>world.minute || !world.findGeneratedNaturalChunk(s.coord) || (i && !(world.snowpacks[i-1].coord<s.coord))) return false;
    }
    return true;
}
}
