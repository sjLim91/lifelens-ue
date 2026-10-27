#include <array>
#include <cassert>
#include <utility>
#include <iostream>
#include <vector>
#include "lifelens/ResidentSensedResourceAgency.h"
#include "lifelens/ResidentLiveRecoveryEvidence.h"
#include "lifelens/ResidentLiveSuccessLearningEvidence.h"
#include "lifelens/ResidentGatherRouteRecovery.h"
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

    // AL-1: an actual Core-executed food harvest teaches the SAME resident
    // which of TWO currently reachable food sources to select during its
    // ordinary urgent Hunger Gather. Only direct memory differs from a
    // second identical World; no model text, bonus food or forced goal.
    {
        World foodWorld=world;
        // The real harvest receipt must be temporally valid for the Core
        // New Game snapshot used in the identical-memory ablation below.
        foodWorld.minute=0;
        foodWorld.resourceNodes={
            {201,MaterialKind::PlantFood,12,12,false,0,
                {position.x+1,position.y}},
            {202,MaterialKind::PlantFood,12,12,false,0,
                {position.x+2,position.y}}
        };
        CivilizationUtilityDecision harvested;
        harvested.intent=CivilizationIntent::Gather;
        harvested.material=MaterialKind::PlantFood;
        harvested.item=ItemKind::RawMaterial;
        harvested.resourceNode=202;
        harvested.quantity=2;
        const auto actualHarvest=executeCivilizationDecisionAtPosition(
            foodWorld,foodWorld.characters.front(),harvested,
            foodWorld.resourceNodes[1].pos);
        assert(actualHarvest.executed &&actualHarvest.success
            &&actualHarvest.event.quantity>0);
        rememberCivilizationAttemptOutcome(
            foodWorld.characters.front(),harvested,foodWorld.minute,true);
        const int collected=foodWorld.characters.front().civilization.inventory.count(
            ItemKind::RawMaterial,MaterialKind::PlantFood);
        assert(collected>0);
        // Genuine consumption of prior collected food; a resident still
        // needs to gather again later, as opposed to inventing scarcity.
        assert(foodWorld.characters.front().civilization.inventory.remove(
            ItemKind::RawMaterial,MaterialKind::PlantFood,collected));
        foodWorld.characters.front().needs.hunger=0.95;
        World noExperience=foodWorld;
        noExperience.characters.front().memory.entries.clear();
        const auto base=urgentSurvivalProvisionDecisionAtPosition(
            noExperience,noExperience.characters.front(),position);
        const auto lived=urgentSurvivalProvisionDecisionAtPosition(
            foodWorld,foodWorld.characters.front(),position);
        const auto disabled=urgentSurvivalProvisionDecisionAtPosition(
            foodWorld,foodWorld.characters.front(),position,false);
        assert(base.intent==CivilizationIntent::Gather
            &&base.resourceNode==201);
        assert(disabled.resourceNode==base.resourceNode);
        assert(lived.intent==CivilizationIntent::Gather
            &&lived.resourceNode==202
            &&lived.material==MaterialKind::PlantFood);
        assert(foodWorld.resourceNodes[0].quantity==12);
        assert(foodWorld.resourceNodes[1].quantity
            ==noExperience.resourceNodes[1].quantity);
        assert(foodWorld.rng==noExperience.rng);

        // Now run the EXACT Core planning/physical pending-token authority.
        // The resident's learned source must be issued AND actually harvested,
        // not merely preferred in a read-only shadow candidate.
        Simulation learnedRuntime(77,0,4);
        learnedRuntime.setupNewGame();
        auto liveSnapshot=learnedRuntime.captureSnapshot();
        const CharacterId owner=liveSnapshot.world.characters.front().id;
        assert(owner==foodWorld.characters.front().id);
        liveSnapshot.runtime.at(owner).pos=position;
        // Preserve every generated natural patch/resource identity and its
        // immutable max/biome provenance. Replacing generated nodes wholesale
        // would correctly fail Core snapshot validation. Harvest their current
        // stock through the existing Inventory-backed resource operation;
        // the two test food sources are additional explicit live objects.
        for(auto& node:liveSnapshot.world.resourceNodes){
            Inventory removedStock;
            node.harvest(node.quantity,removedStock);
            node.regenerationPerDay=0;
        }
        liveSnapshot.world.resourceNodes.insert(
            liveSnapshot.world.resourceNodes.end(),
            foodWorld.resourceNodes.begin(),foodWorld.resourceNodes.end());
        liveSnapshot.world.storageSites.clear();
        for(auto& resident:liveSnapshot.world.characters){
            if(resident.id!=owner){
                resident.alive=false;
                resident.deathMinute=liveSnapshot.world.minute;
            }else{
                resident.memory=foodWorld.characters.front().memory;
                resident.civilization.inventory=Inventory{};
                resident.needs={0.95,0.05,0.05,0.05,0.05};
            }
        }
        auto baselineSnapshot=liveSnapshot;
        baselineSnapshot.world.characters.front().memory.entries.clear();
        std::string error;
        assert(learnedRuntime.restoreSnapshot(liveSnapshot,&error));
        learnedRuntime.setExternalPhysicalExecution(true);
        Simulation counterfactual(77,0,4);
        assert(counterfactual.restoreSnapshot(baselineSnapshot,&error));
        counterfactual.setExternalPhysicalExecution(true);
        GridPos realOrigin{};
        assert(learnedRuntime.runtimePosition(owner,realOrigin));
        const auto plannedBefore=urgentSurvivalProvisionDecisionAtPosition(
            learnedRuntime.world(),learnedRuntime.world().characters.front(),
            realOrigin);
        const auto nativeBefore=urgentSurvivalProvisionDecisionAtPosition(
            counterfactual.world(),counterfactual.world().characters.front(),
            realOrigin);
        learnedRuntime.step();
        counterfactual.step();
        const auto issued=learnedRuntime.observePendingContextAction(owner);
        const auto otherIssued=counterfactual.observePendingContextAction(owner);
        if(!issued.active ||!otherIssued.active
           ||issued.kind!=ContextActionKind::Civilization
           ||issued.civilizationIntent!=CivilizationIntent::Gather
           ||issued.material!=MaterialKind::PlantFood
           ||issued.resourceNode!=202 ||otherIssued.resourceNode!=201){
            std::cerr<<"AL1_FOOD_CORE_CONTEXT_FAILURE"
                <<" planned_intent="<<static_cast<int>(plannedBefore.intent)
                <<" planned_source="<<plannedBefore.resourceNode
                <<" native_intent="<<static_cast<int>(nativeBefore.intent)
                <<" native_source="<<nativeBefore.resourceNode
                <<" live_kind="<<static_cast<int>(issued.kind)
                <<" live_intent="<<static_cast<int>(issued.civilizationIntent)
                <<" live_material="<<materialName(issued.material)
                <<" live_source="<<issued.resourceNode
                <<" baseline_source="<<otherIssued.resourceNode
                <<" actor_memory="<<learnedRuntime.world().characters.front()
                    .memory.entries.size()
                <<" actual_x="<<realOrigin.x
                <<" actual_y="<<realOrigin.y
                <<"\n";
        }
        assert(issued.active &&otherIssued.active);
        assert(issued.kind==ContextActionKind::Civilization
            &&issued.civilizationIntent==CivilizationIntent::Gather
            &&issued.material==MaterialKind::PlantFood
            &&issued.resourceNode==202);
        assert(otherIssued.resourceNode==201);
        assert(issued.token!=0 &&otherIssued.token!=0);
        const auto stockAt=[&](ResourceNodeId source){
            const auto& nodes=learnedRuntime.world().resourceNodes;
            const auto found=std::find_if(nodes.begin(),nodes.end(),
                [source](const ResourceNode& node){return node.id==source;});
            assert(found!=nodes.end());
            return found->quantity;
        };
        const int stockBefore=stockAt(202);
        assert(learnedRuntime.completeExternalContextAction(
            owner,issued.token,issued.targetPos));
        const int newStock=stockAt(202);
        assert(newStock<stockBefore);
        assert(learnedRuntime.world().characters.front()
            .civilization.inventory.count(
                ItemKind::RawMaterial,MaterialKind::PlantFood)>0);
        assert(std::any_of(learnedRuntime.logs().begin(),
            learnedRuntime.logs().end(),[](const std::string& line){
                return line.find("AL1_CORE_URGENT_FOOD_LEARNED_TOKEN")
                    !=std::string::npos;
            }));
        assert(!std::any_of(counterfactual.logs().begin(),
            counterfactual.logs().end(),[](const std::string& line){
                return line.find("AL1_CORE_URGENT_FOOD_LEARNED_TOKEN")
                    !=std::string::npos;
            }));
    }

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

    // AL-1: a genuinely failed Core navigation route changes this ONE
    // resident's next executable Gather source under the ordinary agency
    // production selector. No AI proposal, fake inventory, or world depletion.
    World routeWorld=world;
    routeWorld.resourceNodes.push_back(
        {103,MaterialKind::Stone,12,12,false,0,
            {position.x+1,position.y}});
    ResidentAgencyChoice requestedGather=fallback;
    requestedGather.decision.civilization.intent=CivilizationIntent::Gather;
    requestedGather.decision.civilization.resourceNode=101;
    requestedGather.decision.civilization.material=MaterialKind::Stone;
    requestedGather.decision.civilization.item=ItemKind::RawMaterial;
    requestedGather.decision.civilization.utility=0.95;
    requestedGather.decision.utility=0.95;

    // A genuinely executed Core Gather success (not a model claim) teaches
    // this resident a short-lived preferred source in its SAME-material
    // optional Gather slot. Use equal geometry and identical post-action
    // inventory/stocks in both worlds; only the direct success receipt
    // differs. The Core's unmodified choice still prefers the smaller id.
    World learnedSuccessWorld=world;
    learnedSuccessWorld.resourceNodes.push_back(
        {103,MaterialKind::Stone,12,12,false,0,world.resourceNodes[0].pos});
    auto successful103=requestedGather.decision.civilization;
    successful103.resourceNode=103;
    const auto successfulAttempt=executeCivilizationDecisionAtPosition(
        learnedSuccessWorld,learnedSuccessWorld.characters[0],
        successful103,world.resourceNodes[0].pos);
    assert(successfulAttempt.executed &&successfulAttempt.success
        &&successfulAttempt.event.quantity>0);
    rememberCivilizationAttemptOutcome(
        learnedSuccessWorld.characters[0],successful103,480,true);
    World withoutSuccessMemory=learnedSuccessWorld;
    withoutSuccessMemory.characters[0].memory.entries.clear();
    const auto unlearnedSuccessChoice=chooseResidentSensedResourceGoal(
        withoutSuccessMemory,withoutSuccessMemory.characters[0],
        position,relationships,requestedGather,nullptr,false);
    const auto learnedSuccessChoice=chooseResidentSensedResourceGoal(
        learnedSuccessWorld,learnedSuccessWorld.characters[0],
        position,relationships,requestedGather,nullptr,false);
    assert(unlearnedSuccessChoice.agency.decision.civilization.resourceNode==101);
    assert(learnedSuccessChoice.agency.decision.civilization.resourceNode==103);
    assert(learnedSuccessChoice.agency.authority
        ==ResidentAgencyAuthority::PersonalGoal);
    const auto actualLearnedEvidence=observeResidentLiveSuccessLearningEvidence(
        learnedSuccessWorld,learnedSuccessWorld.characters[0],position,
        requestedGather,learnedSuccessChoice);
    assert(actualLearnedEvidence
        &&actualLearnedEvidence->actor==1
        &&actualLearnedEvidence->originalSource==101
        &&actualLearnedEvidence->learnedSource==103
        &&actualLearnedEvidence->material==MaterialKind::Stone
        &&actualLearnedEvidence->firsthandSuccessMinute==480
        &&actualLearnedEvidence->choiceMinute==480);
    assert(!observeResidentLiveSuccessLearningEvidence(
        withoutSuccessMemory,withoutSuccessMemory.characters[0],position,
        requestedGather,unlearnedSuccessChoice));
    // Neither hearsay nor another mind's directly witnessed event can
    // create a firsthand learned-choice receipt for this resident.
    World rumourWorld=learnedSuccessWorld;
    for(auto& memory:rumourWorld.characters[0].memory.entries)
        if(memory.where=="resource-node:103"
           &&memory.tags.size()==3
           &&memory.tags[2]=="success")
            memory.source=MemorySource::ToldByOther;
    assert(!observeResidentLiveSuccessLearningEvidence(
        rumourWorld,rumourWorld.characters[0],position,
        requestedGather,learnedSuccessChoice));

    assert(learnedSuccessChoice.agency.decision.civilization.material
        ==MaterialKind::Stone);
    assert(withoutSuccessMemory.resourceNodes[1].quantity
        ==learnedSuccessWorld.resourceNodes[1].quantity);
    assert(withoutSuccessMemory.rng==learnedSuccessWorld.rng);
    const auto otherSuccessChoice=chooseResidentSensedResourceGoal(
        learnedSuccessWorld,learnedSuccessWorld.characters[1],
        position,relationships,requestedGather,nullptr,false);
    assert(otherSuccessChoice.agency.decision.civilization.resourceNode==101);
    ResidentAgencyChoice strategySuccess=requestedGather;
    strategySuccess.authority=ResidentAgencyAuthority::ResidentCognition;
    strategySuccess.cognitiveIntent=CognitiveIntentKind::ImproveFoodSecurity;
    strategySuccess.contextFingerprint=88442;
    const auto heldStrategy=chooseResidentSensedResourceGoal(
        learnedSuccessWorld,learnedSuccessWorld.characters[0],
        position,relationships,strategySuccess,nullptr,false);
    assert(heldStrategy.agency.authority
        ==ResidentAgencyAuthority::ResidentCognition);
    assert(heldStrategy.agency.contextFingerprint==88442
        &&heldStrategy.agency.cognitiveIntent
            ==CognitiveIntentKind::ImproveFoodSecurity);
    assert(heldStrategy.agency.decision.civilization.resourceNode==103);
    const auto cognitiveEvidence=observeResidentLiveSuccessLearningEvidence(
        learnedSuccessWorld,learnedSuccessWorld.characters[0],position,
        strategySuccess,heldStrategy);
    assert(cognitiveEvidence
        &&cognitiveEvidence->actor==1
        &&cognitiveEvidence->learnedSource==103);

    auto urgentSuccess=learnedSuccessWorld.characters[0];
    urgentSuccess.needs.sleep=1.0;
    assert(chooseResidentSensedResourceGoal(
        learnedSuccessWorld,urgentSuccess,position,relationships,
        requestedGather,nullptr,false).agency.decision.civilization.resourceNode
            ==101);

    const auto noFailure=chooseResidentSensedResourceGoal(
        routeWorld,routeWorld.characters[0],position,relationships,
        requestedGather,nullptr,false);
    assert(noFailure.agency.decision.civilization.resourceNode==101);
    const std::string failedRouteKey="Civilization:Gather:Stone";
    assert(rememberResidentGatherRouteFailure(
        routeWorld.characters[0],failedRouteKey,101,47001,480));
    const auto changed=chooseResidentSensedResourceGoal(
        routeWorld,routeWorld.characters[0],position,relationships,
        requestedGather,nullptr,false);
    assert(changed.agency.authority==ResidentAgencyAuthority::PersonalGoal);
    assert(changed.agency.decision.kind==UnifiedDecisionKind::Civilization);
    assert(changed.agency.decision.civilization.intent==CivilizationIntent::Gather);
    assert(changed.agency.decision.civilization.resourceNode==103);
    // This branch was learned from a real pre-extraction Core navigation
    // failure; the audit must not misrepresent it as a failed harvest.
    const auto routeEvidence=observeResidentLiveRecoveryEvidence(
        routeWorld,routeWorld.characters[0],position,requestedGather,changed);
    // Immediate off-cadence continuation uses a REAL witnessed Core
    // navigation failure and the same resident, material and current source
    // facts; it must not require a new model thought or 15-minute agency tick.
    auto noMemoryRecovery=chooseResidentGatherRecoveryAfterCoreRouteFailure(
        routeWorld,routeWorld.characters[1],position,
        requestedGather.decision.civilization);
    assert(!noMemoryRecovery);
    const auto immediate=chooseResidentGatherRecoveryAfterCoreRouteFailure(
        routeWorld,routeWorld.characters[0],position,
        requestedGather.decision.civilization);
    assert(immediate && immediate->resourceNode==103
        &&immediate->material==MaterialKind::Stone
        &&immediate->intent==CivilizationIntent::Gather);
    auto bodilyEmergency=routeWorld.characters[0];
    bodilyEmergency.needs.sleep=1.0;
    assert(!chooseResidentGatherRecoveryAfterCoreRouteFailure(
        routeWorld,bodilyEmergency,position,
        requestedGather.decision.civilization));
    const auto unearned=chooseResidentGatherRecoveryAfterCoreRouteFailure(
        routeWorld,routeWorld.characters[0],position,
        changed.agency.decision.civilization);
    assert(!unearned);

    assert(routeEvidence
        &&routeEvidence->actor==routeWorld.characters[0].id
        &&routeEvidence->failedSource==101
        &&routeEvidence->selectedSource==103
        &&routeEvidence->firsthandFailureMinute==480
        &&routeEvidence->failedPendingToken==47001
        &&routeEvidence->directNavigationFailure);
    const auto noRecovery=chooseResidentSensedResourceGoal(
        routeWorld,routeWorld.characters[1],position,relationships,
        requestedGather,nullptr,false);
    assert(!observeResidentLiveRecoveryEvidence(
        routeWorld,routeWorld.characters[1],position,
        requestedGather,noRecovery));
    const auto neighbour=chooseResidentSensedResourceGoal(
        routeWorld,routeWorld.characters[1],position,relationships,
        requestedGather,nullptr,false);
    assert(neighbour.agency.decision.civilization.resourceNode==101);
    // AL-1: a single accepted resident cognition retains its identity and
    // target material while Core recovers from its firsthand blocked route.
    // No separate fallback agent competes for this strategic slot.
    ResidentAgencyChoice cognitiveGather=requestedGather;
    cognitiveGather.authority=ResidentAgencyAuthority::ResidentCognition;
    cognitiveGather.cognitiveIntent=CognitiveIntentKind::ImproveFoodSecurity;
    cognitiveGather.contextFingerprint=88442;
    const auto cognitionBeforeFailure=chooseResidentSensedResourceGoal(
        routeWorld,routeWorld.characters[1],position,relationships,
        cognitiveGather,nullptr,false);
    assert(cognitionBeforeFailure.agency.authority
        ==ResidentAgencyAuthority::ResidentCognition);
    assert(cognitionBeforeFailure.agency.decision.civilization.resourceNode==101);
    const auto cognitiveRecovery=chooseResidentSensedResourceGoal(
        routeWorld,routeWorld.characters[0],position,relationships,
        cognitiveGather,nullptr,false);
    assert(cognitiveRecovery.agency.authority
        ==ResidentAgencyAuthority::ResidentCognition);
    assert(cognitiveRecovery.agency.cognitiveIntent
        ==CognitiveIntentKind::ImproveFoodSecurity);
    assert(cognitiveRecovery.agency.contextFingerprint==88442);
    assert(cognitiveRecovery.agency.decision.civilization.resourceNode==103);
    assert(cognitiveRecovery.agency.decision.civilization.material
        ==MaterialKind::Stone);
    // Actual urgent survival must be able to preempt this model strategy.
    const auto urgentCognition=chooseResidentSensedResourceGoal(
        routeWorld,routeWorld.characters[0],position,relationships,
        cognitiveGather,nullptr,true);
    assert(urgentCognition.agency.decision.civilization.resourceNode==101);
    assert(urgentCognition.agency.authority
        ==ResidentAgencyAuthority::ResidentCognition);

    const auto blockedCognition=cognitiveGather;
    auto changedStrategy=blockedCognition;
    changedStrategy.decision.civilization.intent=CivilizationIntent::Explore;
    assert(chooseResidentSensedResourceGoal(
        routeWorld,routeWorld.characters[0],position,relationships,
        changedStrategy,nullptr,false).agency.decision.civilization.intent
            ==CivilizationIntent::Explore);

    GridPos accessible{};
    assert(resolveCivilizationResourceAccessGridPosition(
        routeWorld,103,accessible));
    std::vector<GridPos> actualRoute;
    assert(buildCoreGroundRoute(routeWorld,position,accessible,1,actualRoute));
    const int beforeAlternate=routeWorld.resourceNodes.back().quantity;
    const auto actualGather=executeCivilizationDecisionAtPosition(
        routeWorld,routeWorld.characters[0],
        changed.agency.decision.civilization,accessible);
    assert(actualGather.executed &&actualGather.success
        &&actualGather.event.quantity>0);
    assert(routeWorld.resourceNodes.back().quantity
        ==beforeAlternate-actualGather.event.quantity);
    assert(routeWorld.resourceNodes.front().quantity==12);
    assert(routeWorld.characters[1].civilization.inventory.stacks().empty());

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
