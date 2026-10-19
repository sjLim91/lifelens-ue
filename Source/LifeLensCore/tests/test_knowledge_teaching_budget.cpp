#include <iostream>
#include <string>

#include "lifelens/Simulation.h"
#include "lifelens/ResidentCognitiveTeachingPreference.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)){ \
        std::cerr<<"CHECK failed: " #expr<<" line "<<__LINE__<<'\n'; \
        return false; \
    } \
} while(false)

static bool runTeachingWindowScenario(
    bool nearEnough,
    bool boundaryMinute,
    bool busyTeacher=false)
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
    if(busyTeacher){
        snapshot.runtime[teacherId].plan.push_back(
            {ActionType::Idle,0,120});
    }
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
    if(boundaryMinute && nearEnough && !busyTeacher){
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

static bool runCognitiveTeachingPreferenceScenario()
{
    Simulation sim(818188);
    sim.setupNewGame();
    auto snapshot=sim.captureSnapshot();
    CHECK(snapshot.world.characters.size()>=3);
    const CharacterId teacher=snapshot.world.characters[0].id;
    const CharacterId learnerA=snapshot.world.characters[1].id;
    const CharacterId learnerB=snapshot.world.characters[2].id;

    for(auto& resident:snapshot.world.characters){
        resident.needs={0.04,0.04,0.04,0.04,0.04};
        resident.civilization.character=resident.id;
        resident.civilization.knowledge=KnowledgeState{};
    }
    auto& source=snapshot.world.characters[0];
    source.civilization.knowledge.learn(
        TechniqueId::SharpFlake,KnowledgeLevel::Mastered,0.99);
    source.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Flint,8,0.6,1.0
    });
    for(auto& entry:snapshot.runtime){
        auto& runtime=entry.second;
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
    snapshot.runtime[teacher].pos={0,0};
    snapshot.runtime[learnerA].pos={1,0};
    snapshot.runtime[learnerB].pos={2,0};
    for(std::size_t i=3;i<snapshot.world.characters.size();++i)
        snapshot.runtime[snapshot.world.characters[i].id].pos={
            SettlementServiceRadiusGrid*4,0};
    snapshot.world.minute=KnowledgeTeachingDecisionIntervalMinutes-1;
    for(const auto learner:{learnerA,learnerB}){
        auto& rel=snapshot.relationships.getOrCreate(learner,teacher);
        rel.trust=1.0;
        rel.respect=1.0;
        rel.familiarity=1.0;
        rel.comfort=1.0;
    }

    CHECK(sim.restoreSnapshot(snapshot));
    Character& liveTeacher=sim.world().characters.front();
    CHECK(registerTechniqueOrigin(
        sim.socialKnowledge(),liveTeacher,TechniqueId::SharpFlake,
        sim.world().minute,CivilizationEventType::Crafted,
        sim.world().seed)!=nullptr);
    snapshot=sim.captureSnapshot();

    Simulation baseline(818188);
    CHECK(baseline.restoreSnapshot(snapshot));
    baseline.step();
    const auto ordinary=baseline.observePendingContextAction(teacher);
    CHECK(ordinary.active);
    CHECK(ordinary.kind==ContextActionKind::KnowledgeTeaching);
    CHECK(ordinary.targetResident==learnerA ||
          ordinary.targetResident==learnerB);
    const CharacterId requested=ordinary.targetResident==learnerA
        ? learnerB : learnerA;

    Simulation chosen(818188);
    CHECK(chosen.restoreSnapshot(snapshot));
    const CognitiveRequest request=chosen.observeCognitiveRequest(
        teacher,CognitiveTriggerKind::Reflection);
    CognitiveProposal proposal;
    proposal.actor=teacher;
    proposal.intent=CognitiveIntentKind::TeachKnowledge;
    proposal.priority=1.0;
    proposal.targetResident=requested;
    proposal.rationale="teach existing knowledge when feasible";
    CHECK(chosen.acceptCognitiveProposal(
        request,proposal,"local-model-test","digest-test"));
    const auto& journal=chosen.acceptedCognitiveDecisions();
    CHECK(journal.size()==1);
    CHECK(residentCognitiveTeachingPreference(
        chosen.world(),journal,teacher,requested)>0.0);
    CHECK(residentCognitiveTeachingPreference(
        chosen.world(),journal,teacher,ordinary.targetResident)==0.0);
    const auto saved=chosen.captureSnapshot();
    Simulation replay(818188);
    CHECK(replay.restoreSnapshot(saved));
    chosen.step();
    replay.step();
    const auto selected=chosen.observePendingContextAction(teacher);
    CHECK(selected.active);
    CHECK(selected.kind==ContextActionKind::KnowledgeTeaching);
    CHECK(selected.targetResident==requested);
    CHECK(selected.technique==TechniqueId::SharpFlake);
    CHECK(selected.token==replay.observePendingContextAction(teacher).token);
    CHECK(replay.observePendingContextAction(teacher).targetResident
        ==requested);
    CHECK(selected.targetResident!=ordinary.targetResident);
    // Decision was not a fabricated knowledge transfer.
    const Character* target=cognitiveCharacter(
        chosen.world(),requested);
    CHECK(target!=nullptr);
    CHECK(!target->civilization.knowledge.knowsAtLeast(
        TechniqueId::SharpFlake,KnowledgeLevel::Reproducible));

    // A newer other goal suppresses the stale preferred teaching target.
    auto altered=journal;
    altered.front().intent=CognitiveIntentKind::ImproveFoodSecurity;
    altered.front().target=0;
    CHECK(residentCognitiveTeachingPreference(
        chosen.world(),altered,teacher,requested)==0.0);
    auto tooOld=journal;
    tooOld.front().minute=
        chosen.world().minute-ResidentAgencyGoalHorizonMinutes-1;
    CHECK(residentCognitiveTeachingPreference(
        chosen.world(),tooOld,teacher,requested)==0.0);
    return true;
}

static bool runAll()
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

    // Teaching must not erase an already active self-care/work plan.
    CHECK(runTeachingWindowScenario(true,true,true));
    CHECK(runCognitiveTeachingPreferenceScenario());

    std::cout
        <<"P0 teaching budget/local opportunity regression passed\n";
    return true;
}

int main()
{
    return runAll() ? 0 : 1;
}
