#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>

#include "lifelens/ResidentKnownResourceMemory.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

static ResourceNode node(ResourceNodeId id,GridPos position,int quantity=9)
{
    ResourceNode result;
    result.id=id;
    result.pos=position;
    result.material=MaterialKind::Wood;
    result.quantity=result.maxQuantity=quantity;
    return result;
}

int main()
{
    // Controlled sensor/codec fixtures, not natural recovery acceptance.
    World world;
    world.minute=100;
    Character resident;
    resident.id=1;
    world.characters.push_back(resident);
    resident.id=2;
    world.characters.push_back(resident);
    auto& actor=world.characters.front();
    const GridPos actual{-22,31};
    world.resourceNodes={node(100,actual),node(101,{-22,37}),node(102,{-22,38})};
    const auto unchangedStock=world.resourceNodes.front().quantity;
    assert(rememberResidentKnownLocalResources(world,actor,actual)==2);
    assert(world.resourceNodes.front().quantity==unchangedStock);
    auto known=residentKnownResources(actor,world.minute);
    assert(known.size()==2 &&known[0].nodeId==100 &&known[1].nodeId==101);
    assert(known[0].observedQuantity==9 &&known[0].position.x==-22
        &&known[0].position.y==31 &&known[0].minute==100);
    assert(known[0].confidence>0.0 &&known[0].confidence<0.60);
    assert(residentKnownResources(world.characters[1],world.minute).empty());
    const MemoryRecord original=actor.memory.entries.front();
    assert(original.source==MemorySource::Inferred &&!original.witnessed);

    // Recalling an out-of-sight site never refreshes its state from World.
    world.resourceNodes.front().quantity=0;
    ++world.minute;
    assert(rememberResidentKnownLocalResources(world,actor,{900,900})==0);
    known=residentKnownResources(actor,world.minute);
    assert(known.front().observedQuantity==9 &&known.front().minute==100);
    world.resourceNodes.front().quantity=4;
    assert(rememberResidentKnownLocalResources(world,actor,actual)==0);
    known=residentKnownResources(actor,world.minute);
    assert(known.front().observedQuantity==4 &&known.front().minute==101);

    ResidentKnownResource parsed;
    assert(readResidentKnownResource(original,actor.id,100,parsed));
    assert(readResidentKnownResource(original,actor.id,1540,parsed));
    assert(!readResidentKnownResource(original,actor.id,1541,parsed));
    assert(!readResidentKnownResource(original,actor.id,99,parsed));
    assert(!readResidentKnownResource(original,2,100,parsed));
    for(int invalid=0;invalid<15;++invalid){
        auto corrupt=original;
        switch(invalid){
            case 0: corrupt.who=2; break;
            case 1: corrupt.sourceCharacter=2; break;
            case 2: corrupt.source=MemorySource::ToldByOther; break;
            case 3: corrupt.witnessed=true; break;
            case 4: corrupt.confidence=std::numeric_limits<double>::quiet_NaN(); break;
            case 5: corrupt.decayPerDay=std::numeric_limits<double>::infinity(); break;
            case 6: corrupt.tags[1]="node:18446744073709551616"; break;
            case 7: corrupt.tags[2]="material:99"; break;
            case 8: corrupt.tags[3]="quantity:0"; break;
            case 9: corrupt.where="grid:2147483648,0"; break;
            case 10: corrupt.where="grid:0,0,0"; break;
            case 11: corrupt.tags.push_back("model-proposal"); break;
            case 12: corrupt.what="model thinks a resource exists"; break;
            case 13: corrupt.tags[1]="node:00100"; break;
            case 14: corrupt.confidence=-0.01; break;
        }
        assert(!readResidentKnownResource(corrupt,actor.id,100,parsed));
    }

    MemoryRecord important;
    important.what="actual private outcome remains untouched";
    important.tags={"action_experience","Civilization:Gather:Wood","failure"};
    actor.memory.add(important);
    // Observe genuinely local distinct sites in successive fixture positions;
    // bounded retention does not delete independent outcome records.
    for(int i=0;i<20;++i){
        ++world.minute;
        const GridPos position{i*20,0};
        world.resourceNodes={node(200+i,position)};
        assert(rememberResidentKnownLocalResources(world,actor,position)==1);
    }
    known=residentKnownResources(actor,world.minute);
    assert(known.size()==ResidentKnownResourceMemoryLimit);
    assert(known.front().nodeId==219 &&known.back().nodeId==208);
    assert(actor.memory.entries.size()==ResidentKnownResourceMemoryLimit+1);
    assert(std::any_of(actor.memory.entries.begin(),actor.memory.entries.end(),
        [&](const MemoryRecord& memory){return memory.what==important.what;}));
    // Four nearest facts remain the observation cap, regardless of density.
    world.resourceNodes.clear();
    for(int i=0;i<9;++i) world.resourceNodes.push_back(node(300+i,{0,0}));
    ++world.minute;
    assert(rememberResidentKnownLocalResources(world,actor,{0,0})==4);
    known=residentKnownResources(actor,world.minute);
    for(const auto& memory:known) assert(memory.nodeId<304);

    // This schema uses the existing validated MemoryRecord codec; no binary
    // format, legacy record interpretation or persistent Core field changes.
    Simulation simulation(42);
    simulation.setupNewGame();
    auto& savedActor=simulation.world().characters.front();
    auto savedMemory=original;
    savedMemory.who=savedMemory.sourceCharacter=savedActor.id;
    savedMemory.minute=simulation.world().minute;
    savedActor.memory.add(savedMemory);
    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(simulation.captureSnapshot(),bytes,&error));
    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    std::vector<std::uint8_t> reencoded;
    assert(encodeSimulationSnapshot(decoded,reencoded,&error));
    assert(reencoded==bytes);
    const auto restored=residentKnownResources(decoded.world.characters.front(),decoded.world.minute);
    assert(restored.size()==1 &&restored.front().nodeId==100
        &&restored.front().observedQuantity==9 &&restored.front().position.x==-22);
    std::cout<<"KNOWN_RESOURCE_MEMORY provenance=own bounded=12 sensor=6/4"
        <<" remote_stock=not_refreshed horizon=1440 codec=equal\n";
}
