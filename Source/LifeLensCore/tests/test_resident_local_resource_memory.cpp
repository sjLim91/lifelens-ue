#include <cassert>
#include <string>

#include "lifelens/ResidentLocalResourceMemory.h"
#include "lifelens/Simulation.h"

using namespace lifelens;

static ObservedLocalResource observation(ResourceNodeId id,GridPos pos)
{
    return {id,MaterialKind::Wood,pos,2,ObservedResourceAbundance::Present};
}

int main()
{
    Character actor;
    actor.id=15;
    MemoryRecord important;
    important.what="real firsthand action success";
    important.minute=715;
    important.tags={"action_experience","Civilization:Gather:Wood","success"};
    actor.memory.add(important);
    ResidentLocalPerception view;
    view.actor=actor.id;
    view.minute=720;
    view.radiusCells=6;
    view.proximityOnly=true;
    for(ResourceNodeId id=1;id<=18;++id)
        view.nearbyResources.push_back(observation(id,{2,0}));
    assert(rememberResidentSensedLocalResources(actor,view)==18);
    assert(actor.memory.entries.size()==ResidentSensedResourceMemoryLimit+1);
    assert(actor.memory.entries.front().what==important.what);
    std::size_t inferred=0;
    for(const auto& memory:actor.memory.entries){
        if(memory.tags.empty() ||memory.tags[0]!="proximity_resource_inference")
            continue;
        ++inferred;
        assert(memory.source==MemorySource::Inferred);
        assert(!memory.witnessed);
        assert(memory.sourceCharacter==actor.id);
        assert(memory.where=="grid:2,0");
        assert(memory.what.find("접근 가능한지는 알 수 없다")!=std::string::npos);
    }
    assert(inferred==ResidentSensedResourceMemoryLimit);
    assert(rememberResidentSensedLocalResources(actor,view)==6);
    assert(actor.memory.entries.size()==ResidentSensedResourceMemoryLimit+1);
    const auto last=actor.memory.entries.back().tags[1];
    view.minute+=720;
    view.nearbyResources.resize(1);
    view.nearbyResources[0].nodeId=18;
    view.nearbyResources[0].position={3,1};
    assert(rememberResidentSensedLocalResources(actor,view)==0);
    assert(actor.memory.entries.back().tags[1]==last);
    assert(actor.memory.entries.back().where=="grid:3,1");
    assert(actor.memory.entries.back().minute==1440);
    // A neighbour's scan never writes into this mind.
    view.actor=99;
    assert(rememberResidentSensedLocalResources(actor,view)==0);
    assert(actor.memory.entries.size()==ResidentSensedResourceMemoryLimit+1);
    view.actor=15;view.radiusCells=9;
    assert(rememberResidentSensedLocalResources(actor,view)==0);

    Simulation sim(88123);
    sim.setupDemo();
    GridPos position{};
    const auto self=sim.world().characters.front().id;
    assert(sim.runtimePosition(self,position));
    sim.world().resourceNodes.clear();
    ResourceNode near;
    near.id=9001;
    near.material=MaterialKind::Water;
    near.pos=position;
    near.quantity=8;
    near.maxQuantity=8;
    sim.world().resourceNodes.push_back(near);
    sim.world().minute=720;
    sim.step();
    const auto& after=sim.world().characters.front();
    bool remembered=false;
    for(const auto& memory:after.memory.entries)
        if(memory.tags.size()==3
           &&memory.tags[0]=="proximity_resource_inference"
           &&memory.tags[1]=="node:9001"){
            remembered=true;
            assert(memory.source==MemorySource::Inferred);
            assert(!memory.witnessed);
        }
    assert(remembered);
    const auto firstCount=after.memory.entries.size();
    sim.step();
    assert(sim.world().characters.front().memory.entries.size()>=firstCount);
    return 0;
}
