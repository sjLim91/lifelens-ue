#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/WildlifePopulationCodec.h"
#include <cassert>
#include <iostream>
#include <vector>

using namespace lifelens;

static WildlifePopulation population(WorldSeed seed,ChunkCoord coord={0,0})
{
    WildlifeHabitat habitat;
    habitat.available=true;
    habitat.coord=coord;
    habitat.forage01=1.0;
    habitat.shelter01=0.8;
    habitat.carryingCapacity={{24,12,4,0,2}};
    auto state=startWildlifePopulation(makeWorldGenesisIdentity(seed,0,4),habitat);
    state.animals.clear();
    return state;
}

static void addHare(WildlifePopulation& state,GridPos pos,double hunger=0.3)
{
    auto animal=wildlifeCreateAnimal(state,WildlifeSpecies::Hare,true);
    animal.position=pos;
    animal.hunger01=hunger;
    animal.sex=WildlifeSex::Male; // no births in stock-accounting fixtures
    state.animals.push_back(animal);
}

int main()
{
    // Actual stock is shared; duplicate day calls cannot consume it twice.
    {
        auto state=population(4242001);
        for(int i=0;i<3;++i) addHare(state,{10,10});
        std::vector<ResourceNode> plants={
            {1,MaterialKind::PlantFood,5,5,true,1,{10,10}}};
        const auto result=stepWildlifeDay(state,1,{},&plants);
        assert(result.advanced &&result.foraged==3 &&plants[0].quantity==2);
        assert(!stepWildlifeDay(state,1,{},&plants).advanced);
        assert(plants[0].quantity==2);
        // No remote grazing; no food synthesized from habitat.forage01=1.
        for(auto& animal:state.animals) animal.position={0,0};
        assert(stepWildlifeDay(state,2,{},&plants).foraged==0);
        assert(plants[0].quantity==2);
        // Finite fiber supports food competition without converting it to
        // a resident inventory stack.
        plants={{2,MaterialKind::Fiber,1,1,true,1,{0,0}}};
        assert(wildlifeConsumePlants(state,state.animals[0],plants,2880)==1);
        assert(wildlifeConsumePlants(state,state.animals[1],plants,2880)==0);
        assert(plants[0].quantity==0);
    }
    // A depleted habitat causes actual hunger and mortality despite its
    // unchanged procedural carrying capacity. No artificial culling is used.
    {
        auto hungry=population(4242001);
        for(int i=0;i<8;++i) addHare(hungry,{10,10},0.85);
        std::vector<ResourceNode> empty;
        auto day1=stepWildlifeDay(hungry,1,{},&empty);
        auto day2=stepWildlifeDay(hungry,2,{},&empty);
        assert(day1.foraged==0 &&day1.births==0);
        assert(day2.naturalDeaths==8 &&hungry.animals.empty());
    }
    // Deer browse young regrowth, never consume mature timber. Tree/node
    // accounting and the existing sapling regeneration path stay consistent.
    {
        auto state=population(4242001);
        auto deer=wildlifeCreateAnimal(state,WildlifeSpecies::Deer,true);
        deer.position={10,10};
        std::vector<ResourceNode> wood={
            {3,MaterialKind::Wood,8,8,true,1,{10,10}}};
        wood[0].ensureTrees();
        assert(wildlifeConsumePlants(state,deer,wood,1440)==0);
        wood[0].trees[0].bornMinute=0;
        assert(wildlifeConsumePlants(state,deer,wood,1440)==1);
        assert(wood[0].quantity==7 &&wood[0].trees[0].remainingWood==0);
        int total=0;
        for(const auto& tree:wood[0].trees) total+=tree.remainingWood;
        assert(total==wood[0].quantity);
        wood[0].ecologyMinute=1440;
        wood[0].regenerateDay();
        assert(wood[0].quantity==8);
        assert(wood[0].trees[0].bornMinute==1440);
        assert(wildlifeConsumePlants(state,deer,wood,wood[0].ecologyMinute)==1);
    }
    // Hungry herbivores move physically toward a real plant, with no jump,
    // chunk crossing or water shortcut. Empty source cannot relieve hunger.
    {
        World world(4242001,0,4);
        const auto landChunk=world.initialStartRegion().region.coord;
        const auto landOrigin=chunkOriginGrid(landChunk);
        auto state=population(world.seed,landChunk);
        GridPos start{},end{};
        bool found=false;
        for(int x=landOrigin.x+2;x<landOrigin.x+27 &&!found;++x)
            for(int y=landOrigin.y+2;y<landOrigin.y+29 &&!found;++y){
                bool land=true;
                for(int dx=0;dx<=4;++dx)
                    land=land &&coreGroundTraversable(world,{x+dx,y});
                if(land){start={x,y};end={x+4,y};found=true;}
            }
        assert(found);
        addHare(state,start,0.6);
        std::vector<ResourceNode> plants={
            {4,MaterialKind::Fiber,10,10,true,1,end}};
        const double hunger=state.animals[0].hunger01;
        assert(stepWildlifeHour(state,1,{},&plants)==1);
        assert(state.animals[0].position.x==start.x+1);
        assert(state.animals[0].position.y==start.y);
        assert(state.animals[0].hunger01==hunger);
        auto negative=population(world.seed,{-1,-1});
        addHare(negative,{-1,-1},0.6);
        plants[0].pos={0,0}; // neighbouring chunk is not this population's stock
        assert(wildlifeConsumePlants(negative,negative.animals[0],plants,1440)==0);
    }

    World world(4242001,0,4);
    world.resourceNodes.clear();
    world.storageSites.clear();
    world.wildlifeEnabled=true;
    const auto landChunk=world.initialStartRegion().region.coord;
    const auto landOrigin=chunkOriginGrid(landChunk);
    GridPos position{};
    bool found=false;
    for(int x=landOrigin.x+2;x<landOrigin.x+29 &&!found;++x)
        for(int y=landOrigin.y+2;y<landOrigin.y+29 &&!found;++y)
            if(coreGroundTraversable(world,{x,y})
               &&coreGroundTraversable(world,{x+1,y})){
                position={x,y};found=true;
            }
    assert(found);
    auto state=population(world.seed,landChunk);
    addHare(state,{position.x+1,position.y});
    world.wildlifePopulations={state};
    Character actor;
    actor.id=1;
    actor.civilization.character=actor.id;
    actor.civilization.gatheringSkill=1.0;
    actor.needs={0.9,0.05,0.05,0.05,0.05};

    const auto decision=urgentSurvivalProvisionDecisionAtPosition(world,actor,position);
    assert(decision.intent==CivilizationIntent::Hunt);
    assert(decision.resourceNode==state.animals.front().id);
    // Planning is read-only. A child, satiated resident or resident already
    // carrying edible provisions does not get a speculative hunt.
    assert(world.wildlifePopulations[0].animals.size()==1);
    auto child=actor;child.lifeStage=LifeStage::Child;
    assert(bestHuntDecisionAtPosition(world,child,position).intent==CivilizationIntent::None);
    auto fed=actor;fed.needs.hunger=0.1;
    assert(bestHuntDecisionAtPosition(world,fed,position).intent==CivilizationIntent::None);
    fed=actor;
    fed.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::PlantFood,1,0.5,1.0});
    assert(bestHuntDecisionAtPosition(world,fed,position).intent==CivilizationIntent::None);
    // Distance gates are rechecked at completion, not only during planning.
    assert(!executeCivilizationDecisionAtPosition(
        world,actor,decision,{position.x+20,position.y}).executed);
    const auto initialHunger=actor.needs.hunger;
    bool caught=false;
    for(int attempt=0;attempt<64 &&!caught;++attempt){
        world.minute=500+20*attempt;
        const auto result=executeCivilizationDecisionAtPosition(
            world,actor,decision,position);
        assert(result.executed);
        if(!result.success){
            assert(actor.needs.hunger==initialHunger);
            assert(world.wildlifePopulations[0].animals.size()==1);
        }else{
            caught=true;
            assert(result.event.type==CivilizationEventType::Hunted);
            assert(actor.needs.hunger<initialHunger);
            assert(world.wildlifePopulations[0].animals.empty());
        }
    }
    assert(caught);
    bool failureObserved=false;
    for(int attempt=0;attempt<128 &&!failureObserved;++attempt){
        auto failureWorld=world;
        auto failureActor=actor;
        failureActor.needs.hunger=initialHunger;
        failureWorld.wildlifePopulations={state};
        failureWorld.minute=500+20*attempt;
        const auto result=executeCivilizationDecisionAtPosition(
            failureWorld,failureActor,decision,position);
        if(result.executed &&!result.success){
            failureObserved=true;
            assert(result.event.type==CivilizationEventType::HuntFailed);
            assert(failureActor.needs.hunger==initialHunger);
            assert(failureWorld.wildlifePopulations[0].animals.size()==1);
        }
    }
    assert(failureObserved);
    const auto after=actor.needs.hunger;
    assert(!executeCivilizationDecisionAtPosition(world,actor,decision,position).executed);
    assert(actor.needs.hunger==after);
    assert(actor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)==0); // no fake plant/meat
    assert(validWildlifePopulation(world.wildlifePopulations[0]));
    // Deer require a real surviving tool. Predators/fish are not free food.
    assert(!residentCanHuntSpecies(actor,WildlifeSpecies::Deer));
    actor.civilization.inventory.add({ItemKind::SharpFlake,MaterialKind::Flint,1,0.8,1.0});
    assert(residentCanHuntSpecies(actor,WildlifeSpecies::Deer));
    assert(!residentCanHuntSpecies(actor,WildlifeSpecies::Tiger));
    assert(!residentCanHuntSpecies(actor,WildlifeSpecies::FreshwaterFish));
    {
        auto threat=wildlifeCreateAnimal(world.wildlifePopulations[0],WildlifeSpecies::Wolf,true);
        threat.position=position;
        world.wildlifePopulations[0].animals.push_back(threat);
        assert(!subsistenceHuntSafe(world,position));
        world.wildlifePopulations[0].animals.clear();
    }

    // Production handoff uses real resources too, not only standalone tests.
    // Fixture starts one minute before the authoritative daily boundary.
    {
        Simulation sim(77,0,4);
        sim.setupNewGame();
        auto& w=sim.world();
        for(auto& resident:w.characters){
            resident.alive=false;
            resident.deathMinute=w.minute;
        }
        w.minute=1439;
        const auto coord=w.initialStartRegionCoord;
        const auto origin=chunkOriginGrid(coord);
        const GridPos plantPos{origin.x+5,origin.y+5};
        auto grazers=population(w.seed,coord);
        addHare(grazers,plantPos);
        w.wildlifePopulations={grazers};
        w.resourceNodes={{1,MaterialKind::Fiber,5,5,false,0,plantPos}};
        sim.step();
        assert(w.minute==1440);
        assert(w.resourceNodes[0].quantity==4);
        assert(w.wildlifePopulations[0].dayIndex==1);
    }

    // Actual Simulation scheduling -> pending token -> early/remote/stale ACK
    // rejection -> elapsed work -> single result, with binary continuation.
    {
        Simulation sim(77,0,4);
        sim.setupNewGame();
        sim.setExternalPhysicalExecution(true);
        auto& w=sim.world();
        const auto id=w.characters.front().id;
        for(auto& resident:w.characters){
            resident.needs={0.05,0.05,0.05,0.05,0.05};
            if(resident.id!=id){resident.alive=false;resident.deathMinute=w.minute;}
        }
        auto& hunter=w.characters.front();
        hunter.needs.hunger=0.98;
        hunter.civilization.inventory=Inventory{};
        for(auto& node:w.resourceNodes){
            Inventory discarded;
            node.harvest(node.quantity,discarded);
            node.regenerationPerDay=0;
        }
        w.storageSites.clear();
        GridPos from{};
        assert(sim.runtimePosition(id,from));
        auto prey=population(w.seed,chunkCoordForGrid(from));
        addHare(prey,from);
        w.wildlifePopulations={prey};
        sim.step();
        const auto pending=sim.observePendingContextAction(id);
        assert(pending.active &&pending.civilizationIntent==CivilizationIntent::Hunt);
        assert(!sim.completeExternalContextAction(id,pending.token,from)); // too early
        sim.runMinutes(20);
        // Critical hunger must preserve the same food-acquisition token,
        // rather than cancel/reissue it every minute before work can finish.
        assert(sim.observePendingContextAction(id).token==pending.token);
        assert(!sim.completeExternalContextAction(id,pending.token+1,from));
        const auto saved=sim.captureSnapshot();
        std::vector<std::uint8_t> bytes;
        std::string error;
        assert(encodeSimulationSnapshot(saved,bytes,&error));
        SimulationStateSnapshot decoded;
        assert(decodeSimulationSnapshot(bytes,decoded,&error));
        Simulation resumed(1);
        assert(resumed.restoreSnapshot(decoded,&error));
        resumed.setExternalPhysicalExecution(true);
        assert(resumed.observePendingContextAction(id).civilizationIntent==CivilizationIntent::Hunt);
        assert(sim.completeExternalContextAction(id,pending.token,from));
        assert(resumed.completeExternalContextAction(id,pending.token,from));
        assert(!sim.completeExternalContextAction(id,pending.token,from));
        assert(sim.world().wildlifePopulations[0].animals.size()
            ==resumed.world().wildlifePopulations[0].animals.size());
        assert(sim.world().characters.front().needs.hunger
            ==resumed.world().characters.front().needs.hunger);
    }
    std::cout<<"wildlife food web and resident subsistence passed\n";
}
