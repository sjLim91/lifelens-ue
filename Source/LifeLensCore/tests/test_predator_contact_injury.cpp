#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

#include "lifelens/ResidentPredatorResponse.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

static std::uint64_t injectHungryContactTiger(Simulation& sim)
{
    auto& world=sim.world();
    assert(world.wildlifeEnabled);
    const CharacterId actor=world.characters.front().id;
    GridPos location{};
    assert(sim.runtimePosition(actor,location));
    const ChunkCoord chunk=chunkCoordForGrid(location);
    for(auto& pop:world.wildlifePopulations) pop.animals.clear();
    auto it=std::find_if(world.wildlifePopulations.begin(),
        world.wildlifePopulations.end(),[chunk](const WildlifePopulation& p){
            return p.habitat.coord==chunk;
        });
    assert(it!=world.wildlifePopulations.end());
    auto& population=*it;
    population.habitat.carryingCapacity[
        wildlifeSpeciesIndex(WildlifeSpecies::Tiger)]=2;

    // Find a real deterministic strike roll for a stable Core seed and hour.
    // No arbitrary force-hit flag is available in runtime or production.
    for(int attempt=0;attempt<128;++attempt){
        auto animal=wildlifeCreateAnimal(
            population,WildlifeSpecies::Tiger,true);
        animal.position={location.x+1,location.y};
        animal.hunger01=1.0;
        const double roll=wildlifeRoll(
            population,animal.id,world.minute/60,
            ResidentPredatorAttackDomain^actor);
        if(roll>=0.70) continue;
        const auto id=animal.id;
        population.animals.push_back(animal);
        return id;
    }
    assert(false && "unable to find a deterministic genuine strike roll");
    return 0;
}

static int attackMemoryCount(
    const Character& resident,std::uint64_t predatorId)
{
    int count=0;
    for(const auto& memory:resident.memory.entries){
        if(memory.tags.size()!=2
           ||memory.tags[0]!="predator_contact_injury"
           ||memory.tags[1]!=std::to_string(predatorId)) continue;
        ++count;
        assert(memory.source==MemorySource::DirectWitness);
        assert(memory.witnessed);
        assert(memory.sourceCharacter==resident.id);
        assert(memory.what.find("호랑이")!=std::string::npos);
    }
    return count;
}

int main()
{
    Simulation sim(77,0,4),replay(77,0,4);
    sim.setupNewGame();
    replay.setupNewGame();
    const CharacterId actor=sim.world().characters.front().id;
    GridPos first{},second{};
    assert(sim.runtimePosition(actor,first));
    assert(replay.runtimePosition(actor,second));
    assert(sameGridPos(first,second));
    const auto tigerId=injectHungryContactTiger(sim);
    assert(injectHungryContactTiger(replay)==tigerId);
    const auto contact=observeResidentPredatorContact(
        sim.world(),actor,first);
    assert(contact.found &&contact.animalId==tigerId);
    assert(contact.distance==1);
    assert(contact.hunger01==1.0 &&contact.roll<0.70);

    // Real Core minute transition: injury, witnessed memory and locomotion
    // are all sourced from the same actual animal and resident coordinates.
    const double before=sim.world().characters.front().health.injurySeverity;
    const int episodes=sim.world().characters.front().health.accidentEpisodes;
    sim.step();
    replay.step();
    const Character& injured=sim.world().characters.front();
    const Character& same=replay.world().characters.front();
    assert(injured.health.injurySeverity>before);
    assert(injured.health.injurySeverity==same.health.injurySeverity);
    assert(injured.health.accidentEpisodes==episodes+1);
    assert(injured.health.accidentEpisodes==same.health.accidentEpisodes);
    assert(injured.health.lastAccidentMinute==480);
    assert(injured.emotion.fear>0.0);
    assert(attackMemoryCount(injured,tigerId)==1);
    assert(attackMemoryCount(same,tigerId)==1);

    // The Core injury persists and impairs locomotion rather than merely
    // swapping a display animation without physical effect.
    assert(healthFunctionalCapacity01(injured.health)
        <healthFunctionalCapacity01(HealthState{}));
    const double after=injured.health.injurySeverity;
    sim.step();
    assert(sim.world().characters.front().health.injurySeverity==after);
    assert(attackMemoryCount(sim.world().characters.front(),tigerId)==1);

    // Observing a predator is not an injury. No strike without close, alive,
    // sufficiently hungry animals AND the physical hourly event boundary.
    Simulation witness(77,0,4);
    witness.setupNewGame();
    const auto contactId=injectHungryContactTiger(witness);
    auto& candidate=witness.world().wildlifePopulations;
    WildlifeAnimal* animal=nullptr;
    for(auto& pop:candidate)
        for(auto& a:pop.animals)
            if(a.id==contactId) animal=&a;
    assert(animal!=nullptr);
    const GridPos actorPos=first;
    auto& resident=witness.world().characters.front();
    animal->hunger01=0.20;
    assert(!applyResidentPredatorContactInjury(
        witness.world(),resident,actorPos));
    animal->hunger01=1.0;
    animal->position.x+=20;
    assert(!applyResidentPredatorContactInjury(
        witness.world(),resident,actorPos));
    animal->position=first;
    animal->alive=false;
    assert(!applyResidentPredatorContactInjury(
        witness.world(),resident,actorPos));
    animal->alive=true;
    witness.world().minute=481;
    assert(!applyResidentPredatorContactInjury(
        witness.world(),resident,actorPos));
    assert(resident.health.accidentEpisodes==0);
    assert(attackMemoryCount(resident,contactId)==0);

    // Exact same-world-state snapshot codec preserves injury severity and
    // source evidence, while a restored resident cannot magically recover.
    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(
        sim.captureSnapshot(),bytes,&error));
    SimulationStateSnapshot state;
    assert(decodeSimulationSnapshot(bytes,state,&error));
    Simulation restored(77,0,4);
    assert(restored.restoreSnapshot(state,&error));
    assert(restored.world().characters.front().health.injurySeverity==after);
    assert(restored.world().characters.front().health.accidentEpisodes
        ==episodes+1);
    assert(attackMemoryCount(restored.world().characters.front(),tigerId)==1);

    return 0;
}
