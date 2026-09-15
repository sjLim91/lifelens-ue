#include <cassert>
#include <string>
#include <vector>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/PrimitiveSanitation.h"
#include "lifelens/SanitationProblemRecognition.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

namespace {

MemoryRecord sanitationMemory(CharacterId sourceCharacter,int minute)
{
    MemoryRecord memory;
    memory.who=0;
    memory.sourceCharacter=sourceCharacter;
    memory.what="experienced unsanitary surroundings";
    memory.where="grid:2,-1";
    memory.minute=minute;
    memory.emotionValence=-0.55;
    memory.emotionIntensity=0.55;
    memory.importance=0.55;
    memory.confidence=1.0;
    memory.witnessed=true;
    memory.source=MemorySource::DirectWitness;
    memory.decayPerDay=0.012;
    memory.tags={"environment","contamination","human_waste","avoidance","sanitation"};
    memory.normalize();
    return memory;
}

void establishSanitationProblem(Character& character)
{
    character.memory.add(sanitationMemory(character.id,100));
    character.memory.add(sanitationMemory(character.id,220));
    const SanitationProblemRecognitionResult recognized=
        recognizeSanitationProblem(character,240);
    assert(recognized.recognized);
    assert(hasRecognizedSanitationProblem(character));
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
    // No lived/recognized sanitation problem means no sanitation experiment.
    World cleanWorld(77);
    cleanWorld.minute=300;
    Character unaware;
    unaware.id=10;
    unaware.civilization.character=unaware.id;
    unaware.personality.curiosity=0.9;
    unaware.personality.openness=0.9;
    unaware.personality.patience=0.9;
    unaware.civilization.learningSkill=0.9;

    const PrimitiveSanitationOpportunity unawareOpportunity=
        evaluatePrimitiveSanitationOpportunity(
            cleanWorld.seed,unaware,cleanWorld.environmentalResidues,cleanWorld.minute);
    assert(!unawareOpportunity.problemRecognized);
    assert(!unawareOpportunity.siteAvailable);
    const CivilizationUtilityDecision unawareDecision=
        bestExperimentDecision(cleanWorld,unaware);
    assert(unawareDecision.technique!=TechniqueId::DesignatedSanitationArea);

    // Repeated firsthand sanitation evidence creates the causal seed. With a
    // clean candidate site, the existing civilization Experiment intent can now
    // select a designated sanitation-area experiment.
    Character aware=unaware;
    aware.id=11;
    aware.civilization.character=aware.id;
    establishSanitationProblem(aware);

    const PrimitiveSanitationOpportunity opportunityA=
        evaluatePrimitiveSanitationOpportunity(
            cleanWorld.seed,aware,cleanWorld.environmentalResidues,cleanWorld.minute);
    const PrimitiveSanitationOpportunity opportunityB=
        evaluatePrimitiveSanitationOpportunity(
            cleanWorld.seed,aware,cleanWorld.environmentalResidues,cleanWorld.minute);
    assert(opportunityA.problemRecognized);
    assert(opportunityA.siteAvailable);
    assert(opportunityA.siteExposure<PrimitiveSanitationCleanSiteExposureLimit);
    assert(opportunityA.suggestedSite.x==opportunityB.suggestedSite.x);
    assert(opportunityA.suggestedSite.y==opportunityB.suggestedSite.y);

    const CivilizationUtilityDecision sanitationDecision=
        bestExperimentDecision(cleanWorld,aware);
    assert(sanitationDecision.intent==CivilizationIntent::Experiment);
    assert(sanitationDecision.experiment==ExperimentKind::DesignateSanitationArea);
    assert(sanitationDecision.technique==TechniqueId::DesignatedSanitationArea);

    // Recognition alone is insufficient if every candidate relief site around
    // the settlement is already perceptibly contaminated.
    World dirtyWorld(77);
    dirtyWorld.minute=300;
    static const GridPos directions[8]={
        {1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1}
    };
    for(int distance=5;distance<=7;++distance){
        for(const GridPos direction:directions){
            dirtyWorld.environmentalResidues.deposit(
                EnvironmentalResidueKind::HumanWaste,
                {direction.x*distance,direction.y*distance},
                99,dirtyWorld.minute,1.0,1.0,1);
        }
    }
    const PrimitiveSanitationOpportunity blocked=
        evaluatePrimitiveSanitationOpportunity(
            dirtyWorld.seed,aware,dirtyWorld.environmentalResidues,dirtyWorld.minute);
    assert(blocked.problemRecognized);
    assert(!blocked.siteAvailable);
    const CivilizationUtilityDecision blockedDecision=
        bestExperimentDecision(dirtyWorld,aware);
    assert(blockedDecision.technique!=TechniqueId::DesignatedSanitationArea);

    // The low-level experiment contract independently enforces both causal
    // gates, so a caller cannot fabricate sanitation progress by constructing a
    // CivilizationUtilityDecision directly.
    Inventory inventory;
    KnowledgeState knowledge;
    ExperimentContext rejected;
    rejected.worldSeed=7;
    rejected.actor=2;
    rejected.attemptIndex=0;
    rejected.kind=ExperimentKind::DesignateSanitationArea;
    rejected.material=MaterialKind::Unknown;
    rejected.sanitationProblemRecognized=false;
    rejected.sanitationSiteAvailable=true;
    assert(!attemptExperiment(rejected,inventory,knowledge).attempted);
    rejected.sanitationProblemRecognized=true;
    rejected.sanitationSiteAvailable=false;
    assert(!attemptExperiment(rejected,inventory,knowledge).attempted);
    assert(knowledge.level(TechniqueId::DesignatedSanitationArea)==KnowledgeLevel::Unknown);

    // Failure is deterministic and retained as a hypothesis rather than being
    // discarded. Seed 1 / actor 1 produces a high deterministic roll.
    ExperimentContext failure;
    failure.worldSeed=1;
    failure.actor=1;
    failure.attemptIndex=0;
    failure.kind=ExperimentKind::DesignateSanitationArea;
    failure.material=MaterialKind::Unknown;
    failure.learningSkill=1.0;
    failure.curiosity=1.0;
    failure.patience=1.0;
    failure.sanitationProblemRecognized=true;
    failure.sanitationSiteAvailable=true;
    KnowledgeState failedKnowledge;
    const ExperimentResult failed=
        attemptExperiment(failure,inventory,failedKnowledge);
    assert(failed.attempted);
    assert(!failed.success);
    assert(failed.roll>=failed.successChance);
    assert(failed.event.type==CivilizationEventType::ExperimentFailed);
    assert(failedKnowledge.level(TechniqueId::DesignatedSanitationArea)==KnowledgeLevel::Hypothesized);

    KnowledgeState failedAgainKnowledge;
    const ExperimentResult failedAgain=
        attemptExperiment(failure,inventory,failedAgainKnowledge);
    assert(failedAgain.roll==failed.roll);
    assert(failedAgain.successChance==failed.successChance);
    assert(failedAgain.success==failed.success);

    // Success is also deterministic and creates personal reproducible knowledge
    // only. It does not mutate World objects or another resident's knowledge.
    ExperimentContext success;
    success.worldSeed=7;
    success.actor=2;
    success.attemptIndex=0;
    success.kind=ExperimentKind::DesignateSanitationArea;
    success.material=MaterialKind::Unknown;
    success.learningSkill=0.0;
    success.curiosity=0.0;
    success.patience=0.0;
    success.sanitationProblemRecognized=true;
    success.sanitationSiteAvailable=true;
    KnowledgeState discovererKnowledge;
    const ExperimentResult discovered=
        attemptExperiment(success,inventory,discovererKnowledge);
    assert(discovered.attempted);
    assert(discovered.success);
    assert(discovered.event.type==CivilizationEventType::Discovered);
    assert(discovered.discovered==TechniqueId::DesignatedSanitationArea);
    assert(discovererKnowledge.knowsAtLeast(
        TechniqueId::DesignatedSanitationArea,KnowledgeLevel::Reproducible));

    Character other;
    other.id=3;
    other.civilization.character=other.id;
    assert(other.civilization.knowledge.level(
        TechniqueId::DesignatedSanitationArea)==KnowledgeLevel::Unknown);
    assert(cleanWorld.objects.empty());

    // The new personal technique remains within the existing civilization
    // snapshot extension; no new global unlock flag or layout is needed.
    Simulation original(5050);
    original.setupNewGame();
    assert(!original.world().characters.empty());
    Character& originalResident=original.world().characters.front();
    const CharacterId residentId=originalResident.id;
    originalResident.civilization.knowledge.learn(
        TechniqueId::DesignatedSanitationArea,
        KnowledgeLevel::Reproducible,0.84);

    const SimulationStateSnapshot captured=original.captureSnapshot();
    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(captured,bytes,&error));
    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));

    Simulation restored(1);
    assert(restored.restoreSnapshot(decoded,&error));
    const Character* restoredResident=findCharacter(restored,residentId);
    assert(restoredResident!=nullptr);
    assert(restoredResident->civilization.knowledge.knowsAtLeast(
        TechniqueId::DesignatedSanitationArea,KnowledgeLevel::Reproducible));
    assert(restoredResident->civilization.knowledge.confidence(
        TechniqueId::DesignatedSanitationArea)>=0.84);

    return 0;
}
