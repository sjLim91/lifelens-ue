#include "lifelens/WildlifePopulationCodec.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

using namespace lifelens;

static void equalState(
    const WildlifePopulation& a,const WildlifePopulation& b)
{
    assert(a.worldSeed==b.worldSeed);
    assert(a.generationVersion==b.generationVersion);
    assert(a.dayIndex==b.dayIndex);
    assert(a.nextSerial==b.nextSerial);
    assert(a.habitat.available==b.habitat.available);
    assert(a.habitat.freshwaterAvailable==b.habitat.freshwaterAvailable);
    assert(a.habitat.geographicallyConditioned==b.habitat.geographicallyConditioned);
    assert(a.habitat.regionalWeatherApplied==b.habitat.regionalWeatherApplied);
    assert(a.habitat.coord==b.habitat.coord);
    assert(a.habitat.forage01==b.habitat.forage01);
    assert(a.habitat.shelter01==b.habitat.shelter01);
    assert(a.habitat.moisture01==b.habitat.moisture01);
    assert(a.habitat.carryingCapacity==b.habitat.carryingCapacity);
    assert(a.animals.size()==b.animals.size());
    for(std::size_t i=0;i<a.animals.size();++i){
        const auto& x=a.animals[i];
        const auto& y=b.animals[i];
        assert(x.id==y.id && x.species==y.species && x.sex==y.sex);
        assert(x.alive==y.alive && x.ageDays==y.ageDays);
        assert(x.position.x==y.position.x && x.position.y==y.position.y);
        assert(x.hunger01==y.hunger01);
    }
}

static WildlifePopulation create()
{
    const auto identity=makeWorldGenesisIdentity(874213954,0,3);
    WildlifeHabitat habitat;
    habitat.available=true;
    habitat.coord={4,-2};
    habitat.freshwaterAvailable=true;
    habitat.forage01=0.90;
    habitat.shelter01=0.80;
    habitat.moisture01=0.72;
    habitat.carryingCapacity={{24,11,3,28}};
    return startWildlifePopulation(identity,habitat);
}

static void invalidEncode(WildlifePopulation state)
{
    std::vector<std::uint8_t> output={1,2,3};
    std::string reason;
    assert(!encodeWildlifePopulation(state,output,&reason));
    assert(!reason.empty());
    assert((output==std::vector<std::uint8_t>{1,2,3}));
}

int main()
{
    auto original=create();
    assert(validWildlifePopulation(original));
    std::vector<std::uint8_t> bytes,otherBytes;
    assert(encodeWildlifePopulation(original,bytes));
    assert(bytes.size()<WildlifeMaxBinaryBytes);
    assert(encodeWildlifePopulation(original,otherBytes));
    assert(bytes==otherBytes);
    WildlifePopulation restored{};
    assert(decodeWildlifePopulation(bytes,restored));
    equalState(original,restored);
    assert(stepWildlifeDay(original,1).advanced);
    assert(stepWildlifeDay(restored,1).advanced);
    equalState(original,restored);

    // Full long-run checkpoint/restart: same entities and decisions.
    for(int day=2;day<=1000;++day){
        assert(stepWildlifeDay(original,day).advanced);
        if(day%17==0){
            std::vector<std::uint8_t> saved;
            assert(encodeWildlifePopulation(restored,saved));
            WildlifePopulation checkpoint;
            assert(decodeWildlifePopulation(saved,checkpoint));
            restored=std::move(checkpoint);
        }
        assert(stepWildlifeDay(restored,day).advanced);
        equalState(original,restored);
    }
    const auto previous=restored;
    std::vector<std::uint8_t> snapshot;
    assert(encodeWildlifePopulation(restored,snapshot));
    assert(decodeWildlifePopulation(snapshot,restored));
    equalState(previous,restored);

    // Errors are atomic: not even the time/seed/serial can be partially read
    // over a live population when decode rejects bytes.
    auto reject=[&](std::vector<std::uint8_t> bad){
        WildlifePopulation output=previous;
        std::string error;
        assert(!decodeWildlifePopulation(bad,output,&error));
        assert(!error.empty());
        equalState(previous,output);
    };
    reject({});
    reject({1,2,3});
    auto tampered=snapshot;
    tampered[0]='X';
    reject(tampered);
    tampered=snapshot;
    tampered[4]=42; // binary schema version
    reject(tampered);
    tampered=snapshot;
    tampered[tampered.size()/2]^=1U;
    reject(tampered);
    tampered=snapshot;
    tampered.pop_back();
    reject(tampered);
    tampered=snapshot;
    tampered.push_back(0);
    reject(tampered);
    tampered.assign(WildlifeMaxBinaryBytes+1,0);
    reject(tampered);

    auto wrong=previous;
    wrong.worldSeed=0;
    invalidEncode(wrong);
    wrong=previous;
    wrong.generationVersion=0;
    invalidEncode(wrong);
    wrong=previous;
    wrong.dayIndex=-1;
    invalidEncode(wrong);
    wrong=previous;
    wrong.nextSerial=0;
    invalidEncode(wrong);
    wrong=previous;
    wrong.habitat.geographicallyConditioned=true;
    invalidEncode(wrong);
    wrong=previous;
    wrong.habitat.regionalWeatherApplied=true;
    invalidEncode(wrong);
    wrong=previous;
    wrong.habitat.forage01=std::numeric_limits<double>::quiet_NaN();
    invalidEncode(wrong);
    wrong=previous;
    wrong.habitat.carryingCapacity[0]=-1;
    invalidEncode(wrong);
    wrong=previous;
    wrong.habitat.carryingCapacity[0]=257;
    invalidEncode(wrong);
    wrong=previous;
    wrong.habitat.freshwaterAvailable=false;
    invalidEncode(wrong);
    wrong=previous;
    wrong.habitat.available=false;
    invalidEncode(wrong);
    wrong=previous;
    wrong.habitat.coord.x=std::numeric_limits<int>::max();
    invalidEncode(wrong);

    if(!previous.animals.empty()){
        wrong=previous;
        wrong.animals.push_back(wrong.animals.front());
        invalidEncode(wrong);
        wrong=previous;
        wrong.animals.front().species=static_cast<WildlifeSpecies>(255);
        invalidEncode(wrong);
        wrong=previous;
        wrong.animals.front().hunger01=std::numeric_limits<double>::infinity();
        invalidEncode(wrong);
        wrong=previous;
        wrong.animals.front().position.x=4000000;
        invalidEncode(wrong);
    }

    // Empty but valid, with explicit unavailable status, is round-trippable.
    auto empty=create();
    empty.habitat.available=false;
    empty.habitat.carryingCapacity={{0,0,0,0}};
    empty.animals.clear();
    assert(encodeWildlifePopulation(empty,bytes));
    assert(decodeWildlifePopulation(bytes,restored));
    equalState(empty,restored);
    assert(!stepWildlifeDay(restored,1).advanced);
    return 0;
}
