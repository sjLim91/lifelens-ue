#include <iostream>
#include <string>
#include <vector>

#include "lifelens/Facility.h"
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
    CHECK(!facilityKindConstructible(FacilityKind::FirePit));

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

    // NEW GAME must remove every constructed result and return to nature-only
    // production state. No facility or storage may leak across a restart.
    restored.setupNewGame();
    CHECK(restored.world().facilities.empty());
    CHECK(restored.world().storageSites.empty());

    std::cout << "facility authority + persistence passed\n";
    return 0;
}
