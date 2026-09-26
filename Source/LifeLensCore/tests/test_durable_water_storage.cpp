#include <iostream>
#include <string>
#include <vector>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/ContextAction.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/SocialUtility.h"
#include "lifelens/UtilityAI.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static int waterCount(const Inventory& inventory)
{
    return inventory.count(ItemKind::RawMaterial,MaterialKind::Water);
}

static bool hasLiveWaterNode(const World& world)
{
    for(const ResourceNode& node:world.resourceNodes){
        if(node.id!=0 && node.material==MaterialKind::Water && node.quantity>0){
            return true;
        }
    }
    return false;
}

int main()
{
    Simulation simulation(640041,910021);
    simulation.setupNewGame();
    World& world=simulation.world();
    CHECK(!world.characters.empty());
    CHECK(hasLiveWaterNode(world));

    Character& resident=world.characters.front();
    GridPos residentPosition{};
    CHECK(simulation.runtimePosition(resident.id,residentPosition));
    resident.needs.hunger=0.10;
    resident.needs.thirst=0.91;
    CHECK(waterCount(resident.civilization.inventory)==0);

    StorageSite storage;
    storage.id=7001;
    storage.pos={7,-3};
    storage.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Water,6,0.5,1.0});
    world.storageSites={storage};

    // Urgent thirst must consume settlement logistics before making another
    // trip to a natural water resource.
    const CivilizationUtilityDecision urgent=
        urgentSurvivalProvisionGatherDecision(world,resident);
    CHECK(urgent.intent==CivilizationIntent::Retrieve);
    CHECK(urgent.storage==7001);
    CHECK(urgent.material==MaterialKind::Water);
    CHECK(urgent.quantity==2);

    GridPos target{};
    SanitationSiteId sanitationSite=0;
    CHECK(civilizationContextRequiresSpatialTarget(urgent));
    CHECK(resolveCivilizationContextTarget(
        world,resident,urgent,residentPosition,target,sanitationSite));
    CHECK(target.x==7 && target.y==-3);
    CHECK(sanitationSite==0);

    const int waterBefore=
        waterCount(resident.civilization.inventory)
        +waterCount(world.storageSites[0].inventory);
    const CivilizationExecutionResult retrieved=
        executeCivilizationDecision(world,resident,urgent);
    CHECK(retrieved.executed && retrieved.success);
    CHECK(retrieved.event.type==CivilizationEventType::Retrieved);
    CHECK(retrieved.event.quantity==2);
    CHECK(waterCount(resident.civilization.inventory)==2);
    CHECK(waterCount(world.storageSites[0].inventory)==4);
    CHECK(
        waterCount(resident.civilization.inventory)
        +waterCount(world.storageSites[0].inventory)
        ==waterBefore);

    // Existing physical Drink authority becomes available only because Core now
    // owns real carried water. Retrieval itself never relieves thirst.
    CHECK(emergencyAffordanceAvailableFor(resident,Goal::Drink));
    CHECK(resident.needs.thirst==0.91);

    // Ordinary stocking keeps a small carried reserve and banks the surplus.
    world.storageSites[0].inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Water,4);
    resident.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Water,4,0.5,1.0});
    CHECK(waterCount(resident.civilization.inventory)==6);
    const CivilizationUtilityDecision store=
        bestStoreDecision(world,resident);
    CHECK(store.intent==CivilizationIntent::Store);
    CHECK(store.storage==7001);
    CHECK(store.material==MaterialKind::Water);
    CHECK(store.quantity==4);

    const int stockBefore=
        waterCount(resident.civilization.inventory)
        +waterCount(world.storageSites[0].inventory);
    const CivilizationExecutionResult stored=
        executeCivilizationDecision(world,resident,store);
    CHECK(stored.executed && stored.success);
    CHECK(stored.event.type==CivilizationEventType::Stored);
    CHECK(waterCount(resident.civilization.inventory)==2);
    CHECK(waterCount(world.storageSites[0].inventory)==4);
    CHECK(
        waterCount(resident.civilization.inventory)
        +waterCount(world.storageSites[0].inventory)
        ==stockBefore);

    // If the settlement reserve is empty, urgent survival falls back to the
    // authoritative natural ResourceNode instead of synthesizing water.
    CHECK(resident.civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Water,2));
    CHECK(world.storageSites[0].inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Water,4));
    CHECK(waterCount(resident.civilization.inventory)==0);
    CHECK(waterCount(world.storageSites[0].inventory)==0);

    const CivilizationUtilityDecision fallback=
        urgentSurvivalProvisionGatherDecision(world,resident);
    CHECK(fallback.intent==CivilizationIntent::Gather);
    CHECK(fallback.material==MaterialKind::Water);
    CHECK(fallback.resourceNode!=0);

    // Generic inventory/storage authority already participates in snapshots;
    // verify the new water reserve path round-trips without a parallel state.
    world.storageSites[0].inventory.add({
        ItemKind::RawMaterial,MaterialKind::Water,5,0.5,1.0});
    resident.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Water,1,0.5,1.0});

    const SimulationStateSnapshot snapshot=simulation.captureSnapshot();
    std::vector<std::uint8_t> bytes;
    std::string error;
    CHECK(encodeSimulationSnapshot(snapshot,bytes,&error));
    CHECK(error.empty());

    SimulationStateSnapshot decoded;
    CHECK(decodeSimulationSnapshot(bytes,decoded,&error));
    CHECK(error.empty());
    CHECK(!decoded.world.storageSites.empty());
    CHECK(!decoded.world.characters.empty());
    CHECK(waterCount(decoded.world.storageSites[0].inventory)==5);
    CHECK(waterCount(
        decoded.world.characters.front().civilization.inventory)==1);

    std::cout << "durable water storage/retrieval authority passed\n";
    return 0;
}
