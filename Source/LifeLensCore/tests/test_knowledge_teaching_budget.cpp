#include <iostream>
#include <string>

#include "lifelens/Simulation.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)){ \
        std::cerr<<"CHECK failed: " #expr<<" line "<<__LINE__<<'\n'; \
        return false; \
    } \
} while(false)

static bool runTeachingWindowScenario(bool nearEnough,bool boundaryMinute)
{
    Simulation sim(818181);
    sim.setupNewGame();
    SimulationStateSnapshot snapshot=sim.captureSnapshot();
    CHECK(snapshot.world.characters.size()>=2);

    const CharacterId teacherId=snapshot.world.characters[0].id;
    const CharacterId learnerId=snapshot.world.characters[1].id;

    for(Character& resident:snapshot.world.characters){
        resident.needs={0.05,0.05,0.05,0.05,0.05};
        resident.civilization.character=resident.id;
        if(resident.id!=learnerId){
            resident.civilization.knowledge.learn(
                TechniqueId::SharpFlake,
                KnowledgeLevel::Reproducible,
                0.90);
        }
    }
    snapshot.world.characters[0].civilization.knowledge.learn(
        TechniqueId::SharpFlake,
        KnowledgeLevel::Mastered,
        0.99);

    for(auto& [id,runtime]:snapshot.runtime){
        runtime.goal=Goal::Idle;
        runtime.plan.clear();
        runtime.pendingContext.clear();
        runtime.penaltyUntilMinute=0;
        runtime.navigationRoute.clear();
        runtime.navigationRouteIndex=0;
        runtime.navigationHasTarget=false;
        runtime.navigationArrived=false;
        runtime.navigationRouteFailed=false;
    }

    snapshot.runtime[teacherId].pos={0,0};
    snapshot.runtime[learnerId].pos=
        nearEnough
            ? GridPos{1,0}
            : GridPos{SettlementServiceRadiusGrid*3,0};
    snapshot.world.minute=
        boundaryMinute
            ? KnowledgeTeachingDecisionIntervalMinutes-1
            : KnowledgeTeachingDecisionIntervalMinutes-2;

    std::string error;
    CHECK(sim.restoreSnapshot(snapshot,&error));
    CHECK(error.empty());

    Character* teacher=nullptr;
    Character* learner=nullptr;
    for(Character& resident:sim.world().characters){
        if(resident.id==teacherId) teacher=&resident;
        if(resident.id==learnerId) learner=&resident;
    }
    CHECK(teacher!=nullptr);
    CHECK(learner!=nullptr);

    Relationship& learnerToTeacher=
        sim.relationships().getOrCreate(learnerId,teacherId);
    learnerToTeacher.trust=1.0;
    learnerToTeacher.respect=1.0;
    learnerToTeacher.familiarity=1.0;
    learnerToTeacher.comfort=1.0;

    CHECK(registerTechniqueOrigin(
        sim.socialKnowledge(),
        *teacher,
        TechniqueId::SharpFlake,
        sim.world().minute,
        CivilizationEventType::Crafted,
        sim.world().seed)!=nullptr);

    sim.step();

    const PendingContextActionObservation pending=
        sim.observePendingContextAction(teacherId);
    if(boundaryMinute && nearEnough){
        CHECK(pending.active);
        CHECK(pending.kind==ContextActionKind::KnowledgeTeaching);
        CHECK(pending.targetResident==learnerId);
        CHECK(pending.technique==TechniqueId::SharpFlake);
    }else{
        CHECK(
            !pending.active
            || pending.kind!=ContextActionKind::KnowledgeTeaching);
    }
    return true;
}

int main()
{
    CHECK(contextActionTimeoutMinutes(
        ContextActionKind::KnowledgeTeaching)==3*60);

    Character teacher;
    teacher.id=1;
    teacher.alive=true;
    teacher.lifeStage=LifeStage::Adult;
    teacher.civilization.character=1;
    teacher.civilization.knowledge.learn(
        TechniqueId::SharpFlake,
        KnowledgeLevel::Mastered,
        0.99);

    Character learner;
    learner.id=2;
    learner.alive=true;
    learner.lifeStage=LifeStage::Adult;
    learner.civilization.character=2;
    learner.civilization.knowledge.learn(
        TechniqueId::SharpFlake,
        KnowledgeLevel::Understood,
        0.60);

    CHECK(!techniqueTeachingCanAdvance(
        teacher,learner,TechniqueId::SharpFlake));
    teacher.civilization.inventory.add({
        ItemKind::RawMaterial,
        MaterialKind::Flint,
        2,
        0.6,
        1.0
    });
    CHECK(techniqueTeachingCanAdvance(
        teacher,learner,TechniqueId::SharpFlake));

    // Exact six-hour boundary + local opportunity => teaching may start.
    CHECK(runTeachingWindowScenario(true,true));

    // Same knowledge/social state but too far apart => no dedicated teaching
    // expedition is launched.
    CHECK(runTeachingWindowScenario(false,true));

    // Nearby residents still wait for the fixed six-hour decision boundary.
    CHECK(runTeachingWindowScenario(true,false));

    std::cout
        <<"P0 teaching budget/local opportunity regression passed\n";
    return 0;
}
