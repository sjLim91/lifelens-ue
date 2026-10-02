#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "SettlementNetwork.h"
#include "SocietyEconomy.h"

namespace lifelens {

inline constexpr int InterSettlementTradeMaxDistanceChunks=12;
inline constexpr double InterSettlementTradeMissionThreshold=0.60;

struct InterSettlementTradeMission {
    bool available=false;
    CharacterId traveler=0;
    CharacterId partner=0;
    SettlementClusterId originSettlement=0;
    SettlementClusterId destinationSettlement=0;
    GridPos originAnchor{};
    GridPos destinationAnchor{};
    SocietyExchangePlan exchange{};
    int distanceGrid=0;
    double settlementComplement01=0.0;
    double score=0.0;
};

struct SettlementTradeRouteObservation {
    std::uint64_t id=0;
    SettlementClusterId firstSettlement=0;
    SettlementClusterId secondSettlement=0;
    GridPos firstAnchor{};
    GridPos secondAnchor{};
    int partnerCount=0;
    int exchangeCount=0;
    int distanceGrid=0;
    bool active=false;
};

struct SettlementTradeNetworkObservation {
    int routeCount=0;
    int activeRouteCount=0;
    int interSettlementPartnershipCount=0;
    int exchangeEvidenceCount=0;
    std::vector<SettlementTradeRouteObservation> routes;
};

inline const SettlementClusterObservation* settlementClusterForPosition(
    const SettlementNetworkObservation& network,
    GridPos position)
{
    const SettlementClusterObservation* best=nullptr;
    int bestDistance=SettlementServiceRadiusGrid+1;
    for(const SettlementClusterObservation& settlement:network.settlements){
        if(!settlement.active || !settlement.established) continue;
        const int distance=manhattan(position,settlement.anchor);
        if(distance>SettlementServiceRadiusGrid) continue;
        if(best==nullptr || distance<bestDistance
           || (distance==bestDistance && settlement.id<best->id)){
            best=&settlement;
            bestDistance=distance;
        }
    }
    return best;
}

inline std::map<CharacterId,SettlementClusterId> settlementResidentAssignments(
    const SettlementNetworkObservation& network,
    const SettlementPopulation& population)
{
    std::map<CharacterId,SettlementClusterId> result;
    for(const auto& resident:population){
        const SettlementClusterObservation* settlement=
            settlementClusterForPosition(network,resident.second);
        if(settlement!=nullptr){
            result[resident.first]=settlement->id;
        }
    }
    return result;
}

inline const SettlementClusterObservation* settlementById(
    const SettlementNetworkObservation& network,
    SettlementClusterId id)
{
    for(const SettlementClusterObservation& settlement:network.settlements){
        if(settlement.id==id) return &settlement;
    }
    return nullptr;
}

inline int settlementResidentCountForMaterialDemand(
    const SettlementPopulation& population,
    const std::map<CharacterId,SettlementClusterId>& assignments,
    SettlementClusterId settlement)
{
    int count=0;
    for(const auto& resident:population){
        const auto it=assignments.find(resident.first);
        if(it!=assignments.end() && it->second==settlement) ++count;
    }
    return count;
}

inline int settlementMaterialUnits(
    const World& world,
    const SettlementPopulation& population,
    const SettlementNetworkObservation& network,
    SettlementClusterId settlementId,
    MaterialKind material)
{
    const SettlementClusterObservation* settlement=
        settlementById(network,settlementId);
    if(settlement==nullptr) return 0;
    const auto assignments=
        settlementResidentAssignments(network,population);

    int total=0;
    for(const Character& resident:world.characters){
        if(!resident.alive) continue;
        const auto it=assignments.find(resident.id);
        if(it==assignments.end() || it->second!=settlementId) continue;
        total+=resident.civilization.inventory.count(
            ItemKind::RawMaterial,material);
    }
    for(const StorageSite& storage:world.storageSites){
        if(storage.id==0
           || manhattan(storage.pos,settlement->anchor)
                >SettlementServiceRadiusGrid){
            continue;
        }
        total+=storage.inventory.count(ItemKind::RawMaterial,material);
    }
    return total;
}

inline int settlementMaterialDesiredUnits(
    const World& world,
    const SettlementPopulation& population,
    const SettlementNetworkObservation& network,
    SettlementClusterId settlementId,
    MaterialKind material)
{
    const SettlementClusterObservation* settlement=
        settlementById(network,settlementId);
    if(settlement==nullptr) return 0;
    const auto assignments=
        settlementResidentAssignments(network,population);
    const int residents=std::max(
        1,
        settlementResidentCountForMaterialDemand(
            population,assignments,settlementId));

    int desired=0;
    switch(material){
        case MaterialKind::PlantFood:
            desired=residents*2;
            break;
        case MaterialKind::Wood:
        case MaterialKind::Fiber:
        case MaterialKind::Clay:
        case MaterialKind::Stone:
        case MaterialKind::Flint:
            desired=residents;
            break;
        case MaterialKind::CopperOre:
        case MaterialKind::TinOre:
        case MaterialKind::Charcoal:
        case MaterialKind::CopperMetal:
        case MaterialKind::TinMetal:
        case MaterialKind::Bronze:
            desired=1;
            break;
        default:
            desired=0;
            break;
    }

    for(const ConstructedFacility& facility:world.facilities){
        if(facility.state==FacilityState::Operational
           || facility.state==FacilityState::Ruined
           || manhattan(facility.pos,settlement->anchor)
                >SettlementServiceRadiusGrid){
            continue;
        }
        for(const FacilityMaterialRequirement& requirement:
            facility.requirements){
            if(requirement.material==material){
                desired+=std::max(
                    0,requirement.required-requirement.delivered);
            }
        }
    }
    return desired;
}

inline int settlementMaterialSurplusUnits(
    const World& world,
    const SettlementPopulation& population,
    const SettlementNetworkObservation& network,
    SettlementClusterId settlement,
    MaterialKind material)
{
    return std::max(
        0,
        settlementMaterialUnits(
            world,population,network,settlement,material)
        -settlementMaterialDesiredUnits(
            world,population,network,settlement,material));
}

inline int settlementMaterialDeficitUnits(
    const World& world,
    const SettlementPopulation& population,
    const SettlementNetworkObservation& network,
    SettlementClusterId settlement,
    MaterialKind material)
{
    return std::max(
        0,
        settlementMaterialDesiredUnits(
            world,population,network,settlement,material)
        -settlementMaterialUnits(
            world,population,network,settlement,material));
}

inline SocietyExchangePlan reversedSocietyExchangePlan(
    const SocietyExchangePlan& plan)
{
    if(!plan.valid()) return {};
    SocietyExchangePlan reversed;
    reversed.first=plan.second;
    reversed.second=plan.first;
    reversed.firstGives=plan.secondGives;
    reversed.secondGives=plan.firstGives;
    reversed.quantityEach=plan.quantityEach;
    reversed.score=plan.score;
    return reversed;
}

inline double settlementTradeMobility01(
    const Character& resident,
    const SocialKnowledgeBook& book)
{
    const ResidentSocietyStatus status=
        observeResidentSocietyStatus(resident);
    const double roleBonus=
        status.role==SocietyRole::Storekeeper ? 0.16
        : status.role==SocietyRole::Forager ? 0.08
        : 0.0;
    const double networkBonus=
        residentInstitutionMember(
            book,resident.id,SocietyInstitutionKind::ExchangeNetwork)
        ? 0.12 : 0.0;
    return societyClamp01(
        0.34*resident.personality.curiosity
        +0.30*resident.personality.adaptability
        +0.20*resident.personality.sociability
        +roleBonus
        +networkBonus);
}

inline InterSettlementTradeMission bestInterSettlementTradeMission(
    const World& world,
    const SettlementNetworkObservation& network,
    const SettlementPopulation& population,
    const RelationshipBook& relationships,
    const SocialKnowledgeBook& knowledge,
    const std::set<CharacterId>* eligibleResidents=nullptr)
{
    InterSettlementTradeMission best;
    if(network.activeSettlementCount<2) return best;

    const auto assignments=
        settlementResidentAssignments(network,population);
    const int maxDistance=
        WorldChunkSpanGridCells*InterSettlementTradeMaxDistanceChunks;

    for(std::size_t i=0;i<world.characters.size();++i){
        const Character& first=world.characters[i];
        if(!first.alive || requiresDirectCare(first.lifeStage)) continue;
        if(eligibleResidents!=nullptr
           && eligibleResidents->count(first.id)==0) continue;
        const auto firstAssignment=assignments.find(first.id);
        if(firstAssignment==assignments.end()) continue;

        for(std::size_t j=i+1;j<world.characters.size();++j){
            const Character& second=world.characters[j];
            if(!second.alive || requiresDirectCare(second.lifeStage)) continue;
            if(eligibleResidents!=nullptr
               && eligibleResidents->count(second.id)==0) continue;
            const auto secondAssignment=assignments.find(second.id);
            if(secondAssignment==assignments.end()
               || firstAssignment->second==secondAssignment->second){
                continue;
            }

            const auto firstPos=population.find(first.id);
            const auto secondPos=population.find(second.id);
            if(firstPos==population.end() || secondPos==population.end()){
                continue;
            }
            const int distance=manhattan(
                firstPos->second,secondPos->second);
            if(distance<=1 || distance>maxDistance) continue;

            SocietyExchangePlan plan=
                bestMutualExchangePlan(first,second,relationships);
            if(!plan.valid()) continue;

            const int firstSurplus=settlementMaterialSurplusUnits(
                world,population,network,
                firstAssignment->second,plan.firstGives);
            const int secondDeficit=settlementMaterialDeficitUnits(
                world,population,network,
                secondAssignment->second,plan.firstGives);
            const int secondSurplus=settlementMaterialSurplusUnits(
                world,population,network,
                secondAssignment->second,plan.secondGives);
            const int firstDeficit=settlementMaterialDeficitUnits(
                world,population,network,
                firstAssignment->second,plan.secondGives);
            if(firstSurplus<plan.quantityEach
               || secondDeficit<plan.quantityEach
               || secondSurplus<plan.quantityEach
               || firstDeficit<plan.quantityEach){
                continue;
            }

            const double complement=societyClamp01(
                static_cast<double>(
                    std::min(firstSurplus,secondDeficit)
                    +std::min(secondSurplus,firstDeficit))
                /static_cast<double>(
                    std::max(1,2*plan.quantityEach)));
            const bool partnership=hasSocietyTradePartnership(
                knowledge,first.id,second.id);
            const bool bothNetworkMembers=
                residentInstitutionMember(
                    knowledge,first.id,SocietyInstitutionKind::ExchangeNetwork)
                && residentInstitutionMember(
                    knowledge,second.id,SocietyInstitutionKind::ExchangeNetwork);
            const double distance01=societyClamp01(
                static_cast<double>(distance)
                /static_cast<double>(std::max(1,maxDistance)));

            const double firstMobility=
                settlementTradeMobility01(first,knowledge);
            const double secondMobility=
                settlementTradeMobility01(second,knowledge);
            const bool secondTravels=
                secondMobility>firstMobility+1e-12
                || (std::abs(secondMobility-firstMobility)<=1e-12
                    && second.id<first.id);

            const SocietyExchangePlan travelPlan=
                secondTravels
                    ? reversedSocietyExchangePlan(plan)
                    : plan;
            const SettlementClusterId origin=
                secondTravels
                    ? secondAssignment->second
                    : firstAssignment->second;
            const SettlementClusterId destination=
                secondTravels
                    ? firstAssignment->second
                    : secondAssignment->second;
            const SettlementClusterObservation* originSettlement=
                settlementById(network,origin);
            const SettlementClusterObservation* destinationSettlement=
                settlementById(network,destination);
            if(originSettlement==nullptr || destinationSettlement==nullptr){
                continue;
            }

            const double score=societyClamp01(
                0.62*plan.score
                +0.18*complement
                +0.12*std::max(firstMobility,secondMobility)
                +(partnership ? 0.08 : 0.0)
                +(bothNetworkMembers ? 0.05 : 0.0)
                -0.10*distance01);
            if(score<=best.score+1e-12) continue;

            best.available=true;
            best.traveler=travelPlan.first;
            best.partner=travelPlan.second;
            best.originSettlement=origin;
            best.destinationSettlement=destination;
            best.originAnchor=originSettlement->anchor;
            best.destinationAnchor=destinationSettlement->anchor;
            best.exchange=travelPlan;
            best.distanceGrid=distance;
            best.settlementComplement01=complement;
            best.score=score;
        }
    }
    return best;
}

inline std::uint64_t settlementTradeRouteId(
    SettlementClusterId first,
    SettlementClusterId second)
{
    const SettlementClusterId low=std::min(first,second);
    const SettlementClusterId high=std::max(first,second);
    std::uint64_t value=mixKnowledge64(
        0x534554544C545244ull^low);
    value=mixKnowledge64(value^(high<<1));
    return value==0 ? 1 : value;
}

inline std::string interSettlementTradeFactPrefix(
    SettlementClusterId first,
    SettlementClusterId second)
{
    const SettlementClusterId low=std::min(first,second);
    const SettlementClusterId high=std::max(first,second);
    return std::string("intersettlement-exchange:")
        +std::to_string(low)+":"
        +std::to_string(high)+":";
}

inline std::string interSettlementTradePairPrefix(
    SettlementClusterId firstSettlement,
    SettlementClusterId secondSettlement,
    CharacterId firstResident,
    CharacterId secondResident)
{
    const CharacterId lowResident=
        std::min(firstResident,secondResident);
    const CharacterId highResident=
        std::max(firstResident,secondResident);
    return interSettlementTradeFactPrefix(
        firstSettlement,secondSettlement)
        +std::to_string(lowResident)+":"
        +std::to_string(highResident)+":";
}

inline bool isInterSettlementTradeFact(
    const SocialFact& fact,
    SettlementClusterId firstSettlement,
    SettlementClusterId secondSettlement)
{
    const std::string prefix=
        interSettlementTradeFactPrefix(
            firstSettlement,secondSettlement);
    return fact.proposition.rfind(prefix,0)==0;
}

inline SocialFactId interSettlementTradeFactId(
    std::uint64_t seed,
    SettlementClusterId firstSettlement,
    SettlementClusterId secondSettlement,
    CharacterId firstResident,
    CharacterId secondResident,
    MaterialKind firstGives,
    MaterialKind secondGives,
    int minute)
{
    const SettlementClusterId lowSettlement=
        std::min(firstSettlement,secondSettlement);
    const SettlementClusterId highSettlement=
        std::max(firstSettlement,secondSettlement);
    const CharacterId lowResident=
        std::min(firstResident,secondResident);
    const CharacterId highResident=
        std::max(firstResident,secondResident);

    std::uint64_t value=mixKnowledge64(
        seed^0x4953545241444558ull);
    value=mixKnowledge64(value^lowSettlement);
    value=mixKnowledge64(value^(highSettlement<<1));
    value=mixKnowledge64(value^lowResident);
    value=mixKnowledge64(value^(highResident<<1));
    value=mixKnowledge64(
        value^static_cast<std::uint64_t>(
            static_cast<int>(firstGives)+1));
    value=mixKnowledge64(
        value^(static_cast<std::uint64_t>(
            static_cast<int>(secondGives)+1)<<8));
    value=mixKnowledge64(
        value^static_cast<std::uint64_t>(
            std::max(0,minute)));
    return value==0 ? 1 : value;
}

inline const SocialFact* registerInterSettlementTradeFact(
    SocialKnowledgeBook& book,
    Character& traveler,
    Character& partner,
    SettlementClusterId originSettlement,
    SettlementClusterId destinationSettlement,
    const SocietyExchangePlan& exchange,
    int minute,
    std::uint64_t seed)
{
    if(!exchange.valid()
       || traveler.id==0
       || partner.id==0
       || traveler.id==partner.id
       || originSettlement==0
       || destinationSettlement==0
       || originSettlement==destinationSettlement){
        return nullptr;
    }

    SocialFact fact;
    fact.id=interSettlementTradeFactId(
        seed,
        originSettlement,
        destinationSettlement,
        traveler.id,
        partner.id,
        exchange.firstGives,
        exchange.secondGives,
        minute);
    fact.subject=traveler.id;
    fact.proposition=interSettlementTradePairPrefix(
        originSettlement,
        destinationSettlement,
        traveler.id,
        partner.id)
        +std::to_string(
            static_cast<int>(exchange.firstGives))
        +":"
        +std::to_string(
            static_cast<int>(exchange.secondGives));
    fact.where="inter-settlement-trade";
    fact.eventMinute=minute;
    fact.supports=true;
    fact.importance=0.74;
    fact.confidence=0.98;
    fact.emotionValence=0.20;
    fact.emotionIntensity=0.30;
    if(!book.registerFact(fact)) return nullptr;

    book.recordDirectWitness(
        fact.id,
        traveler.id,
        traveler.memory,
        traveler.beliefs,
        minute);
    book.recordDirectWitness(
        fact.id,
        partner.id,
        partner.memory,
        partner.beliefs,
        minute);
    return book.findFact(fact.id);
}

inline int interSettlementTradeFactCount(
    const SocialKnowledgeBook& book,
    SettlementClusterId firstSettlement,
    SettlementClusterId secondSettlement)
{
    int count=0;
    for(const SocialFact& fact:book.facts()){
        if(isInterSettlementTradeFact(
                fact,firstSettlement,secondSettlement)){
            ++count;
        }
    }
    return count;
}

inline int interSettlementTradePairFactCount(
    const SocialKnowledgeBook& book,
    SettlementClusterId firstSettlement,
    SettlementClusterId secondSettlement,
    CharacterId firstResident,
    CharacterId secondResident)
{
    const std::string prefix=
        interSettlementTradePairPrefix(
            firstSettlement,
            secondSettlement,
            firstResident,
            secondResident);
    int count=0;
    for(const SocialFact& fact:book.facts()){
        if(fact.proposition.rfind(prefix,0)==0) ++count;
    }
    return count;
}

inline SettlementTradeNetworkObservation observeSettlementTradeNetwork(
    const World& world,
    const SocialKnowledgeBook& knowledge,
    const SettlementNetworkObservation& network,
    const SettlementPopulation& population)
{
    (void)population;
    SettlementTradeNetworkObservation result;

    for(std::size_t i=0;i<network.settlements.size();++i){
        const SettlementClusterObservation& first=
            network.settlements[i];
        if(!first.established) continue;

        for(std::size_t j=i+1;j<network.settlements.size();++j){
            const SettlementClusterObservation& second=
                network.settlements[j];
            if(!second.established) continue;

            const int exchanges=
                interSettlementTradeFactCount(
                    knowledge,first.id,second.id);
            if(exchanges<=0) continue;

            int partnerPairs=0;
            for(std::size_t a=0;a<world.characters.size();++a){
                const Character& firstResident=
                    world.characters[a];
                if(firstResident.id==0) continue;
                for(std::size_t b=a+1;b<world.characters.size();++b){
                    const Character& secondResident=
                        world.characters[b];
                    if(secondResident.id==0) continue;
                    if(interSettlementTradePairFactCount(
                            knowledge,
                            first.id,
                            second.id,
                            firstResident.id,
                            secondResident.id)<=0){
                        continue;
                    }
                    ++partnerPairs;
                    if(hasSocietyTradePartnership(
                            knowledge,
                            firstResident.id,
                            secondResident.id)){
                        ++result.interSettlementPartnershipCount;
                    }
                }
            }

            SettlementTradeRouteObservation route;
            route.id=settlementTradeRouteId(
                first.id,second.id);
            route.firstSettlement=first.id;
            route.secondSettlement=second.id;
            route.firstAnchor=first.anchor;
            route.secondAnchor=second.anchor;
            route.partnerCount=partnerPairs;
            route.exchangeCount=exchanges;
            route.distanceGrid=manhattan(
                first.anchor,second.anchor);
            route.active=
                first.active
                && second.active
                && exchanges>=2;

            result.exchangeEvidenceCount+=exchanges;
            if(route.active) ++result.activeRouteCount;
            result.routes.push_back(route);
        }
    }

    result.routeCount=
        static_cast<int>(result.routes.size());
    std::sort(
        result.routes.begin(),
        result.routes.end(),
        [](const SettlementTradeRouteObservation& a,
           const SettlementTradeRouteObservation& b){
            return a.id<b.id;
        });
    return result;
}

} // namespace lifelens
