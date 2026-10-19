#pragma once
#include <algorithm>
#include <unordered_set>
#include "SimulationSnapshot.h"
namespace lifelens {
inline constexpr char DependentEscortMagic[]={'L','L','D','E','S','C','0','1'};
inline bool validDependentEscorts(const SimulationStateSnapshot& snapshot){
    std::unordered_set<CharacterId> claimed;
    for(const auto& entry:snapshot.runtime){
        const auto& e=entry.second.dependentEscort;
        if(!e.active()){
            if(e.site!=0 ||e.startedMinute!=0 ||e.returning
               ||e.origin.x!=0 ||e.origin.y!=0)return false;
            continue;
        }
        if(e.child==entry.first ||e.site==0 ||e.startedMinute<0
           ||e.startedMinute>snapshot.world.minute
           ||!claimed.insert(e.child).second)return false;
        const auto child=snapshot.runtime.find(e.child);
        if(child==snapshot.runtime.end()
           ||child->second.pos.x!=entry.second.pos.x
           ||child->second.pos.y!=entry.second.pos.y)return false;
        bool actorExists=false,childExists=false,siteExists=false;
        for(const auto& actor:snapshot.world.characters){
            actorExists|=actor.id==entry.first;
            childExists|=actor.id==e.child;
        }
        for(const auto& site:snapshot.world.primitiveSanitationSites)
            siteExists|=site.id==e.site;
        if(!actorExists ||!childExists ||!siteExists)return false;
        const auto& p=entry.second.pendingContext;
        if(p.active() &&p.kind==ContextActionKind::Parenting
           &&(p.parentingTarget!=e.child ||p.parentingAction!=ParentingAction::ToiletAssist
              ||p.sanitationSiteId!=e.site))return false;
    }
    return true;
}
template<typename W> void writeDependentEscorts(W& w,const SimulationStateSnapshot& snapshot){
    w.raw(DependentEscortMagic,sizeof(DependentEscortMagic));w.u32(1);
    std::vector<CharacterId> actors;
    for(const auto& entry:snapshot.runtime)
        if(entry.second.dependentEscort.active())actors.push_back(entry.first);
    std::sort(actors.begin(),actors.end());
    w.u32(static_cast<std::uint32_t>(actors.size()));
    for(const auto actor:actors){
        const auto& e=snapshot.runtime.at(actor).dependentEscort;
        w.u64(actor);w.u64(e.child);w.u64(e.site);
        w.i32(e.origin.x);w.i32(e.origin.y);w.i32(e.startedMinute);w.boolean(e.returning);
    }
}
template<typename R> bool readDependentEscorts(R& r,SimulationStateSnapshot& snapshot){
    char magic[sizeof(DependentEscortMagic)]{};std::uint32_t version=0,count=0;
    if(!r.raw(magic,sizeof(magic))
       ||!std::equal(magic,magic+sizeof(magic),DependentEscortMagic)
       ||!r.u32(version) ||version!=1 ||!r.count(count)
       ||count>snapshot.runtime.size())return false;
    std::unordered_set<CharacterId> seen;
    for(std::uint32_t i=0;i<count;++i){
        CharacterId actor=0;DependentEscortState e;
        if(!r.u64(actor) ||!seen.insert(actor).second
           ||!r.u64(e.child) ||e.child==0 ||!r.u64(e.site)
           ||!r.i32(e.origin.x) ||!r.i32(e.origin.y) ||!r.i32(e.startedMinute)
           ||!r.boolean(e.returning))return false;
        auto found=snapshot.runtime.find(actor);
        if(found==snapshot.runtime.end())return false;
        found->second.dependentEscort=e;
    }
    return validDependentEscorts(snapshot);
}
}
