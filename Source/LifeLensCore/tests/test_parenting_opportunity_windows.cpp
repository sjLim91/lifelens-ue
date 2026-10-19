#include <cassert>
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
using namespace lifelens;

int main()
{
    Character caregiver,child;
    caregiver.id=1;
    caregiver.lifeStage=LifeStage::Adult;
    child.id=5;
    child.lifeStage=LifeStage::Toddler;
    child.parentIds={1};
    child.needs={0.70,0.10,0.10,0.10,0.10};
    Relationship bond;
    ParentingContext context;
    context.waterAvailable=false;
    // This fixture reproduces the real competing routine-development score.
    const auto all=chooseParentingAction(caregiver,child,bond,context);
    assert(all.valid);
    const auto physical=chooseParentingAction(
        caregiver,child,bond,context,false,false);
    assert(physical.valid &&physical.action==ParentingAction::Feed);
    const auto ordinary=chooseParentingAction(caregiver,child,bond,context,true,true);
    assert(ordinary.action==all.action &&ordinary.utility==all.utility);

    Simulation sim(909090);
    sim.setupNewGame();
    auto snapshot=sim.captureSnapshot();
    const auto parent=snapshot.world.characters.front().id;
    const auto otherParent=snapshot.world.characters[1].id;
    auto& liveParent=snapshot.world.characters.front();
    liveParent.needs={0.02,0.02,0.02,0.02,0.02};
    liveParent.civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::PlantFood,5,0.8,1.0});
    child.id=998877;
    child.parentIds={parent,otherParent};
    child.civilization.character=child.id;
    child.alive=true;
    child.hasBirthMinute=true;
    child.birthMinute=0;
    const GridPos home=snapshot.runtime.at(parent).pos;
    snapshot.world.characters.push_back(child);
    assert(snapshot.genealogy.registerBirth(child.id,parent,otherParent));
    SimulationRuntimeSnapshot baby;
    baby.pos=home;
    snapshot.runtime.emplace(child.id,baby);
    auto& runtime=snapshot.runtime.at(parent);
    runtime.goal=Goal::Idle;
    runtime.plan.clear();
    runtime.pendingContext.clear();
    snapshot.world.minute=30; // physical care window, not daily education
    snapshot.world.externalPhysicalExecution=true;
    assert(sim.restoreSnapshot(snapshot));
    assert(sim.acceptedCognitiveDecisions().empty());
    sim.step();
    const auto pending=sim.observePendingContextAction(parent);
    assert(pending.active &&pending.kind==ContextActionKind::Parenting);
    assert(pending.targetResident==child.id);
    assert(pending.parentingAction==ParentingAction::Feed);

    std::vector<std::uint8_t> bytes;
    assert(encodeSimulationSnapshot(sim.captureSnapshot(),bytes));
    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded));
    Simulation replay(909090);
    assert(replay.restoreSnapshot(decoded));
    // Physical execution is a client adapter choice, not persisted world truth.
    replay.setExternalPhysicalExecution(true);
    const int before=sim.world().characters.front().civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood);
    assert(sim.completeExternalContextAction(parent,pending.token,home));
    assert(replay.completeExternalContextAction(parent,pending.token,home));
    assert(sim.world().characters.front().civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)==before-1);
    const auto* fed=findObservedCharacter(sim.world(),child.id);
    assert(fed &&fed->needs.hunger<0.70);
    assert(!sim.completeExternalContextAction(parent,pending.token,home));
    std::vector<std::uint8_t> actual,expected;
    assert(encodeSimulationSnapshot(sim.captureSnapshot(),actual));
    assert(encodeSimulationSnapshot(replay.captureSnapshot(),expected));
    assert(actual==expected);
}
