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
    planningWorld.characters[1].lifeStage=LifeStage::Infant;
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

    // Runtime integration: the same household decision must become two
    // authoritative Core navigation actions toward the exact same frontier.
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
    SimulationStateSnapshot snapshot=
        simulation.captureSnapshot();
    CHECK(snapshot.world.characters.size()>=2);

    const CharacterId firstId=snapshot.world.characters[0].id;
    const CharacterId secondId=snapshot.world.characters[1].id;
    snapshot.world.characters.resize(2);
    // Keep only the two runtime entries.
    for(auto it=snapshot.runtime.begin();it!=snapshot.runtime.end();){
        if(it->first!=firstId && it->first!=secondId){
            it=snapshot.runtime.erase(it);
        }else{
            ++it;
        }
    }

    snapshot.households=HouseholdBook{};
    CHECK(snapshot.households.create(
        1,{firstId,secondId}));
    snapshot.world.minute=
        HouseholdMigrationDecisionIntervalMinutes*2;
    snapshot.world.facilities.clear();
    snapshot.world.storageSites.clear();
    makeLocalRegionScarce(snapshot.world,origin);

    for(Character& resident:snapshot.world.characters){
        resident.lifeStage=LifeStage::Adult;
        resident.personality.curiosity=0.90;
        resident.personality.adaptability=0.88;
        resident.needs.hunger=0.20;
        resident.needs.thirst=0.24;
        resident.needs.sleep=0.18;
        resident.needs.bladder=0.16;
        resident.needs.hygiene=0.18;
        resident.civilization.character=resident.id;

        auto runtime=snapshot.runtime.find(resident.id);
        CHECK(runtime!=snapshot.runtime.end());
        runtime->second.pos=origin;
        runtime->second.goal=Goal::Idle;
        runtime->second.plan.clear();
        runtime->second.pendingContext.clear();
        runtime->second.penaltyUntilMinute=0;
        runtime->second.navigationRoute.clear();
        runtime->second.navigationRouteIndex=0;
        runtime->second.navigationHasTarget=false;
        runtime->second.navigationArrived=false;
        runtime->second.navigationRouteFailed=false;
    }

    std::string error;
    CHECK(simulation.restoreSnapshot(snapshot,&error));
    CHECK(error.empty());

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
    CHECK(simulation.runtimePosition(firstId,firstPos));
    CHECK(simulation.runtimePosition(secondId,secondPos));
    CHECK(firstPos.x!=origin.x || firstPos.y!=origin.y);
    CHECK(secondPos.x!=origin.x || secondPos.y!=origin.y);

    std::cout
        << "C6-D household migration and settlement decline passed\n";
    return 0;
}
