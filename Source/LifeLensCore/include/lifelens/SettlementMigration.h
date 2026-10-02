#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

#include "Facility.h"
#include "Household.h"
#include "LifeCycle.h"
#include "MigrationPressure.h"
#include "SettlementNetwork.h"
#include "World.h"

namespace lifelens {

// C6-D: an individual may discover a frontier alone, but durable relocation is
// allowed to become a household decision only when the co-located autonomous
// members share enough pressure to move together.
inline constexpr double HouseholdMigrationConsensusThreshold = 0.52;
inline constexpr double HouseholdMigrationLeaderThreshold =
    MigrationCandidatePressureThreshold;
inline constexpr int HouseholdMigrationDecisionIntervalMinutes = 6*60;

// Empty infrastructure should not remain permanently pristine. This extra wear
// is deliberately slow (about one season to major degradation) and stacks with
// ordinary weather/use wear already owned by facility progression.
inline constexpr double AbandonedSettlementHourlyWear = 0.00055;

struct HouseholdMigrationPlan {
    bool available=false;
    HouseholdId householdId=0;
    CharacterId leader=0;
    MaterialKind bottleneckMaterial=MaterialKind::Unknown;
    GridPos origin{};
    GridPos target{};
    ChunkCoord targetChunk{};
    double leaderPressure01=0.0;
    double consensusPressure01=0.0;
    std::vector<CharacterId> members;
};

enum class SettlementLifecycleState : std::uint8_t {
    Inhabited=0,
    Vulnerable=1,
    Declining=2,
    Abandoned=3
};

struct SettlementLifecycleEntry {
    SettlementClusterId settlementId=0;
    GridPos anchor{};
    int residentCount=0;
    int operationalFacilityCount=0;
    int storageSiteCount=0;
    double vitality01=0.0;
    SettlementLifecycleState state=SettlementLifecycleState::Inhabited;
};

struct SettlementLifecycleObservation {
    int inhabitedCount=0;
    int decliningCount=0;
    int abandonedCount=0;
    std::vector<SettlementLifecycleEntry> settlements;
};

inline const Character* householdMigrationCharacter(
    const World& world,
    CharacterId id)
{
    for(const Character& character:world.characters){
        if(character.id==id) return &character;
    }
    return nullptr;
}

inline HouseholdMigrationPlan chooseHouseholdMigrationPlan(
    const World& world,
    const HouseholdBook& households,
    const SettlementPopulation& population)
{
    HouseholdMigrationPlan best;
    double bestScore=-1.0;

    for(const Household& household:households.all()){
        if(household.id==0 || household.members.size()<2) continue;

        std::vector<CharacterId> members;
        std::vector<const Character*> autonomous;
        GridPos origin{};
        bool haveOrigin=false;
        bool separated=false;

        for(const HouseholdMember& member:household.members){
            const Character* character=
                householdMigrationCharacter(world,member.characterId);
            if(character==nullptr || !character->alive) continue;
            const auto positionIt=population.find(character->id);
            if(positionIt==population.end()) continue;

            if(!haveOrigin){
                origin=positionIt->second;
                haveOrigin=true;
            }else if(
                manhattan(origin,positionIt->second)>SettlementServiceRadiusGrid){
                separated=true;
                break;
            }

            members.push_back(character->id);
            if(!requiresDirectCare(character->lifeStage)
               && lifeStageProfile(character->lifeStage).autonomy>=0.35){
                autonomous.push_back(character);
            }
        }

        // Dependent transport needs a dedicated carry/accompany action. Do not
        // fake it by letting an infant walk a frontier route or teleport.
        if(separated || !haveOrigin || members.size()<2
           || autonomous.size()!=members.size()){
            continue;
        }

        double totalPressure=0.0;
        int pressureCount=0;
        MigrationPressureObservation leaderPressure;
        for(const Character* character:autonomous){
            const auto positionIt=population.find(character->id);
            if(positionIt==population.end()) continue;
            const MigrationPressureObservation pressure=
                observeMigrationPressure(
                    world,*character,positionIt->second,&population);
            totalPressure+=pressure.pressure01;
            ++pressureCount;
            if(pressure.candidate
               && pressure.hasFrontierTarget
               && (
                    !leaderPressure.candidate
                    || pressure.pressure01>leaderPressure.pressure01+1e-12
                    || (
                        std::abs(pressure.pressure01-leaderPressure.pressure01)
                            <=1e-12
                        && character->id<leaderPressure.residentId
                    )
               )){
                leaderPressure=pressure;
            }
        }

        if(!leaderPressure.candidate
           || !leaderPressure.hasFrontierTarget
           || leaderPressure.pressure01<HouseholdMigrationLeaderThreshold
           || pressureCount<=0){
            continue;
        }

        const double consensus=
            totalPressure/static_cast<double>(pressureCount);
        if(consensus<HouseholdMigrationConsensusThreshold) continue;

        const double score=
            0.62*leaderPressure.pressure01+0.38*consensus;
        if(best.available && score<bestScore-1e-12) continue;
        if(best.available
           && std::abs(score-bestScore)<=1e-12
           && household.id>best.householdId){
            continue;
        }

        best.available=true;
        best.householdId=household.id;
        best.leader=leaderPressure.residentId;
        best.bottleneckMaterial=leaderPressure.bottleneckMaterial;
        best.origin=origin;
        best.target=leaderPressure.frontierTarget;
        best.targetChunk=leaderPressure.frontierChunk;
        best.leaderPressure01=leaderPressure.pressure01;
        best.consensusPressure01=consensus;
        best.members=std::move(members);
        bestScore=score;
    }

    return best;
}

inline SettlementLifecycleObservation observeSettlementLifecycle(
    const World& world,
    const SettlementPopulation& population)
{
    SettlementLifecycleObservation result;
    const SettlementNetworkObservation network=
        observeSettlementNetwork(world,&population);
    result.settlements.reserve(network.settlements.size());

    for(const SettlementClusterObservation& cluster:network.settlements){
        SettlementLifecycleEntry entry;
        entry.settlementId=cluster.id;
        entry.anchor=cluster.anchor;
        entry.residentCount=cluster.residentCount;
        entry.operationalFacilityCount=cluster.operationalFacilityCount;
        entry.storageSiteCount=cluster.storageSiteCount;

        const int infrastructure=
            cluster.operationalFacilityCount+cluster.storageSiteCount;
        if(cluster.residentCount<=0){
            if(cluster.operationalFacilityCount>0){
                entry.state=SettlementLifecycleState::Declining;
                entry.vitality01=0.18;
                ++result.decliningCount;
            }else{
                entry.state=SettlementLifecycleState::Abandoned;
                entry.vitality01=0.0;
                ++result.abandonedCount;
            }
        }else if(infrastructure<=0){
            entry.state=SettlementLifecycleState::Vulnerable;
            entry.vitality01=0.35;
            ++result.inhabitedCount;
        }else{
            entry.state=SettlementLifecycleState::Inhabited;
            entry.vitality01=std::min(
                1.0,
                0.45
                +0.10*static_cast<double>(
                    std::min(cluster.residentCount,3))
                +0.08*static_cast<double>(
                    std::min(infrastructure,3)));
            ++result.inhabitedCount;
        }
        result.settlements.push_back(entry);
    }
    return result;
}

inline void advanceAbandonedSettlementDecayOneHour(
    World& world,
    const SettlementPopulation& population)
{
    if(world.minute%60!=0) return;
    const SettlementLifecycleObservation lifecycle=
        observeSettlementLifecycle(world,population);

    for(const SettlementLifecycleEntry& settlement:lifecycle.settlements){
        if(settlement.state!=SettlementLifecycleState::Declining) continue;
        for(ConstructedFacility& facility:world.facilities){
            if(!facilityOperationalAndActive(facility)
               || !facilitySupportsMaintenance(facility.kind)
               || manhattan(facility.pos,settlement.anchor)
                    >SettlementServiceRadiusGrid){
                continue;
            }
            applyFacilityWear(facility,AbandonedSettlementHourlyWear);
        }
    }
}

} // namespace lifelens
