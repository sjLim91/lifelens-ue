#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include "lifelens/AutonomousGoalContract.h"

using namespace lifelens;

int main()
{
    World world(17021);
    world.characters.clear();
    Character resident;
    resident.id=1;
    resident.needs.thirst=0.75;
    resident.civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::Stone,2,0.5,1.0});
    Character friendResident;
    friendResident.id=2;
    world.characters={resident,friendResident};

    RelationshipBook relationships;
    relationships.getOrCreate(1,2).trust=0.4;

    const CognitiveRequest request=buildCognitiveRequest(
        world.characters.front(),relationships,world.minute,
        CognitiveTriggerKind::Reflection,
        cognitiveStrategicIntentCatalog(),8,8,8);
    assert(request.actor==1);
    assert(request.relationships.size()==1);

    // No invention of a new technology/epoch is needed to compose a new
    // desired outcome from independent measurable Core truth predicates.
    AutonomousGoalCandidate goal;
    goal.actor=1;
    goal.createdMinute=world.minute;
    goal.expiresMinute=world.minute+1440;
    goal.priority=0.64;
    goal.evidence={{AutonomousGoalEvidenceKind::ActorNeed,0},
                   {AutonomousGoalEvidenceKind::Related,0}};
    AutonomousGoalPredicate water;
    water.metric=AutonomousGoalMetric::ActorNeed;
    water.need=AutonomousNeedKind::Thirst;
    water.comparison=AutonomousGoalCompare::AtMost;
    water.threshold=0.30;
    AutonomousGoalPredicate materials;
    materials.metric=AutonomousGoalMetric::ActorRawMaterialCount;
    materials.material=MaterialKind::Stone;
    materials.comparison=AutonomousGoalCompare::AtLeast;
    materials.threshold=4.0;
    AutonomousGoalPredicate trust;
    trust.metric=AutonomousGoalMetric::RelationshipTrust;
    trust.targetResident=2;
    trust.comparison=AutonomousGoalCompare::AtLeast;
    trust.threshold=0.60;
    goal.desiredState={water,materials,trust};

    const auto rngBefore=world.rng;
    auto snapshotBefore=world.characters;
    assert(validateAutonomousGoalCandidate(world,request,goal).accepted);
    auto result=autonomousGoalSatisfiedByTruth(world,relationships,goal);
    assert(result.has_value() && !*result);
    assert(world.rng==rngBefore);
    assert(world.characters.front().needs.thirst==0.75);
    assert(world.characters.front().civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Stone)==2);
    assert(world.characters.size()==snapshotBefore.size());

    AutonomousHypothesisCandidate hypothesis;
    hypothesis.actor=goal.actor;
    hypothesis.createdMinute=goal.createdMinute;
    hypothesis.probability=0.68;
    hypothesis.estimatedCost=10.0;
    hypothesis.predictedChanges={water,materials};
    hypothesis.evidence={{AutonomousGoalEvidenceKind::ActorNeed,0}};
    assert(validateAutonomousHypothesisCandidate(
        world,request,goal,hypothesis).accepted);

    // The goal is a condition to be measured, NOT an instruction that
    // directly alters the Core. Only unrelated test mutation can satisfy it.
    world.characters.front().needs.thirst=0.20;
    world.characters.front().civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::Stone,2,0.5,1.0});
    relationships.getOrCreate(1,2).trust=0.75;
    result=autonomousGoalSatisfiedByTruth(world,relationships,goal);
    assert(result.has_value() && *result);

    auto bad=goal;
    bad.priority=std::numeric_limits<double>::quiet_NaN();
    assert(!validateAutonomousGoalCandidate(world,request,bad).accepted);
    bad=goal;
    bad.actor=999;
    assert(!validateAutonomousGoalCandidate(world,request,bad).accepted);
    bad=goal;
    bad.createdMinute+=1;
    assert(!validateAutonomousGoalCandidate(world,request,bad).accepted);
    bad=goal;
    bad.expiresMinute=goal.createdMinute+MaxAutonomousGoalHorizonMinutes+1;
    assert(!validateAutonomousGoalCandidate(world,request,bad).accepted);
    bad=goal;
    bad.evidence={{AutonomousGoalEvidenceKind::Remembered,0}};
    assert(!validateAutonomousGoalCandidate(world,request,bad).accepted);
    bad=goal;
    bad.evidence={{AutonomousGoalEvidenceKind::ActorNeed,0},
                  {AutonomousGoalEvidenceKind::ActorNeed,0}};
    assert(!validateAutonomousGoalCandidate(world,request,bad).accepted);
    bad=goal;
    bad.desiredState.push_back(water);
    assert(!validateAutonomousGoalCandidate(world,request,bad).accepted);
    bad=goal;
    bad.desiredState[2].targetResident=999;
    assert(!validateAutonomousGoalCandidate(world,request,bad).accepted);
    bad=goal;
    bad.desiredState[1].threshold=3.1;
    assert(!validateAutonomousGoalCandidate(world,request,bad).accepted);
    bad=goal;
    bad.desiredState[0].threshold=
        std::numeric_limits<double>::infinity();
    assert(!validateAutonomousGoalCandidate(world,request,bad).accepted);
    bad=goal;
    bad.desiredState[0].metric=static_cast<AutonomousGoalMetric>(900);
    assert(!validateAutonomousGoalCandidate(world,request,bad).accepted);
    bad=goal;
    bad.desiredState.clear();
    assert(!validateAutonomousGoalCandidate(world,request,bad).accepted);
    bad=goal;
    for(int i=0;i<8;++i) bad.desiredState.push_back(water);
    assert(!validateAutonomousGoalCandidate(world,request,bad).accepted);
    bad=goal;
    bad.evidence={{static_cast<AutonomousGoalEvidenceKind>(999),0}};
    assert(!validateAutonomousGoalCandidate(world,request,bad).accepted);

    auto badHypothesis=hypothesis;
    badHypothesis.probability=
        std::numeric_limits<double>::quiet_NaN();
    assert(!validateAutonomousHypothesisCandidate(
        world,request,goal,badHypothesis).accepted);
    badHypothesis=hypothesis;
    badHypothesis.estimatedCost=-1.0;
    assert(!validateAutonomousHypothesisCandidate(
        world,request,goal,badHypothesis).accepted);
    badHypothesis=hypothesis;
    badHypothesis.actor=2;
    assert(!validateAutonomousHypothesisCandidate(
        world,request,goal,badHypothesis).accepted);

    // Firsthand and reported knowledge cannot be conflated: the current
    // bounded CognitiveRequest does not yet retain memory.source.
    // Therefore B3 cannot certify witness provenance or authorize action.
    assert(world.rng==rngBefore);
    return 0;
}
