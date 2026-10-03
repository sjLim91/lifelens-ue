#include <cassert>
#include <iostream>
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

static ConstructedFacility facility(FacilityId id,FacilityKind kind)
{
    ConstructedFacility f; f.id=id; f.kind=kind;
    f.state=FacilityState::Operational; f.active=true; f.durability=1;
    return f;
}

int main()
{
    Simulation sim(4242001); sim.setupNewGame();
    assert(sim.observeCivilizationWorld().era.id=="NaturalSurvival");
    auto initial=sim.captureSnapshot(); std::string error;
    SimulationStateSnapshot decoded;
    std::vector<std::uint8_t> bytes;
    assert(encodeSimulationSnapshot(initial,bytes,&error));
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    Simulation restored(1); assert(restored.restoreSnapshot(decoded,&error));
    assert(restored.observeCivilizationWorld().era.id==sim.observeCivilizationWorld().era.id);
    auto& world=sim.world(); auto& resident=world.characters.front();
    const auto era=[&](){return sim.observeCivilizationWorld().era.id;};
    resident.civilization.knowledge.learn(TechniqueId::SharpFlake,KnowledgeLevel::Observed,0.5);
    assert(era()=="NaturalSurvival");
    world.facilities.push_back(facility(99101,FacilityKind::SleepingPlace));
    assert(era()=="NaturalSurvival");
    world.facilities.push_back(facility(99102,FacilityKind::PrimitiveStorage));
    world.facilities.push_back(facility(99103,FacilityKind::FirePit));
    StorageSite storage; storage.id=99104; world.storageSites.push_back(storage);
    for(auto technique:{TechniqueId::PrimitiveStorage,TechniqueId::FireMaking})
        resident.civilization.knowledge.learn(technique,KnowledgeLevel::Reproducible,0.95);
    assert(era()=="EarlySettlement");
    resident.civilization.knowledge.learn(TechniqueId::Cultivation,KnowledgeLevel::Observed,0.5);
    assert(era()=="EarlySettlement");
    auto plot=facility(99105,FacilityKind::CultivatedPlot); plot.cropPlanted=true;
    world.facilities.push_back(plot);
    assert(era()=="EarlySettlement");
    resident.civilization.knowledge.learn(TechniqueId::Cultivation,KnowledgeLevel::Reproducible,0.95);
    assert(era()=="AgrarianSettlement");
    resident.civilization.knowledge.learn(TechniqueId::CopperSmelting,KnowledgeLevel::Observed,0.5);
    assert(era()=="AgrarianSettlement");
    world.facilities.push_back(facility(99106,FacilityKind::Furnace));
    resident.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Charcoal,10,1,1});
    resident.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::CopperOre,10,1,1});
    resident.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::CopperMetal,10,1,1});
    resident.civilization.knowledge.learn(TechniqueId::CopperSmelting,KnowledgeLevel::Reproducible,0.95);
    assert(era()=="CopperMetallurgy");
    resident.civilization.knowledge.learn(TechniqueId::BronzeAlloying,KnowledgeLevel::Observed,0.5);
    assert(era()=="CopperMetallurgy");
    resident.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::TinOre,10,1,1});
    resident.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::TinMetal,10,1,1});
    resident.civilization.inventory.add({ItemKind::BronzeAxe,MaterialKind::Bronze,1,1,1});
    for(auto technique:{TechniqueId::TinSmelting,TechniqueId::BronzeAlloying})
        resident.civilization.knowledge.learn(technique,KnowledgeLevel::Reproducible,0.95);
    assert(era()=="BronzeTechnology");
    const auto before=era(); world.minute+=10000*1440;
    assert(era()==before);
    for(auto& f:world.facilities) if(f.kind==FacilityKind::Furnace){f.state=FacilityState::Ruined;f.active=false;f.durability=0;}
    assert(era()=="AgrarianSettlement");
    resident.alive=false;
    assert(era()=="NaturalSurvival");
    std::cout<<"operational era evidence, discovery distinction, regression, time independence, save/load PASS\n";
}
