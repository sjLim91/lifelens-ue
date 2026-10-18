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
        vivid.importance=1.0;
        vivid.confidence=1.0;
        vivid.emotionIntensity=1.0;
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
    return 0;
}
