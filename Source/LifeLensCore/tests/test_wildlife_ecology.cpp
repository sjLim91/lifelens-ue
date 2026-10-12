#include "lifelens/WildlifeEcology.h"
#include <cassert>
#include <cstdint>
#include <limits>

using namespace lifelens;

static void samePopulation(
    const WildlifePopulation& a,const WildlifePopulation& b)
{
    assert(a.worldSeed==b.worldSeed);
    assert(a.generationVersion==b.generationVersion);
    assert(a.dayIndex==b.dayIndex);
    assert(a.nextSerial==b.nextSerial);
    assert(a.animals.size()==b.animals.size());
    for(std::size_t i=0;i<a.animals.size();++i){
        const auto& l=a.animals[i];
        const auto& r=b.animals[i];
        assert(l.id==r.id && l.species==r.species);
        assert(l.sex==r.sex && l.position.x==r.position.x
            && l.position.y==r.position.y);
        assert(l.ageDays==r.ageDays && l.hunger01==r.hunger01);
        assert(l.alive==r.alive);
    }
}

int main()
{
    const auto identity=makeWorldGenesisIdentity(4242001,0,3);
    const auto alternativePopulationSeed=makeWorldGenesisIdentity(
        4242001,999999,3);
    const auto habitat=deriveWildlifeHabitat(identity,{0,0});
    const auto again=deriveWildlifeHabitat(identity,{0,0});
    assert(habitat.available && again.available);
    assert(!habitat.geographicallyConditioned);
    assert(!habitat.regionalWeatherApplied);
    assert(habitat.freshwaterAvailable==again.freshwaterAvailable);
    assert(habitat.carryingCapacity==again.carryingCapacity);
    assert(habitat.forage01==again.forage01);
    assert(habitat.shelter01==again.shelter01);
    assert(deriveWildlifeHabitat(
        alternativePopulationSeed,{0,0}).carryingCapacity
        ==habitat.carryingCapacity);

    // An ocean is not automatically a fresh-water fish habitat.
    bool foundOcean=false;
    for(int x=-12;x<=12 && !foundOcean;++x){
        for(int y=-12;y<=12 && !foundOcean;++y){
            const ChunkCoord c{x,y};
            if(deriveMacroSurfaceFacts(identity,c).surfaceClass
                !=MacroSurfaceClass::Ocean) continue;
            const auto ocean=deriveWildlifeHabitat(identity,c);
            assert(ocean.available);
            for(int capacity:ocean.carryingCapacity) assert(capacity==0);
            foundOcean=true;
        }
    }
    assert(foundOcean);

    // This explicit *procedural* fixture represents potential local wildlife.
    // It is not a real geographic animal sighting, resident hunt, or inventory.
    WildlifeHabitat eco{};
    eco.available=true;
    eco.coord={0,0};
    eco.freshwaterAvailable=true;
    eco.forage01=0.93;
    eco.shelter01=0.83;
    eco.moisture01=0.81;
    eco.carryingCapacity={{22,10,3,22}};

    auto a=startWildlifePopulation(identity,eco);
    auto b=startWildlifePopulation(identity,eco);
    auto otherPopulationSeed=startWildlifePopulation(alternativePopulationSeed,eco);
    samePopulation(a,b);
    samePopulation(a,otherPopulationSeed);
    assert(!a.animals.empty());
    assert(a.animals.front().alive);
    assert(a.animals.front().id!=a.animals.back().id);
    const auto before=a;
    assert(!stepWildlifeDay(a,0).advanced);
    assert(!stepWildlifeDay(a,2).advanced);
    samePopulation(a,b);
    const auto first=stepWildlifeDay(a,1);
    const auto firstCopy=stepWildlifeDay(b,1);
    assert(first.advanced && firstCopy.advanced);
    samePopulation(a,b);
    assert(!stepWildlifeDay(a,1).advanced); // no duplicate tick
    samePopulation(a,b);

    std::int64_t births=first.births;
    std::int64_t deaths=first.naturalDeaths+first.predationDeaths;
    for(int day=2;day<=1000;++day){
        auto ra=stepWildlifeDay(a,day);
        auto rb=stepWildlifeDay(b,day);
        assert(ra.advanced && rb.advanced);
        assert(ra.births==rb.births);
        assert(ra.naturalDeaths==rb.naturalDeaths);
        assert(ra.predationDeaths==rb.predationDeaths);
        births+=ra.births;
        deaths+=ra.naturalDeaths+ra.predationDeaths;
        samePopulation(a,b);
        for(std::size_t s=0;s<WildlifeSpeciesCount;++s){
            assert(wildlifeAliveCount(a,static_cast<WildlifeSpecies>(s))
                <=eco.carryingCapacity[s]);
        }
        for(const auto& animal:a.animals){
            assert(animal.alive && animal.hunger01>=0 && animal.hunger01<1);
            assert(animal.position.x>=0 && animal.position.x<WildlifeCellsPerChunk);
            assert(animal.position.y>=0 && animal.position.y<WildlifeCellsPerChunk);
        }
        assert(a.animals.size()<=static_cast<std::size_t>(22+10+3+22));
    }
    assert(births>0);
    assert(deaths>0);
    assert(a.animals.size()<=before.animals.size()+static_cast<std::size_t>(births));
    // Explicit in-memory snapshot clone continues with identical identity.
    auto resumed=a;
    auto replay=a;
    for(int day=1001;day<=1100;++day){
        stepWildlifeDay(resumed,day);
        stepWildlifeDay(replay,day);
    }
    samePopulation(resumed,replay);

    WildlifeHabitat unavailable{};
    auto noWorld=startWildlifePopulation(identity,unavailable);
    assert(noWorld.animals.empty());
    assert(!stepWildlifeDay(noWorld,1).advanced);

    // The standalone ECO-A1 model must never create meat without a verified
    // hunt/carcass/inventory pipeline in the future ECO-B phase.
    static_assert(WildlifeSpeciesCount==4);
    return 0;
}
