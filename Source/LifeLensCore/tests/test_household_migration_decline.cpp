#include <algorithm>
#include <iostream>

#include "lifelens/SettlementMigration.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

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

    // Two equally pressured households remain physically eligible without a
    // model. A current accepted goal can choose among them, not manufacture
    // migration pressure, a dependent-transport action or a destination.
    {
        World competing=planningWorld;
        competing.minute=1;
        competing.characters.push_back(makeMigrant(3,"Third"));
        competing.characters.push_back(makeMigrant(4,"Fourth"));
        HouseholdBook groups=households;
        CHECK(groups.create(2,{3,4}));
        SettlementPopulation positions=population;
        positions.emplace(3,origin);
        positions.emplace(4,GridPos{1,0});
        const auto ordinary=chooseHouseholdMigrationPlan(
            competing,groups,positions);
        CHECK(ordinary.available &&ordinary.householdId==1);
        AcceptedCognitiveDecision thought;
        thought.actor=3;
        thought.minute=1;
        thought.trigger=CognitiveTriggerKind::Reflection;
        thought.contextFingerprint=790104;
        thought.backendIdentity="local-test";
        thought.intent=CognitiveIntentKind::MigrateHousehold;
        thought.priority=1.0;
        thought.rationaleDigest="existing-household-pressure";
        std::vector<AcceptedCognitiveDecision> journal;
        CHECK(appendAcceptedCognitiveDecision(journal,thought,competing));
        const auto selected=chooseHouseholdMigrationPlan(
            competing,groups,positions,&journal);
        CHECK(selected.available &&selected.householdId==2);
        CHECK(selected.consensusPressure01==ordinary.consensusPressure01);
        CHECK(selected.leaderPressure01>=HouseholdMigrationLeaderThreshold);
        CHECK(selected.members==std::vector<CharacterId>({3,4}));

        competing.characters[3].lifeStage=LifeStage::Baby;
        const auto noInfantWalking=chooseHouseholdMigrationPlan(
            competing,groups,positions,&journal);
        CHECK(noInfantWalking.available &&noInfantWalking.householdId==1);
        competing.characters[3].lifeStage=LifeStage::Adult;
        for(auto& resident:competing.characters)
            for(const auto material:MigrationFoundationalMaterials)
                resident.civilization.inventory.add({
                    ItemKind::RawMaterial,material,100,0.8,1.0});
        CHECK(!chooseHouseholdMigrationPlan(
            competing,groups,positions,&journal).available);
    }

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

    // Physical infrastructure remains observable, but with authoritative
    // population available an empty/declining cluster is not an "active
    // settlement". Only the lived cluster counts as active.
    const SettlementNetworkObservation declineNetwork=
        observeSettlementNetwork(
            declineWorld,&declinePopulation);
    CHECK(declineNetwork.settlementCount==2);
    CHECK(declineNetwork.activeSettlementCount==1);

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

    // Abandonment decay is structural/environmental, not limited to the three
    // facility kinds that have ordinary resident Repair actions. Empty fire,
    // storage, furnace and cultivation infrastructure must also deteriorate.
    World broadDecayWorld(606304);
    broadDecayWorld.characters.clear();
    broadDecayWorld.facilities.clear();
    broadDecayWorld.storageSites.clear();
    Character distantWitness=makeMigrant(30,"Witness");
    broadDecayWorld.characters.push_back(distantWitness);
    const GridPos abandonedAnchor{0,0};
    const GridPos witnessPos{SettlementServiceRadiusGrid*5,0};
    for(const auto kind:{
            FacilityKind::PrimitiveStorage,
            FacilityKind::FirePit,
            FacilityKind::Furnace,
            FacilityKind::CultivatedPlot}){
        broadDecayWorld.facilities.push_back(
            operationalFacility(
                300+static_cast<FacilityId>(
                    broadDecayWorld.facilities.size()),
                kind,
                abandonedAnchor,
                1.0));
    }
    SettlementPopulation broadDecayPopulation{{distantWitness.id,witnessPos}};
    broadDecayWorld.minute=60;
    std::vector<double> broadBefore;
    for(const auto& facility:broadDecayWorld.facilities){
        broadBefore.push_back(facility.durability);
    }
    advanceAbandonedSettlementDecayOneHour(
        broadDecayWorld,broadDecayPopulation);
    CHECK(broadDecayWorld.facilities.size()==broadBefore.size());
    for(std::size_t index=0;index<broadBefore.size();++index){
        CHECK(broadDecayWorld.facilities[index].durability<broadBefore[index]);
    }

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

    // Runtime choice, binary persistence and actual locomotion use the same
    // accepted input. No extra action slot or model call is created on replay.
    {
        Simulation choice(606303,0,CurrentWorldGenerationVersion,rules);
        choice.setupNewGame();
        auto ready=choice.captureSnapshot();
        CHECK(ready.world.characters.size()==4);
        const auto a=ready.world.characters[0].id;
        const auto b=ready.world.characters[1].id;
        const auto d=ready.world.characters[2].id;
        const auto e=ready.world.characters[3].id;
        const GridPos home=ready.runtime.at(a).pos;
        ready.households=HouseholdBook{};
        CHECK(ready.households.create(1,{a,b}));
        CHECK(ready.households.create(2,{d,e}));
        for(auto& resident:ready.world.characters){
            resident.lifeStage=LifeStage::Adult;
            resident.personality.curiosity=0.90;
            resident.personality.adaptability=0.88;
            resident.needs={0.20,0.24,0.18,0.16,0.18};
            resident.civilization.inventory=Inventory{};
            auto& runtime=ready.runtime.at(resident.id);
            runtime.pos=home;
            runtime.goal=Goal::Idle;
            runtime.plan.clear();
            runtime.pendingContext.clear();
            runtime.penaltyUntilMinute=10000;
        }
        CHECK(choice.restoreSnapshot(ready));
        depleteLiveRegion(choice.world(),home);
        choice.world().minute=HouseholdMigrationDecisionIntervalMinutes*2;
        CHECK(choice.observeHouseholdMigrationPlan().householdId==1);
        const auto request=choice.observeCognitiveRequest(
            d,CognitiveTriggerKind::MigrationPressure);
        CognitiveProposal proposal;
        proposal.actor=d;
        proposal.intent=CognitiveIntentKind::MigrateHousehold;
        proposal.priority=1.0;
        CHECK(choice.acceptCognitiveProposal(
            request,proposal,"local-test","real-household-migration"));
        const auto chosen=choice.observeHouseholdMigrationPlan();
        CHECK(chosen.available &&chosen.householdId==2);
        std::vector<std::uint8_t> bytes;
        CHECK(encodeSimulationSnapshot(choice.captureSnapshot(),bytes));
        SimulationStateSnapshot decoded;
        CHECK(decodeSimulationSnapshot(bytes,decoded));
        Simulation replay(606303,0,CurrentWorldGenerationVersion,rules);
        CHECK(replay.restoreSnapshot(decoded));
        choice.step();
        replay.step();
        for(const auto id:{d,e}){
            const auto pending=choice.observePendingContextAction(id);
            const auto restored=replay.observePendingContextAction(id);
            CHECK(pending.active);
            CHECK(pending.civilizationIntent==CivilizationIntent::Explore);
            CHECK(sameGridPos(pending.targetPos,chosen.target));
            CHECK(pending.token==restored.token);
            GridPos position{};
            CHECK(choice.runtimePosition(id,position));
            CHECK(manhattan(home,position)<=1);
        }
        for(int minute=0;minute<12;++minute){
            choice.step();
            replay.step();
        }
        GridPos moved{};
        CHECK(choice.runtimePosition(d,moved));
        CHECK(!sameGridPos(moved,home));
        std::vector<std::uint8_t> expected,actual;
        CHECK(encodeSimulationSnapshot(choice.captureSnapshot(),expected));
        CHECK(encodeSimulationSnapshot(replay.captureSnapshot(),actual));
        CHECK(expected==actual);
    }

    std::cout
        << "C6-D household migration and settlement decline passed\n";
    return 0;
}
