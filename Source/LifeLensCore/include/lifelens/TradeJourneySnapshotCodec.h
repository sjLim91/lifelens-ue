#pragma once

#include <algorithm>
#include <cstdint>
#include <unordered_set>

#include "SimulationSnapshot.h"

namespace lifelens {

constexpr char TradeJourneySnapshotExtensionMagic[]={'L','L','T','J','R','N','0','1'};
constexpr std::uint32_t TradeJourneySnapshotExtensionVersion=1;

template<typename WriterT>
void writeTradeJourneySnapshotExtension(
    WriterT& w,
    const SimulationStateSnapshot& snapshot)
{
    w.raw(
        TradeJourneySnapshotExtensionMagic,
        sizeof(TradeJourneySnapshotExtensionMagic));
    w.u32(TradeJourneySnapshotExtensionVersion);

    std::uint32_t activeCount=0;
    for(const auto& entry:snapshot.runtime){
        if(entry.second.tradeJourney.active) ++activeCount;
    }
    w.u32(activeCount);

    for(const auto& entry:snapshot.runtime){
        const TradeJourneyState& journey=entry.second.tradeJourney;
        if(!journey.active) continue;

        w.u64(entry.first);
        w.u64(journey.partner);
        w.real(journey.utility);
        w.u64(journey.originSettlement);
        w.u64(journey.destinationSettlement);
        w.i32(journey.originPos.x);
        w.i32(journey.originPos.y);
        w.enumeration(journey.firstGives);
        w.enumeration(journey.secondGives);
        w.i32(journey.quantityEach);
        w.i32(journey.legStartedMinute);
        w.boolean(journey.returning);
        w.boolean(journey.exchanged);
    }
}

template<typename ReaderT>
bool readTradeJourneySnapshotExtension(
    ReaderT& r,
    SimulationStateSnapshot& snapshot)
{
    char magic[sizeof(TradeJourneySnapshotExtensionMagic)]{};
    std::uint32_t version=0;
    std::uint32_t count=0;
    if(!r.raw(magic,sizeof(magic))
       || !std::equal(
           magic,
           magic+sizeof(magic),
           TradeJourneySnapshotExtensionMagic)
       || !r.u32(version)
       || version!=TradeJourneySnapshotExtensionVersion
       || !r.count(count)){
        return false;
    }

    for(auto& entry:snapshot.runtime){
        entry.second.tradeJourney.clear();
    }

    std::unordered_set<CharacterId> seen;
    for(std::uint32_t i=0;i<count;++i){
        CharacterId actor=0;
        TradeJourneyState journey;
        journey.active=true;
        if(!r.u64(actor)
           || actor==0
           || !seen.insert(actor).second
           || !r.u64(journey.partner)
           || !r.real(journey.utility)
           || !r.u64(journey.originSettlement)
           || !r.u64(journey.destinationSettlement)
           || !r.i32(journey.originPos.x)
           || !r.i32(journey.originPos.y)
           || !r.enumeration(journey.firstGives)
           || !r.enumeration(journey.secondGives)
           || !r.i32(journey.quantityEach)
           || !r.i32(journey.legStartedMinute)
           || !r.boolean(journey.returning)
           || !r.boolean(journey.exchanged)
           || !validTradeJourneyState(journey)){
            return false;
        }

        auto runtime=snapshot.runtime.find(actor);
        if(runtime==snapshot.runtime.end()) return false;
        runtime->second.tradeJourney=journey;
    }
    return true;
}

} // namespace lifelens
