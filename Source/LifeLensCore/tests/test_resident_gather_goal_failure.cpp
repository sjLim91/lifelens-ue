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

} // namespace

int main()
{
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
