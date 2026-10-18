#include <algorithm>
#include <cassert>
#include <string>

#include "lifelens/CognitiveDecisionJournal.h"

using namespace lifelens;

int main()
{
    Character actor;
    actor.id=19;
    actor.alive=true;
    RelationshipBook relationships;
    constexpr int minute=1200;

    MemoryRecord oldDirect;
    oldDirect.who=actor.id;
    oldDirect.sourceCharacter=actor.id;
    oldDirect.minute=1170;
    oldDirect.what="previous actual harvest";
    oldDirect.importance=1.0;
    oldDirect.confidence=1.0;
    oldDirect.emotionIntensity=1.0;
    oldDirect.source=MemorySource::DirectWitness;
    oldDirect.witnessed=true;
    oldDirect.tags={"action_experience","Civilization:Gather:PlantFood","success"};
    actor.memory.add(oldDirect);
    for(int i=0;i<7;++i){
        MemoryRecord vivid;
        vivid.who=actor.id;
        vivid.minute=1198;
        vivid.what="unrelated vivid experience "+std::to_string(i);
        vivid.importance=0.85;
        vivid.confidence=0.90;
        vivid.emotionIntensity=0.85;
        actor.memory.add(vivid);
    }
    MemoryRecord sighting;
    sighting.who=actor.id;
    sighting.sourceCharacter=actor.id;
    sighting.minute=1199;
    sighting.what="close wild tiger actually witnessed";
    sighting.importance=0.01;
    sighting.confidence=0.04;
    sighting.emotionIntensity=0.01;
    sighting.source=MemorySource::DirectWitness;
    sighting.witnessed=true;
    sighting.tags={"predator_encounter","432"};
    actor.memory.add(sighting);

    auto observe=[&](const Character& person,int atMinute,std::size_t count){
        return buildCognitiveRequest(
            person,relationships,atMinute,CognitiveTriggerKind::Reflection,
            {CognitiveIntentKind::ImproveShelter,
             CognitiveIntentKind::ExploreOpportunity},
            count,8,8);
    };
    const CognitiveRequest first=observe(actor,minute,8);
    assert(first.memories.size()==8);
    bool noticed=false,oldSeen=false;
    for(const auto& m:first.memories){
        if(m.what==sighting.what){
            noticed=true;
            assert(m.source==MemorySource::DirectWitness &&m.witnessed);
        }
        if(m.what==oldDirect.what) oldSeen=true;
    }
    assert(noticed &&oldSeen);
    const auto fingerprint=cognitiveRequestFingerprint(first);
    std::reverse(actor.memory.entries.begin(),actor.memory.entries.end());
    assert(cognitiveRequestFingerprint(observe(actor,minute,8))==fingerprint);

    // A firsthand injury after seeing the animal supersedes the mere sighting
    // when the cognition budget can expose only ONE recent risk event.
    MemoryRecord injury=sighting;
    injury.minute=1200;
    injury.what="directly injured by a wild tiger";
    injury.tags={"predator_contact_injury","432"};
    actor.memory.add(injury);
    const auto newer=observe(actor,minute,8);
    bool injuredEvidence=false;
    for(const auto& m:newer.memories)
        if(m.what==injury.what &&m.witnessed) injuredEvidence=true;
    assert(injuredEvidence);
    const auto single=observe(actor,minute,1);
    assert(single.memories.size()==1);
    assert(single.memories.front().what==injury.what);

    // Hearing a story, using inferred provenance, or a later narrated tiger
    // must never replace the resident's actually witnessed injury.
    MemoryRecord hearsay=injury;
    hearsay.minute=1201;
    hearsay.what="somebody said a tiger attacked";
    hearsay.source=MemorySource::ToldByOther;
    hearsay.witnessed=false;
    actor.memory.add(hearsay);
    MemoryRecord inferred=hearsay;
    inferred.minute=1202;
    inferred.what="imagined predator risk";
    inferred.source=MemorySource::Inferred;
    actor.memory.add(inferred);
    const auto withoutForged=observe(actor,1202,1);
    assert(withoutForged.memories.size()==1);
    assert(withoutForged.memories.front().what==injury.what);

    // The special recency guarantee expires after one day, and never
    // overrides the hard bounds or inserts fictional new evidence.
    const auto expired=observe(actor,2642,8);
    for(const auto& m:expired.memories)
        assert(m.what!=injury.what);
    assert(observe(actor,minute,0).memories.empty());
    return 0;
}
