#include "lifelens/Simulation.h"
#include "lifelens/WildlifeObservation.h"
#include <cassert>
#include <sstream>
#include <vector>

using namespace lifelens;

static WildlifePopulation testHabitat()
{
    const auto identity=makeWorldGenesisIdentity(4242001,0,4);
    WildlifeHabitat h;
    h.available=true;
    h.coord={0,0};
    h.forage01=0.90;
    h.shelter01=0.80;
    h.moisture01=0.85;
    h.carryingCapacity={{20,10,4,0,2}};
    auto p=startWildlifePopulation(identity,h);
    p.animals.clear();
    auto tiger=wildlifeCreateAnimal(p,WildlifeSpecies::Tiger,true);
    tiger.position={15,15};
    tiger.hunger01=0.8;
    p.animals.push_back(tiger);
    auto deer=wildlifeCreateAnimal(p,WildlifeSpecies::Deer,true);
    deer.position={17,17};
    p.animals.push_back(deer);
    return p;
}

int main()
{
    const std::vector<GridPos> humans={{16,15}};
    auto base=testHabitat();
    auto reacting=base;
    auto replay=base;
    assert(wildlifeBehavior(base,base.animals.front())
        !=WildlifeBehavior::Flee);
    assert(wildlifeBehavior(base,base.animals.front(),humans)
        ==WildlifeBehavior::Flee);
    assert(wildlifeBehavior(base,base.animals.back(),humans)
        ==WildlifeBehavior::Flee);
    const auto tigerId=base.animals.front().id;
    const auto humanBefore=wildlifeNearestResidentDistance(
        base.animals.front().position,humans);
    assert(humanBefore==1);
    const auto deerBefore=wildlifeNearestResidentDistance(
        base.animals.back().position,humans);

    assert(stepWildlifeHour(reacting,9,humans)>0);
    assert(stepWildlifeHour(replay,9,humans)>0);
    assert(reacting.animals.front().id==tigerId);
    assert(reacting.animals.front().position.x==replay.animals.front().position.x);
    assert(reacting.animals.front().position.y==replay.animals.front().position.y);
    assert(wildlifeNearestResidentDistance(
        reacting.animals.front().position,humans)>humanBefore);
    assert(wildlifeNearestResidentDistance(
        reacting.animals.back().position,humans)>deerBefore);

    // Even when hungry with deer in hunting range, a tiger close to humans
    // does not stage a "peaceful companion hunt" right through the humans.
    auto day=testHabitat(),dayCopy=testHabitat();
    const auto tigerHunger=day.animals.front().hunger01;
    assert(stepWildlifeDay(day,1,humans).advanced);
    assert(stepWildlifeDay(dayCopy,1,humans).advanced);
    assert(day.animals.size()==dayCopy.animals.size());
    assert(day.animals.front().id==dayCopy.animals.front().id);
    assert(day.animals.front().position.x==dayCopy.animals.front().position.x);
    assert(day.animals.front().position.y==dayCopy.animals.front().position.y);
    assert(wildlifeNearestResidentDistance(
        day.animals.front().position,humans)>humanBefore);
    assert(day.animals.front().hunger01>tigerHunger);
    // No false deer kill / meat gain because the wild tiger fled a person.
    assert(day.animals.size()==2);

    // The original isolated wildlife contract remains exactly unchanged
    // when there is no human runtime data (legacy/standalone fixtures).
    auto original=testHabitat(),emptyInput=testHabitat();
    stepWildlifeHour(original,5);
    stepWildlifeHour(emptyInput,5,{});
    assert(original.animals.size()==emptyInput.animals.size());
    for(std::size_t i=0;i<original.animals.size();++i){
        assert(original.animals[i].position.x==emptyInput.animals[i].position.x);
        assert(original.animals[i].position.y==emptyInput.animals[i].position.y);
    }

    // The observer must show the same Flee state as the Core response,
    // not invent attacks, wounds or ownership by a resident.
    World observational(4242001,0,4);
    observational.wildlifeEnabled=true;
    observational.wildlifePopulations.push_back(base);
    std::ostringstream stream;
    appendWildlifeObservation(stream,observational,{0,0},0,humans);
    assert(stream.str().find("\"species\":\"Tiger\"")!=std::string::npos);
    assert(stream.str().find("\"behavior\":\"Flee\"")!=std::string::npos);

    // Production NEW GAME founders spawn in their own Core start region.
    // Wildlife must be positioned away from their four original locations.
    Simulation simulation(77,0,4);
    simulation.setupNewGame();
    const auto initialResidents=simulation.livingResidentPositions();
    assert(initialResidents.size()==4);
    for(const auto& p:simulation.world().wildlifePopulations)
        for(const auto& a:p.animals)
            assert(!wildlifeResidentClose(a,initialResidents));

    // Inject an independent tiger into a v4 runtime to verify the hourly
    // Simulation->wildlife human-position handoff, not just the helper.
    const ChunkCoord start=simulation.world().initialStartRegionCoord;
    auto it=std::find_if(simulation.world().wildlifePopulations.begin(),
                         simulation.world().wildlifePopulations.end(),
                         [start](const WildlifePopulation& p){
                             return p.habitat.coord==start;
                         });
    assert(it!=simulation.world().wildlifePopulations.end());
    it->habitat.carryingCapacity[wildlifeSpeciesIndex(WildlifeSpecies::Tiger)]=2;
    auto nearTiger=wildlifeCreateAnimal(*it,WildlifeSpecies::Tiger,true);
    nearTiger.position={initialResidents[0].x+1,initialResidents[0].y};
    nearTiger.hunger01=0.1;
    const auto insertedId=nearTiger.id;
    it->animals.push_back(nearTiger);
    simulation.setExternalPhysicalExecution(true);
    simulation.runMinutes(60);
    const auto currentResidents=simulation.livingResidentPositions();
    const auto pop=std::find_if(
        simulation.world().wildlifePopulations.begin(),
        simulation.world().wildlifePopulations.end(),
        [start](const WildlifePopulation& p){return p.habitat.coord==start;});
    assert(pop!=simulation.world().wildlifePopulations.end());
    const auto tiger=std::find_if(pop->animals.begin(),pop->animals.end(),
        [insertedId](const WildlifeAnimal& a){return a.id==insertedId;});
    assert(tiger!=pop->animals.end() &&tiger->alive);
    assert(wildlifeNearestResidentDistance(
        tiger->position,currentResidents)>1);
    assert(simulation.world().minute==540);
    return 0;
}
