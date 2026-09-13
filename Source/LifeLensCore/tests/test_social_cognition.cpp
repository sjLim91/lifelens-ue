#include <cassert>
#include <cmath>
#include <string>

#include "lifelens/SocialCognition.h"

using namespace lifelens;

int main()
{
    RelationshipBook relationships;

    Character receiver;
    receiver.id = 2;
    receiver.name = "Receiver";

    SocialEvent helped;
    helped.actor = 1;
    helped.recipient = 2;
    helped.type = SocialEventType::Help;
    helped.intensity = 1.0;
    helped.minute = 100;
    helped.where = "kitchen";
    helped.source = MemorySource::DirectWitness;
    helped.witnessed = true;

    const SocialCognitionResult helpResult = processSocialEvent(receiver, relationships, helped);

    assert(receiver.emotion.joy > 0.0);
    assert(receiver.memory.entries.size() == 1);
    assert(receiver.memory.entries.front().who == 1);
    assert(receiver.memory.entries.front().what == "help");
    assert(receiver.memory.entries.front().where == "kitchen");

    const BeliefRecord* reliable = receiver.beliefs.find(1, "is_reliable");
    assert(reliable != nullptr);
    assert(reliable->stance > 0.0);
    assert(reliable->confidence > 0.0);
    assert(helpResult.beliefProposition == "is_reliable");

    const Relationship* receiverToActor = relationships.find(2, 1);
    assert(receiverToActor != nullptr);
    assert(receiverToActor->trust > 0.0);
    assert(receiverToActor->respect > 0.0);

    // A third-party direct witness can form a belief, but the personal relationship
    // effect is deliberately weaker than being the recipient of the action.
    Character witness;
    witness.id = 3;
    witness.name = "Witness";
    const SocialCognitionResult witnessResult = processSocialEvent(witness, relationships, helped);
    const Relationship* witnessToActor = relationships.find(3, 1);
    assert(witnessToActor != nullptr);
    assert(witnessToActor->respect > 0.0);
    assert(witnessToActor->trust < receiverToActor->trust);
    assert(witnessResult.perspectiveScale < helpResult.perspectiveScale);

    // Betrayal should affect emotion, memory, belief and relationship in one pass.
    Character betrayed;
    betrayed.id = 5;
    betrayed.name = "Betrayed";
    Relationship& prior = relationships.getOrCreate(5, 1);
    prior.affection = 0.75;
    prior.trust = 0.80;
    prior.respect = 0.70;

    SocialEvent betrayal;
    betrayal.actor = 1;
    betrayal.recipient = 5;
    betrayal.type = SocialEventType::Betrayal;
    betrayal.intensity = 1.0;
    betrayal.minute = 200;
    betrayal.where = "storage";
    betrayal.source = MemorySource::DirectWitness;
    betrayal.witnessed = true;

    const SocialCognitionResult betrayalResult = processSocialEvent(betrayed, relationships, betrayal);
    const BeliefRecord* trustworthy = betrayed.beliefs.find(1, "is_trustworthy");
    const Relationship* betrayedToActor = relationships.find(5, 1);

    assert(betrayed.emotion.anger > 0.0);
    assert(betrayed.emotion.sadness > 0.0);
    assert(trustworthy != nullptr);
    assert(trustworthy->stance < 0.0);
    assert(betrayalResult.beliefSupports == false);
    assert(betrayedToActor != nullptr);
    assert(betrayedToActor->trust < 0.80);
    assert(betrayedToActor->affection < 0.75);
    assert(betrayedToActor->conflict > 0.0);
    assert(betrayedToActor->grudge > 0.0);

    // Hearsay must not be treated as strongly as directly experiencing the event.
    Character hearsayObserver;
    hearsayObserver.id = 6;
    hearsayObserver.name = "HearsayObserver";
    SocialEvent hearsay = betrayal;
    hearsay.recipient = 5;
    hearsay.minute = 210;
    hearsay.source = MemorySource::ToldByOther;
    hearsay.sourceCharacter = 7;
    hearsay.witnessed = false;

    const SocialCognitionResult hearsayResult = processSocialEvent(hearsayObserver, relationships, hearsay);
    const BeliefRecord* hearsayBelief = hearsayObserver.beliefs.find(1, "is_trustworthy");
    assert(hearsayBelief != nullptr);
    assert(hearsayBelief->stance < 0.0);
    assert(betrayalResult.beliefConfidence > hearsayResult.beliefConfidence);
    assert(betrayalResult.relationshipScale > hearsayResult.relationshipScale);

    // Conflicting romantic evidence must coexist rather than overwrite history.
    Character romanticObserver;
    romanticObserver.id = 8;
    romanticObserver.name = "RomanticObserver";

    SocialEvent intimacy;
    intimacy.actor = 9;
    intimacy.recipient = 8;
    intimacy.type = SocialEventType::Intimacy;
    intimacy.intensity = 1.0;
    intimacy.minute = 300;
    intimacy.source = MemorySource::DirectWitness;
    intimacy.witnessed = true;

    processSocialEvent(romanticObserver, relationships, intimacy);
    const BeliefRecord* romanticBefore = romanticObserver.beliefs.find(9, "romantically_interested");
    assert(romanticBefore != nullptr);
    const double stanceBeforeRejection = romanticBefore->stance;
    assert(stanceBeforeRejection > 0.0);

    SocialEvent rejection = intimacy;
    rejection.type = SocialEventType::Rejection;
    rejection.minute = 360;
    processSocialEvent(romanticObserver, relationships, rejection);

    const BeliefRecord* romanticAfter = romanticObserver.beliefs.find(9, "romantically_interested");
    assert(romanticAfter != nullptr);
    assert(romanticAfter->supportCount == 1);
    assert(romanticAfter->contradictionCount == 1);
    assert(romanticAfter->stance < stanceBeforeRejection);
    assert(romanticObserver.memory.entries.size() == 2);

    return 0;
}
