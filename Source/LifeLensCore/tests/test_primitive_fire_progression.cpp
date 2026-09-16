#include <cassert>
#include <cmath>

#include "lifelens/PrimitiveFireProgression.h"

using namespace lifelens;

namespace {

Character makeFireKeeper(CharacterId id)
{
    Character resident;
    resident.id=id;
    resident.civilization.character=id;
    resident.civilization.knowledge.learn(
        TechniqueId::FireMaking,KnowledgeLevel::Reproducible,0.8);
    return resident;
}

} // namespace

int main()
{
    const FacilityConstructionSpec spec=facilityConstructionSpec(FacilityKind::FirePit);
    assert(spec.kind==FacilityKind::FirePit);
    assert(std::fabs(spec.requiredWork-6.0)<1e-9);
    assert(spec.requirements.size()==2);

    World world(9917);
    world.facilities.clear();
    world.storageSites.clear();
    Character resident=makeFireKeeper(7);
    resident.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Stone,5,0.5,1.0});
    resident.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Wood,5,0.5,1.0});

    assert(!hasOperationalFirePit(world));
    assert(primitiveFirePitProject(world)==nullptr);

    const PrimitiveFirePitSiteOpportunity site=choosePrimitiveFirePitSite(world,resident.id);
    assert(site.available);
    ConstructedFacility* project=establishPrimitiveFirePitProject(world,resident,site.pos);
    assert(project!=nullptr);
    const FacilityId firePitId=project->id;
    assert(project->kind==FacilityKind::FirePit);
    assert(project->state==FacilityState::Planned);
    assert(!project->active);

    assert(deliverFacilityMaterial(
        *project,resident.civilization.inventory,MaterialKind::Stone,5)==5);
    assert(deliverFacilityMaterial(
        *project,resident.civilization.inventory,MaterialKind::Wood,2)==2);
    assert(facilityMaterialsComplete(*project));

    PrimitiveFirePitWorkResult work;
    for(int i=0;i<8 && !work.completed;++i){
        work=workOnPrimitiveFirePit(world,resident,1.25);
        assert(work.worked);
    }
    assert(work.completed);
    assert(hasOperationalFirePit(world));
    project=primitiveFirePitProject(world);
    assert(project!=nullptr);
    assert(project->id==firePitId);
    assert(project->active);
    assert(project->state==FacilityState::Operational);

    // Three wood remained after the physical ring was built. Fueling transfers
    // real inventory into the authoritative pit state.
    assert(resident.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Wood)==3);
    assert(fuelPrimitiveFirePit(world,resident,firePitId,2)==2);
    assert(project->fuelUnits==2);
    assert(resident.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Wood)==1);

    assert(ignitePrimitiveFirePit(world,resident,firePitId));
    assert(project->lit);
    assert(project->heatLevel>0.0);
    assert(project->burnMinutesRemaining==PrimitiveFireBurnMinutesPerWoodUnit);

    for(int i=0;i<PrimitiveFireBurnMinutesPerWoodUnit;++i){
        advancePrimitiveFireOneMinute(world);
        ++world.minute;
    }
    assert(project->lit);
    assert(project->fuelUnits==1);
    assert(project->charcoalUnits==1);

    for(int i=0;i<PrimitiveFireBurnMinutesPerWoodUnit;++i){
        advancePrimitiveFireOneMinute(world);
        ++world.minute;
    }
    assert(!project->lit);
    assert(project->fuelUnits==0);
    assert(project->charcoalUnits==2);
    assert(project->heatLevel>0.0);

    assert(collectPrimitiveFirePitCharcoal(world,resident,firePitId,8)==2);
    assert(project->charcoalUnits==0);
    assert(resident.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Charcoal)==2);

    for(int i=0;i<8;++i){
        advancePrimitiveFireOneMinute(world);
        ++world.minute;
    }
    assert(project->heatLevel==0.0);
    assert(validConstructedFacility(*project));

    // Knowledge is a real prerequisite; a resident cannot create or operate a
    // fire pit just because the presentation or world knows its location.
    World ignorantWorld(17);
    ignorantWorld.facilities.clear();
    Character ignorant;
    ignorant.id=9;
    ignorant.civilization.character=9;
    const PrimitiveFirePitSiteOpportunity ignorantSite=
        choosePrimitiveFirePitSite(ignorantWorld,ignorant.id);
    assert(ignorantSite.available);
    assert(establishPrimitiveFirePitProject(
        ignorantWorld,ignorant,ignorantSite.pos)==nullptr);

    return 0;
}
