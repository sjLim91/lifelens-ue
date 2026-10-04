#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "SimulationSnapshot.h"

namespace lifelens {

constexpr char TradeJourneySnapshotExtensionMagic[]={
    'L','L','T','R','D','J','0','1'
};
constexpr std::uint32_t TradeJourneySnapshotExtensionVersion=1;

template<typename WriterT>
void writeTradeJourneyState(
    WriterT& w,
    const TradeJourneyState& journey)
{
    w.boolean(journey.active);
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

template<typename ReaderT>
bool readTradeJourneyState(
    ReaderT& r,
    TradeJourneyState& journey)
{
    if(!r.boolean(journey.active)
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
       || !r.boolean(journey.exchanged)){
        return false;
    }
    return journey.active && validTradeJourneyState(journey);
}

template<typename WriterT>
void writeTradeJourneySnapshotExtension(
    WriterT& w,
    const std::unordered_map<CharacterId,SimulationRuntimeSnapshot>& runtime)
{
    w.raw(
        TradeJourneySnapshotExtensionMagic,
        sizeof(TradeJourneySnapshotExtensionMagic));
    w.u32(TradeJourneySnapshotExtensionVersion);

    std::vector<CharacterId> active;
    active.reserve(runtime.size());
    for(const auto& entry:runtime){
        if(entry.second.tradeJourney.active) active.push_back(entry.first);
    }
    std::sort(active.begin(),active.end());

    w.u32(static_cast<std::uint32_t>(active.size()));
    for(CharacterId id:active){
        w.u64(id);
        writeTradeJourneyState(w,runtime.at(id).tradeJourney);
    }
}

template<typename ReaderT>
bool readTradeJourneySnapshotExtension(
    ReaderT& r,
    std::unordered_map<CharacterId,SimulationRuntimeSnapshot>& runtime)
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

    for(auto& entry:runtime) entry.second.tradeJourney.clear();

    std::vector<CharacterId> seen;
    seen.reserve(count);
    for(std::uint32_t i=0;i<count;++i){
        CharacterId id=0;
        TradeJourneyState journey;
        if(!r.u64(id)
           || id==0
           || std::find(seen.begin(),seen.end(),id)!=seen.end()
           || !readTradeJourneyState(r,journey)){
            return false;
        }
        auto it=runtime.find(id);
        if(it==runtime.end()) return false;
        it->second.tradeJourney=journey;
        seen.push_back(id);
    }
    return true;
}

} // namespace lifelens
