#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "lifelens/CognitiveAgent.h"

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

    std::cout<<"cognitive agent read-only context + fail-closed proposal contract passed\n";
    return 0;
}
