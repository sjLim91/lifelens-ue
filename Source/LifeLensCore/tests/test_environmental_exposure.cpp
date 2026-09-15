#include <cassert>
#include <string>

#include "lifelens/EnvironmentalExposure.h"
#include "lifelens/Simulation.h"

using namespace lifelens;

namespace {

bool hasTag(const MemoryRecord& memory,const std::string& tag)
{
    return std::find(memory.tags.begin(),memory.tags.end(),tag)!=memory.tags.end();
}

Simulation makeUrgentToiletSimulation(std::uint64_t seed)
{
    Simulation simulation(seed);
    simulation.setupNewGame();
    simulation.setExternalPhysicalExecution(true);
    simulation.world().minute=481;
    for(auto& resident:simulation.world().characters){
        resident.needs={0.01,0.01,0.01,0.01,0.01};
    }
    simulation.world().characters.front().needs.bladder=0.99;
    simulation.runMinutes(5);
    return simulation;
}

} // namespace

int main()
{
    // Direct exposure creates an immediate hygiene burden, emotional discomfort,
    // and a location-specific sanitation memory without spamming duplicates.
    Character exposed;
    exposed.id=77;
    exposed.name="ExposureResident";
    exposed.needs.hygiene=0.10;

    EnvironmentalResidueField field;
    field.deposit(EnvironmentalResidueKind::HumanWaste,{2,-1},77,100,2.0,0.80,3);

    const double hygieneBefore=exposed.needs.hygiene;
    const EnvironmentalExposureResult first=perceiveEnvironmentalContamination(
        exposed,field,{2,-1},100);
    assert(first.perceived);
    assert(first.exposure>=0.79);
    assert(first.hygieneBurden>0.0);
    assert(first.memoryRecorded);
    assert(exposed.needs.hygiene>hygieneBefore);
    assert(exposed.emotion.anxiety>0.0);
    assert(exposed.memory.entries.size()==1);
    assert(hasTag(exposed.memory.entries.front(),"contamination"));
    assert(hasTag(exposed.memory.entries.front(),"avoidance"));
    assert(exposed.memory.entries.front().where==environmentalGridLabel({2,-1}));

    const EnvironmentalExposureResult repeated=perceiveEnvironmentalContamination(
        exposed,field,{2,-1},120);
    assert(repeated.perceived);
    assert(!repeated.memoryRecorded);
    assert(exposed.memory.entries.size()==1);

    // A resident's deterministic outdoor choice changes once the original site
    // becomes dirty and is remembered as unpleasant.
    Character chooser;
    chooser.id=91;
    chooser.name="Chooser";
    EnvironmentalResidueField choiceField;
    const GridPos original=chooseLowExposureOutdoorReliefPosition(
        424242,chooser,choiceField,200);
    choiceField.deposit(
        EnvironmentalResidueKind::HumanWaste,original,chooser.id,200,3.0,1.0,3);
    assert(perceiveEnvironmentalContamination(
        chooser,choiceField,original,200).memoryRecorded);
    const GridPos avoided=chooseLowExposureOutdoorReliefPosition(
        424242,chooser,choiceField,201);
    assert(avoided.x!=original.x || avoided.y!=original.y);
    assert(outdoorReliefAvoidanceScore(chooser,choiceField,avoided,201)
        < outdoorReliefAvoidanceScore(chooser,choiceField,original,201));

    // Integration: external world completion deposits residue at the real ACK
    // location, Core remembers it, and the next recommendation avoids it.
    Simulation simulation=makeUrgentToiletSimulation(9292);
    const CharacterId actorId=simulation.world().characters.front().id;
    GridPos firstRecommended{};
    assert(simulation.recommendedOutdoorReliefPosition(actorId,firstRecommended));
    assert(simulation.completeExternalPhysicalAction(actorId,true,firstRecommended));

    const Character& actor=simulation.world().characters.front();
    bool foundSanitationMemory=false;
    for(const auto& memory:actor.memory.entries){
        if(memory.where==environmentalGridLabel(firstRecommended)
           && hasTag(memory,"sanitation")){
            foundSanitationMemory=true;
            break;
        }
    }
    assert(foundSanitationMemory);

    GridPos nextRecommended{};
    assert(simulation.recommendedOutdoorReliefPosition(actorId,nextRecommended));
    assert(nextRecommended.x!=firstRecommended.x || nextRecommended.y!=firstRecommended.y);

    // Authoritative residue + memory + position already live in the Core snapshot,
    // so the same avoidance result must survive restore.
    const SimulationStateSnapshot saved=simulation.captureSnapshot();
    Simulation restored(1);
    std::string error;
    assert(restored.restoreSnapshot(saved,&error));
    assert(error.empty());
    GridPos restoredRecommendation{};
    assert(restored.recommendedOutdoorReliefPosition(actorId,restoredRecommendation));
    assert(restoredRecommendation.x==nextRecommended.x);
    assert(restoredRecommendation.y==nextRecommended.y);

    return 0;
}
