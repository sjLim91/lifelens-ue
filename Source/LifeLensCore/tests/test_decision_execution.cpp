#include <cassert>

#include "lifelens/DecisionExecution.h"

using namespace lifelens;

static SmartObject makeFridge()
{
    SmartObject fridge;
    fridge.id = 500;
    fridge.kind = ObjectKind::Fridge;
    fridge.pos = {1, 0};
    fridge.effectPerTick.hunger = -0.08;
    fridge.useDurationTicks = 8;
    return fridge;
}

int main()
{
    // Approach should create an actual positive social event for the target.
    World approachWorld(1);
    Character approachActor;
    approachActor.id = 1;
    approachActor.personality.sociability = 0.95;
    approachActor.personality.curiosity = 0.75;
    approachActor.personality.introversion = 0.10;
    Character approachTarget;
    approachTarget.id = 2;
    approachWorld.characters = {approachActor, approachTarget};

    RelationshipBook approachRelationships;
    Relationship& actorView = approachRelationships.getOrCreate(1, 2);
    actorView.affection = 0.84;
    actorView.trust = 0.86;
    actorView.comfort = 0.82;
    actorView.familiarity = 0.90;

    DecisionExecutionResult approachResult = executeUnifiedDecision(
        approachWorld,
        approachRelationships,
        1,
        0.18,
        "living_room");

    assert(approachResult.decision.kind == UnifiedDecisionKind::Social);
    assert(approachResult.decision.social.intent == SocialIntent::Approach);
    assert(approachResult.socialExecuted);
    assert(approachResult.generatedSocialEvent);
    assert(approachResult.eventType == SocialEventType::PositiveInteraction);

    const Character* approachTargetAfter = findCharacter(approachWorld, 2);
    assert(approachTargetAfter != nullptr);
    assert(approachTargetAfter->memory.entries.size() == 1);
    assert(approachTargetAfter->memory.entries.front().what == "positive_interaction");
    const BeliefRecord* friendlyBelief = approachTargetAfter->beliefs.find(1, "is_friendly");
    assert(friendlyBelief != nullptr);
    assert(friendlyBelief->stance > 0.0);
    const Relationship* targetView = approachRelationships.find(2, 1);
    assert(targetView != nullptr);
    assert(targetView->affection > 0.0);
    assert(targetView->trust > 0.0);

    // Repair should lower the target's conflict/grudge toward the actor.
    World repairWorld(2);
    Character repairActor;
    repairActor.id = 10;
    repairActor.personality.agreeableness = 0.98;
    repairActor.personality.empathy = 0.96;
    repairActor.personality.patience = 0.96;
    repairActor.personality.riskTolerance = 0.65;
    Character repairTarget;
    repairTarget.id = 11;
    repairWorld.characters = {repairActor, repairTarget};

    RelationshipBook repairRelationships;
    Relationship& repairActorView = repairRelationships.getOrCreate(10, 11);
    repairActorView.affection = 0.80;
    repairActorView.trust = 0.68;
    repairActorView.comfort = 0.72;
    repairActorView.familiarity = 0.88;
    repairActorView.conflict = 0.64;
    repairActorView.fear = 0.02;
    repairActorView.grudge = 0.03;

    Relationship& repairTargetView = repairRelationships.getOrCreate(11, 10);
    repairTargetView.affection = 0.55;
    repairTargetView.trust = 0.38;
    repairTargetView.conflict = 0.66;
    repairTargetView.grudge = 0.52;
    const double conflictBeforeRepair = repairTargetView.conflict;
    const double grudgeBeforeRepair = repairTargetView.grudge;

    DecisionExecutionResult repairResult = executeUnifiedDecision(
        repairWorld,
        repairRelationships,
        10);

    assert(repairResult.decision.social.intent == SocialIntent::Repair);
    assert(repairResult.socialExecuted);
    const Relationship* repairedTargetView = repairRelationships.find(11, 10);
    assert(repairedTargetView != nullptr);
    assert(repairedTargetView->conflict < conflictBeforeRepair);
    assert(repairedTargetView->grudge < grudgeBeforeRepair);
    const Character* repairTargetAfter = findCharacter(repairWorld, 11);
    assert(repairTargetAfter != nullptr);
    const BeliefRecord* repairBelief = repairTargetAfter->beliefs.find(10, "wants_repair");
    assert(repairBelief != nullptr);
    assert(repairBelief->stance > 0.0);

    // Comfort should actually reduce distress on the target.
    World comfortWorld(3);
    Character comfortActor;
    comfortActor.id = 20;
    comfortActor.personality.empathy = 0.99;
    comfortActor.personality.agreeableness = 0.94;
    comfortActor.personality.sociability = 0.70;
    Character comfortTarget;
    comfortTarget.id = 21;
    comfortTarget.emotion.sadness = 0.92;
    comfortTarget.emotion.anxiety = 0.68;
    comfortTarget.emotion.refreshSummary();
    comfortWorld.characters = {comfortActor, comfortTarget};

    RelationshipBook comfortRelationships;
    Relationship& comfortActorView = comfortRelationships.getOrCreate(20, 21);
    comfortActorView.affection = 0.74;
    comfortActorView.trust = 0.72;
    comfortActorView.comfort = 0.70;
    comfortActorView.familiarity = 0.76;

    const double sadnessBefore = comfortWorld.characters[1].emotion.sadness;
    const double anxietyBefore = comfortWorld.characters[1].emotion.anxiety;
    DecisionExecutionResult comfortResult = executeUnifiedDecision(
        comfortWorld,
        comfortRelationships,
        20);

    assert(comfortResult.decision.social.intent == SocialIntent::Comfort);
    assert(comfortResult.socialExecuted);
    assert(comfortWorld.characters[1].emotion.sadness < sadnessBefore);
    assert(comfortWorld.characters[1].emotion.anxiety < anxietyBefore);
    const Relationship* comfortTargetView = comfortRelationships.find(21, 20);
    assert(comfortTargetView != nullptr);
    assert(comfortTargetView->affection > 0.0);
    assert(comfortTargetView->trust > 0.0);

    // Avoid is a real executed decision but should not fabricate an event for the target.
    World avoidWorld(4);
    Character avoidActor;
    avoidActor.id = 30;
    avoidActor.personality.riskTolerance = 0.05;
    avoidActor.personality.agreeableness = 0.05;
    avoidActor.personality.empathy = 0.05;
    avoidActor.personality.patience = 0.05;
    avoidActor.emotion.fear = 0.60;
    avoidActor.emotion.anxiety = 0.55;
    avoidActor.emotion.refreshSummary();
    Character avoidTarget;
    avoidTarget.id = 31;
    avoidWorld.characters = {avoidActor, avoidTarget};

    RelationshipBook avoidRelationships;
    Relationship& avoidRelation = avoidRelationships.getOrCreate(30, 31);
    avoidRelation.fear = 0.86;
    avoidRelation.conflict = 0.72;
    avoidRelation.grudge = 0.64;
    const double fearBeforeAvoid = avoidWorld.characters[0].emotion.fear;
    const double anxietyBeforeAvoid = avoidWorld.characters[0].emotion.anxiety;

    DecisionExecutionResult avoidResult = executeUnifiedDecision(
        avoidWorld,
        avoidRelationships,
        30);

    assert(avoidResult.decision.social.intent == SocialIntent::Avoid);
    assert(avoidResult.socialExecuted);
    assert(!avoidResult.generatedSocialEvent);
    assert(avoidWorld.characters[0].emotion.fear < fearBeforeAvoid);
    assert(avoidWorld.characters[0].emotion.anxiety < anxietyBeforeAvoid);
    assert(avoidWorld.characters[1].memory.entries.empty());

    // Urgent physical needs remain a physical plan handoff, not a social execution.
    World physicalWorld(5);
    Character hungry;
    hungry.id = 40;
    hungry.needs.hunger = 0.99;
    hungry.personality.sociability = 1.0;
    Character closeFriend;
    closeFriend.id = 41;
    physicalWorld.characters = {hungry, closeFriend};
    physicalWorld.objects.push_back(makeFridge());

    RelationshipBook physicalRelationships;
    Relationship& close = physicalRelationships.getOrCreate(40, 41);
    close.affection = 0.95;
    close.trust = 0.95;
    close.comfort = 0.95;
    close.familiarity = 0.95;

    DecisionExecutionResult physicalResult = executeUnifiedDecision(
        physicalWorld,
        physicalRelationships,
        40);

    assert(physicalResult.decision.kind == UnifiedDecisionKind::Physical);
    assert(physicalResult.decision.physicalGoal == Goal::Eat);
    assert(!physicalResult.socialExecuted);
    assert(physicalWorld.characters[1].memory.entries.empty());

    return 0;
}
