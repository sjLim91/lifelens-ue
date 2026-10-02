#include <algorithm>
#include <iostream>

#include "lifelens/SettlementMigration.h"
#include "lifelens/Simulation.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character makeMigrant(CharacterId id,const char* name)
{
    Character resident;
    resident.id=id;
    resident.name=name;
    resident.alive=true;
    resident.lifeStage=LifeStage::Adult;
    resident.civilization.character=id;
    resident.personality.curiosity=0.90;
    resident.personality.adaptability=0.88;
    resident.needs.hunger=0.20;
    resident.needs.thirst=0.24;
    resident.needs.sleep=0.18;
    resident.needs.bladder=0.16;
    resident.needs.hygiene=0.18;
    return resident;
}

static void makeLocalRegionScarce(World& world,GridPos anchor)
{
    world.resourceNodes.clear();
    world.storageSites.clear();
    world.generatedNaturalChunks.clear();
    const ChunkCoord center=chunkCoordForGrid(anchor);
    for(int dx=-ResourceExplorationMaxRadiusChunks;
        dx<=ResourceExplorationMaxRadiusChunks;++dx){
        for(int dy=-ResourceExplorationMaxRadiusChunks;
            dy<=ResourceExplorationMaxRadiusChunks;++dy){
            world.materializeNaturalChunk({center.x+dx,center.y+dy});
        }
    }
    for(ResourceNode& node:world.resourceNodes){
        node.quantity=0;
    }
}

static void depleteLiveRegion(World& world,GridPos anchor)
{
    const ChunkCoord center=chunkCoordForGrid(anchor);
    for(int dx=-ResourceExplorationMaxRadiusChunks;
        dx<=ResourceExplorationMaxRadiusChunks;++dx){
        for(int dy=-ResourceExplorationMaxRadiusChunks;
            dy<=ResourceExplorationMaxRadiusChunks;++dy){
            const ChunkCoord chunk{center.x+dx,center.y+dy};
            if(world.findGeneratedNaturalChunk(chunk)==nullptr){
                world.materializeNaturalChunk(chunk);
            }
        }
    }
    for(ResourceNode& node:world.resourceNodes){
        node.quantity=0;
    }
    world.storageSites.clear();
    world.facilities.clear();
}

static ConstructedFacility operationalFacility(
    FacilityId id,
    FacilityKind kind,
    GridPos pos,
    double durability)
{
    ConstructedFacility facility;
    facility.id=id;
    facility.kind=kind;
    facility.pos=pos;
    facility.state=FacilityState::Operational;
    facility.active=true;
    facility.durability=durability;
    return facility;
}

int main()
{
    const GridPos origin{0,0};

    // Pure decision model: two co-located adults in one household should turn
    // shared local depletion into one common frontier destination.
    World planningWorld(606301);
    planningWorld.characters.clear();
    planningWorld.facilities.clear();
    planningWorld.storageSites.clear();
    planningWorld.generatedNaturalChunks.clear();
    planningWorld.characters={
        makeMigrant(1,"First"),
        makeMigrant(2,"Second")
    };
    makeLocalRegionScarce(planningWorld,origin);

    HouseholdBook households;
    CHECK(households.create(1,{1,2}));
    SettlementPopulation population{{1,origin},{2,{1,0}}};

    const HouseholdMigrationPlan plan=
        chooseHouseholdMigrationPlan(
            planningWorld,households,population);
    CHECK(plan.available);
    CHECK(plan.householdId==1);
    CHECK(plan.members.size()==2);
    CHECK(plan.bottleneckMaterial!=MaterialKind::Unknown);
    CHECK(plan.consensusPressure01>=HouseholdMigrationConsensusThreshold);
    CHECK(manhattan(plan.target,origin)>SettlementServiceRadiusGrid);
    CHECK(planningWorld.findGeneratedNaturalChunk(plan.targetChunk)==nullptr);

    // A dependent household is intentionally deferred until a real carry /
    // accompany action exists; C6-D must never fake infant locomotion.
    planningWorld.characters[1].lifeStage=LifeStage::Baby;
    const HouseholdMigrationPlan dependentPlan=
        chooseHouseholdMigrationPlan(
            planningWorld,households,population);
    CHECK(!dependentPlan.available);

    // Empty infrastructure becomes declining and physically loses durability,
    // while an inhabited settlement is not charged abandonment wear.
    World declineWorld(606302);
    declineWorld.characters.clear();
    declineWorld.facilities.clear();
    declineWorld.storageSites.clear();
    declineWorld.characters={
        makeMigrant(10,"Away"),
        makeMigrant(20,"Home")
    };

    const GridPos emptySite{0,0};
    const GridPos inhabitedSite{SettlementServiceRadiusGrid*3,0};
    declineWorld.facilities.push_back(
        operationalFacility(100,FacilityKind::Shelter,emptySite,1.0));
    declineWorld.facilities.push_back(
        operationalFacility(200,FacilityKind::Shelter,inhabitedSite,1.0));

    StorageSite emptyStorage;
    emptyStorage.id=100;
    emptyStorage.pos=emptySite;
    declineWorld.storageSites.push_back(emptyStorage);

    StorageSite inhabitedStorage;
    inhabitedStorage.id=200;
    inhabitedStorage.pos=inhabitedSite;
    declineWorld.storageSites.push_back(inhabitedStorage);

    SettlementPopulation declinePopulation{
        {10,{SettlementServiceRadiusGrid*6,0}},
        {20,inhabitedSite}
    };
    SettlementLifecycleObservation lifecycle=
        observeSettlementLifecycle(
            declineWorld,declinePopulation);
    CHECK(lifecycle.decliningCount==1);
    CHECK(lifecycle.inhabitedCount==1);

    declineWorld.minute=60;
    const double emptyBefore=declineWorld.facilities[0].durability;
    const double inhabitedBefore=declineWorld.facilities[1].durability;
    advanceAbandonedSettlementDecayOneHour(
        declineWorld,declinePopulation);
    CHECK(declineWorld.facilities[0].durability<emptyBefore);
    CHECK(declineWorld.facilities[1].durability==inhabitedBefore);

    declineWorld.facilities[0].durability=
        AbandonedSettlementHourlyWear*0.5;
    declineWorld.minute=120;
    advanceAbandonedSettlementDecayOneHour(
        declineWorld,declinePopulation);
    CHECK(declineWorld.facilities[0].state==FacilityState::Ruined);

    lifecycle=observeSettlementLifecycle(
        declineWorld,declinePopulation);
    CHECK(lifecycle.abandonedCount==1);

    // Runtime integration: use the real Simulation world and its public
    // authority. Do not hand-edit world-generation snapshot internals merely
    // to manufacture scarcity.
    SimulationRuleset rules=DefaultSimulationRuleset;
    rules.needs.hungerPerMinute=0.0;
    rules.needs.thirstPerMinute=0.0;
    rules.needs.sleepPerMinute=0.0;
    rules.needs.bladderPerMinute=0.0;
    rules.needs.hygienePerMinute=0.0;

    Simulation simulation(
        606303,
        0,
        CurrentWorldGenerationVersion,
        rules);
    simulation.setupNewGame();
    CHECK(simulation.world().characters.size()>=2);

    Character& first=simulation.world().characters[0];
    Character& second=simulation.world().characters[1];
    const CharacterId firstId=first.id;
    const CharacterId secondId=second.id;

    GridPos firstOrigin{};
    GridPos secondOrigin{};
    CHECK(simulation.runtimePosition(firstId,firstOrigin));
    CHECK(simulation.runtimePosition(secondId,secondOrigin));
    CHECK(manhattan(firstOrigin,secondOrigin)<=SettlementServiceRadiusGrid);

    simulation.households()=HouseholdBook{};
    CHECK(simulation.households().create(
        1,{firstId,secondId}));

    for(Character* resident:{&first,&second}){
        resident->lifeStage=LifeStage::Adult;
        resident->personality.curiosity=0.90;
        resident->personality.adaptability=0.88;
        resident->needs.hunger=0.20;
        resident->needs.thirst=0.24;
        resident->needs.sleep=0.18;
        resident->needs.bladder=0.16;
        resident->needs.hygiene=0.18;
        resident->civilization.character=resident->id;
    }

    depleteLiveRegion(simulation.world(),firstOrigin);
    simulation.world().minute=
        HouseholdMigrationDecisionIntervalMinutes*2;

    const HouseholdMigrationPlan runtimePlan=
        simulation.observeHouseholdMigrationPlan();
    CHECK(runtimePlan.available);
    CHECK(runtimePlan.householdId==1);
    CHECK(runtimePlan.members.size()==2);

    simulation.step();

    const PendingContextActionObservation firstPending=
        simulation.observePendingContextAction(firstId);
    const PendingContextActionObservation secondPending=
        simulation.observePendingContextAction(secondId);
    CHECK(firstPending.active);
    CHECK(secondPending.active);
    CHECK(firstPending.kind==ContextActionKind::Civilization);
    CHECK(secondPending.kind==ContextActionKind::Civilization);
    CHECK(firstPending.civilizationIntent==CivilizationIntent::Explore);
    CHECK(secondPending.civilizationIntent==CivilizationIntent::Explore);
    CHECK(firstPending.targetPos.x==secondPending.targetPos.x);
    CHECK(firstPending.targetPos.y==secondPending.targetPos.y);

    GridPos firstPos{};
    GridPos secondPos{};
    bool firstMoved=false;
    bool secondMoved=false;
    for(int minute=0;minute<12 && (!firstMoved || !secondMoved);++minute){
        CHECK(simulation.runtimePosition(firstId,firstPos));
        CHECK(simulation.runtimePosition(secondId,secondPos));
        firstMoved=firstMoved
            || firstPos.x!=firstOrigin.x || firstPos.y!=firstOrigin.y;
        secondMoved=secondMoved
            || secondPos.x!=secondOrigin.x || secondPos.y!=secondOrigin.y;
        if(!firstMoved || !secondMoved) simulation.step();
    }
    CHECK(firstMoved);
    CHECK(secondMoved);

    std::cout
        << "C6-D household migration and settlement decline passed\n";
    return 0;
}
