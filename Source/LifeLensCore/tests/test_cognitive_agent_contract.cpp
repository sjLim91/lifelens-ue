#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "lifelens/CognitiveAgent.h"
#include "lifelens/ActionExperienceLearning.h"

using namespace lifelens;

int main()
{
    World world(712345);

    Character actor;
    actor.id=1;
    actor.name="Actor";
    actor.alive=true;
    actor.lifeStage=LifeStage::Adult;
    actor.needs.hunger=0.48;
    actor.needs.thirst=0.31;
    actor.personality.conscientiousness=0.87;
    actor.personality.curiosity=0.22;
    actor.emotion.anxiety=0.45;
    actor.emotion.refreshSummary();

    Character ally;
    ally.id=2;
    ally.name="Ally";
    ally.alive=true;
    ally.lifeStage=LifeStage::Adult;

    Character dead;
    dead.id=3;
    dead.name="Dead";
    dead.alive=false;
    dead.lifeStage=LifeStage::Adult;

    world.characters={actor,ally,dead};
    Character& liveActor=world.characters[0];

    MemoryRecord routine;
    routine.who=2;
    routine.what="shared ordinary meal";
    routine.where="camp";
    routine.minute=900;
    routine.importance=0.20;
    routine.confidence=0.9;
    routine.tags={"food"};
    liveActor.memory.add(routine);

    MemoryRecord shortage;
    shortage.what="food stores nearly failed";
    shortage.where="camp";
    shortage.source=MemorySource::ToldByOther;
    shortage.witnessed=false;
    shortage.minute=980;
    shortage.importance=0.95;
    shortage.confidence=1.0;
    shortage.emotionValence=-0.8;
    shortage.emotionIntensity=0.9;
    shortage.tags={"food","scarcity"};
    liveActor.memory.add(shortage);

    MemoryRecord stale;
    stale.who=2;
    stale.what="old disagreement";
    stale.where="camp";
    stale.minute=10;
    stale.importance=0.45;
    stale.confidence=0.5;
    stale.tags={"social"};
    liveActor.memory.add(stale);

    BeliefRecord& reliable=
        liveActor.beliefs.getOrCreate(2,"helps with difficult work");
    reliable.applyEvidence(true,2.0,990);
    BeliefRecord& weak=
        liveActor.beliefs.getOrCreate(2,"likes distant exploration");
    weak.applyEvidence(true,0.1,995);

    RelationshipBook relationships;
    Relationship& relation=relationships.getOrCreate(1,2);
    relation.affection=0.8;
    relation.trust=0.9;
    relation.respect=0.7;
    relation.familiarity=0.8;

    const std::vector<CognitiveIntentKind> allowed={
        CognitiveIntentKind::ImproveFoodSecurity,
        CognitiveIntentKind::ExpandCultivation,
        CognitiveIntentKind::CooperateWithResident
    };
    const CognitiveRequest request=buildCognitiveRequest(
        liveActor,relationships,1000,
        CognitiveTriggerKind::ResourceScarcity,
        allowed,2,1,1);

    assert(request.actor==1);
    assert(request.trigger==CognitiveTriggerKind::ResourceScarcity);
    assert(request.memories.size()==2);
    assert(request.memories.front().what=="food stores nearly failed");
    assert(request.memories.front().source==MemorySource::ToldByOther);
    assert(!request.memories.front().witnessed);
    assert(request.memories.back().source==MemorySource::DirectWitness);
    assert(request.memories.back().witnessed);
    assert(request.beliefs.size()==1);
    assert(request.beliefs.front().proposition=="helps with difficult work");
    assert(request.relationships.size()==1);
    assert(request.relationships.front().target==2);

    CognitiveProposal accepted;
    accepted.actor=1;
    accepted.intent=CognitiveIntentKind::CooperateWithResident;
    accepted.priority=0.82;
    accepted.targetResident=2;
    accepted.rationale="Food risk is important and this resident is trusted.";
    auto validation=validateCognitiveProposal(world,request,accepted);
    assert(validation.accepted);
    assert(validation.issue==CognitiveValidationIssue::None);

    CognitiveProposal invented=accepted;
    invented.intent=CognitiveIntentKind::MigrateHousehold;
    invented.targetResident=0;
    validation=validateCognitiveProposal(world,request,invented);
    assert(!validation.accepted);
    assert(validation.issue==CognitiveValidationIssue::IntentNotAllowed);

    CognitiveProposal missingTarget=accepted;
    missingTarget.targetResident=0;
    validation=validateCognitiveProposal(world,request,missingTarget);
    assert(!validation.accepted);
    assert(validation.issue==CognitiveValidationIssue::MissingTarget);

    CognitiveProposal deadTarget=accepted;
    deadTarget.targetResident=3;
    validation=validateCognitiveProposal(world,request,deadTarget);
    assert(!validation.accepted);
    assert(validation.issue==CognitiveValidationIssue::InvalidTarget);

    CognitiveProposal badPriority=accepted;
    badPriority.priority=std::nan("");
    validation=validateCognitiveProposal(world,request,badPriority);
    assert(!validation.accepted);
    assert(validation.issue==CognitiveValidationIssue::InvalidPriority);

    CognitiveProposal strayTarget;
    strayTarget.actor=1;
    strayTarget.intent=CognitiveIntentKind::ImproveFoodSecurity;
    strayTarget.priority=0.7;
    strayTarget.targetResident=2;
    validation=validateCognitiveProposal(world,request,strayTarget);
    assert(!validation.accepted);
    assert(validation.issue==CognitiveValidationIssue::UnexpectedTarget);

    CognitiveRequest noTrigger=request;
    noTrigger.trigger=CognitiveTriggerKind::None;
    CognitiveProposal food;
    food.actor=1;
    food.intent=CognitiveIntentKind::ImproveFoodSecurity;
    food.priority=0.75;
    validation=validateCognitiveProposal(world,noTrigger,food);
    assert(!validation.accepted);
    assert(validation.issue==CognitiveValidationIssue::MissingTrigger);

    // A large batch of unrelated salient recollections cannot starve one
    // RECENT firsthand action outcome from the resident's next LLM context.
    // The maximum context size and ordinary recall ranking are preserved.
    Character recalled=liveActor;
    recalled.memory.entries.clear();
    for(int i=0;i<12;++i){
        MemoryRecord episode;
        episode.what="unrelated powerful memory "+std::to_string(i);
        episode.minute=1000;
        episode.importance=0.98;
        episode.confidence=1.0;
        episode.source=MemorySource::DirectWitness;
        episode.witnessed=true;
        recalled.memory.add(episode);
    }
    rememberActionOutcome(recalled,"Civilization:Gather:PlantFood",999,false);
    const MemoryRecord* personalAction=nullptr;
    for(const auto& record:recalled.memory.entries)
        if(record.tags.size()==3
           &&record.tags[0]=="action_experience")
            personalAction=&record;
    assert(personalAction!=nullptr);
    int higherRankedEpisodes=0;
    for(const auto& record:recalled.memory.entries)
        if(record.what.find("unrelated powerful memory")==0
           &&record.recallScore(1000)>personalAction->recallScore(1000))
            ++higherRankedEpisodes;
    assert(higherRankedEpisodes>=4); // normally omitted without one reserved slot
    const auto learned=buildCognitiveRequest(
        recalled,relationships,1000,CognitiveTriggerKind::Reflection,
        allowed,4,1,1);
    assert(learned.memories.size()==4);
    bool foundPersonalExperience=false;
    for(const auto& evidence:learned.memories){
        if(evidence.source==MemorySource::DirectWitness
           &&evidence.witnessed
           &&evidence.tags.size()==3
           &&evidence.tags[0]=="action_experience"
           &&evidence.tags[1]=="Civilization:Gather:PlantFood"
           &&evidence.tags[2]=="failure")
            foundPersonalExperience=true;
    }
    assert(foundPersonalExperience);

    // Reported/hypothetical outcomes never qualify for the reserved slot.
    Character hearsay=recalled;
    for(auto& entry:hearsay.memory.entries)
        if(entry.tags.size()==3
           &&entry.tags[0]=="action_experience"){
            entry.source=MemorySource::ToldByOther;
            entry.witnessed=false;
        }
    const auto untrusted=buildCognitiveRequest(
        hearsay,relationships,1000,CognitiveTriggerKind::Reflection,
        allowed,4,1,1);
    for(const auto& evidence:untrusted.memories)
        assert(evidence.tags.empty() ||evidence.tags[0]!="action_experience");

    // Zero-sized requests remain empty; no slot allocation side effect.
    const auto zero=buildCognitiveRequest(
        recalled,relationships,1000,CognitiveTriggerKind::Reflection,
        allowed,0,1,1);
    assert(zero.memories.empty());

    std::cout<<"cognitive agent read-only context + fail-closed proposal contract passed\n";
    return 0;
}
