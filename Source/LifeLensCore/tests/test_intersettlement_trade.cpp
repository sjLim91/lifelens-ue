#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/CoreNavigation.h"
#include "lifelens/SettlementTrade.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character makeTrader(
    CharacterId id,
    const char* name,
    MaterialKind material,
    int quantity,
    double mobility)
{
    Character resident;
    resident.id=id;
    resident.name=name;
    resident.alive=true;
    resident.lifeStage=LifeStage::Adult;
    resident.civilization.character=id;
    resident.civilization.inventory.add(
        {ItemKind::RawMaterial,material,quantity,0.8,1.0});
    resident.personality.curiosity=mobility;
    resident.personality.adaptability=mobility;
    resident.personality.sociability=mobility;
    resident.needs.hunger=0.05;
    resident.needs.thirst=0.05;
    resident.needs.sleep=0.05;
    resident.needs.bladder=0.05;
    resident.needs.hygiene=0.05;
    return resident;
}

static bool appendOperationalStorage(
    World& world,
    FacilityId facilityId,
    StorageId storageId,
    GridPos pos,
    CharacterId owner)
{
    StorageSite storage;
    storage.id=storageId;
    storage.pos=pos;
    world.storageSites.push_back(storage);

    ConstructedFacility facility=
        makeFacilityConstructionSite(
            facilityId,
            FacilityKind::PrimitiveStorage,
            pos,
            owner,
            world.minute);
    for(auto& requirement:facility.requirements){
        requirement.delivered=requirement.required;
    }
    facility.constructionWork=facility.requiredWork;
    if(!activateConstructedFacility(
            facility,storageId,world.minute)){
        return false;
    }
    world.facilities.push_back(std::move(facility));
    return true;
}

static bool findReachableTradeDestination(
    const World& world,
    GridPos start,
    GridPos& outDestination,
    int& outRouteLength)
{
    const int minimumDistance=SettlementServiceRadiusGrid*3;
    const int maximumDistance=
        WorldChunkSpanGridCells*InterSettlementTradeMaxDistanceChunks
        -WorldChunkSpanGridCells;

    const int directions[8][2]={
        {1,0},{-1,0},{0,1},{0,-1},
        {1,1},{1,-1},{-1,1},{-1,-1}
    };
    for(int distance=minimumDistance;
        distance<=maximumDistance;
        distance+=WorldChunkSpanGridCells){
        for(const auto& direction:directions){
            GridPos candidate{
                start.x+direction[0]*distance,
                start.y+direction[1]*distance
            };
            if(!coreGroundTraversable(world,candidate)) continue;
            std::vector<GridPos> route;
            if(!buildCoreGroundRoute(
                    world,start,candidate,1,route)){
                continue;
            }
            if(static_cast<int>(route.size())
               <=SettlementServiceRadiusGrid*2){
                continue;
            }
            outDestination=candidate;
            outRouteLength=static_cast<int>(route.size());
            return true;
        }
    }
    return false;
}

int main()
{
    // Pure Core economics: two settled clusters with complementary stock
    // should produce a long-range mission, but merely choosing the mission must
    // not mutate either inventory.
    World modelWorld(606201);
    modelWorld.characters.clear();
    modelWorld.facilities.clear();
    modelWorld.storageSites.clear();
    modelWorld.resourceNodes.clear();

    Character woodTrader=
        makeTrader(1,"WoodTrader",MaterialKind::Wood,8,1.0);
    Character stoneTrader=
        makeTrader(2,"StoneTrader",MaterialKind::Stone,8,0.15);
    modelWorld.characters={woodTrader,stoneTrader};

    const GridPos firstPos{0,0};
    const GridPos secondPos{
        SettlementServiceRadiusGrid*3,
        0
    };
    CHECK(appendOperationalStorage(
        modelWorld,1,1,firstPos,1));
    CHECK(appendOperationalStorage(
        modelWorld,2,2,secondPos,2));

    SettlementPopulation modelPopulation{
        {1,firstPos},
        {2,secondPos}
    };
    const SettlementNetworkObservation modelNetwork=
        observeSettlementNetwork(
            modelWorld,&modelPopulation);
    CHECK(modelNetwork.activeSettlementCount==2);

    RelationshipBook modelRelationships;
    modelRelationships.getOrCreate(1,2).trust=0.95;
    modelRelationships.getOrCreate(2,1).trust=0.95;
    SocialKnowledgeBook modelKnowledge;

    const int woodBefore=
        modelWorld.characters[0].civilization.inventory.count(
            ItemKind::RawMaterial,MaterialKind::Wood);
    const int stoneBefore=
        modelWorld.characters[1].civilization.inventory.count(
            ItemKind::RawMaterial,MaterialKind::Stone);

    const InterSettlementTradeMission mission=
        bestInterSettlementTradeMission(
            modelWorld,
            modelNetwork,
            modelPopulation,
            modelRelationships,
            modelKnowledge);

    CHECK(mission.available);
    CHECK(mission.traveler==1);
    CHECK(mission.partner==2);
    CHECK(mission.originSettlement!=mission.destinationSettlement);
    CHECK(mission.exchange.valid());
    CHECK(mission.score>=InterSettlementTradeMissionThreshold);
    CHECK(
        modelWorld.characters[0].civilization.inventory.count(
            ItemKind::RawMaterial,MaterialKind::Wood)
        ==woodBefore);
    CHECK(
        modelWorld.characters[1].civilization.inventory.count(
            ItemKind::RawMaterial,MaterialKind::Stone)
        ==stoneBefore);

    // Trade-route evidence belongs to the settlement pair, not to whichever
    // settlement the traveler happens to be standing inside after the trip.
    CHECK(registerInterSettlementTradeFact(
        modelKnowledge,
        modelWorld.characters[0],
        modelWorld.characters[1],
        mission.originSettlement,
        mission.destinationSettlement,
        mission.exchange,
        10,
        modelWorld.seed)!=nullptr);

    SettlementTradeNetworkObservation route=
        observeSettlementTradeNetwork(
            modelWorld,
            modelKnowledge,
            modelNetwork,
            modelPopulation);
    CHECK(route.routeCount==1);
    CHECK(route.activeRouteCount==0);
    CHECK(route.exchangeEvidenceCount==1);
    CHECK(route.routes.front().partnerCount==1);

    CHECK(registerInterSettlementTradeFact(
        modelKnowledge,
        modelWorld.characters[0],
        modelWorld.characters[1],
        mission.originSettlement,
        mission.destinationSettlement,
        mission.exchange,
        20,
        modelWorld.seed)!=nullptr);

    route=observeSettlementTradeNetwork(
        modelWorld,
        modelKnowledge,
        modelNetwork,
        modelPopulation);
    CHECK(route.routeCount==1);
    CHECK(route.activeRouteCount==1);
    CHECK(route.routes.front().active);
    CHECK(route.routes.front().exchangeCount==2);

    // End-to-end runtime: schedule at the hourly economy boundary, walk to the
    // other settlement, exchange only after arrival, then physically return.
    Simulation simulation(606202);
    simulation.setupNewGame();
    SimulationStateSnapshot snapshot=
        simulation.captureSnapshot();
    CHECK(snapshot.world.characters.size()>=2);

    CharacterId firstId=snapshot.world.characters[0].id;
    CharacterId secondId=snapshot.world.characters[1].id;
    auto firstRuntime=snapshot.runtime.find(firstId);
    auto secondRuntime=snapshot.runtime.find(secondId);
    CHECK(firstRuntime!=snapshot.runtime.end());
    CHECK(secondRuntime!=snapshot.runtime.end());

    const GridPos runtimeOrigin=firstRuntime->second.pos;
    GridPos runtimeDestination{};
    int oneWayRouteLength=0;
    CHECK(findReachableTradeDestination(
        snapshot.world,
        runtimeOrigin,
        runtimeDestination,
        oneWayRouteLength));
    CHECK(manhattan(
        runtimeOrigin,runtimeDestination)
        >SettlementServiceRadiusGrid);

    snapshot.world.minute=59;
    snapshot.world.facilities.clear();
    snapshot.world.storageSites.clear();

    for(Character& resident:snapshot.world.characters){
        resident.civilization.inventory=Inventory{};
        resident.needs.hunger=0.05;
        resident.needs.thirst=0.05;
        resident.needs.sleep=0.05;
        resident.needs.bladder=0.05;
        resident.needs.hygiene=0.05;
        resident.personality.curiosity=0.10;
        resident.personality.adaptability=0.10;
        resident.personality.sociability=0.10;

        auto runtime=snapshot.runtime.find(resident.id);
        CHECK(runtime!=snapshot.runtime.end());
        runtime->second.goal=Goal::Idle;
        runtime->second.plan.clear();
        runtime->second.actionIndex=0;
        runtime->second.pendingContext.clear();
        runtime->second.navigationRoute.clear();
        runtime->second.navigationRouteIndex=0;
        runtime->second.navigationHasTarget=false;
        runtime->second.navigationArrived=false;
        runtime->second.navigationRouteFailed=false;
        runtime->second.socialActive=false;
        runtime->second.civilizationActive=false;
        runtime->second.penaltyUntilMinute=10000;
        runtime->second.pos=runtimeOrigin;
    }

    Character& runtimeFirst=snapshot.world.characters[0];
    Character& runtimeSecond=snapshot.world.characters[1];
    runtimeFirst.personality.curiosity=1.0;
    runtimeFirst.personality.adaptability=1.0;
    runtimeFirst.personality.sociability=1.0;
    runtimeFirst.civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::Wood,10,0.8,1.0});
    runtimeSecond.civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::Stone,10,0.8,1.0});

    firstRuntime=snapshot.runtime.find(firstId);
    secondRuntime=snapshot.runtime.find(secondId);
    firstRuntime->second.pos=runtimeOrigin;
    secondRuntime->second.pos=runtimeDestination;

    CHECK(appendOperationalStorage(
        snapshot.world,
        1001,
        1001,
        runtimeOrigin,
        firstId));
    CHECK(appendOperationalStorage(
        snapshot.world,
        2001,
        2001,
        runtimeDestination,
        secondId));

    snapshot.relationships.getOrCreate(
        firstId,secondId).trust=0.95;
    snapshot.relationships.getOrCreate(
        secondId,firstId).trust=0.95;

    std::string error;
    CHECK(simulation.restoreSnapshot(snapshot,&error));
    CHECK(error.empty());

    const SettlementNetworkObservation runtimeNetworkBefore=
        simulation.observeSettlementNetwork();
    CHECK(runtimeNetworkBefore.activeSettlementCount==2);
    const SettlementClusterObservation* firstSettlement=
        settlementClusterForPosition(
            runtimeNetworkBefore,runtimeOrigin);
    const SettlementClusterObservation* secondSettlement=
        settlementClusterForPosition(
            runtimeNetworkBefore,runtimeDestination);
    CHECK(firstSettlement!=nullptr);
    CHECK(secondSettlement!=nullptr);
    CHECK(firstSettlement->id!=secondSettlement->id);
    const SettlementClusterId runtimeOriginSettlement=
        firstSettlement->id;
    const SettlementClusterId runtimeDestinationSettlement=
        secondSettlement->id;

    const int firstWoodBefore=
        simulation.world().characters[0]
            .civilization.inventory.count(
                ItemKind::RawMaterial,MaterialKind::Wood);
    const int firstStoneBefore=
        simulation.world().characters[0]
            .civilization.inventory.count(
                ItemKind::RawMaterial,MaterialKind::Stone);

    simulation.step(); // 59 -> 60, hourly economy schedules the trip.

    PendingContextActionObservation pending=
        simulation.observePendingContextAction(firstId);
    CHECK(pending.active);
    CHECK(pending.kind==ContextActionKind::Trade);
    CHECK(pending.targetResident==secondId);
    CHECK(
        simulation.world().characters[0]
            .civilization.inventory.count(
                ItemKind::RawMaterial,MaterialKind::Wood)
        ==firstWoodBefore);
    CHECK(
        simulation.world().characters[0]
            .civilization.inventory.count(
                ItemKind::RawMaterial,MaterialKind::Stone)
        ==firstStoneBefore);

    // The packed trade payload is part of the existing pending-context codec
    // and must round-trip before a single movement step occurs.
    {
        SimulationStateSnapshot inFlight=
            simulation.captureSnapshot();
        const auto runtime=inFlight.runtime.find(firstId);
        CHECK(runtime!=inFlight.runtime.end());
        const TradeContextPayload payload=
            tradeContextPayload(
                runtime->second.pendingContext);
        CHECK(payload.partner==secondId);
        CHECK(payload.originSettlement==runtimeOriginSettlement);
        CHECK(payload.destinationSettlement==runtimeDestinationSettlement);
        CHECK(!payload.returning);
        CHECK(payload.originPos.x==runtimeOrigin.x);
        CHECK(payload.originPos.y==runtimeOrigin.y);

        std::vector<std::uint8_t> bytes;
        CHECK(encodeSimulationSnapshot(
            inFlight,bytes,&error));
        CHECK(error.empty());
        SimulationStateSnapshot decoded;
        CHECK(decodeSimulationSnapshot(
            bytes,decoded,&error));
        CHECK(error.empty());
        const auto decodedRuntime=
            decoded.runtime.find(firstId);
        CHECK(decodedRuntime!=decoded.runtime.end());
        const TradeContextPayload decodedPayload=
            tradeContextPayload(
                decodedRuntime->second.pendingContext);
        CHECK(decodedPayload.partner==payload.partner);
        CHECK(decodedPayload.originSettlement==payload.originSettlement);
        CHECK(decodedPayload.destinationSettlement==payload.destinationSettlement);
        CHECK(decodedPayload.originPos.x==payload.originPos.x);
        CHECK(decodedPayload.originPos.y==payload.originPos.y);
        CHECK(decodedPayload.returning==payload.returning);
    }

    GridPos firstPosition{};
    CHECK(simulation.runtimePosition(
        firstId,firstPosition));
    CHECK(firstPosition.x==runtimeOrigin.x);
    CHECK(firstPosition.y==runtimeOrigin.y);

    simulation.step(); // First real travel minute.
    CHECK(simulation.runtimePosition(
        firstId,firstPosition));
    CHECK(
        firstPosition.x!=runtimeOrigin.x
        || firstPosition.y!=runtimeOrigin.y);
    CHECK(
        simulation.world().characters[0]
            .civilization.inventory.count(
                ItemKind::RawMaterial,MaterialKind::Wood)
        ==firstWoodBefore);

    bool exchanged=false;
    bool returningObserved=false;
    bool returned=false;
    const int maxRuntimeMinutes=3*24*60-10;
    for(int i=0;i<maxRuntimeMinutes;++i){
        simulation.step();

        if(!exchanged
           && interSettlementTradeFactCount(
                simulation.socialKnowledge(),
                runtimeOriginSettlement,
                runtimeDestinationSettlement)>0){
            exchanged=true;
            CHECK(
                simulation.world().characters[0]
                    .civilization.inventory.count(
                        ItemKind::RawMaterial,MaterialKind::Wood)
                ==firstWoodBefore-1);
            CHECK(
                simulation.world().characters[0]
                    .civilization.inventory.count(
                        ItemKind::RawMaterial,MaterialKind::Stone)
                ==firstStoneBefore+1);

            const SimulationStateSnapshot duringReturn=
                simulation.captureSnapshot();
            const auto runtime=
                duringReturn.runtime.find(firstId);
            CHECK(runtime!=duringReturn.runtime.end());
            CHECK(runtime->second.pendingContext.active());
            CHECK(runtime->second.pendingContext.kind
                ==ContextActionKind::Trade);
            const TradeContextPayload payload=
                tradeContextPayload(
                    runtime->second.pendingContext);
            CHECK(payload.returning);
            returningObserved=true;
        }

        if(exchanged){
            pending=
                simulation.observePendingContextAction(
                    firstId);
            if(!pending.active){
                CHECK(simulation.runtimePosition(
                    firstId,firstPosition));
                CHECK(contextActionNearTarget(
                    firstPosition,runtimeOrigin,1));
                returned=true;
                break;
            }
        }
    }

    CHECK(exchanged);
    CHECK(returningObserved);
    CHECK(returned);

    const SettlementTradeNetworkObservation runtimeTradeNetwork=
        simulation.observeSettlementTradeNetwork();
    CHECK(runtimeTradeNetwork.routeCount==1);
    CHECK(runtimeTradeNetwork.exchangeEvidenceCount==1);
    CHECK(runtimeTradeNetwork.routes.front().firstSettlement
        ==std::min(
            runtimeOriginSettlement,
            runtimeDestinationSettlement));
    CHECK(runtimeTradeNetwork.routes.front().secondSettlement
        ==std::max(
            runtimeOriginSettlement,
            runtimeDestinationSettlement));

    std::cout
        << "C6-C inter-settlement travel trade and return passed"
        << " (one-way route cells=" << oneWayRouteLength << ")\n";
    return 0;
}
