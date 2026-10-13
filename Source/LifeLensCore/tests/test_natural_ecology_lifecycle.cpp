#include "lifelens/NaturalEcologyValidation.h"
#include "lifelens/NaturalEcologyLifecycle.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include <cassert>
#include <numeric>
using namespace lifelens;
int main()
{
    World rewound{77}; rewound.materializeNaturalChunk({0,0}); rewound.minute=5;
    advanceNaturalEcology(rewound); assert(validNaturalEcology(rewound));
    World w{77}; w.minute=10000;
    auto& node=w.resourceNodes[2]; node.ecologyMinute=w.minute; node.ensureTrees();
    assert(node.trees[0].bornMinute!=node.trees[1].bornMinute);
    auto delayed=node; delayed.trees.clear(); delayed.ecologyMinute+=5000; delayed.ensureTrees();
    assert(delayed.trees[0].bornMinute==node.trees[0].bornMinute);
    auto rng=w.rng; Inventory inventory;
    const auto id=node.id;
    const int before=node.quantity;
    assert(node.harvest(25,inventory)==25);
    assert(node.quantity==before-25 && node.id==id);
    assert(node.trees[0].remainingWood==0 && node.trees[0].felledMinute==w.minute);
    assert(node.harvest(0,inventory)==0);
    assert(w.rng==rng);
    node.regenerateDay();
    assert(node.trees[0].generation==1 && node.trees[0].bornMinute==w.minute);
    int remaining=0; for(const auto& t:node.trees) remaining+=t.remainingWood;
    assert(remaining==node.quantity);
    SnowpackState snow{{0,0},0.0,0}; DynamicEnvironmentObservation weather;
    weather.simulationMinute=60; weather.airTemperatureC=-5;
    weather.precipitationType=PrecipitationType::Snow; weather.precipitationIntensity01=1;
    advanceSnowpack(snow,weather); const double depth=snow.depthMeters; assert(depth>0);
    advanceSnowpack(snow,weather); assert(snow.depthMeters==depth);
    weather.simulationMinute=120; weather.precipitationType=PrecipitationType::None;
    advanceSnowpack(snow,weather); assert(snow.depthMeters==depth);
    weather.simulationMinute=180; weather.airTemperatureC=10;
    advanceSnowpack(snow,weather); assert(snow.depthMeters<depth);
    Simulation a{77}; a.runMinutes(10);
    auto snapshot=a.captureSnapshot(); snapshot.world.materializeNaturalChunk({0,0});
    snapshot.world.snowpacks.push_back({{0,0},0.17,snapshot.world.minute});
    auto& wood=snapshot.world.resourceNodes[2]; wood.ecologyMinute=snapshot.world.minute; wood.ensureTrees();
    wood.harvest(24,inventory);
    std::vector<std::uint8_t> bytes; std::string error;
    assert(encodeSimulationSnapshot(snapshot,bytes,&error));
    SimulationStateSnapshot decoded; assert(decodeSimulationSnapshot(bytes,decoded,&error));
    assert(decoded.world.snowpacks[0].depthMeters==0.17);
    assert(decoded.world.resourceNodes[2].trees[0].felledMinute==wood.trees[0].felledMinute);
    Simulation b{77},c{77}; assert(b.restoreSnapshot(decoded,&error)); assert(c.restoreSnapshot(decoded,&error));
    b.runMinutes(60); c.runMinutes(60);
    std::vector<std::uint8_t> x,y;
    assert(encodeSimulationSnapshot(b.captureSnapshot(),x,&error));
    assert(encodeSimulationSnapshot(c.captureSnapshot(),y,&error)); assert(x==y);
    const char marker[]={'L','L','E','C','O','L','0','1'};
    const auto found=std::search(bytes.begin(),bytes.end(),marker,marker+8);
    std::vector<std::uint8_t> legacy(bytes.begin(),found);
    assert(decodeSimulationSnapshot(legacy,decoded,&error));
    assert(decoded.world.snowpacks.empty());
    assert(c.restoreSnapshot(decoded,&error)); c.runMinutes(2);
    bytes.pop_back(); assert(!decodeSimulationSnapshot(bytes,decoded,&error));
}
