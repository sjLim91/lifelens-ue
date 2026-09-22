#include <cstdint>
#include <iostream>
#include <string>

#include "lifelens/CivilizationKnowledgeTransmission.h"
#include "lifelens/Simulation.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

int main()
{
    constexpr std::uint64_t seed=626262;
    Simulation sim(seed);
    sim.setupNewGame();
    sim.setExternalPhysicalExecution(true);

    CHECK(sim.world().characters.size()>=2);
    Character& teacher=sim.world().characters[0];
    Character& learner=sim.world().characters[1];
    for(std::size_t i=2;i<sim.world().characters.size();++i){
        sim.world().characters[i].alive=false;
    }

    teacher.needs={1.0,1.0,1.0,1.0,1.0};
    learner.needs={1.0,1.0,1.0,1.0,1.0};
    teacher.civilization.knowledge.learn(
        TechniqueId::SharpFlake,KnowledgeLevel::Mastered,0.99);
    learner.civilization.knowledge.learn(
        TechniqueId::SharpFlake,KnowledgeLevel::Understood,0.72);
    learner.civilization.learningSkill=1.0;
    learner.personality.curiosity=1.0;
    learner.personality.openness=1.0;
    learner.civilization.inventory.add(
        {ItemKind::RawMaterial,MaterialKind::Flint,4,0.5,1.0});

    Relationship& learnerToTeacher=sim.relationships().getOrCreate(
        learner.id,teacher.id);
    learnerToTeacher.trust=1.0;
    learnerToTeacher.respect=1.0;
    learnerToTeacher.familiarity=1.0;
    learnerToTeacher.comfort=1.0;

    sim.world().minute=539;
    const KnowledgeReceipt* origin=registerTechniqueOrigin(
        sim.socialKnowledge(),teacher,TechniqueId::SharpFlake,
        sim.world().minute,CivilizationEventType::Crafted,seed);
    CHECK(origin!=nullptr);
    const SocialFactId factId=origin->factId;
    CHECK(sim.socialKnowledge().findReceipt(learner.id,factId)==nullptr);

    sim.runMinutes(1);
    CHECK(sim.world().minute==540);

    const PendingContextActionObservation pending=
        sim.observePendingContextAction(teacher.id);
    CHECK(pending.active);
    CHECK(pending.kind==ContextActionKind::KnowledgeTeaching);
    CHECK(pending.targetResident==learner.id);
    CHECK(pending.technique==TechniqueId::SharpFlake);
    CHECK(pending.token!=0);

    GridPos learnerPos{};
    CHECK(sim.runtimePosition(learner.id,learnerPos));

    CHECK(!sim.completeExternalContextAction(
        teacher.id,pending.token+1,learnerPos));
    CHECK(sim.observePendingContextAction(teacher.id).active);
    CHECK(sim.socialKnowledge().findReceipt(learner.id,factId)==nullptr);

    const GridPos farPos{learnerPos.x+8,learnerPos.y+8};
    CHECK(!sim.completeExternalContextAction(
        teacher.id,pending.token,farPos));
    CHECK(sim.observePendingContextAction(teacher.id).active);
    CHECK(sim.socialKnowledge().findReceipt(learner.id,factId)==nullptr);

    const SimulationStateSnapshot snapshot=sim.captureSnapshot();
    std::string error;
    Simulation restored(1);
    CHECK(restored.restoreSnapshot(snapshot,&error));
    CHECK(error.empty());
    const auto restoredPending=
        restored.observePendingContextAction(teacher.id);
    CHECK(restoredPending.active);
    CHECK(restoredPending.token==pending.token);
    CHECK(restoredPending.kind==ContextActionKind::KnowledgeTeaching);
    CHECK(restored.socialKnowledge().findReceipt(learner.id,factId)==nullptr);

    CHECK(sim.completeExternalContextAction(
        teacher.id,pending.token,learnerPos));
    CHECK(!sim.observePendingContextAction(teacher.id).active);
    CHECK(sim.socialKnowledge().findReceipt(learner.id,factId)!=nullptr);
    CHECK(sim.world().characters[1].civilization.knowledge.knowsAtLeast(
        TechniqueId::SharpFlake,KnowledgeLevel::Understood));

    std::cout << "civilization knowledge spatial teaching authority passed\n";
    return 0;
}
