#include <iostream>
#include <string>
#include <vector>

#include "lifelens/Facility.h"
#include "lifelens/SettlementProgression.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static StorageId nextStorageIdFor(const World& world)
{
    StorageId next=1;
    for(const auto& storage:world.storageSites) if(storage.id>=next) next=storage.id+1;
    return next;
}

int main()
{
    const FacilityConstructionSpec spec=facilityConstructionSpec(FacilityKind::PrimitiveStorage);
    CHECK(spec.requiredWork==8.0);
    CHECK(spec.requirements.size()==2);
    CHECK(facilityKindConstructible(FacilityKind::PrimitiveStorage));

    const FacilityConstructionSpec fireSpec=facilityConstructionSpec(FacilityKind::FirePit);
    CHECK(fireSpec.requiredWork==6.0);
    CHECK(fireSpec.requirements.size()==2);
    CHECK(facilityKindConstructible(FacilityKind::FirePit));

    const FacilityConstructionSpec furnaceSpec=facilityConstructionSpec(FacilityKind::Furnace);
    CHECK(furnaceSpec.requiredWork==12.0);
    CHECK(furnaceSpec.requirements.size()==2);
    CHECK(facilityMissingMaterial(makeFacilityConstructionSite(1,FacilityKind::Furnace,{0,0},1,0),MaterialKind::Stone)==8);
    CHECK(facilityMissingMaterial(makeFacilityConstructionSite(1,FacilityKind::Furnace,{0,0},1,0),MaterialKind::Clay)==6);
    CHECK(facilityKindConstructible(FacilityKind::Furnace));

    const FacilityConstructionSpec workSurfaceSpec=facilityConstructionSpec(FacilityKind::WorkSurface);
    CHECK(workSurfaceSpec.requiredWork==7.0);
    CHECK(workSurfaceSpec.requirements.size()==2);
    CHECK(facilityMissingMaterial(makeFacilityConstructionSite(1,FacilityKind::WorkSurface,{0,0},1,0),MaterialKind::Wood)==3);
    CHECK(facilityMissingMaterial(makeFacilityConstructionSite(1,FacilityKind::WorkSurface,{0,0},1,0),MaterialKind::Stone)==2);
    CHECK(facilityKindConstructible(FacilityKind::WorkSurface));
    CHECK(facilitySupportsCrafting(FacilityKind::WorkSurface));

    const FacilityConstructionSpec sleepingSpec=facilityConstructionSpec(FacilityKind::SleepingPlace);
    CHECK(sleepingSpec.requiredWork==5.0);
    CHECK(sleepingSpec.requirements.size()==2);
    CHECK(facilityMissingMaterial(makeFacilityConstructionSite(1,FacilityKind::SleepingPlace,{0,0},1,0),MaterialKind::Fiber)==4);
    CHECK(facilityMissingMaterial(makeFacilityConstructionSite(1,FacilityKind::SleepingPlace,{0,0},1,0),MaterialKind::Wood)==2);
    CHECK(facilityKindConstructible(FacilityKind::SleepingPlace));
    CHECK(facilityProvidesSleep(FacilityKind::SleepingPlace));
    CHECK(!facilityProvidesWeatherProtection(FacilityKind::SleepingPlace));

    const FacilityConstructionSpec shelterSpec=facilityConstructionSpec(FacilityKind::Shelter);
    CHECK(shelterSpec.requiredWork==14.0);
    CHECK(shelterSpec.requirements.size()==2);
    CHECK(facilityMissingMaterial(makeFacilityConstructionSite(1,FacilityKind::Shelter,{0,0},1,0),MaterialKind::Wood)==8);
    CHECK(facilityMissingMaterial(makeFacilityConstructionSite(1,FacilityKind::Shelter,{0,0},1,0),MaterialKind::Fiber)==5);
    CHECK(facilityKindConstructible(FacilityKind::Shelter));
    CHECK(facilityProvidesSleep(FacilityKind::Shelter));
    CHECK(facilityProvidesWeatherProtection(FacilityKind::Shelter));

    // C1 settlement facilities are built from nothing: deterministic Core site,
    // real material delivery, real work, then operational authority.
    Simulation settlement(991122);
    settlement.setupNewGame();
    CHECK(settlement.world().facilities.empty());
    Character& sleeper=settlement.world().characters[0];
    sleeper.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Wood,2,0.5,1.0});
    sleeper.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Fiber,4,0.5,1.0});
    GridPos sleeperActivityAnchor{};
    CHECK(settlement.runtimePosition(sleeper.id,sleeperActivityAnchor));
    const SettlementFacilitySiteOpportunity sleepSite=
        chooseSettlementFacilitySite(
            settlement.world(),sleeper.id,FacilityKind::SleepingPlace,
            sleeperActivityAnchor);
    CHECK(sleepSite.available);
    ConstructedFacility* sleepProject=establishSettlementFacilityProject(
        settlement.world(),sleeper.id,FacilityKind::SleepingPlace,sleepSite.pos);
    CHECK(sleepProject!=nullptr);
    CHECK(sleepProject->state==FacilityState::Planned);
    CHECK(deliverFacilityMaterial(*sleepProject,sleeper.civilization.inventory,MaterialKind::Wood,8)==2);
    CHECK(deliverFacilityMaterial(*sleepProject,sleeper.civilization.inventory,MaterialKind::Fiber,8)==4);
    CHECK(facilityMaterialsComplete(*sleepProject));
    const SettlementFacilityWorkResult sleepWork=workOnSettlementFacility(
        settlement.world(),sleeper,sleepProject->id,20.0);
    CHECK(sleepWork.worked);
    CHECK(sleepWork.completed);
    CHECK(hasOperationalSettlementFacility(settlement.world(),FacilityKind::SleepingPlace));
    CHECK(settlementFacilityProject(settlement.world(),FacilityKind::SleepingPlace)!=nullptr);
    CHECK(settlementFacilityProject(settlement.world(),FacilityKind::SleepingPlace)->linkedStorage==0);
    CHECK(validConstructedFacility(*settlementFacilityProject(settlement.world(),FacilityKind::SleepingPlace)));

    std::string settlementError;
    std::vector<std::uint8_t> settlementBytes;
    CHECK(encodeSimulationSnapshot(settlement.captureSnapshot(),settlementBytes,&settlementError));
    CHECK(settlementError.empty());
    SimulationStateSnapshot settlementDecoded;
    CHECK(decodeSimulationSnapshot(settlementBytes,settlementDecoded,&settlementError));
    CHECK(settlementError.empty());
    CHECK(settlementDecoded.world.facilities.size()==1);
    CHECK(settlementDecoded.world.facilities[0].kind==FacilityKind::SleepingPlace);
    CHECK(settlementDecoded.world.facilities[0].state==FacilityState::Operational);
    CHECK(settlementDecoded.world.facilities[0].linkedStorage==0);
    CHECK(validConstructedFacility(settlementDecoded.world.facilities[0]));

    Simulation source(880088);
    source.setupNewGame();
    CHECK(source.world().storageSites.empty());
    CHECK(source.world().facilities.empty());
    CHECK(source.world().characters.size()==4);

    Character& builder=source.world().characters[0];
    builder.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Wood,4,0.5,1.0});
    builder.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Fiber,2,0.5,1.0});

    const GridPos center=source.world().initialStartRegionCenterGrid();
    const GridPos sitePos{center.x+3,center.y};
    ConstructedFacility site=makeFacilityConstructionSite(
        nextFacilityId(source.world().facilities),FacilityKind::PrimitiveStorage,
        sitePos,builder.id,source.world().minute);
    CHECK(site.id!=0);
    CHECK(site.state==FacilityState::Planned);
    CHECK(site.pos.x==sitePos.x && site.pos.y==sitePos.y);
    CHECK(validConstructedFacility(site));
    source.world().facilities.push_back(site);

    ConstructedFacility& building=source.world().facilities[0];
    CHECK(deliverFacilityMaterial(building,builder.civilization.inventory,MaterialKind::Wood,2)==2);
    CHECK(building.state==FacilityState::UnderConstruction);
    CHECK(facilityMissingMaterial(building,MaterialKind::Wood)==2);
    CHECK(facilityMissingMaterial(building,MaterialKind::Fiber)==2);
    CHECK(!facilityMaterialsComplete(building));
    CHECK(!applyFacilityConstructionWork(building,builder.id,3.0));
    CHECK(building.constructionWork==0.0);
    CHECK(validConstructedFacility(building));

    std::string error;
    std::vector<std::uint8_t> partialBytes;
    CHECK(encodeSimulationSnapshot(source.captureSnapshot(),partialBytes,&error));
    CHECK(error.empty());

    SimulationStateSnapshot partialDecoded;
    CHECK(decodeSimulationSnapshot(partialBytes,partialDecoded,&error));
    CHECK(error.empty());
    CHECK(partialDecoded.world.facilities.size()==1);
    const ConstructedFacility& decodedBuilding=partialDecoded.world.facilities[0];
    CHECK(decodedBuilding.id==building.id);
    CHECK(decodedBuilding.state==FacilityState::UnderConstruction);
    CHECK(decodedBuilding.pos.x==sitePos.x && decodedBuilding.pos.y==sitePos.y);
    CHECK(facilityMissingMaterial(decodedBuilding,MaterialKind::Wood)==2);
    CHECK(facilityMissingMaterial(decodedBuilding,MaterialKind::Fiber)==2);
    CHECK(validConstructedFacility(decodedBuilding));

    Simulation restored(1);
    CHECK(restored.restoreSnapshot(partialDecoded,&error));
    CHECK(error.empty());
    CHECK(restored.world().facilities.size()==1);
    Character& restoredBuilder=restored.world().characters[0];
    ConstructedFacility& restoredBuilding=restored.world().facilities[0];

    CHECK(deliverFacilityMaterial(restoredBuilding,restoredBuilder.civilization.inventory,MaterialKind::Wood,8)==2);
    CHECK(deliverFacilityMaterial(restoredBuilding,restoredBuilder.civilization.inventory,MaterialKind::Fiber,8)==2);
    CHECK(facilityMaterialsComplete(restoredBuilding));
    CHECK(!applyFacilityConstructionWork(restoredBuilding,restoredBuilder.id,3.0));
    CHECK(restoredBuilding.constructionWork==3.0);
    CHECK(applyFacilityConstructionWork(restoredBuilding,restoredBuilder.id,5.0));
    CHECK(facilityWorkComplete(restoredBuilding));
    CHECK(restoredBuilding.state==FacilityState::UnderConstruction);
    CHECK(!restoredBuilding.active);

    StorageSite storage;
    storage.id=nextStorageIdFor(restored.world());
    storage.pos=restoredBuilding.pos;
    restored.world().storageSites.push_back(storage);
    CHECK(activateConstructedFacility(
        restoredBuilding,storage.id,restored.world().minute+8));
    CHECK(restoredBuilding.state==FacilityState::Operational);
    CHECK(restoredBuilding.active);
    CHECK(restoredBuilding.linkedStorage==storage.id);
    CHECK(validConstructedFacility(restoredBuilding));

    std::vector<std::uint8_t> completeBytes;
    CHECK(encodeSimulationSnapshot(restored.captureSnapshot(),completeBytes,&error));
    CHECK(error.empty());
    SimulationStateSnapshot completeDecoded;
    CHECK(decodeSimulationSnapshot(completeBytes,completeDecoded,&error));
    CHECK(error.empty());
    CHECK(completeDecoded.world.facilities.size()==1);
    CHECK(completeDecoded.world.storageSites.size()==1);
    CHECK(completeDecoded.world.facilities[0].state==FacilityState::Operational);
    CHECK(completeDecoded.world.facilities[0].linkedStorage==completeDecoded.world.storageSites[0].id);
    CHECK(completeDecoded.world.storageSites[0].pos.x==sitePos.x);
    CHECK(completeDecoded.world.storageSites[0].pos.y==sitePos.y);
    CHECK(validConstructedFacility(completeDecoded.world.facilities[0]));

    restored.setupNewGame();
    CHECK(restored.world().facilities.empty());
    CHECK(restored.world().storageSites.empty());

    std::cout << "facility authority + persistence passed\n";
    return 0;
}
