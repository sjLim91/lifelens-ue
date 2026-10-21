#include <algorithm>
#include <cassert>
#include <vector>

#include "lifelens/AutonomousExperimentPlanner.h"
#include "lifelens/Simulation.h"

using namespace lifelens;

static CognitiveRequest currentView(
    const World& world,const RelationshipBook& relationships,
    const Character& actor,GridPos position)
{
    auto request=buildCognitiveRequest(
        actor,relationships,world.minute,CognitiveTriggerKind::Reflection,
        cognitiveStrategicIntentCatalog(),
        DefaultCognitiveMemoryContextLimit,
        DefaultCognitiveBeliefContextLimit,
        DefaultCognitiveRelationshipContextLimit);
    const auto sensed=observeLocalResourceProximity(
        world,actor.id,position,3);
    assert(sensed.actor==actor.id);
    request.localProximityObserved=true;
    request.localObserverPosition=sensed.observerPosition;
    request.localRadiusCells=sensed.radiusCells;
    request.localResources=sensed.nearbyResources;
    return request;
}

static AutonomousGoalCandidate twoMaterialGoal(
    const CognitiveRequest& request,int originalExpiration)
{
    AutonomousGoalCandidate goal;
    goal.actor=request.actor;
    goal.createdMinute=request.minute;
    goal.expiresMinute=originalExpiration;
    goal.priority=0.65;
    for(MaterialKind kind:{MaterialKind::Stone,MaterialKind::Wood}){
        const auto it=std::find_if(
            request.localResources.begin(),request.localResources.end(),
            [kind](const ObservedLocalResource& found){
                return found.material==kind;
            });
        assert(it!=request.localResources.end());
        AutonomousGoalPredicate need;
        need.metric=AutonomousGoalMetric::ActorRawMaterialCount;
        need.material=kind;
        need.comparison=AutonomousGoalCompare::AtLeast;
        need.threshold=1.0;
        goal.desiredState.push_back(need);
        goal.evidence.push_back({
            AutonomousGoalEvidenceKind::ObservedLocalResource,
            static_cast<std::size_t>(
                it-request.localResources.begin())
        });
    }
    return goal;
}

int main()
{
    World world(73144,0,1);
    world.minute=480;
    world.storageSites.clear();
    world.facilities.clear();
    Character actor;
    actor.id=11;
    actor.lifeStage=LifeStage::Adult;
    actor.needs={0.05,0.05,0.05,0.05,0.05};
    actor.civilization.character=11;
    Character peer=actor;
    peer.id=22;
    peer.civilization.character=22;
    world.characters={actor,peer};
    const auto start=world.initialStartRegion().region.coord;
    const auto origin=chunkOriginGrid(start);
    GridPos position{};
    bool found=false;
    for(int x=origin.x+2;x<origin.x+28 &&!found;++x)
        for(int y=origin.y+2;y<origin.y+28 &&!found;++y){
            if(coreGroundTraversable(world,{x,y})
               &&coreGroundTraversable(world,{x+1,y})
               &&coreGroundTraversable(world,{x+2,y})){
                position={x,y};found=true;
            }
        }
    assert(found);
    world.resourceNodes={
        {101,MaterialKind::Stone,8,8,false,0,{position.x+1,position.y}},
        {102,MaterialKind::Wood,8,8,false,0,{position.x+2,position.y}}
    };
    const auto initialRng=world.rng;
    const auto initialPeerMemory=world.characters[1].memory.entries.size();
    RelationshipBook relationships;
    const auto request=currentView(
        world,relationships,world.characters.front(),position);
    const int deadline=world.minute+1440;
    const auto goal=twoMaterialGoal(request,deadline);
    assert(validateAutonomousGoalCandidate(world,request,goal).accepted);
    assert(!*autonomousGoalSatisfiedByTruth(world,relationships,goal));

    // This is a real two-step *candidate*, not a virtual extraction or
    // a promise that the action loop has already completed either step.
    const auto proposed=composeAutonomousExperimentOptions(
        world,relationships,request,goal);
    assert(proposed.status==AutonomousExperimentStatus::CandidatesOnly);
    assert(!proposed.alternatives.empty());
    const auto& steps=proposed.alternatives.front().steps;
    assert(steps.size()==2);
    assert(steps[0].kind==AutonomousAffordanceKind::ExistingCivilizationGather);
    assert(steps[1].kind==AutonomousAffordanceKind::ExistingCivilizationGather);
    assert(steps[0].resourceNode==101 &&steps[1].resourceNode==102);
    assert(world.characters.front().civilization.inventory.stacks().empty());
    assert(world.resourceNodes[0].quantity==8 &&world.resourceNodes[1].quantity==8);

    // Each step is actually committed by Core's existing physical
    // civilization executor. Never simulate success inside the planner.
    CivilizationUtilityDecision first;
    first.intent=CivilizationIntent::Gather;
    first.material=steps[0].material;
    first.resourceNode=steps[0].resourceNode;
    first.quantity=1;
    const auto firstResult=executeCivilizationDecisionAtPosition(
        world,world.characters.front(),first,
        world.resourceNodes[0].pos);
    assert(firstResult.executed &&firstResult.success);
    assert(firstResult.event.quantity>0);
    assert(world.resourceNodes[0].quantity==8-firstResult.event.quantity);
    assert(world.resourceNodes[1].quantity==8);
    assert(!*autonomousGoalSatisfiedByTruth(world,relationships,goal));

    // A fresh decision after a real outcome must omit the already-met
    // prerequisite; stale Step 1 must NOT be repeated or double-debited.
    ++world.minute;
    const auto updated=currentView(
        world,relationships,world.characters.front(),position);
    const auto updatedGoal=twoMaterialGoal(updated,deadline);
    assert(validateAutonomousGoalCandidate(world,updated,updatedGoal).accepted);
    const auto next=composeAutonomousExperimentOptions(
        world,relationships,updated,updatedGoal);
    assert(next.status==AutonomousExperimentStatus::CandidatesOnly);
    assert(next.alternatives.size()==1);
    const auto& remaining=next.alternatives.front().steps;
    assert(remaining.size()==1);
    assert(remaining.front().resourceNode==102);
    CivilizationUtilityDecision second;
    second.intent=CivilizationIntent::Gather;
    second.material=remaining.front().material;
    second.resourceNode=remaining.front().resourceNode;
    second.quantity=1;
    const auto secondResult=executeCivilizationDecisionAtPosition(
        world,world.characters.front(),second,world.resourceNodes[1].pos);
    assert(secondResult.executed &&secondResult.success);
    assert(secondResult.event.quantity>0);
    assert(*autonomousGoalSatisfiedByTruth(world,relationships,updatedGoal));
    assert(world.resourceNodes[0].quantity==8-firstResult.event.quantity);
    assert(world.resourceNodes[1].quantity==8-secondResult.event.quantity);
    assert(world.characters[1].civilization.inventory.stacks().empty());
    assert(world.characters[1].memory.entries.size()==initialPeerMemory);
    assert(world.rng==initialRng);
    assert(!composeAutonomousExperimentOptions(
        world,relationships,currentView(
            world,relationships,world.characters.front(),position),
        updatedGoal).alternatives.size());
    return 0;
}
