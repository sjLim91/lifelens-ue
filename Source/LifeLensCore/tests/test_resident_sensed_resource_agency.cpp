#include <array>
#include <cassert>
#include <utility>
#include <iostream>
#include <vector>
#include "lifelens/ResidentSensedResourceAgency.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

static Character resident(CharacterId id)
{
    Character c;
    c.id=id;
    c.civilization.character=id;
    c.needs={0.05,0.05,0.05,0.05,0.05};
    c.personality.curiosity=1.0;
    c.personality.sociability=0.0;
    c.lifeStage=LifeStage::Adult;
    return c;
}

int main()
{
    World world(77,0,4);
    world.minute=480;
    world.resourceNodes.clear();
    world.storageSites.clear();
    world.facilities.clear();
    world.characters={resident(1),resident(2)};
    RelationshipBook relationships;
    const auto chunk=world.initialStartRegion().region.coord;
    const auto origin=chunkOriginGrid(chunk);
    GridPos position{};
    bool land=false;
    for(int x=origin.x+2;x<origin.x+28 &&!land;++x)
        for(int y=origin.y+2;y<origin.y+29 &&!land;++y){
            if(coreGroundTraversable(world,{x,y})
               &&coreGroundTraversable(world,{x+1,y})
               &&coreGroundTraversable(world,{x+2,y})){
                position={x,y};land=true;
            }
        }
    assert(land);
    world.resourceNodes={
        {101,MaterialKind::Stone,12,12,false,0,{position.x+2,position.y}},
        {102,MaterialKind::Stone,12,12,false,0,{position.x+40,position.y}}
    };
    ResidentAgencyChoice fallback;
    fallback.decision.kind=UnifiedDecisionKind::Civilization;
    fallback.decision.civilization.intent=CivilizationIntent::Explore;
    fallback.decision.utility=0.01;
    const auto rng=world.rng;
    const auto beforeMemory=world.characters[0].memory.entries.size();
    const auto selected=chooseResidentSensedResourceGoal(
        world,world.characters[0],position,relationships,fallback,nullptr,false);
    assert(selected.personalGoal);
    assert(selected.agency.authority==ResidentAgencyAuthority::PersonalGoal);
    assert(selected.agency.decision.kind==UnifiedDecisionKind::Civilization);
    assert(selected.agency.decision.civilization.intent==CivilizationIntent::Gather);
    assert(selected.agency.decision.civilization.resourceNode==101);
    assert(world.resourceNodes[0].quantity==12 &&world.resourceNodes[1].quantity==12);
    assert(world.characters[0].civilization.inventory.stacks().empty());
    assert(world.characters[0].memory.entries.size()==beforeMemory);
    assert(world.rng==rng &&world.minute==480);
    assert(!*autonomousGoalSatisfiedByTruth(world,relationships,*selected.personalGoal));

    // Production resident-goal choice must re-evaluate firsthand evidence
    // against the LIVE source, not permanently obey a stale depletion belief.
    World revivedWorld=world;
    auto& revivedActor=revivedWorld.characters[0];
    revivedWorld.resourceNodes.front().quantity=0;
    const auto observedFailure=diagnoseResidentGatherFailure(
        revivedWorld,revivedActor,selected.agency.decision.civilization,
        position);
    assert(observedFailure==ResidentGatherFailureCause::SourceExhausted);
    rememberCivilizationAttemptOutcome(
        revivedActor,selected.agency.decision.civilization,480,false,
        observedFailure);
    // The exact Core source is later replenished. The resident needs a fresh
    // observation, not a fabricated success or an arbitrary memory reset.
    revivedWorld.minute=495;
    revivedWorld.resourceNodes.front().quantity=12;
    const auto depletion=buildResidentResourceOutcomeIndex(revivedActor,495);
    assert(depletion.find(101,"Civilization:Gather:Stone").reason
        ==ResidentResourceOutcomeReason::SourceUnavailable);
    const auto revived=chooseResidentSensedResourceGoal(
        revivedWorld,revivedActor,position,relationships,fallback,nullptr,false);
    assert(revived.personalGoal);
    assert(revived.agency.decision.civilization.resourceNode==101);
    assert(revivedWorld.resourceNodes.front().quantity==12);
    // No other resident imports this direct failure, and no world transition
    // or phantom Gather success is manufactured by cognition.
    assert(revivedWorld.characters[1].memory.entries.empty());
    assert(revivedWorld.characters[0].civilization.inventory.stacks().empty());
    assert(revivedWorld.rng==rng &&revivedWorld.minute==495);

    // A real scarcity Explore with slightly stronger intrinsic utility must
    // not be displaced by the personal resource curiosity bonus alone.
    auto shortageExplore=fallback;
    shortageExplore.decision.utility=selected.agency.decision.utility+0.03;
    assert(shortageExplore.decision.utility<1.0);
    const auto retainedExplore=chooseResidentSensedResourceGoal(
        world,world.characters[0],position,relationships,shortageExplore,nullptr,false);
    assert(!retainedExplore.personalGoal);
    assert(retainedExplore.agency.decision.civilization.intent
        ==CivilizationIntent::Explore);

    // The neighbour cannot use the first resident's scan. Current personal
    // position is rescanned instead of accepting another mind's request.
    const auto remote=chooseResidentSensedResourceGoal(
        world,world.characters[1],{position.x+20,position.y},relationships,
        fallback,nullptr,false);
    assert(!remote.personalGoal);
    assert(remote.agency.authority==fallback.authority);

    // Actual distance and resource execution boundaries still own output.
    auto& actor=world.characters[0];
    const auto decision=selected.agency.decision.civilization;
    const auto blocked=executeCivilizationDecisionAtPosition(
        world,actor,decision,{position.x+20,position.y});
    assert(!blocked.executed &&world.resourceNodes[0].quantity==12);
    assert(!*autonomousGoalSatisfiedByTruth(world,relationships,*selected.personalGoal));
    GridPos access{};
    assert(resolveCivilizationResourceAccessGridPosition(world,101,access));
    const auto executed=executeCivilizationDecisionAtPosition(world,actor,decision,access);
    assert(executed.executed &&executed.success &&executed.event.quantity>0);
    assert(world.resourceNodes[0].quantity==12-executed.event.quantity);
    assert(actor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Stone)==executed.event.quantity);
    assert(world.characters[1].civilization.inventory.stacks().empty());
    assert(*autonomousGoalSatisfiedByTruth(world,relationships,*selected.personalGoal));
    assert(!chooseResidentSensedResourceGoal(
        world,actor,position,relationships,fallback,nullptr,false).personalGoal);
    actor.civilization.inventory=Inventory{};

    auto cognition=fallback;
    cognition.authority=ResidentAgencyAuthority::ResidentCognition;
    assert(!chooseResidentSensedResourceGoal(
        world,actor,position,relationships,cognition,nullptr,false).personalGoal);
    auto social=fallback;
    social.decision.kind=UnifiedDecisionKind::Social;
    social.decision.social={SocialIntent::Comfort,2,0.5};
    assert(!chooseResidentSensedResourceGoal(
        world,actor,position,relationships,social,nullptr,false).personalGoal);
    auto construction=fallback;
    construction.decision.civilization.intent=CivilizationIntent::Craft;
    construction.decision.civilization.facilityAction=FacilityBuildAction::Work;
    assert(!chooseResidentSensedResourceGoal(
        world,actor,position,relationships,construction,nullptr,false).personalGoal);
    auto bodily=fallback;
    bodily.decision.kind=UnifiedDecisionKind::Physical;
    bodily.decision.physicalGoal=Goal::Sleep;
    assert(!chooseResidentSensedResourceGoal(
        world,actor,position,relationships,bodily,nullptr,false).personalGoal);
    assert(!chooseResidentSensedResourceGoal(
        world,actor,position,relationships,fallback,nullptr,true).personalGoal);
    actor.needs.thirst=0.99; // independently check urgency, even with false flag
    assert(!chooseResidentSensedResourceGoal(
        world,actor,position,relationships,fallback,nullptr,false).personalGoal);
    actor.needs.thirst=0.05;
    // Ordinary nonurgent sleep pressure must also preserve a recovery
    // opportunity instead of starting optional curiosity travel.
    actor.needs.sleep=0.56;
    assert(!chooseResidentSensedResourceGoal(
        world,actor,position,relationships,fallback,nullptr,false).personalGoal);
    actor.needs.sleep=0.05;
    actor.lifeStage=LifeStage::Child;
    assert(!chooseResidentSensedResourceGoal(
        world,actor,position,relationships,fallback,nullptr,false).personalGoal);
    actor.lifeStage=LifeStage::Adult;
    world.resourceNodes[0].quantity=0; // memory/goal cannot restore live stock
    assert(!chooseResidentSensedResourceGoal(
        world,actor,position,relationships,fallback,nullptr,false).personalGoal);
    world.resourceNodes[0].quantity=12;
    world.minute=481;
    assert(!chooseResidentSensedResourceGoal(
        world,actor,position,relationships,fallback,nullptr,false).personalGoal);

    // Optional curiosity must not retry the same genuinely inaccessible
    // source endlessly. The resident's native fallback remains untouched.
    world.minute=495; // legal 15-minute decision boundary
    rememberCivilizationAttemptOutcome(
        actor,decision,495,false,ResidentGatherFailureCause::ResourceAccessUnavailable);
    const auto failedAgency=chooseResidentSensedResourceGoal(
        world,actor,position,relationships,fallback,nullptr,false);
    assert(!failedAgency.personalGoal);
    assert(failedAgency.agency.authority==fallback.authority);
    assert(world.resourceNodes.front().quantity==12);
    // M2-09: a personally failed source does not trap the resident in an
    // infinite same-node retry. Replan from the SAME live, bounded local
    // perception to a second genuinely reachable source and prove one real
    // Core Gather result, without forcing another AI decision slot.
    World rerouteWorld=world;
    rerouteWorld.resourceNodes.push_back(
        {103,MaterialKind::Stone,12,12,false,0,
            {position.x+1,position.y}});
    auto& rerouteActor=rerouteWorld.characters[0];
    const auto reroute=chooseResidentSensedResourceGoal(
        rerouteWorld,rerouteActor,position,relationships,fallback,nullptr,false);
    assert(reroute.personalGoal);
    assert(reroute.agency.authority==ResidentAgencyAuthority::PersonalGoal);
    assert(reroute.agency.decision.civilization.resourceNode==103);
    GridPos rerouteAccess{};
    assert(resolveCivilizationResourceAccessGridPosition(
        rerouteWorld,103,rerouteAccess));
    const auto rerouteOutcome=executeCivilizationDecisionAtPosition(
        rerouteWorld,rerouteActor,reroute.agency.decision.civilization,
        rerouteAccess);
    assert(rerouteOutcome.executed &&rerouteOutcome.success
        &&rerouteOutcome.event.quantity>0);
    rememberCivilizationAttemptOutcome(
        rerouteActor,reroute.agency.decision.civilization,
        rerouteWorld.minute,true);
    assert(rerouteActor.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Stone)>0);
    assert(rerouteWorld.resourceNodes.front().quantity==12);
    assert(!rerouteWorld.characters[1].memory.entries.size());
    assert(world.resourceNodes.size()==2);
    assert(world.resourceNodes.front().quantity==12);
    assert(!failedAgency.personalGoal); // original failure still meaningful
    // Another actor has not witnessed this access failure.
    const auto independent=chooseResidentSensedResourceGoal(
        world,world.characters[1],position,relationships,fallback,nullptr,false);
    assert(independent.personalGoal);
    assert(independent.agency.decision.civilization.resourceNode==101);
    // Real later success resets the personal source exclusion, without any
    // invented World resource regeneration or a stored cooldown flag.
    world.minute=510;
    rememberCivilizationAttemptOutcome(actor,decision,510,true);
    const auto recovered=chooseResidentSensedResourceGoal(
        world,actor,position,relationships,fallback,nullptr,false);
    assert(recovered.personalGoal);
    assert(recovered.agency.decision.civilization.resourceNode==101);

    // Production one-slot planning -> pending actual Gather -> binary resume ->
    // single output. No external model/accepted cognition is required.
    Simulation sim(77,0,4);
    sim.setupNewGame();
    sim.setExternalPhysicalExecution(true);
    auto& w=sim.world();
    const auto id=w.characters.front().id;
    for(auto& c:w.characters){
        c.needs={0.05,0.05,0.05,0.05,0.05};
        c.civilization.inventory=Inventory{};
        if(c.id!=id){c.alive=false;c.deathMinute=w.minute;}
    }
    w.characters.front().personality.curiosity=1.0;
    w.characters.front().personality.openness=1.0;
    w.characters.front().personality.adaptability=1.0;
    w.characters.front().personality.patience=1.0;
    w.characters.front().civilization.gatheringSkill=1.0;
    for(auto& node:w.resourceNodes){
        Inventory removed;
        node.harvest(node.quantity,removed);
        node.regenerationPerDay=0;
    }
    GridPos actual{};
    assert(sim.runtimePosition(id,actual));
    assert(!sim.observePendingContextAction(id).active);
    // This is a controlled later-life world fixture: actual shared raw stock
    // covers non-food scarcity while the actor still carries no ingredients.
    // Otherwise a genuinely depleted Flint/Fiber/Clay world should keep
    // exploring instead of allowing a tiny optional food-stock preference to
    // override its much stronger scarcity need.
    const std::array<MaterialKind,5> coveredMaterials={{
        MaterialKind::Flint,MaterialKind::Fiber,MaterialKind::Clay,
        MaterialKind::Stone,MaterialKind::Wood
    }};
    StorageSite reserve;
    reserve.id=9003;
    reserve.pos=actual;
    for(const auto material:coveredMaterials)
        reserve.inventory.add({ItemKind::RawMaterial,material,5,0.5,1.0});
    w.storageSites.push_back(std::move(reserve));
    // Keep a real local water supply: depleting every natural source would
    // correctly make the baseline prefer freshwater exploration, not optional
    // food-stock gathering. No vessel or carried inventory is injected.
    w.resourceNodes.push_back(
        {9002,MaterialKind::Water,12,12,false,0,actual});
    // Ordinary provision Gather has no distance penalty inside this range:
    // equal live supplies preserve vector order and would pick remote 9000.
    // Only the new personal sensed goal can prefer the observed local 9001.
    w.resourceNodes.push_back(
        {9000,MaterialKind::PlantFood,12,12,false,0,{actual.x+40,actual.y}});
    w.resourceNodes.push_back(
        {9001,MaterialKind::PlantFood,12,12,false,0,actual});
    assert(bestGatherDecisionAtPosition(w,w.characters.front(),actual,
        nullptr,MaterialKind::PlantFood).resourceNode==9000);
    SettlementPopulation fixturePopulation;
    for(const auto& c:w.characters){
        GridPos at{};
        if(sim.runtimePosition(c.id,at)) fixturePopulation.emplace(c.id,at);
    }
    for(const auto material:coveredMaterials)
        assert(civilizationResourceExplorationPressure(
            w,w.characters.front(),material,actual)==0.0);
    assert(civilizationResourceExplorationPressure(
        w,w.characters.front(),MaterialKind::Water,actual)==0.0);
    assert(bestResourceExplorationDecisionAtPosition(
        w,w.characters.front(),actual,&fixturePopulation).intent
            ==CivilizationIntent::None);
    const auto nativeDecision=chooseUnifiedUtilityDecisionAtPosition(
        w,w.characters.front(),sim.relationships(),actual,0.18,0.14,
        &fixturePopulation,&sim.socialKnowledge(),&sim.households());
    if(nativeDecision.kind!=UnifiedDecisionKind::Civilization
       ||nativeDecision.civilization.intent!=CivilizationIntent::Gather
       ||nativeDecision.civilization.resourceNode!=9000){
        std::cerr<<"two-source fixture native kind="
            <<static_cast<int>(nativeDecision.kind)
            <<" intent="<<civilizationIntentName(nativeDecision.civilization.intent)
            <<" material="<<materialName(nativeDecision.civilization.material)
            <<" node="<<nativeDecision.civilization.resourceNode
            <<" utility="<<nativeDecision.utility<<"\n";
    }
    assert(nativeDecision.kind==UnifiedDecisionKind::Civilization);
    assert(nativeDecision.civilization.intent==CivilizationIntent::Gather);
    assert(nativeDecision.civilization.resourceNode==9000);
    const auto nativeAgency=chooseSingleResidentAgencyDecision(
        w,w.characters.front(),actual,sim.relationships(),nativeDecision,
        nullptr,&fixturePopulation,std::nullopt,false);
    const auto agencyBefore=chooseResidentSensedResourceGoal(
        w,w.characters.front(),actual,sim.relationships(),nativeAgency,
        &fixturePopulation,false,&sim.socialKnowledge(),&sim.households());
    assert(agencyBefore.personalGoal);
    assert(agencyBefore.agency.decision.civilization.resourceNode==9001);
    sim.step();
    const auto pending=sim.observePendingContextAction(id);
    assert(pending.active &&pending.kind==ContextActionKind::Civilization);
    assert(pending.civilizationIntent==CivilizationIntent::Gather);
    assert(pending.resourceNode==9001 &&pending.hasSpatialTarget);
    assert(w.resourceNodes.back().quantity==12);
    assert(w.characters.front().civilization.inventory.stacks().empty());
    const GridPos wrong{actual.x+20,actual.y};
    assert(!sim.completeExternalContextAction(id,pending.token,wrong));
    assert(w.resourceNodes.back().quantity==12);

    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(sim.captureSnapshot(),bytes,&error));
    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    Simulation resumed(1);
    assert(resumed.restoreSnapshot(decoded,&error));
    resumed.setExternalPhysicalExecution(true);
    assert(resumed.observePendingContextAction(id).token==pending.token);
    assert(sim.completeExternalContextAction(id,pending.token,pending.targetPos));
    assert(resumed.completeExternalContextAction(id,pending.token,pending.targetPos));
    assert(!sim.completeExternalContextAction(id,pending.token,pending.targetPos));
    const auto count=w.characters.front().civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood);
    assert(count>0 &&w.resourceNodes.back().quantity==12-count);
    const auto untouched=std::find_if(w.resourceNodes.begin(),w.resourceNodes.end(),
        [](const ResourceNode& node){return node.id==9000;});
    assert(untouched!=w.resourceNodes.end() &&untouched->quantity==12);
    assert(count==resumed.world().characters.front().civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood));
    assert(*autonomousGoalSatisfiedByTruth(
        w,sim.relationships(),*agencyBefore.personalGoal));
    std::size_t realSuccesses=0;
    for(const auto& memory:w.characters.front().memory.entries){
        if(memory.source==MemorySource::DirectWitness &&memory.witnessed
           &&memory.sourceCharacter==id &&memory.who==id
           &&memory.where=="resource-node:9001"
           &&memory.tags==std::vector<std::string>{
                "action_experience","Civilization:Gather:PlantFood","success"})
            ++realSuccesses;
    }
    assert(realSuccesses==1); // inferred proximity text is never a real outcome
}
