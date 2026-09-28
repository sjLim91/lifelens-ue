#include <cassert>
#include <iostream>
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

static ConstructedFacility completedFixture(FacilityId id, FacilityKind kind, GridPos pos, CharacterId owner, int minute)
{
    auto facility=makeFacilityConstructionSite(id,kind,pos,owner,minute);
    for(auto& requirement:facility.requirements) requirement.delivered=requirement.required;
    facility.constructionWork=facility.requiredWork;
    assert(activateConstructedFacility(facility,0,minute));
    return facility;
}

int main()
{
    Simulation simulation(770031);
    simulation.setupNewGame();
    auto& world=simulation.world();
    assert(world.facilities.empty());
    const GridPos anchor=world.initialStartRegionCenterGrid();
    SettlementPopulation population;
    for(auto& resident:world.characters){
        population.emplace(resident.id,anchor);
        resident.needs={0.1,0.1,0.58,0.1,0.1};
    }
    auto& actor=world.characters.front();
    const CharacterId owner=actor.id;
    const auto demand=[&](FacilityKind kind,GridPos pos){
        return observeSettlementFacilityDemand(world,owner,kind,pos,&population);
    };
    assert(demand(FacilityKind::SleepingPlace,anchor).residents==4);
    assert(demand(FacilityKind::SleepingPlace,anchor).operationalCapacity==0);

    // Four real residents build four beds in sequence, with paid material/work.
    // A completed first bed must neither hide later projects nor end all demand.
    for(int built=0;built<4;++built){
        const auto site=chooseSettlementFacilitySite(world,owner,FacilityKind::SleepingPlace,anchor,&population);
        if(!site.available){
            std::cerr << "No site after " << built << " beds\n";
            return 1;
        }
        for(const auto& existing:world.facilities)
            assert(!facilityFootprintsConflict(FacilityKind::SleepingPlace,site.pos,existing.kind,existing.pos));
        auto* project=establishSettlementFacilityProject(world,owner,FacilityKind::SleepingPlace,site.pos,&population);
        assert(project && !project->active && project->constructionWork==0.0);
        const FacilityId id=project->id;
        assert(demand(FacilityKind::SleepingPlace,anchor).operationalCapacity==built);
        assert(!chooseSettlementFacilitySite(world,world.characters[1].id,FacilityKind::SleepingPlace,anchor,&population).available);
        assert(!establishSettlementFacilityProject(world,world.characters[1].id,FacilityKind::SleepingPlace,
            {site.pos.x+20,site.pos.y},&population));
        assert(!workOnSettlementFacility(world,actor,id,100.0).worked);
        const auto requirements=project->requirements;
        for(const auto& requirement:requirements){
            actor.civilization.inventory.add({ItemKind::RawMaterial,requirement.material,requirement.required,0.5,1.0});
            const int before=actor.civilization.inventory.count(ItemKind::RawMaterial,requirement.material);
            assert(deliverFacilityMaterial(*project,actor.civilization.inventory,requirement.material,requirement.required)==requirement.required);
            assert(actor.civilization.inventory.count(ItemKind::RawMaterial,requirement.material)==before-requirement.required);
        }
        const auto next=bestSettlementFoundationDecision(world,actor,anchor,&population);
        assert(next.facility==id && next.facilityAction==FacilityBuildAction::Work);
        assert(workOnSettlementFacility(world,actor,id,100.0).completed);
        assert(facilityOperationalAndActive(*project));
    }
    assert(!demand(FacilityKind::SleepingPlace,anchor).unmet());
    assert(!chooseSettlementFacilitySite(world,owner,FacilityKind::SleepingPlace,anchor,&population).available);
    assert(settlementFacilityNeedPressure(world,actor,anchor,FacilityKind::SleepingPlace,&population)==0.0);

    // Multiple facilities remain authoritative through the real save codec.
    std::vector<std::uint8_t> bytes;
    std::string error;
    if(!encodeSimulationSnapshot(simulation.captureSnapshot(),bytes,&error)){
        std::cerr << "Snapshot encoding failed: " << error << '\n';
        return 1;
    }
    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    assert(decoded.world.facilities.size()==world.facilities.size());
    for(std::size_t index=0;index<world.facilities.size();++index){
        assert(decoded.world.facilities[index].id==world.facilities[index].id);
        assert(decoded.world.facilities[index].state==world.facilities[index].state);
        assert(manhattan(decoded.world.facilities[index].pos,world.facilities[index].pos)==0);
    }

    // Birth/population growth changes demand; death and leaving the area do not
    // leave phantom residents. Distant infrastructure does not serve this camp.
    Character newcomer=actor;
    newcomer.id=999;
    newcomer.civilization.character=newcomer.id;
    newcomer.lifeStage=LifeStage::Baby;
    world.characters.push_back(newcomer);
    population.emplace(newcomer.id,anchor);
    assert(demand(FacilityKind::SleepingPlace,anchor).canPlan());
    assert(demand(FacilityKind::Shelter,anchor).residents==5);
    assert(demand(FacilityKind::WorkSurface,anchor).residents==4);
    world.characters.back().alive=false;
    assert(!demand(FacilityKind::SleepingPlace,anchor).canPlan());
    const GridPos distant{anchor.x+SettlementServiceRadiusGrid*4,anchor.y};
    for(auto& location:population) location.second=distant;
    assert(demand(FacilityKind::SleepingPlace,anchor).residents==0);
    assert(demand(FacilityKind::SleepingPlace,distant).operationalCapacity==0);
    assert(demand(FacilityKind::SleepingPlace,distant).canPlan());
    // Missing authoritative positions fail closed rather than relocate people.
    population.erase(world.characters[1].id);
    assert(demand(FacilityKind::SleepingPlace,distant).residents==3);

    // Local shelters and workshops have shared, finite planning capacity.
    world.facilities.clear();
    for(auto& location:population) location.second=anchor;
    population.emplace(world.characters[1].id,anchor);
    world.facilities.push_back(completedFixture(100,FacilityKind::Shelter,anchor,owner,world.minute));
    assert(!demand(FacilityKind::Shelter,anchor).unmet());
    world.characters.back().alive=true;
    population[999]=anchor;
    assert(demand(FacilityKind::Shelter,anchor).canPlan());
    // A roof does not manufacture dedicated bedding.
    assert(demand(FacilityKind::SleepingPlace,anchor).canPlan());
    world.facilities.push_back(completedFixture(101,FacilityKind::WorkSurface,{anchor.x+20,anchor.y},owner,world.minute));
    assert(demand(FacilityKind::WorkSurface,anchor).canPlan());
    world.facilities.push_back(completedFixture(102,FacilityKind::WorkSurface,{anchor.x-20,anchor.y},owner,world.minute));
    assert(!demand(FacilityKind::WorkSurface,anchor).canPlan());

    // Maintenance searches all local facilities; a healthy first entry cannot
    // conceal a damaged second one. Ruins don't provide service capacity.
    for(auto& resident:world.characters) resident.needs={0.1,0.1,0.1,0.1,0.1};
    world.facilities.back().durability=0.4;
    auto& maintainer=world.characters.front();
    maintainer.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Wood,2,0.5,1.0});
    const auto repair=bestSettlementFoundationDecision(world,maintainer,anchor,&population);
    assert(repair.facilityAction==FacilityBuildAction::Repair && repair.facility==102);
    assert(executeCivilizationDecisionAtPosition(world,maintainer,repair,world.facilities.back().pos,&population).success);
    assert(ruinConstructedFacility(world.facilities.back()));
    assert(demand(FacilityKind::WorkSurface,anchor).canPlan());

    // Exercise the actual Simulation -> unified utility -> pending context
    // path. With one existing bed and four residents, a second plan must exist.
    Simulation live(770031);
    live.setupNewGame();
    live.setExternalPhysicalExecution(true);
    auto& liveWorld=live.world();
    GridPos liveAnchor{};
    assert(live.runtimePosition(liveWorld.characters.front().id,liveAnchor));
    liveWorld.facilities.push_back(completedFixture(1,FacilityKind::SleepingPlace,
        {liveAnchor.x+14,liveAnchor.y},liveWorld.characters.front().id,liveWorld.minute));
    for(auto& resident:liveWorld.characters){
        resident.needs={0.1,0.1,0.58,0.1,0.1};
        resident.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Water,3,0.5,1.0});
        resident.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::PlantFood,3,0.5,1.0});
    }
    bool additionalPlan=false;
    for(int minute=0;minute<31 && !additionalPlan;++minute){
        live.step();
        for(const auto& resident:liveWorld.characters){
            const auto pending=live.observePendingContextAction(resident.id);
            if(pending.active && pending.facilityKind==FacilityKind::SleepingPlace
               && pending.facilityAction==FacilityBuildAction::Plan){
                additionalPlan=true;
                break;
            }
        }
    }
    assert(additionalPlan);
    std::cout << "Settlement capacity, local demand, paid construction, maintenance, persistence and runtime wiring passed\n";
}
