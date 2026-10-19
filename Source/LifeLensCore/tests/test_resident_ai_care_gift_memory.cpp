#include <algorithm>
#include <cassert>
#include <string>

#include "lifelens/CognitiveDecisionJournal.h"
#include "lifelens/ResidentInjuryProvisionCare.h"

using namespace lifelens;

static Character makeResident(CharacterId id)
{
    Character c;
    c.id=id;
    c.alive=true;
    c.lifeStage=LifeStage::Adult;
    c.civilization.character=id;
    c.needs={0.10,0.10,0.10,0.10,0.10};
    return c;
}

static CognitiveRequest observe(const Character& resident,int minute,
                                std::size_t capacity)
{
    RelationshipBook relationships;
    return buildCognitiveRequest(
        resident,relationships,minute,CognitiveTriggerKind::Reflection,
        {CognitiveIntentKind::CooperateWithResident},capacity,8,8);
}

static void addDistractions(Character& resident)
{
    MemoryRecord earlier;
    earlier.who=resident.id;
    earlier.sourceCharacter=resident.id;
    earlier.minute=1170;
    earlier.what="old firsthand gather success";
    earlier.source=MemorySource::DirectWitness;
    earlier.witnessed=true;
    earlier.importance=1.0;
    earlier.confidence=1.0;
    earlier.emotionIntensity=1.0;
    earlier.tags={"action_experience","Civilization:Gather:PlantFood","success"};
    resident.memory.add(earlier);
    for(int i=0;i<7;++i){
        MemoryRecord vivid;
        vivid.who=resident.id;
        vivid.minute=1199;
        vivid.what="vivid unrelated event "+std::to_string(i);
        vivid.importance=0.90;
        vivid.confidence=0.92;
        vivid.emotionIntensity=0.85;
        resident.memory.add(vivid);
    }
}

static void verifyGiftContext(Character& actor,const char* role)
{
    const auto first=observe(actor,1200,8);
    assert(first.memories.size()==8);
    bool giftVisible=false,oldVisible=false;
    for(const auto& m:first.memories){
        if(m.tags.size()==3 &&m.tags[0]=="injury_care_provision"){
            giftVisible=true;
            assert(m.tags[1]=="PlantFood");
            assert(m.tags[2]==role);
            assert(m.who==actor.id);
            assert(m.source==MemorySource::DirectWitness);
            assert(m.witnessed);
            assert(m.minute==1200);
        }
        if(m.what=="old firsthand gather success")
            oldVisible=true;
    }
    assert(giftVisible &&oldVisible);

    const auto hash=cognitiveRequestFingerprint(first);
    std::reverse(actor.memory.entries.begin(),actor.memory.entries.end());
    assert(cognitiveRequestFingerprint(observe(actor,1200,8))==hash);
    const auto limited=observe(actor,1200,1);
    assert(limited.memories.size()==1);
    assert(limited.memories.front().tags[0]=="injury_care_provision");

    // Another person's report of giving food is NOT my direct experience;
    // a newer rumour must not displace my real own gift.
    MemoryRecord hearsay;
    hearsay.who=actor.id;
    hearsay.sourceCharacter=999;
    hearsay.minute=1201;
    hearsay.what="someone claimed a care gift happened";
    hearsay.source=MemorySource::ToldByOther;
    hearsay.witnessed=false;
    hearsay.confidence=0.01;
    hearsay.importance=0.01;
    hearsay.emotionIntensity=0.0;
    hearsay.tags={"injury_care_provision","PlantFood","given"};
    actor.memory.add(hearsay);
    const auto rumor=observe(actor,1201,1);
    assert(rumor.memories.size()==1);
    assert(rumor.memories.front().source==MemorySource::DirectWitness);
    assert(rumor.memories.front().tags[2]==role);

    assert(observe(actor,1200,0).memories.empty());
}

int main()
{
    Character helper=makeResident(21);
    Character injured=makeResident(22);
    injured.health.injurySeverity=0.65;
    injured.needs.hunger=0.83;
    helper.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::PlantFood,3,0.7,0.9});
    addDistractions(helper);
    addDistractions(injured);

    const auto gift=provideInjuryRecoveryProvisions(helper,injured,1200);
    assert(gift==InjuryProvisionGift::PlantFood);
    assert(helper.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)==2);
    assert(injured.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)==1);
    assert(injured.needs.hunger==0.83);
    assert(injured.health.injurySeverity==0.65);

    // The two separate firsthand perspectives both reach local cognition.
    verifyGiftContext(helper,"given");
    verifyGiftContext(injured,"received");
    return 0;
}
