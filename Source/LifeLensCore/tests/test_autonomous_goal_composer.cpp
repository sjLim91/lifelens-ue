#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <vector>

#include "lifelens/AutonomousGoalComposer.h"
#include "lifelens/UtilityAI.h"

using namespace lifelens;

static CognitiveRequest requestFor(
    const World& world,
    const RelationshipBook& relationships,
    CharacterId actor)
{
    const Character* character=cognitiveCharacter(world,actor);
    assert(character);
    return buildCognitiveRequest(
        *character,relationships,world.minute,
        CognitiveTriggerKind::Reflection,
        cognitiveStrategicIntentCatalog(),8,8,8);
}

int main()
{
    World world(778899);
    world.characters.clear();

    Character social;
    social.id=11;
    social.needs.hunger=0.38;
    social.needs.thirst=0.51;
    social.personality.sociability=1.0;
    social.personality.empathy=0.9;

    MemoryRecord direct;
    direct.what="saw a depleted nearby water source";
    direct.where="grid:1,1";
    direct.minute=world.minute-10;
    direct.source=MemorySource::DirectWitness;
    direct.witnessed=true;
    direct.confidence=1.0;
    direct.importance=0.9;
    direct.tags={"water","scarcity"};
    social.memory.add(direct);

    Character reserved=social;
    reserved.id=22;
    reserved.personality.sociability=0.0;
    reserved.personality.empathy=0.1;
    reserved.memory.entries.clear();
    direct.source=MemorySource::ToldByOther;
    direct.witnessed=false;
    reserved.memory.add(direct);

    Character acquaintance;
    acquaintance.id=33;
    world.characters={social,reserved,acquaintance};

    RelationshipBook relationships;
    relationships.getOrCreate(11,33).trust=0.10;
    relationships.getOrCreate(22,33).trust=0.10;

    const auto ra=requestFor(world,relationships,11);
    const auto rb=requestFor(world,relationships,22);
    const auto rngBefore=world.rng;
    const auto worldBefore=world.characters;

    const auto socialGoals=composeAutonomousGoalCandidates(world,ra,6);
    const auto reservedGoals=composeAutonomousGoalCandidates(world,rb,6);
    assert(!socialGoals.empty() &&!reservedGoals.empty());
    assert(socialGoals.size()<=6 &&reservedGoals.size()<=6);
    assert(socialGoals[0].desiredState[0].metric
           ==AutonomousGoalMetric::RelationshipTrust);
    assert(reservedGoals[0].desiredState[0].metric
           ==AutonomousGoalMetric::ActorNeed);

    const auto hasComposite=[](const auto& goals){
        return std::any_of(goals.begin(),goals.end(),[](const auto& goal){
            return goal.desiredState.size()>1;
        });
    };
    assert(hasComposite(socialGoals));
    assert(hasComposite(reservedGoals));
    for(const auto& goal:socialGoals){
        assert(validateAutonomousGoalCandidate(world,ra,goal).accepted);
        assert(goal.actor==11);
        assert(goal.createdMinute==world.minute);
        assert(goal.expiresMinute==world.minute+1440);
        assert(goal.priority>=0.0 && goal.priority<=1.0);
    }
    for(const auto& goal:reservedGoals){
        assert(validateAutonomousGoalCandidate(world,rb,goal).accepted);
        assert(goal.actor==22);
    }

    // The same Core context generates exactly the same ordering and values.
    const auto replay=composeAutonomousGoalCandidates(world,ra,6);
    assert(replay.size()==socialGoals.size());
    for(std::size_t i=0;i<replay.size();++i){
        assert(replay[i].actor==socialGoals[i].actor);
        assert(replay[i].priority==socialGoals[i].priority);
        assert(replay[i].desiredState.size()
               ==socialGoals[i].desiredState.size());
        for(std::size_t j=0;j<replay[i].desiredState.size();++j){
            const auto& a=replay[i].desiredState[j];
            const auto& b=socialGoals[i].desiredState[j];
            assert(a.metric==b.metric);
            assert(a.need==b.need);
            assert(a.targetResident==b.targetResident);
            assert(a.threshold==b.threshold);
        }
    }

    // A rumor may be considered, but direct experience has higher personal
    // salience. No source is converted into objective world truth.
    auto reported=ra;
    assert(!reported.memories.empty());
    reported.memories[0].source=MemorySource::ToldByOther;
    reported.memories[0].witnessed=false;
    const auto directGoals=composeAutonomousGoalCandidates(world,ra,6);
    const auto hearsayGoals=composeAutonomousGoalCandidates(world,reported,6);
    const auto waterPriority=[](const auto& goals){
        for(const auto& goal:goals){
            if(goal.desiredState.size()==1
               &&goal.desiredState[0].metric==AutonomousGoalMetric::ActorNeed
               &&goal.desiredState[0].need==AutonomousNeedKind::Thirst)
                return goal.priority;
        }
        return -1.0;
    };
    assert(waterPriority(directGoals)>waterPriority(hearsayGoals));

    // The one resident mind can independently propose a material goal
    // because Core's tiny proximity view contains a REAL nearby node.
    // It cannot invent remote stock, a visual sighting or harvest success.
    auto exploratory=requestFor(world,relationships,33);
    const auto sensor=observeLocalResourceProximity(
        world,33,world.resourceNodes.front().pos,4);
    assert(sensor.actor==33 &&!sensor.nearbyResources.empty());
    exploratory.localProximityObserved=true;
    exploratory.localObserverPosition=sensor.observerPosition;
    exploratory.localRadiusCells=sensor.radiusCells;
    exploratory.localResources=sensor.nearbyResources;
    const auto fromSensor=composeAutonomousGoalCandidates(
        world,exploratory,6);
    const AutonomousGoalCandidate* materialGoal=nullptr;
    for(const auto& item:fromSensor)
        if(item.desiredState.size()==1
           &&item.desiredState[0].metric
               ==AutonomousGoalMetric::ActorRawMaterialCount){
            materialGoal=&item;
            break;
        }
    assert(materialGoal!=nullptr);
    assert(materialGoal->evidence.size()==1);
    assert(materialGoal->evidence[0].kind
        ==AutonomousGoalEvidenceKind::ObservedLocalResource);
    assert(validateAutonomousGoalCandidate(
        world,exploratory,*materialGoal).accepted);
    const auto noSensor=composeAutonomousGoalCandidates(
        world,requestFor(world,relationships,33),6);
    for(const auto& item:noSensor)
        for(const auto& condition:item.desiredState)
            assert(condition.metric
                !=AutonomousGoalMetric::ActorRawMaterialCount);
    auto inventedSensor=exploratory;
    inventedSensor.localResources[materialGoal->evidence[0].index].nodeId
        =99999999;
    assert(validateAutonomousGoalCandidate(
        world,inventedSensor,*materialGoal).issue
            ==AutonomousGoalIssue::InvalidEvidence);
    // A formerly real node disappearing also invalidates the proposal.
    const auto linkedIndex=materialGoal->evidence[0].index;
    const auto nodeId=exploratory.localResources[linkedIndex].nodeId;
    const auto actualNode=std::find_if(
        world.resourceNodes.begin(),world.resourceNodes.end(),
        [&](const ResourceNode& n){return n.id==nodeId;});
    assert(actualNode!=world.resourceNodes.end());
    const auto originalQuantity=actualNode->quantity;
    actualNode->quantity=0;
    assert(validateAutonomousGoalCandidate(
        world,exploratory,*materialGoal).issue
            ==AutonomousGoalIssue::InvalidEvidence);
    actualNode->quantity=originalQuantity;

    // Cognition only proposes: no Need goal weight or simulation truth alters.
    assert(world.rng==rngBefore);
    assert(world.characters.size()==worldBefore.size());
    assert(world.characters[0].needs.thirst==worldBefore[0].needs.thirst);
    assert(world.characters[0].memory.entries.size()
           ==worldBefore[0].memory.entries.size());
    assert(world.characters[0].personality.sociability
           ==worldBefore[0].personality.sociability);
    assert(std::abs(
        scoreGoal(world,world.characters[0],Goal::Drink)
        -scoreGoal(world,worldBefore[0],Goal::Drink))<1e-12);

    assert(composeAutonomousGoalCandidates(world,ra,0).empty());
    assert(composeAutonomousGoalCandidates(world,ra,100).size()<=6);
    auto forged=ra;
    forged.actor=4444;
    assert(composeAutonomousGoalCandidates(world,forged).empty());
    forged=ra;
    forged.minute+=1;
    assert(composeAutonomousGoalCandidates(world,forged).empty());
    forged=ra;
    forged.trigger=CognitiveTriggerKind::None;
    assert(composeAutonomousGoalCandidates(world,forged).empty());

    // A resident without social evidence never gets a ghost target goal.
    auto isolated=ra;
    isolated.relationships.clear();
    const auto solo=composeAutonomousGoalCandidates(world,isolated,6);
    for(const auto& goal:solo){
        for(const auto& predicate:goal.desiredState)
            assert(predicate.metric!=AutonomousGoalMetric::RelationshipTrust);
    }

    // Insensible actor status invalidates even purely diagnostic proposals.
    world.characters[0].alive=false;
    assert(composeAutonomousGoalCandidates(world,ra).empty());
    return 0;
}
