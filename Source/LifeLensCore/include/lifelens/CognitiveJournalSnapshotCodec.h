#pragma once
#include <algorithm>
#include <cstdint>
#include "SimulationSnapshot.h"

namespace lifelens {
inline constexpr char CognitiveJournalSnapshotExtensionMagic[]=
    {'L','L','C','G','J','N','0','1'};
inline constexpr std::uint32_t CognitiveJournalSnapshotExtensionVersion=1;

template<typename W>
void writeCognitiveJournalSnapshotExtension(
    W& w,const SimulationStateSnapshot& snapshot)
{
    w.raw(CognitiveJournalSnapshotExtensionMagic,
          sizeof(CognitiveJournalSnapshotExtensionMagic));
    const bool observed=std::any_of(
        snapshot.acceptedCognitiveDecisions.begin(),
        snapshot.acceptedCognitiveDecisions.end(),
        [](const AcceptedCognitiveDecision& e){return e.observedMinute>=0;});
    w.u32(observed ? 2 : CognitiveJournalSnapshotExtensionVersion);
    w.u32(static_cast<std::uint32_t>(snapshot.acceptedCognitiveDecisions.size()));
    for(const auto& e:snapshot.acceptedCognitiveDecisions){
        w.u64(e.actor);
        w.i32(e.minute);
        w.enumeration(e.trigger);
        w.u64(e.contextFingerprint);
        w.string(e.backendIdentity);
        w.enumeration(e.intent);
        w.real(e.priority);
        w.u64(e.target);
        w.string(e.rationaleDigest);
        if(observed){
            w.i32(e.observedMinute);
            w.u64(e.observedContextFingerprint);
        }
    }
}

template<typename R>
bool readCognitiveJournalSnapshotExtension(
    R& r,SimulationStateSnapshot& snapshot)
{
    char magic[sizeof(CognitiveJournalSnapshotExtensionMagic)]{};
    std::uint32_t version=0,count=0;
    if(!r.raw(magic,sizeof(magic))
       || !std::equal(magic,magic+sizeof(magic),
                      CognitiveJournalSnapshotExtensionMagic)
       || !r.u32(version)
       || (version!=CognitiveJournalSnapshotExtensionVersion &&version!=2)
       || !r.count(count)
       || count>MaxAcceptedCognitiveDecisions)
        return false;
    std::vector<AcceptedCognitiveDecision> events;
    events.reserve(count);
    for(std::uint32_t i=0;i<count;++i){
        AcceptedCognitiveDecision e;
        if(!r.u64(e.actor)
           || !r.i32(e.minute)
           || !r.enumeration(e.trigger)
           || !r.u64(e.contextFingerprint)
           || !r.string(e.backendIdentity)
           || !r.enumeration(e.intent)
           || !r.real(e.priority)
           || !r.u64(e.target)
           || !r.string(e.rationaleDigest))
            return false;
        if(version==2 &&(!r.i32(e.observedMinute)
           ||!r.u64(e.observedContextFingerprint))) return false;
        events.push_back(std::move(e));
    }
    if(!validCognitiveDecisionJournal(events,snapshot.world)) return false;
    snapshot.acceptedCognitiveDecisions=std::move(events);
    return true;
}

} // namespace lifelens
