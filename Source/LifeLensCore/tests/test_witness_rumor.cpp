#include <cassert>
#include <cmath>

#include "lifelens/WitnessRumor.h"

using namespace lifelens;

namespace {

bool near(double a, double b, double eps = 1e-12) {
    return std::fabs(a - b) <= eps;
}

SocialFact makeFoodFact() {
    SocialFact fact;
    fact.id = 1001;
    fact.subject = 2;
    fact.proposition = "took food from shared storage";
    fact.where = "shared kitchen";
    fact.eventMinute = 600;
    fact.supports = true;
    fact.importance = 0.82;
    fact.confidence = 0.96;
    fact.emotionValence = -0.25;
    fact.emotionIntensity = 0.65;
    return fact;
}

} // namespace

int main() {
    constexpr std::uint64_t seed = 424242;

    SocialKnowledgeBook knowledge;
    assert(knowledge.registerFact(makeFoodFact()));
    assert(knowledge.facts().size() == 1);

    // Re-registering the same canonical fact is idempotent, while reusing the
    // same ID for a different proposition is rejected.
    assert(knowledge.registerFact(makeFoodFact()));
    SocialFact conflicting = makeFoodFact();
    conflicting.proposition = "did something else";
    assert(!knowledge.registerFact(conflicting));

    MemoryState witnessMemory;
    BeliefState witnessBeliefs;
    const KnowledgeReceipt* witnessed = knowledge.recordDirectWitness(
        1001, 1, witnessMemory, witnessBeliefs, 601);
    assert(witnessed != nullptr);
    assert(witnessMemory.entries.size() == 1);
    assert(witnessMemory.entries[0].source == MemorySource::DirectWitness);
    assert(witnessMemory.entries[0].witnessed);
    assert(witnessMemory.entries[0].sourceCharacter == 1);
    assert(witnessed->originWitness == 1);
    assert(witnessed->immediateSource == 1);
    assert(witnessed->transmissionPath.size() == 1);
    assert(witnessed->transmissionPath[0] == 1);
    assert(witnessed->hopCount() == 0);

    const BeliefRecord* witnessBelief = witnessBeliefs.find(
        2, "took food from shared storage");
    assert(witnessBelief != nullptr);
    assert(witnessBelief->stance > 0.99);
    assert(witnessBelief->confidence > 0.0);

    // Re-observing the same fact does not duplicate memory/evidence.
    const KnowledgeReceipt* duplicateWitness = knowledge.recordDirectWitness(
        1001, 1, witnessMemory, witnessBeliefs, 602);
    assert(duplicateWitness != nullptr);
    assert(witnessMemory.entries.size() == 1);

    const auto firstStatement = knowledge.makeStatement(1001, 1, 605, seed);
    assert(firstStatement.has_value());
    assert(firstStatement->valid());
    assert(firstStatement->speaker == 1);
    assert(firstStatement->originWitness == 1);
    assert(firstStatement->supports);
    assert(firstStatement->confidence < witnessed->confidence);

    MemoryState receiverMemory;
    BeliefState receiverBeliefs;
    const StatementReceptionOutcome firstHop = knowledge.receiveStatement(
        *firstStatement,
        3,
        0.80,
        receiverMemory,
        receiverBeliefs,
        606,
        seed);
    assert(firstHop.result == StatementReceptionResult::Accepted);
    assert(receiverMemory.entries.size() == 1);
    assert(receiverMemory.entries[0].source == MemorySource::ToldByOther);
    assert(!receiverMemory.entries[0].witnessed);
    assert(receiverMemory.entries[0].sourceCharacter == 1);
    assert(firstHop.acceptedConfidence > 0.25);
    assert(firstHop.acceptedConfidence < firstStatement->confidence);

    const KnowledgeReceipt* receiverReceipt = knowledge.findReceipt(3, 1001);
    assert(receiverReceipt != nullptr);
    assert(receiverReceipt->originWitness == 1);
    assert(receiverReceipt->immediateSource == 1);
    assert(receiverReceipt->transmissionPath.size() == 2);
    assert(receiverReceipt->transmissionPath[0] == 1);
    assert(receiverReceipt->transmissionPath[1] == 3);
    assert(receiverReceipt->hopCount() == 1);

    // Duplicate delivery cannot amplify belief or create extra memories.
    const double receiverBeliefConfidence = firstHop.beliefConfidence;
    const StatementReceptionOutcome duplicateDelivery = knowledge.receiveStatement(
        *firstStatement,
        3,
        1.0,
        receiverMemory,
        receiverBeliefs,
        607,
        seed);
    assert(duplicateDelivery.result == StatementReceptionResult::Duplicate);
    assert(receiverMemory.entries.size() == 1);
    assert(near(
        receiverBeliefs.find(2, "took food from shared storage")->confidence,
        receiverBeliefConfidence));

    // A retelling loses confidence but retains provenance.
    const auto secondStatement = knowledge.makeStatement(1001, 3, 610, seed);
    assert(secondStatement.has_value());
    assert(secondStatement->speaker == 3);
    assert(secondStatement->originWitness == 1);
    assert(secondStatement->transmissionPath.size() == 2);
    assert(secondStatement->confidence < receiverReceipt->confidence);

    MemoryState secondReceiverMemory;
    BeliefState secondReceiverBeliefs;
    const StatementReceptionOutcome secondHop = knowledge.receiveStatement(
        *secondStatement,
        4,
        0.70,
        secondReceiverMemory,
        secondReceiverBeliefs,
        611,
        seed);
    assert(secondHop.result == StatementReceptionResult::Accepted);
    assert(secondHop.acceptedConfidence < firstHop.acceptedConfidence);

    const KnowledgeReceipt* secondReceipt = knowledge.findReceipt(4, 1001);
    assert(secondReceipt != nullptr);
    assert(secondReceipt->transmissionPath.size() == 3);
    assert(secondReceipt->transmissionPath[0] == 1);
    assert(secondReceipt->transmissionPath[1] == 3);
    assert(secondReceipt->transmissionPath[2] == 4);
    assert(secondReceipt->hopCount() == 2);

    // Returning the rumor to a resident already in its provenance path is
    // suppressed before it can create another memory or belief update.
    const StatementReceptionOutcome loopedBack = knowledge.receiveStatement(
        *secondStatement,
        1,
        1.0,
        witnessMemory,
        witnessBeliefs,
        612,
        seed);
    assert(loopedBack.result == StatementReceptionResult::Duplicate ||
           loopedBack.result == StatementReceptionResult::LoopSuppressed);
    assert(witnessMemory.entries.size() == 1);

    // Very low trust is insufficient to convert a rumor into accepted evidence.
    MemoryState lowTrustMemory;
    BeliefState lowTrustBeliefs;
    const StatementReceptionOutcome lowTrust = knowledge.receiveStatement(
        *firstStatement,
        5,
        0.0,
        lowTrustMemory,
        lowTrustBeliefs,
        606,
        seed);
    assert(lowTrust.result == StatementReceptionResult::TooWeak);
    assert(lowTrustMemory.entries.empty());
    assert(lowTrustBeliefs.beliefs.empty());
    assert(!knowledge.hasReceipt(5, 1001));

    // Negative evidence must produce a negative belief stance rather than being
    // silently converted into a positive rumor.
    SocialFact denied;
    denied.id = 2002;
    denied.subject = 7;
    denied.proposition = "was present at the meeting";
    denied.where = "office";
    denied.eventMinute = 700;
    denied.supports = false;
    denied.confidence = 0.92;
    assert(knowledge.registerFact(denied));

    MemoryState negativeMemory;
    BeliefState negativeBeliefs;
    assert(knowledge.recordDirectWitness(
        2002, 6, negativeMemory, negativeBeliefs, 701) != nullptr);
    const BeliefRecord* negativeBelief = negativeBeliefs.find(
        7, "was present at the meeting");
    assert(negativeBelief != nullptr);
    assert(negativeBelief->stance < -0.99);

    // Determinism: identical seed, fact, path and trust produce bit-stable
    // statement/reception results without consuming the simulation RNG stream.
    SocialKnowledgeBook twin;
    assert(twin.registerFact(makeFoodFact()));
    MemoryState twinWitnessMemory;
    BeliefState twinWitnessBeliefs;
    assert(twin.recordDirectWitness(
        1001, 1, twinWitnessMemory, twinWitnessBeliefs, 601) != nullptr);
    const auto twinStatement = twin.makeStatement(1001, 1, 605, seed);
    assert(twinStatement.has_value());
    assert(twinStatement->supports == firstStatement->supports);
    assert(near(twinStatement->confidence, firstStatement->confidence));
    assert(twinStatement->transmissionPath == firstStatement->transmissionPath);

    MemoryState twinReceiverMemory;
    BeliefState twinReceiverBeliefs;
    const StatementReceptionOutcome twinHop = twin.receiveStatement(
        *twinStatement,
        3,
        0.80,
        twinReceiverMemory,
        twinReceiverBeliefs,
        606,
        seed);
    assert(twinHop.result == StatementReceptionResult::Accepted);
    assert(near(twinHop.acceptedConfidence, firstHop.acceptedConfidence));
    assert(near(twinHop.beliefStance, firstHop.beliefStance));
    assert(near(twinHop.beliefConfidence, firstHop.beliefConfidence));

    return 0;
}
