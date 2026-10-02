#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <vector>

#include "Facility.h"
#include "SettlementDemand.h"
#include "World.h"

namespace lifelens {

using SettlementClusterId = std::uint64_t;

struct SettlementClusterObservation {
    SettlementClusterId id=0;
    GridPos anchor{};
    int residentCount=0;
    int facilityCount=0;
    int operationalFacilityCount=0;
    int plannedFacilityCount=0;
    int storageSiteCount=0;
    bool active=false;
    bool established=false;
};

struct SettlementNetworkObservation {
    int settlementCount=0;
    int activeSettlementCount=0;
    int residentAssignedCount=0;
    std::vector<SettlementClusterObservation> settlements;
};

namespace detail {

enum class SettlementNodeKind : std::uint8_t {
    Facility=0,
    Storage=1
};

struct SettlementNode {
    SettlementClusterId key=0;
    SettlementNodeKind kind=SettlementNodeKind::Facility;
    GridPos pos{};
    bool operational=false;
    bool planned=false;
};

inline SettlementClusterId settlementFacilityNodeKey(FacilityId id)
{
    return (static_cast<SettlementClusterId>(id)<<1);
}

inline SettlementClusterId settlementStorageNodeKey(StorageId id)
{
    return (static_cast<SettlementClusterId>(id)<<1)|1ULL;
}

inline std::size_t settlementRoot(
    std::vector<std::size_t>& parent,
    std::size_t index)
{
    while(parent[index]!=index){
        parent[index]=parent[parent[index]];
        index=parent[index];
    }
    return index;
}

inline void settlementUnion(
    std::vector<std::size_t>& parent,
    std::size_t a,
    std::size_t b)
{
    a=settlementRoot(parent,a);
    b=settlementRoot(parent,b);
    if(a==b) return;
    if(a>b) std::swap(a,b);
    parent[b]=a;
}

} // namespace detail

inline SettlementNetworkObservation observeSettlementNetwork(
    const World& world,
    const SettlementPopulation* population=nullptr)
{
    SettlementNetworkObservation result;
    std::vector<detail::SettlementNode> nodes;
    nodes.reserve(world.facilities.size()+world.storageSites.size());

    for(const ConstructedFacility& facility:world.facilities){
        if(facility.id==0 || facility.state==FacilityState::Ruined) continue;
        detail::SettlementNode node;
        node.key=detail::settlementFacilityNodeKey(facility.id);
        node.kind=detail::SettlementNodeKind::Facility;
        node.pos=facility.pos;
        node.operational=facilityOperationalAndActive(facility);
        node.planned=
            facility.state==FacilityState::Planned
            || facility.state==FacilityState::UnderConstruction;
        nodes.push_back(node);
    }
    for(const StorageSite& storage:world.storageSites){
        if(storage.id==0) continue;
        detail::SettlementNode node;
        node.key=detail::settlementStorageNodeKey(storage.id);
        node.kind=detail::SettlementNodeKind::Storage;
        node.pos=storage.pos;
        node.operational=true;
        nodes.push_back(node);
    }

    if(nodes.empty()) return result;

    std::vector<std::size_t> parent(nodes.size());
    for(std::size_t i=0;i<parent.size();++i) parent[i]=i;

    // Infrastructure that can serve the same lived-area radius is one
    // settlement cluster. Distant infrastructure remains independent even
    // though every site still lives in the same World authority.
    for(std::size_t i=0;i<nodes.size();++i){
        for(std::size_t j=i+1;j<nodes.size();++j){
            if(manhattan(nodes[i].pos,nodes[j].pos)
                <=SettlementServiceRadiusGrid){
                detail::settlementUnion(parent,i,j);
            }
        }
    }

    struct Accumulator {
        SettlementClusterId id=0;
        long long x=0;
        long long y=0;
        int nodes=0;
        int facilityCount=0;
        int operationalFacilityCount=0;
        int plannedFacilityCount=0;
        int storageSiteCount=0;
    };

    std::unordered_map<std::size_t,Accumulator> groups;
    for(std::size_t i=0;i<nodes.size();++i){
        const std::size_t root=detail::settlementRoot(parent,i);
        auto& group=groups[root];
        if(group.id==0 || nodes[i].key<group.id) group.id=nodes[i].key;
        group.x+=nodes[i].pos.x;
        group.y+=nodes[i].pos.y;
        ++group.nodes;
        if(nodes[i].kind==detail::SettlementNodeKind::Storage){
            ++group.storageSiteCount;
        }else{
            ++group.facilityCount;
            if(nodes[i].operational) ++group.operationalFacilityCount;
            if(nodes[i].planned) ++group.plannedFacilityCount;
        }
    }

    result.settlements.reserve(groups.size());
    for(const auto& entry:groups){
        const Accumulator& group=entry.second;
        SettlementClusterObservation cluster;
        cluster.id=group.id;
        cluster.anchor={
            static_cast<int>(group.x/std::max(1,group.nodes)),
            static_cast<int>(group.y/std::max(1,group.nodes))
        };
        cluster.facilityCount=group.facilityCount;
        cluster.operationalFacilityCount=group.operationalFacilityCount;
        cluster.plannedFacilityCount=group.plannedFacilityCount;
        cluster.storageSiteCount=group.storageSiteCount;
        cluster.established=
            cluster.operationalFacilityCount>0
            || cluster.storageSiteCount>0;
        result.settlements.push_back(cluster);
    }

    if(population!=nullptr){
        for(const auto& resident:*population){
            const Character* character=nullptr;
            for(const Character& candidate:world.characters){
                if(candidate.id==resident.first){
                    character=&candidate;
                    break;
                }
            }
            if(character==nullptr || !character->alive) continue;

            SettlementClusterObservation* best=nullptr;
            int bestDistance=std::numeric_limits<int>::max();
            for(auto& cluster:result.settlements){
                const int distance=manhattan(resident.second,cluster.anchor);
                if(distance>SettlementServiceRadiusGrid) continue;
                if(best==nullptr || distance<bestDistance
                   || (distance==bestDistance && cluster.id<best->id)){
                    best=&cluster;
                    bestDistance=distance;
                }
            }
            if(best!=nullptr){
                ++best->residentCount;
                ++result.residentAssignedCount;
            }
        }
    }

    for(auto& cluster:result.settlements){
        cluster.active=cluster.residentCount>0
            || cluster.operationalFacilityCount>0
            || cluster.storageSiteCount>0;
        if(cluster.active) ++result.activeSettlementCount;
    }
    result.settlementCount=static_cast<int>(result.settlements.size());

    std::sort(
        result.settlements.begin(),
        result.settlements.end(),
        [](const SettlementClusterObservation& a,
           const SettlementClusterObservation& b){
            return a.id<b.id;
        });

    return result;
}

} // namespace lifelens
