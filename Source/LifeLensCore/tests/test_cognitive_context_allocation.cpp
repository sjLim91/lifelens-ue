#include <cassert>
#include <cstdlib>
#include <iostream>
#include <new>
#include <string>
#include "fixtures/cognitive_request_reference.h"
#include "lifelens/CognitiveDecisionJournal.h"

static thread_local bool measuring=false;
static thread_local std::size_t allocatedBytes=0;
void* operator new(std::size_t bytes){
    if(measuring) allocatedBytes+=bytes;
    if(void* p=std::malloc(bytes ? bytes : 1))return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void* operator new[](std::size_t bytes){return ::operator new(bytes);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}

using namespace lifelens;
int main()
{
    Character actor;
    actor.id=7;
    actor.lifeStage=LifeStage::Adult;
    for(int i=0;i<8000;++i){
        MemoryRecord m;
        m.who=i%3==0 ? actor.id : 9;
        m.sourceCharacter=actor.id;
        m.minute=10000-i%2000;
        m.what=std::string(768,'a'+i%20)+std::to_string(i%97);
        m.where=std::string(128,'x')+std::to_string(i%13);
        m.importance=(i%31)/31.0;
        m.emotionIntensity=(i%17)/17.0;
        m.confidence=(i%23)/23.0;
        m.source=static_cast<MemorySource>(i%3);
        m.witnessed=i%2==0;
        m.tags={"ordinary-event",std::to_string(i%7)};
        actor.memory.add(m);
    }
    for(int minute:{9980,9990}){
        MemoryRecord attempt;
        attempt.who=actor.id;
        attempt.sourceCharacter=actor.id;
        attempt.minute=minute;
        attempt.what="low-salience real failed gather";
        attempt.importance=0.0;
        attempt.confidence=0.01;
        attempt.tags={"action_experience","Civilization:Gather:Wood","failure"};
        actor.memory.add(attempt);
    }
    MemoryRecord fresh;
    fresh.who=actor.id;
    fresh.sourceCharacter=actor.id;
    fresh.minute=9999;
    fresh.what="latest actual aid received";
    fresh.importance=0.0;
    fresh.confidence=0.01;
    fresh.tags={"injury_care_provision"};
    actor.memory.add(fresh);
    const auto count=actor.memory.entries.size();
    const auto firstText=actor.memory.entries.front().what;
    RelationshipBook relationships;
    for(const std::size_t limit:{0,1,2,3,8,32,100}){
        const auto reference=referenceBuildCognitiveRequest(
            actor,relationships,10000,CognitiveTriggerKind::Reflection,
            cognitiveStrategicIntentCatalog(),limit,8,8);
        const auto request=buildCognitiveRequest(
            actor,relationships,10000,CognitiveTriggerKind::Reflection,
            cognitiveStrategicIntentCatalog(),limit,8,8);
        assert(request.memories.size()<=limit);
        assert(cognitiveRequestFingerprint(request)==
               cognitiveRequestFingerprint(reference));
    }

    allocatedBytes=0;measuring=true;
    const auto reference=referenceBuildCognitiveRequest(
        actor,relationships,10000,CognitiveTriggerKind::Reflection,
        cognitiveStrategicIntentCatalog(),8,8,8);
    measuring=false;
    const auto referenceBytes=allocatedBytes;
    allocatedBytes=0;measuring=true;
    const auto request=buildCognitiveRequest(
        actor,relationships,10000,CognitiveTriggerKind::Reflection,
        cognitiveStrategicIntentCatalog(),8,8,8);
    measuring=false;
    const auto candidateBytes=allocatedBytes;
    assert(cognitiveRequestFingerprint(request)==
           cognitiveRequestFingerprint(reference));
    assert(candidateBytes*2<referenceBytes);
    assert(actor.memory.entries.size()==count);
    assert(actor.memory.entries.front().what==firstText);
    const auto retainedFingerprint=cognitiveRequestFingerprint(request);
    actor.memory.entries.clear();
    assert(cognitiveRequestFingerprint(request)==retainedFingerprint);
    std::cout<<"context allocation baseline="<<referenceBytes
             <<" candidate="<<candidateBytes<<" bytes; fingerprints equal\n";
}
