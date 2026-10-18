#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

#include "lifelens/ResidentPredatorResponse.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

static std::uint64_t injectPredator(
    Simulation& simulation,WildlifeSpecies species,GridPos position)
{
    for(auto& population:simulation.world().wildlifePopulations)
        population.animals.clear();
    const ChunkCoord chunk=chunkCoordForGrid(position);
    auto it=std::find_if(
        simulation.world().wildlifePopulations.begin(),
        simulation.world().wildlifePopulations.end(),
        [chunk](const WildlifePopulation& pop){
            return pop.habitat.coord==chunk;
        });
    assert(it!=simulation.world().wildlifePopulations.end());
    it->habitat.carryingCapacity[wildlifeSpeciesIndex(species)]=2;
    auto predator=wildlifeCreateAnimal(*it,species,true);
    predator.position=position;
    const auto animalId=predator.id;
    it->animals.push_back(predator);
    return animalId;
}

static const Character& residentById(const Simulation& simulation,CharacterId id)
{
    const auto& residents=simulation.world().characters;
    auto it=std::find_if(residents.begin(),residents.end(),
        [id](const Character& c){return c.id==id;});
    assert(it!=residents.end());
    return *it;
}

int main()
{
    // The observer needs an actual live predator in the same local region,
    // not a decorative tiger mesh or an inferred/witnessed rumour.
    Simulation sim(77,0,4),replay(77,0,4);
    sim.setupNewGame();
    replay.setupNewGame();
    const CharacterId actor=sim.world().characters.front().id;
    GridPos initial{},initialCopy{};
    assert(sim.runtimePosition(actor,initial));
    assert(replay.runtimePosition(actor,initialCopy));
    assert(sameGridPos(initial,initialCopy));

    // Construct a local predator at the same real Core grid position.
    const GridPos tigerPosition{initial.x+2,initial.y};
    const auto tigerId=injectPredator(sim,WildlifeSpecies::Tiger,tigerPosition);
    assert(tigerId==injectPredator(
        replay,WildlifeSpecies::Tiger,tigerPosition));
    const auto threat=observeResidentPredatorThreat(sim.world(),initial);
    assert(threat.detected);
    assert(threat.animalId==tigerId);
    assert(threat.species==WildlifeSpecies::Tiger);
    assert(threat.distance==2);

    GridPos firstEscape{};
    assert(chooseResidentPredatorEscapeStep(sim.world(),initial,firstEscape));
    assert(coreGroundTraversable(sim.world(),firstEscape));
    assert(manhattan(initial,firstEscape)==1);
    assert(residentPredatorDistance(firstEscape,tigerPosition)>2);

    // One minute is insufficient on some terrains, but Core navigation must
    // eventually move the actual resident away, subject to weather and health.
    std::int64_t maxDistance=2;
    for(int minute=0;minute<24;++minute){
        sim.step();
        replay.step();
        GridPos current{},other{};
        assert(sim.runtimePosition(actor,current));
        assert(replay.runtimePosition(actor,other));
        assert(sameGridPos(current,other));
        maxDistance=std::max(maxDistance,
            residentPredatorDistance(current,tigerPosition));
    }
    assert(maxDistance>2);
    assert(sim.world().minute==504);
    const Character& resident=residentById(sim,actor);
    const Character& other=residentById(replay,actor);
    assert(resident.emotion.fear>0.0);
    assert(resident.emotion.fear==other.emotion.fear);
    int firsthand=0;
    for(const MemoryRecord& memory:resident.memory.entries){
        if(memory.tags.size()!=2
           ||memory.tags[0]!="predator_encounter") continue;
        ++firsthand;
        assert(memory.tags[1]==std::to_string(tigerId));
        assert(memory.source==MemorySource::DirectWitness);
        assert(memory.witnessed);
        assert(memory.sourceCharacter==actor);
        assert(memory.minute>=480);
        assert(memory.what.find("호랑이")!=std::string::npos);
    }
    assert(firsthand==1);
    assert(other.memory.entries.size()==resident.memory.entries.size());
    assert(recentResidentPredatorSighting(resident,504,tigerId));
    assert(!recentResidentPredatorSighting(resident,541,tigerId));

    // The animal's real absence, death, or a wildlife-disabled legacy world
    // must not manufacture an encounter or move a resident.
    World wildlifeFree(77,0,4);
    wildlifeFree.wildlifeEnabled=true;
    assert(!observeResidentPredatorThreat(wildlifeFree,initial).detected);
    assert(!chooseResidentPredatorEscapeStep(
        wildlifeFree,initial,firstEscape));
    wildlifeFree.wildlifeEnabled=false;
    assert(!observeResidentPredatorThreat(wildlifeFree,initial).detected);

    // Direct evidence must survive the existing binary snapshot truth;
    // replaying the exact same saved state must reproduce locomotion.
    std::vector<std::uint8_t> binary,binaryCopy;
    std::string error;
    assert(encodeSimulationSnapshot(sim.captureSnapshot(),binary,&error));
    assert(encodeSimulationSnapshot(replay.captureSnapshot(),binaryCopy,&error));
    assert(binary==binaryCopy);
    SimulationStateSnapshot state;
    assert(decodeSimulationSnapshot(binary,state,&error));
    Simulation restored(77,0,4);
    assert(restored.restoreSnapshot(state,&error));
    assert(restored.world().characters.front().emotion.fear
        ==resident.emotion.fear);
    assert(restored.world().characters.front().memory.entries.size()
        ==resident.memory.entries.size());

    return 0;
}
