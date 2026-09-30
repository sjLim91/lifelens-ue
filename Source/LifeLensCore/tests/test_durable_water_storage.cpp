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
    return portableWaterCount(inventory);
}

static void addFilledWaterContainers(Inventory& inventory,int quantity)
{
    inventory.add({
        ItemKind::SimpleContainer,
        MaterialKind::Clay,
        quantity,
        0.5,
        1.0});
    inventory.add({
        ItemKind::RawMaterial,
        MaterialKind::Water,
        quantity,
        0.5,
        1.0});
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
    addFilledWaterContainers(storage.inventory,6);
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
    // owns real carried water in real containers. Retrieval itself never
    // relieves thirst.
    CHECK(emergencyAffordanceAvailableFor(resident,Goal::Drink));
    CHECK(resident.needs.thirst==0.91);

    // Ordinary stocking keeps a small carried reserve and banks the surplus.
    world.storageSites[0].inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Water,4);
    addFilledWaterContainers(resident.civilization.inventory,4);
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

    // Loose Water is not portable. Natural Water can only be gathered into
    // available empty containers, and urgent survival uses a known source
    // directly instead of inventing carried stock.
    CHECK(resident.civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Water,2));
    CHECK(world.storageSites[0].inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Water,4));
    CHECK(waterCount(resident.civilization.inventory)==0);
    CHECK(waterCount(world.storageSites[0].inventory)==0);

    const CivilizationUtilityDecision directSourceSurvival=
        urgentSurvivalProvisionGatherDecision(world,resident);
    CHECK(directSourceSurvival.intent==CivilizationIntent::None);
    CHECK(actionAvailableFor(world,resident,Goal::Drink));

    ResourceNode* waterNode=nullptr;
    for(ResourceNode& node:world.resourceNodes){
        if(node.material==MaterialKind::Water && node.quantity>0){
            waterNode=&node;
            break;
        }
    }
    CHECK(waterNode!=nullptr);

    // Existing containers are currently empty because their Water was consumed/
    // removed. Filling is limited by exactly that physical capacity.
    const int emptyBefore=emptySimpleContainerCount(
        resident.civilization.inventory);
    CHECK(emptyBefore>=2);
    const int nodeBefore=waterNode->quantity;
    const CivilizationEvent filled=gatherResource(
        resident.civilization,*waterNode,10);
    CHECK(filled.quantity>0);
    CHECK(filled.quantity<=emptyBefore);
    CHECK(waterCount(resident.civilization.inventory)==filled.quantity);
    CHECK(waterNode->quantity==nodeBefore-filled.quantity);

    // Remove both Water and all containers: a bare-handed Gather cannot create
    // portable Water.
    while(resident.civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Water,1)) {}
    while(resident.civilization.inventory.remove(
        ItemKind::SimpleContainer,MaterialKind::Unknown,1,true)) {}
    const int bareNodeBefore=waterNode->quantity;
    const CivilizationEvent bareGather=gatherResource(
        resident.civilization,*waterNode,3);
    CHECK(bareGather.quantity==0);
    CHECK(waterNode->quantity==bareNodeBefore);
    CHECK(rawWaterUnitCount(resident.civilization.inventory)==0);

    // Hygiene demand also retrieves real stored Water even when thirst is low.
    Simulation hygieneSimulation(640042,910022);
    hygieneSimulation.setupNewGame();
    Character& hygieneResident=hygieneSimulation.world().characters.front();
    hygieneResident.needs.hunger=0.05;
    hygieneResident.needs.thirst=0.05;
    hygieneResident.needs.sleep=0.05;
    hygieneResident.needs.bladder=0.05;
    hygieneResident.needs.hygiene=0.95;
    while(hygieneResident.civilization.inventory.remove(
        ItemKind::RawMaterial,MaterialKind::Water,1)) {}
    while(hygieneResident.civilization.inventory.remove(
        ItemKind::SimpleContainer,MaterialKind::Unknown,1,true)) {}

    GridPos hygienePosition{};
    CHECK(hygieneSimulation.runtimePosition(
        hygieneResident.id,hygienePosition));

    StorageSite hygieneStorage;
    hygieneStorage.id=7101;
    hygieneStorage.pos={hygienePosition.x+1,hygienePosition.y};
    addFilledWaterContainers(hygieneStorage.inventory,3);
    hygieneSimulation.world().storageSites={hygieneStorage};

    const CivilizationUtilityDecision hygieneRetrieve=
        bestRetrieveDecisionAtPosition(
            hygieneSimulation.world(),
            hygieneResident,
            hygienePosition);
    CHECK(hygieneRetrieve.intent==CivilizationIntent::Retrieve);
    CHECK(hygieneRetrieve.material==MaterialKind::Water);
    CHECK(hygieneRetrieve.storage==7101);

    const CivilizationExecutionResult hygieneRetrieved=
        executeCivilizationDecisionAtPosition(
            hygieneSimulation.world(),
            hygieneResident,
            hygieneRetrieve,
            hygieneStorage.pos);
    CHECK(hygieneRetrieved.executed && hygieneRetrieved.success);
    CHECK(portableWaterCount(
        hygieneResident.civilization.inventory)>0);
    CHECK(actionAvailableFor(
        hygieneSimulation.world(),
        hygieneResident,
        Goal::Wash));

    // Generic inventory/storage authority already participates in snapshots;
    // verify the new water reserve path round-trips without a parallel state.
    addFilledWaterContainers(world.storageSites[0].inventory,5);
    addFilledWaterContainers(resident.civilization.inventory,1);

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
