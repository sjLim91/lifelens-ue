#include <cassert>
#include "lifelens/ResidentGatherAssessment.h"

using namespace lifelens;

int main()
{
    World world(874213954ULL);
    world.minute=100;
    Character actor;
    actor.id=1;actor.alive=true;actor.lifeStage=LifeStage::Adult;
    actor.needs={0.05,0.05,0.05,0.05,0.05};
    world.characters.push_back(actor);
    auto& resident=world.characters.front();
    const auto origin=chunkOriginGrid(world.initialStartRegion().region.coord);
    GridPos here{};bool found=false;
    for(int y=2;y<28 &&!found;++y) for(int x=2;x<18 &&!found;++x){
        bool dry=true;
        for(int n=0;n<12;++n) dry&=coreGroundTraversable(world,{origin.x+x+n,origin.y+y});
        if(dry){here={origin.x+x,origin.y+y};found=true;}
    }
    assert(found);
    GridPos distant{here.x+10,here.y};
    world.resourceNodes={{101,MaterialKind::Wood,10,10,false,0,distant},
        {102,MaterialKind::Wood,100,100,false,0,{distant.x+1,distant.y}}};
    // Only one source was personally sensed; an unknown remote source is
    // deliberately added after observation to test information boundaries.
    world.resourceNodes.pop_back();
    rememberResidentKnownLocalResources(world,resident,distant);
    world.resourceNodes.push_back({102,MaterialKind::Wood,100,100,false,0,{distant.x+1,distant.y}});
    CivilizationUtilityDecision gather;
    gather.intent=CivilizationIntent::Gather;gather.resourceNode=99;
    gather.material=MaterialKind::Wood;gather.quantity=4;
    auto candidates=assessResidentGatherAlternatives(world,resident,here,gather,220,4);
    assert(candidates.size()==1 &&candidates[0].source==101 &&candidates[0].feasible);
    const int cost=candidates[0].travelMinutes;
    assert(cost>0);
    assert(!assessResidentGatherAlternatives(world,resident,here,gather,100+cost,4)[0].feasible);
    world.resourceNodes[0].quantity=0;
    candidates=assessResidentGatherAlternatives(world,resident,here,gather,220,4);
    assert(candidates[0].quantity==10); // distant change is unknown
    candidates=assessResidentGatherAlternatives(world,resident,distant,gather,220,4);
    const auto unavailable=std::find_if(candidates.begin(),candidates.end(),[](const auto& c){return c.source==101;});
    assert(unavailable!=candidates.end() &&!unavailable->feasible &&unavailable->quantity==0);

    PendingContextAction pending;
    pending.kind=ContextActionKind::Civilization;pending.token=7;
    pending.civilization=gather;
    auto& goal=pending.gatherGoal;
    goal.goalId=goal.token=7;goal.actor=1;goal.material=MaterialKind::Wood;
    goal.createdMinute=goal.issuedMinute=100;goal.expiresMinute=220;
    goal.targetQuantity=8;goal.attempt=1;
    rememberResidentGatherAssessment(resident,pending,101,GatherAssessmentCause::LocalCompetition,false,true,6);
    rememberResidentGatherAssessment(resident,pending,102,GatherAssessmentCause::LocalCompetition,false,true,2);
    ResidentGatherAssessment assessment;
    assert(readResidentGatherAssessment(resident.memory.entries.back(),1,102,assessment));
    assert(assessment.quantity==2 &&!assessment.actualFailure &&assessment.token==7);
    auto corrupt=resident.memory.entries.back();
    corrupt.tags[1]="-1 7 99 5 220 2 0 1";
    assert(!readResidentGatherAssessment(corrupt,1,102,assessment));
    corrupt=resident.memory.entries.back();corrupt.where="resource-node:555";
    assert(!readResidentGatherAssessment(corrupt,1,102,assessment));
    assert(!readResidentGatherAssessment(resident.memory.entries.back(),2,102,assessment));
    for(int n=0;n<25;++n){
        goal.goalId=goal.token=pending.token=100+n;
        goal.createdMinute=goal.issuedMinute=2000*n+100;
        goal.expiresMinute=goal.createdMinute+120;
        rememberResidentGatherAssessment(resident,pending,goal.createdMinute,
            GatherAssessmentCause::DeadlineInsufficient,false,false,0);
    }
    int count=0;
    for(const auto& memory:resident.memory.entries)
        if(!memory.tags.empty() &&memory.tags[0]=="gather_assessment_v1") ++count;
    assert(count==12);
}
