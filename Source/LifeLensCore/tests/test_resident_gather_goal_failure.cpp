#include <cassert>
#include <iostream>
#include "al1_natural_witness.h"
#include "lifelens/ResidentResourceOutcomePreference.h"
#include "lifelens/CoreNavigation.h"
#include "lifelens/ResidentGatherAssessment.h"

using namespace lifelens;

namespace {

// This is a controlled executor regression, not evidence from untouched New
// Game. The fixture supplies work already in flight; every stock change,
// observation, replacement token and subsequent harvest still comes from Core.
void competitionReplan(bool fullAlternative,bool exhaustOriginal)
{
    Simulation sim(874213954ULL);
    sim.setupNewGame();
    auto fixture=sim.captureSnapshot();
    fixture.world.minute=500;
    const auto origin=chunkOriginGrid(fixture.world.initialStartRegion().region.coord);
    GridPos here{};
    bool found=false;
    for(int y=2;y<28 &&!found;++y) for(int x=2;x<20 &&!found;++x){
        bool dry=true;
        for(int dx=0;dx<10;++dx)
            dry&=coreGroundTraversable(fixture.world,{origin.x+x+dx,origin.y+y});
        if(dry){here={origin.x+x,origin.y+y};found=true;}
    }
    assert(found);
    const GridPos originalPosition{here.x+4,here.y};
    const GridPos fullPosition{here.x+8,here.y};
    constexpr ResourceNodeId originalSource=101,partialSource=102,fullSource=103;
    // Keep procedural patch identities/baselines valid in this controlled save.
    // Empty their stock so only the three explicit fixture sources compete.
    for(auto& source:fixture.world.resourceNodes) source.quantity=0;
    fixture.world.resourceNodes.push_back(
        {originalSource,MaterialKind::Stone,exhaustOriginal?4:8,8,false,0,originalPosition});
    fixture.world.resourceNodes.push_back(
        {partialSource,MaterialKind::Stone,2,2,false,0,here});
    if(fullAlternative)
        fixture.world.resourceNodes.push_back(
            {fullSource,MaterialKind::Stone,8,8,false,0,fullPosition});
    const CharacterId worker=fixture.world.characters[0].id;
    const CharacterId observer=fixture.world.characters[1].id;
    for(auto& actor:fixture.world.characters){
        actor.needs={0.05,0.05,0.05,0.05,0.05};
        actor.civilization.inventory={};
        actor.civilization.gatheringSkill=0.5;
        if(actor.id!=observer)
            actor.civilization.inventory.add(
                {ItemKind::RawMaterial,MaterialKind::Stone,actor.id==worker?4:8,0.5,1.0});
        auto& runtime=fixture.runtime.at(actor.id);
        runtime={};
        runtime.pos=actor.id==worker ? originalPosition
            : actor.id==observer ? here : GridPos{here.x,here.y+20};
    }
    const auto pending=[&](CharacterId actor,std::uint64_t token,int quantity){
        PendingContextAction action;
        action.kind=ContextActionKind::Civilization;
        action.token=token;action.issuedMinute=495;
        action.hasSpatialTarget=true;action.targetPos=originalPosition;
        action.civilization.intent=CivilizationIntent::Gather;
        action.civilization.resourceNode=originalSource;
        action.civilization.material=MaterialKind::Stone;
        action.civilization.quantity=quantity;action.civilization.utility=0.6;
        auto& goal=action.gatherGoal;
        goal.goalId=goal.token=token;goal.actor=actor;
        goal.material=MaterialKind::Stone;
        goal.createdMinute=goal.issuedMinute=495;goal.expiresMinute=615;
        goal.inventoryAtIssue=actor==worker?4:0;
        goal.targetQuantity=8;goal.attempt=1;
        return action;
    };
    fixture.runtime.at(worker).pendingContext=pending(worker,1,4);
    fixture.runtime.at(observer).pendingContext=pending(observer,2,8);
    fixture.nextContextActionToken=3;
    std::string error;
    const bool restored=sim.restoreSnapshot(fixture,&error);
    if(!restored) std::cerr<<"competition fixture restore: "<<error<<'\n';
    assert(restored);
    if(fullAlternative){
        const auto alternatives=assessResidentGatherAlternatives(
            sim.world(),al1_test::resident(sim,observer),here,
            fixture.runtime.at(observer).pendingContext.civilization,615,8);
        const auto partial=std::find_if(alternatives.begin(),alternatives.end(),
            [](const auto& candidate){return candidate.source==partialSource;});
        const auto full=std::find_if(alternatives.begin(),alternatives.end(),
            [](const auto& candidate){return candidate.source==fullSource;});
        assert(partial!=alternatives.end() &&full!=alternatives.end());
        assert(partial->feasible &&full->feasible &&partial->score>full->score);
    }
    sim.step(); // Worker really harvests before observer reevaluates its goal.
    assert(al1_test::resource(sim,originalSource).quantity==(exhaustOriginal?0:4));
    assert(al1_test::resident(sim,worker).civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Stone)==8);
    bool witnessed=false;
    for(const auto& memory:al1_test::resident(sim,observer).memory.entries){
        ResidentGatherAssessment assessment;
        if(readResidentGatherAssessment(memory,observer,sim.world().minute,assessment)
           &&assessment.cause==GatherAssessmentCause::LocalCompetition
           &&assessment.token==2 &&!assessment.actualFailure)
            witnessed=true;
    }
    assert(witnessed);
    auto selected=sim.observePendingContextAction(observer);
    if(fullAlternative){
        assert(selected.active &&selected.resourceNode==fullSource);
        assert(selected.token!=2 &&selected.gatherGoal.previousToken==2
            &&selected.gatherGoal.goalId==2 &&selected.gatherGoal.attempt==2
            &&selected.gatherGoal.createdMinute==495 &&selected.gatherGoal.expiresMinute==615);
    }else{
        // With no full-goal alternative, witnessed contention does not discard
        // still-useful stock for the higher-scoring two-unit source.
        assert(selected.active &&selected.token==2 &&selected.resourceNode==originalSource);
    }
    if(exhaustOriginal){
        for(int tick=0;tick<60 &&selected.token==2;++tick){
            sim.step();selected=sim.observePendingContextAction(observer);
        }
        assert(selected.active &&selected.resourceNode==partialSource
            &&selected.gatherGoal.previousToken==2 &&selected.gatherGoal.goalId==2);
    }
    const auto expectedSource=fullAlternative?fullSource:exhaustOriginal?partialSource:originalSource;
    const int expectedQuantity=fullAlternative?8:exhaustOriginal?2:4;
    bool completed=false;
    for(int tick=0;tick<60 &&!completed;++tick){
        sim.step();
        for(const auto& event:sim.logs()){
            if(event.find("AL1_CORE_GOAL_RESULT actor=")==std::string::npos
               ||al1_test::number(event,"pending_token")!=selected.token) continue;
            assert(al1_test::number(event,"source")==expectedSource);
            assert(al1_test::field(event,"outcome")==
                (fullAlternative?"achieved":"progress-unmet"));
            completed=true;
        }
    }
    assert(completed);
    assert(al1_test::resident(sim,observer).civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Stone)==expectedQuantity);
    assert(al1_test::resource(sim,expectedSource).quantity==0);
    const auto ending=sim.captureSnapshot().runtime.at(observer).pos;
    assert(!sameGridPos(ending,here) ||exhaustOriginal);
    std::cout<<"AL1_CONTROLLED_COMPETITION full_alternative="<<fullAlternative
        <<" exhausted_original="<<exhaustOriginal<<" selected_source="<<expectedSource
        <<" inventory_gain="<<expectedQuantity<<" natural_acceptance=NOT_PROVEN\n";
}

// A real first Gather can produce fewer materials than its declared goal.
// The next bounded Core token must reuse that SAME goal; logging a partial
// success and starting an unrelated fresh objective is not completion.
void partialGatherContinuesOriginalGoal()
{
    Simulation sim(874213954ULL);
    sim.setupNewGame();
    auto fixture=sim.captureSnapshot();
    fixture.world.minute=500;
    const CharacterId actor=fixture.world.characters.front().id;
    const GridPos sourcePosition=fixture.runtime.at(actor).pos;
    constexpr ResourceNodeId source=10000003;
    for(auto& node:fixture.world.resourceNodes) node.quantity=0;
    fixture.world.resourceNodes.push_back(
        {source,MaterialKind::Stone,8,8,false,0,sourcePosition});
    for(auto& resident:fixture.world.characters){
        resident.needs={0.05,0.05,0.05,0.05,0.05};
        resident.civilization.inventory={};
        auto& state=fixture.runtime.at(resident.id);
        state={};
        state.pos=resident.id==actor ? sourcePosition
            : GridPos{sourcePosition.x+30,sourcePosition.y+30};
    }
    PendingContextAction first;
    first.token=101;
    first.kind=ContextActionKind::Civilization;
    first.issuedMinute=499;
    first.hasSpatialTarget=true;
    first.targetPos=sourcePosition;
    first.civilization.intent=CivilizationIntent::Gather;
    first.civilization.resourceNode=source;
    first.civilization.material=MaterialKind::Stone;
    first.civilization.quantity=2;
    first.civilization.utility=0.6;
    first.gatherGoal.goalId=first.gatherGoal.token=101;
    first.gatherGoal.actor=actor;
    first.gatherGoal.material=MaterialKind::Stone;
    first.gatherGoal.createdMinute=first.gatherGoal.issuedMinute=499;
    first.gatherGoal.expiresMinute=619;
    first.gatherGoal.inventoryAtIssue=0;
    first.gatherGoal.targetQuantity=4;
    first.gatherGoal.attempt=1;
    first.gatherGoal.composed=true; // Explicit resident-authored resource predicate.
    fixture.runtime.at(actor).pendingContext=first;
    fixture.nextContextActionToken=102;
    std::string error;
    const bool restored=sim.restoreSnapshot(fixture,&error);
    if(!restored) std::cerr<<"partial continuation fixture restore: "<<error<<'\n';
    assert(restored);

    bool firstCompleted=false;
    for(int tick=0;tick<25 &&!firstCompleted;++tick){
        sim.step();
        for(const auto& log:sim.logs())
            if(log.find("AL1_CORE_GOAL_RESULT actor=")!=std::string::npos
               &&al1_test::number(log,"pending_token")==101
               &&al1_test::field(log,"outcome")=="progress-unmet")
                firstCompleted=true;
    }
    assert(firstCompleted);
    assert(al1_test::resident(sim,actor).civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Stone)==2);
    assert(al1_test::resource(sim,source).quantity==6);
    // The physical result remains visible even after the next token is
    // issued. It may NOT be overwritten by Core planning observability.
    const auto activity=sim.observeResidentCivilizationActivity(actor);
    assert(activity.active &&activity.success
        &&activity.kind==CivilizationActivityKind::Gather
        &&activity.resourceNode==source &&activity.quantity==2);
    const auto second=sim.observePendingContextAction(actor);
    assert(second.active &&second.token!=101 &&second.resourceNode==source);
    assert(second.gatherGoal.goalId==101 &&second.gatherGoal.previousToken==101
        &&second.gatherGoal.targetQuantity==4 &&second.gatherGoal.attempt==2
        &&second.gatherGoal.createdMinute==499
        &&second.gatherGoal.expiresMinute==619);

    bool achieved=false;
    for(int tick=0;tick<25 &&!achieved;++tick){
        sim.step();
        for(const auto& log:sim.logs())
            if(log.find("AL1_CORE_GOAL_RESULT actor=")!=std::string::npos
               &&al1_test::number(log,"pending_token")==second.token
               &&al1_test::field(log,"outcome")=="achieved")
                achieved=true;
    }
    assert(achieved);
    assert(al1_test::resident(sim,actor).civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Stone)>=4);
    assert(al1_test::resource(sim,source).quantity<=4);
    assert(!sim.observePendingContextAction(actor).gatherGoal.active());
    std::cout<<"AL1_CONTROLLED_PARTIAL_CONTINUATION actor="<<actor
        <<" goal_id=101 original_token=101 next_token="<<second.token
        <<" physical_goal_achieved=1 natural_acceptance=NOT_PROVEN\n";
}

// Controlled entry point, but *real* survival preemption, physical eating,
// resumed ContextAction issuance and actual Core gathering. Snapshot replay
// must remain byte-identical; the fixture does not claim natural goal selection.
void survivalInterruptResumesSameGoal()
{
    Simulation sim(874213954ULL);
    sim.setupNewGame();
    auto fixture=sim.captureSnapshot();
    fixture.world.minute=500;
    const CharacterId actor=fixture.world.characters.front().id;
    const GridPos location=fixture.runtime.at(actor).pos;
    constexpr ResourceNodeId source=10000004;
    for(auto& node:fixture.world.resourceNodes) node.quantity=0;
    fixture.world.resourceNodes.push_back(
        {source,MaterialKind::Stone,12,12,false,0,location});
    for(auto& resident:fixture.world.characters){
        resident.needs={0.02,0.02,0.02,0.02,0.02};
        resident.civilization.inventory={};
        auto& runtime=fixture.runtime.at(resident.id);
        runtime={};
        runtime.pos=resident.id==actor ? location
            : GridPos{location.x+28,location.y+28};
    }
    auto& worker=fixture.world.characters.front();
    worker.needs={0.96,0.02,0.02,0.02,0.02};
    worker.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::PlantFood,3,0.5,1.0});
    assert(actionAvailableFor(fixture.world,worker,Goal::Eat));

    PendingContextAction old;
    old.token=101;
    old.kind=ContextActionKind::Civilization;
    old.issuedMinute=499;
    old.hasSpatialTarget=true;
    old.targetPos=location;
    old.civilization.intent=CivilizationIntent::Gather;
    old.civilization.resourceNode=source;
    old.civilization.material=MaterialKind::Stone;
    old.civilization.quantity=4;
    old.civilization.utility=0.65;
    old.gatherGoal.goalId=old.gatherGoal.token=101;
    old.gatherGoal.actor=actor;
    old.gatherGoal.material=MaterialKind::Stone;
    old.gatherGoal.createdMinute=old.gatherGoal.issuedMinute=499;
    old.gatherGoal.expiresMinute=619;
    old.gatherGoal.inventoryAtIssue=0;
    old.gatherGoal.targetQuantity=4;
    old.gatherGoal.attempt=1;
    old.gatherGoal.composed=true;
    fixture.runtime.at(actor).pendingContext=old;
    fixture.nextContextActionToken=102;
    std::string error;
    assert(sim.restoreSnapshot(fixture,&error));

    sim.step(); // Actual high hunger preempts the in-flight Stone Gather.
    auto paused=sim.captureSnapshot();
    const auto& suspended=paused.runtime.at(actor).suspendedGather;
    assert(suspended.active());
    assert(suspended.goal.goalId==101 &&suspended.goal.token==101
        &&suspended.goal.expiresMinute==619 &&suspended.goal.attempt==1
        &&suspended.source==source &&suspended.requestedQuantity==4);
    assert(!sim.observePendingContextAction(actor).gatherGoal.active());
    bool loggedInterruption=false;
    for(const auto& log:sim.logs()){
        if(log.find("AL1_CORE_GOAL_SUSPENDED actor=")!=std::string::npos
           &&al1_test::number(log,"actor")==actor
           &&al1_test::number(log,"goal_id")==101)
            loggedInterruption=true;
    }
    assert(loggedInterruption);

    // No original token is revived by reloading the in-flight survival plan.
    const auto bytes=al1_test::bytes(paused);
    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    assert(decoded.runtime.at(actor).suspendedGather.active());
    assert(al1_test::bytes(decoded)==bytes);
    Simulation replay(1);
    assert(replay.restoreSnapshot(decoded,&error));
    assert(al1_test::bytes(replay.captureSnapshot())==bytes);

    bool sawPhysicalRelief=false,sawNewGather=false,sawHarvest=false;
    std::uint64_t resumedToken=0;
    const int stockBefore=al1_test::resource(sim,source).quantity;
    for(int tick=0;tick<116 &&!sawHarvest;++tick){
        sim.step();
        replay.step();
        const auto current=al1_test::bytes(sim.captureSnapshot());
        assert(current==al1_test::bytes(replay.captureSnapshot()));
        const Character& resident=al1_test::resident(sim,actor);
        if(resident.needs.hunger<0.8) sawPhysicalRelief=true;
        const auto token=sim.observePendingContextAction(actor);
        if(token.gatherGoal.active() &&token.gatherGoal.goalId==101
           &&token.gatherGoal.previousToken==101){
            assert(sawPhysicalRelief);
            assert(token.token!=101 &&token.gatherGoal.attempt==2);
            assert(token.gatherGoal.targetQuantity==4
                &&token.gatherGoal.expiresMinute==619);
            resumedToken=token.token;
            sawNewGather=true;
        }
        // A zero-distance Core Gather may issue token 102 and physically
        // harvest all four units within a SINGLE simulation minute. In that
        // legitimate case observePendingContextAction is already cleared;
        // the same-tick authoritative issue/result receipts are the proof.
        for(const auto& event:sim.logs()){
            if(event.find("AL1_CORE_GOAL_RESUMED actor=")!=std::string::npos
               &&al1_test::number(event,"actor")==actor
               &&al1_test::number(event,"goal_id")==101){
                assert(sawPhysicalRelief);
                assert(al1_test::number(event,"previous_token")==101);
                assert(al1_test::number(event,"attempt")==2);
                assert(al1_test::number(event,"target_quantity")==4);
                resumedToken=static_cast<std::uint64_t>(
                    al1_test::number(event,"pending_token"));
                assert(resumedToken>101);
                sawNewGather=true;
            }
            if(event.find("AL1_CORE_GOAL_RESULT actor=")!=std::string::npos
               &&al1_test::number(event,"goal_id")==101
               &&al1_test::field(event,"outcome")=="achieved"
               &&al1_test::number(event,"pending_token")!=101){
                assert(sawNewGather);
                assert(al1_test::number(event,"pending_token")==resumedToken);
                sawHarvest=true;
            }
        }
    }
    if(!(sawPhysicalRelief &&sawNewGather &&sawHarvest)){
        const auto& resident=al1_test::resident(sim,actor);
        const auto current=sim.captureSnapshot();
        const auto& runtime=current.runtime.at(actor);
        std::cerr<<"AL1_SURVIVAL_RESUME_DIAGNOSTIC relief="<<sawPhysicalRelief
            <<" resumed="<<sawNewGather<<" harvest="<<sawHarvest
            <<" minute="<<current.world.minute
            <<" hunger="<<resident.needs.hunger
            <<" thirst="<<resident.needs.thirst
            <<" sleep="<<resident.needs.sleep
            <<" bladder="<<resident.needs.bladder
            <<" hygiene="<<resident.needs.hygiene
            <<" plan_actions="<<runtime.plan.size()
            <<" goal="<<static_cast<int>(runtime.goal)
            <<" pending="<<runtime.pendingContext.token
            <<" pending_kind="<<static_cast<int>(runtime.pendingContext.kind)
            <<" pending_resource="<<runtime.pendingContext.civilization.resourceNode
            <<" suspended="<<runtime.suspendedGather.active()
            <<" stock="<<al1_test::resource(sim,source).quantity<<'\n';
        CivilizationUtilityDecision assess;
        assess.intent=CivilizationIntent::Gather;
        assess.material=MaterialKind::Stone;
        assess.quantity=4;
        for(const auto& candidate:assessResidentGatherAlternatives(
                sim.world(),resident,runtime.pos,assess,619,4)){
            std::cerr<<" candidate="<<candidate.source
                <<" quantity="<<candidate.quantity
                <<" feasible="<<candidate.feasible
                <<" reachable="<<candidate.reachable
                <<" reason="<<candidate.reason<<'\n';
        }
        int reported=0;
        for(const auto& event:sim.logs())
            if(event.find("AL1_CORE_")!=std::string::npos
               ||event.find("preempted")!=std::string::npos){
                if(++reported<=80) std::cerr<<" diag_log "<<event<<'\n';
            }
    }
    assert(sawPhysicalRelief &&sawNewGather &&sawHarvest);
    assert(resumedToken>101);
    assert(al1_test::resident(sim,actor).civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::Stone)>=4);
    assert(al1_test::resource(sim,source).quantity<stockBefore);
    assert(!sim.captureSnapshot().runtime.at(actor).suspendedGather.active());

    // An expired suspended goal is NOT revived even when the actor is safe.
    auto expired=paused;
    expired.runtime.at(actor).suspendedGather.goal.expiresMinute=
        expired.world.minute;
    // Deadline cleanup must work even before the urgent Eat plan settles.
    Simulation urgentOverdue(1);
    assert(urgentOverdue.restoreSnapshot(expired,&error));
    urgentOverdue.step();
    assert(!urgentOverdue.captureSnapshot().runtime.at(actor).suspendedGather.active());
    auto& state=expired.runtime.at(actor);
    state.pendingContext.clear();
    state.plan.clear();
    state.goal=Goal::Idle;
    for(auto& resident:expired.world.characters)
        if(resident.id==actor)
            resident.needs={0.01,0.01,0.01,0.01,0.01};
    Simulation overdue(1);
    assert(overdue.restoreSnapshot(expired,&error));
    for(int tick=0;tick<12;++tick) overdue.step();
    assert(!overdue.captureSnapshot().runtime.at(actor).suspendedGather.active());
    for(const auto& log:overdue.logs())
        if(log.find("AL1_CORE_GOAL_RESUMED actor=")!=std::string::npos
           &&al1_test::number(log,"goal_id")==101)
            assert(false &&"expired Gather cannot issue a fresh token");

    std::cout<<"AL1_CONTROLLED_SURVIVAL_RESUME actor="<<actor
        <<" goal_id=101 original_token=101 resumed_token="<<resumedToken
        <<" actual_survival_relief=1 Core_harvest=1 snapshot_replay=equal"
        <<" natural_acceptance=NOT_PROVEN\n";
}

} // namespace

int main()
{
    survivalInterruptResumesSameGoal();
    partialGatherContinuesOriginalGoal();
    competitionReplan(true,false);
    competitionReplan(false,false);
    competitionReplan(false,true);
    // Controlled contention regression, explicitly NOT natural acceptance:
    // start with a genuinely issued natural goal, then place a competing
    // worker at its target and execute a real Core harvest in the fixture.
    // Core must encounter failure, remember it, keep
    // the same predicate and issue/physically complete a reachable alternative.
    Simulation sim(874213954ULL);
    sim.setupNewGame();
    PendingContextActionObservation original;
    for(int tick=0;tick<14*1440 &&!original.active;++tick){
        sim.step();
        for(const auto& actor:sim.world().characters){
            const auto pending=sim.observePendingContextAction(actor.id);
            if(!pending.gatherGoal.active() ||maximumResidentNeed(actor)>0.65
               ||pending.material==MaterialKind::Water)
                continue;
            const auto sensed=observeLocalResourceProximity(
                sim.world(),actor.id,pending.targetPos,ResidentLocalPerceptionMaxRadius);
            for(const auto& other:sensed.nearbyResources){
                if(other.material!=pending.material ||other.nodeId==pending.resourceNode)
                    continue;
                GridPos access{};
                std::vector<GridPos> route;
                if(resolveCivilizationResourceAccessGridPosition(sim.world(),other.nodeId,access)
                   &&buildCoreGroundRoute(sim.world(),pending.targetPos,access,1,route)){
                    original=pending;
                    break;
                }
            }
            if(original.active) break;
        }
    }
    assert(original.active);
    auto fixture=sim.captureSnapshot();
    auto competitor=std::find_if(fixture.world.characters.begin(),fixture.world.characters.end(),
        [&](const Character& c){return c.id!=original.actor &&c.alive;});
    assert(competitor!=fixture.world.characters.end());
    fixture.runtime.at(competitor->id).pos=original.targetPos;
    CivilizationUtilityDecision preceding;
    preceding.intent=CivilizationIntent::Gather;
    preceding.resourceNode=original.resourceNode;
    preceding.material=original.material;
    preceding.quantity=al1_test::resource(sim,original.resourceNode).quantity;
    const auto drained=executeCivilizationDecisionAtPosition(
        fixture.world,*competitor,preceding,original.targetPos);
    assert(drained.executed &&drained.success &&drained.event.quantity>0);
    std::string error;
    const bool restored=sim.restoreSnapshot(fixture,&error);
    if(!restored) std::cerr<<error<<"\n";
    assert(restored);
    PendingContextActionObservation alternative;
    for(int tick=0;tick<120 &&!alternative.active;++tick){
        sim.step();
        const auto p=sim.observePendingContextAction(original.actor);
        if(p.active &&p.gatherGoal.previousToken==original.token) alternative=p;
    }
    assert(alternative.active &&alternative.resourceNode!=original.resourceNode);
    assert(alternative.gatherGoal.goalId==original.gatherGoal.goalId
        &&alternative.gatherGoal.targetQuantity==original.gatherGoal.targetQuantity
        &&alternative.gatherGoal.attempt==2);
    const auto failure=buildResidentResourceOutcomeIndex(
        al1_test::resident(sim,original.actor),sim.world().minute).find(
            original.resourceNode,"Civilization:Gather:"+std::string(materialName(original.material)));
    assert(failure.hasEvidence &&!failure.succeeded
        &&failure.reason==ResidentResourceOutcomeReason::SourceUnavailable);
    const int held=al1_test::resident(sim,original.actor).civilization.inventory.count(
        ItemKind::RawMaterial,original.material);
    const int stock=al1_test::resource(sim,alternative.resourceNode).quantity;
    // Controlled deadline boundary: even a fresh recovery token must not
    // harvest past its original goal's deadline, through either executor.
    auto expired=sim.captureSnapshot();
    expired.runtime.at(original.actor).pendingContext.gatherGoal.expiresMinute=
        expired.world.minute;
    for(bool external:{false,true}){
        expired.world.externalPhysicalExecution=external;
        Simulation overdue(1);
        assert(overdue.restoreSnapshot(expired,&error));
        const auto memoryCount=al1_test::resident(overdue,original.actor).memory.entries.size();
        if(external)
            assert(!overdue.completeExternalContextAction(original.actor,
                alternative.token,alternative.targetPos));
        else overdue.step();
        assert(!overdue.observePendingContextAction(original.actor).active);
        assert(al1_test::resident(overdue,original.actor).civilization.inventory.count(
            ItemKind::RawMaterial,original.material)==held);
        assert(al1_test::resource(overdue,alternative.resourceNode).quantity==stock);
        const auto& afterMemory=al1_test::resident(overdue,original.actor).memory.entries;
        assert(afterMemory.size()==memoryCount+1);
        ResidentGatherAssessment expiry;
        assert(readResidentGatherAssessment(afterMemory.back(),original.actor,
            overdue.world().minute,expiry));
        assert(expiry.cause==GatherAssessmentCause::DeadlineInsufficient
            &&!expiry.actualFailure &&!expiry.resumable);
        bool reported=false;
        for(const auto& event:overdue.logs())
            if(event.find("AL1_CORE_GOAL_RESULT actor=")!=std::string::npos
               &&al1_test::number(event,"pending_token")==alternative.token
               &&al1_test::field(event,"outcome")=="expired") reported=true;
        assert(reported);
    }
    bool finished=false;
    for(int tick=0;tick<120 &&!finished;++tick){
        sim.step();
        for(const auto& event:sim.logs())
            if(event.find("AL1_CORE_GATHER_RESULT actor=")!=std::string::npos
               &&al1_test::number(event,"pending_token")==alternative.token)
                finished=true;
    }
    assert(finished);
    assert(al1_test::resident(sim,original.actor).civilization.inventory.count(
        ItemKind::RawMaterial,original.material)>held);
    assert(al1_test::resource(sim,alternative.resourceNode).quantity<stock);
    std::cout<<"AL1_CONTROLLED_EXHAUSTION_RECOVERY actor="<<original.actor
        <<" goal_id="<<original.gatherGoal.goalId
        <<" failed_token="<<original.token<<" recovery_token="<<alternative.token
        <<" failed_source="<<original.resourceNode<<" alternative="<<alternative.resourceNode
        <<" physical_completion=1 natural_failure_acceptance=NOT_PROVEN\n";
}
