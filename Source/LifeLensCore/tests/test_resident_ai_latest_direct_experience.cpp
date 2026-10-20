#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

#include "lifelens/CognitiveDecisionJournal.h"

using namespace lifelens;

int main()
{
    Character actor;
    actor.id=19;
    actor.alive=true;
    RelationshipBook relationships;
    constexpr int now=1200;

    // High recall old success was already in the first eight. Seven vivid
    // incidental memories suppress the newest low-salience firsthand
    // resource failure under the original "any firsthand" selection.
    MemoryRecord earlierSuccess;
    earlierSuccess.who=19;
    earlierSuccess.sourceCharacter=19;
    earlierSuccess.minute=1180;
    earlierSuccess.what="old-personal-success";
    earlierSuccess.importance=1.0;
    earlierSuccess.confidence=1.0;
    earlierSuccess.emotionIntensity=1.0;
    earlierSuccess.source=MemorySource::DirectWitness;
    earlierSuccess.witnessed=true;
    earlierSuccess.tags={
        "action_experience","Civilization:Gather:PlantFood","success"
    };
    actor.memory.add(earlierSuccess);

    for(int i=0;i<7;++i){
        MemoryRecord vivid;
        vivid.who=19;
        vivid.minute=1199;
        vivid.what="vivid-unrelated-"+std::to_string(i);
        // Salient enough to dominate the new low-salience failure, but
        // not so strong that it displaces the older maximal-recall success.
        vivid.importance=0.85;
        vivid.confidence=0.90;
        vivid.emotionIntensity=0.85;
        vivid.tags={"ordinary"};
        actor.memory.add(vivid);
    }

    MemoryRecord newFailure;
    newFailure.who=19;
    newFailure.sourceCharacter=19;
    newFailure.minute=1200;
    newFailure.what="new-personal-source-exhausted";
    newFailure.importance=0.01;
    newFailure.confidence=0.05;
    newFailure.emotionIntensity=0.01;
    newFailure.source=MemorySource::DirectWitness;
    newFailure.witnessed=true;
    newFailure.tags={
        "action_experience","Civilization:Gather:PlantFood","failure"
    };
    actor.memory.add(newFailure);

    MemoryRecord newerRumor=newFailure;
    newerRumor.minute=1200;
    newerRumor.what="newer-hearsay-source-exhausted";
    newerRumor.source=MemorySource::ToldByOther;
    newerRumor.witnessed=false;
    newerRumor.importance=0.0;
    newerRumor.confidence=0.01;
    actor.memory.add(newerRumor);

    auto observe=[&](const Character& person,int minute,int limit){
        return buildCognitiveRequest(
            person,relationships,minute,CognitiveTriggerKind::Reflection,
            {CognitiveIntentKind::ImproveFoodSecurity},
            static_cast<std::size_t>(limit),8,8);
    };
    const CognitiveRequest before=observe(actor,now,8);
    assert(before.memories.size()==8);
    bool oldSeen=false,newSeen=false,rumorSeen=false;
    for(const auto& memory:before.memories){
        if(memory.what==earlierSuccess.what) oldSeen=true;
        if(memory.what==newFailure.what){
            newSeen=true;
            assert(memory.source==MemorySource::DirectWitness);
            assert(memory.witnessed);
            assert(memory.minute==now);
        }
        if(memory.what==newerRumor.what) rumorSeen=true;
    }
    assert(oldSeen &&newSeen &&!rumorSeen);

    const auto fingerprint=cognitiveRequestFingerprint(before);
    const CognitiveRequest repeat=observe(actor,now,8);
    assert(cognitiveRequestFingerprint(repeat)==fingerprint);

    // Input storage order must not influence the selection or fingerprint.
    std::reverse(actor.memory.entries.begin(),actor.memory.entries.end());
    const auto reversed=observe(actor,now,8);
    assert(cognitiveRequestFingerprint(reversed)==fingerprint);

    // One record remains bounded in very small contexts too.
    const auto single=observe(actor,now,1);
    assert(single.memories.size()==1);
    assert(single.memories.front().what==newFailure.what);

    // If later REAL success exists it is the newest evidence, replacing
    // the previous failed-outcome preference without forging a conclusion.
    MemoryRecord laterSuccess=newFailure;
    laterSuccess.minute=1201;
    laterSuccess.what="new-personal-recovery";
    laterSuccess.tags[2]="success";
    actor.memory.add(laterSuccess);
    const auto recovered=observe(actor,1201,8);
    bool recoveredPresent=false;
    for(const auto& memory:recovered.memories)
        if(memory.what==laterSuccess.what) recoveredPresent=true;
    assert(recoveredPresent);

    // Missing capacity must not cause implicit memory allocation.
    const auto zero=observe(actor,1201,0);
    assert(zero.memories.empty());

    // The new preference is bounded to the last day. An expired low-salience
    // memory is not entitled to displace unrelated high-recall records.
    Character expired=actor;
    expired.memory.entries.erase(
        std::remove_if(expired.memory.entries.begin(),
                       expired.memory.entries.end(),
                       [](const MemoryRecord& m){
                           return m.what=="new-personal-recovery";
                       }),
        expired.memory.entries.end());
    const auto oldContext=observe(expired,now+1441,8);
    for(const auto& memory:oldContext.memories)
        assert(memory.what!=newFailure.what);

    // Repeated ACTUAL trade failures have the partner in "who", not the
    // actor. In a crowded mind, a single fresh failure would be insufficient
    // to trigger the next locally reasoned alternative. Keep the latest TWO
    // firsthand attempts with this same real partner in the eight slots.
    Character trader;
    trader.id=19;
    trader.alive=true;
    for(int i=0;i<12;++i){
        MemoryRecord distraction;
        distraction.who=19;
        distraction.minute=1199;
        distraction.what="vivid distraction "+std::to_string(i);
        distraction.importance=0.95;
        distraction.confidence=1.0;
        distraction.emotionIntensity=0.95;
        distraction.tags={"unrelated"};
        trader.memory.add(distraction);
    }
    MemoryRecord failedTrade;
    failedTrade.who=23; // the Core-verified counterpart, not self
    failedTrade.sourceCharacter=19;
    failedTrade.minute=1198;
    failedTrade.what="actual-trade-failed-1";
    failedTrade.where="core-context-token:701";
    failedTrade.source=MemorySource::DirectWitness;
    failedTrade.witnessed=true;
    failedTrade.importance=0.01;
    failedTrade.confidence=0.05;
    failedTrade.emotionIntensity=0.01;
    failedTrade.tags={"action_experience","CoreContext:Trade","failure"};
    trader.memory.add(failedTrade);
    failedTrade.minute=1199;
    failedTrade.what="actual-trade-failed-2";
    failedTrade.where="core-context-token:702";
    trader.memory.add(failedTrade);
    auto observeTrade=[&](const Character& candidate){
        return buildCognitiveRequest(
            candidate,relationships,1200,CognitiveTriggerKind::Reflection,
            {CognitiveIntentKind::TradeWithResident},8,8,8);
    };
    auto countTradeAttempts=[](const CognitiveRequest& request){
        int count=0;
        for(const auto& memory:request.memories)
            if(memory.tags.size()==3 &&memory.who==23
               &&memory.tags[1]=="CoreContext:Trade"
               &&memory.tags[2]=="failure") ++count;
        return count;
    };
    const auto direct=observeTrade(trader);
    assert(direct.memories.size()==8);
    assert(countTradeAttempts(direct)==2);
    const auto directFingerprint=cognitiveRequestFingerprint(direct);
    std::reverse(trader.memory.entries.begin(),trader.memory.entries.end());
    const auto reversedTrade=observeTrade(trader);
    assert(countTradeAttempts(reversedTrade)==2);
    assert(cognitiveRequestFingerprint(reversedTrade)==directFingerprint);

    // An unrelated resident's actions cannot become one's own failures,
    // even if a naive client labels the record as a direct witness.
    Character borrowedTrade=trader;
    for(auto& memory:borrowedTrade.memory.entries)
        if(memory.what=="actual-trade-failed-1")
            memory.sourceCharacter=99;
    assert(countTradeAttempts(observeTrade(borrowedTrade))==1);

    // Two separate partners are NOT two attempts at the same partner.
    Character twoPartners=trader;
    for(auto& memory:twoPartners.memory.entries)
        if(memory.what=="actual-trade-failed-1") memory.who=24;
    assert(countTradeAttempts(observeTrade(twoPartners))==1);

    // A fabricated token or a reported failure has no right to displace
    // vivid memories for a misleading two-failure reflection.
    Character fakeToken=trader;
    for(auto& memory:fakeToken.memory.entries)
        if(memory.what=="actual-trade-failed-1") memory.where="model-claimed:701";
    assert(countTradeAttempts(observeTrade(fakeToken))==1);
    Character rumorTrade=trader;
    for(auto& memory:rumorTrade.memory.entries)
        if(memory.what=="actual-trade-failed-1"){
            memory.source=MemorySource::ToldByOther;
            memory.witnessed=false;
        }
    assert(countTradeAttempts(observeTrade(rumorTrade))==1);

    return 0;
}
