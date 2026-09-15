#include <cassert>
#include <cmath>
#include <string>
#include <vector>

#include "lifelens/SanitationProblemRecognition.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

namespace {

MemoryRecord sanitationMemory(
    CharacterId sourceCharacter,
    int minute,
    double importance=0.45,
    double emotionIntensity=0.40)
{
    MemoryRecord memory;
    memory.who=0;
    memory.sourceCharacter=sourceCharacter;
    memory.what="experienced unsanitary surroundings";
    memory.where="grid:2,-1";
    memory.minute=minute;
    memory.emotionValence=-0.45;
    memory.emotionIntensity=emotionIntensity;
    memory.importance=importance;
    memory.confidence=1.0;
    memory.witnessed=true;
    memory.source=MemorySource::DirectWitness;
    memory.decayPerDay=0.012;
    memory.tags={"environment","contamination","human_waste","avoidance","sanitation"};
    memory.normalize();
    return memory;
}

MemoryRecord unrelatedMemory(CharacterId sourceCharacter,int minute)
{
    MemoryRecord memory;
    memory.who=0;
    memory.sourceCharacter=sourceCharacter;
    memory.what="noticed a noisy place";
    memory.where="grid:2,-1";
    memory.minute=minute;
    memory.emotionIntensity=0.8;
    memory.importance=0.9;
    memory.confidence=1.0;
    memory.witnessed=true;
    memory.source=MemorySource::DirectWitness;
    memory.tags={"environment","noise","avoidance"};
    memory.normalize();
    return memory;
}

const Character* findCharacter(const Simulation& simulation,CharacterId id)
{
    for(const auto& character:simulation.world().characters){
        if(character.id==id) return &character;
    }
    return nullptr;
}

} // namespace

int main()
{
    // One ordinary firsthand contamination memory is meaningful but should not
    // instantly manufacture a civilization-level sanitation problem belief.
    Character weak;
    weak.id=10;
    weak.memory.add(sanitationMemory(weak.id,100));
    const SanitationProblemRecognitionResult weakResult=
        recognizeSanitationProblem(weak,120);
    assert(weakResult.qualifyingMemories==1);
    assert(!weakResult.recognized);
    assert(!hasRecognizedSanitationProblem(weak));
    assert(weak.beliefs.find(0,sanitationProblemBeliefProposition())==nullptr);

    // Repeated direct experience crosses the recognition threshold and creates
    // one explicit Belief derived from evidence rather than a global tech flag.
    Character repeated;
    repeated.id=20;
    repeated.memory.add(sanitationMemory(repeated.id,100));
    repeated.memory.add(sanitationMemory(repeated.id,300));
    const SanitationProblemRecognitionResult repeatedResult=
        recognizeSanitationProblem(repeated,320);
    assert(repeatedResult.qualifyingMemories==2);
    assert(repeatedResult.evidenceWeight>=1.25);
    assert(repeatedResult.recognized);
    assert(repeatedResult.becameRecognized);
    assert(hasRecognizedSanitationProblem(repeated));

    const BeliefRecord* repeatedBelief=
        repeated.beliefs.find(0,sanitationProblemBeliefProposition());
    assert(repeatedBelief!=nullptr);
    assert(repeatedBelief->supportCount==2);
    const double supportWeightBefore=repeatedBelief->supportWeight;
    const double confidenceBefore=repeatedBelief->confidence;

    // Re-evaluating unchanged memories is idempotent: no evidence inflation.
    const SanitationProblemRecognitionResult repeatedAgain=
        recognizeSanitationProblem(repeated,320);
    repeatedBelief=repeated.beliefs.find(0,sanitationProblemBeliefProposition());
    assert(repeatedAgain.recognized);
    assert(!repeatedAgain.becameRecognized);
    assert(repeatedBelief!=nullptr);
    assert(repeatedBelief->supportCount==2);
    assert(std::abs(repeatedBelief->supportWeight-supportWeightBefore)<1e-12);
    assert(std::abs(repeatedBelief->confidence-confidenceBefore)<1e-12);

    // A single exceptionally salient firsthand event can be enough, while an
    // equally salient but unrelated memory cannot contribute sanitation evidence.
    Character strong;
    strong.id=30;
    strong.memory.add(sanitationMemory(strong.id,500,1.0,1.0));
    const SanitationProblemRecognitionResult strongResult=
        recognizeSanitationProblem(strong,500);
    assert(strongResult.strongestEvidence>=0.90);
    assert(strongResult.recognized);

    Character unrelated;
    unrelated.id=40;
    unrelated.memory.add(unrelatedMemory(unrelated.id,500));
    unrelated.memory.add(unrelatedMemory(unrelated.id,520));
    const SanitationProblemRecognitionResult unrelatedResult=
        recognizeSanitationProblem(unrelated,530);
    assert(unrelatedResult.qualifyingMemories==0);
    assert(!unrelatedResult.recognized);
    assert(unrelated.beliefs.find(0,sanitationProblemBeliefProposition())==nullptr);

    // Existing Character/Belief snapshot authority persists the recognized
    // problem through binary Save/Load without adding a parallel recognition flag.
    Simulation source(424242);
    source.setupNewGame();
    assert(!source.world().characters.empty());
    source.world().minute=800;
    Character& resident=source.world().characters.front();
    const CharacterId residentId=resident.id;
    resident.memory.add(sanitationMemory(residentId,600));
    resident.memory.add(sanitationMemory(residentId,780));
    assert(recognizeSanitationProblem(resident,800).recognized);

    const SimulationStateSnapshot snapshot=source.captureSnapshot();
    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(snapshot,bytes,&error));
    assert(error.empty());

    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    assert(error.empty());

    Simulation restored(1);
    assert(restored.restoreSnapshot(decoded,&error));
    assert(error.empty());
    const Character* restoredResident=findCharacter(restored,residentId);
    assert(restoredResident!=nullptr);
    assert(hasRecognizedSanitationProblem(*restoredResident));

    const BeliefRecord* restoredBelief=
        restoredResident->beliefs.find(0,sanitationProblemBeliefProposition());
    assert(restoredBelief!=nullptr);
    assert(restoredBelief->supportCount==2);
    assert(restoredBelief->confidence>=0.55);

    return 0;
}
