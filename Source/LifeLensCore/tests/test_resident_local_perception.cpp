#include <cassert>
#include <climits>
#include <cstdint>
#include <vector>

#include "lifelens/ResidentLocalPerception.h"
#include "lifelens/Simulation.h"

using namespace lifelens;

int main()
{
    World world(8877123);
    world.characters.clear();
    world.resourceNodes.clear();
    world.minute=333;
    Character a; a.id=10; a.alive=true;
    Character b; b.id=20; b.alive=true;
    world.characters={a,b};

    const auto makeNode=[](ResourceNodeId id,MaterialKind material,
                           GridPos pos,int quantity){
        ResourceNode node;
        node.id=id;
        node.material=material;
        node.pos=pos;
        node.quantity=quantity;
        node.maxQuantity=quantity;
        return node;
    };
    world.resourceNodes={
        makeNode(10,MaterialKind::Water,{2,1},3),
        makeNode(11,MaterialKind::Stone,{1,1},1),
        makeNode(12,MaterialKind::PlantFood,{4,2},10),
        makeNode(13,MaterialKind::Wood,{11,11},100),
        makeNode(14,MaterialKind::Stone,{0,1},0),
        makeNode(15,MaterialKind::Unknown,{0,0},8),
        makeNode(16,MaterialKind::Clay,{2,0},2)
    };
    const auto beforeWorld=world.resourceNodes;
    const auto beforeRng=world.rng;
    const auto seen=observeLocalResourceProximity(world,10,{0,0},3);
    assert(seen.actor==10 &&seen.minute==333);
    assert(seen.proximityOnly &&seen.radiusCells==3);
    assert(seen.nearbyResources.size()==3);
    assert(seen.nearbyResources[0].nodeId==11);
    assert(seen.nearbyResources[0].distanceCells==2);
    assert(seen.nearbyResources[0].abundance==ObservedResourceAbundance::Scarce);
    assert(seen.nearbyResources[1].nodeId==16);
    assert(seen.nearbyResources[2].nodeId==10);
    assert(seen.nearbyResources[2].abundance==ObservedResourceAbundance::Present);
    for(const auto& entry:seen.nearbyResources)
        assert(entry.nodeId!=12 &&entry.nodeId!=13 &&entry.nodeId!=14
               &&entry.nodeId!=15);
    const auto replay=observeLocalResourceProximity(world,10,{0,0},3);
    assert(replay.nearbyResources.size()==seen.nearbyResources.size());
    for(std::size_t i=0;i<seen.nearbyResources.size();++i)
        assert(seen.nearbyResources[i].nodeId==replay.nearbyResources[i].nodeId);
    assert(world.rng==beforeRng);
    assert(world.resourceNodes.size()==beforeWorld.size());
    for(std::size_t i=0;i<beforeWorld.size();++i)
        assert(world.resourceNodes[i].quantity==beforeWorld[i].quantity);
    assert(observeLocalResourceProximity(world,10,{0,0},100).radiusCells==8);
    assert(observeLocalResourceProximity(world,10,{0,0},3,1)
               .nearbyResources.size()==1);
    assert(observeLocalResourceProximity(world,10,{0,0},0).actor==0);
    assert(observeLocalResourceProximity(world,0,{0,0},3).actor==0);
    assert(observeLocalResourceProximity(world,999,{0,0},3).actor==0);
    world.characters[0].alive=false;
    assert(observeLocalResourceProximity(world,10,{0,0},3).actor==0);
    world.characters[0].alive=true;
    assert(observeLocalResourceProximity(world,20,{11,11},2)
               .nearbyResources.front().nodeId==13);
    const auto extreme=observeLocalResourceProximity(
        world,20,{INT_MIN,INT_MAX},8);
    assert(extreme.nearbyResources.empty()); // no signed int overflow

    Simulation sim(8877123);
    sim.setupDemo();
    auto& actual=sim.world().resourceNodes;
    actual.clear();
    GridPos pos{};
    assert(sim.runtimePosition(1,pos));
    actual.push_back(makeNode(122,MaterialKind::Stone,pos,7));
    actual.push_back(makeNode(123,MaterialKind::Water,
        {pos.x+9,pos.y},7));
    const auto observed=sim.observeResidentLocalPerception(1,3);
    assert(observed.actor==1 &&observed.nearbyResources.size()==1);
    assert(observed.nearbyResources[0].nodeId==122);
    assert(sim.observeResidentLocalPerception(999,3).actor==0);

    const auto current=sim.observeCognitiveRequest(
        1,CognitiveTriggerKind::Reflection);
    assert(current.actor==1 &&current.localProximityObserved);
    assert(current.localRadiusCells==6);
    assert(current.localObserverPosition.x==pos.x);
    assert(current.localObserverPosition.y==pos.y);
    assert(current.localResources.size()==1);
    assert(current.localResources[0].nodeId==122);
    const auto legitimateFingerprint=cognitiveRequestFingerprint(current);
    auto fake=current;
    fake.localResources[0].nodeId=666;
    assert(cognitiveRequestFingerprint(fake)!=legitimateFingerprint);
    CognitiveProposal harmless;
    harmless.actor=1;
    harmless.intent=CognitiveIntentKind::ImproveWaterSecurity;
    harmless.priority=0.80;
    assert(!sim.acceptCognitiveProposal(
        fake,harmless,"local-mind","test-forged-proximity"));
    assert(sim.acceptCognitiveProposal(
        current,harmless,"local-mind","test-verified-proximity"));
    // Once the Core node is removed in the SAME minute, the observed
    // receipt changes and any unsubmitted stale response must be rejected.
    const auto refreshed=sim.observeCognitiveRequest(
        1,CognitiveTriggerKind::Reflection);
    assert(refreshed.localResources.size()==1);
    actual.clear();
    const auto depleted=sim.observeCognitiveRequest(
        1,CognitiveTriggerKind::Reflection);
    assert(depleted.localProximityObserved);
    assert(depleted.localResources.empty());
    assert(cognitiveRequestFingerprint(depleted)!=legitimateFingerprint);
    return 0;
}
