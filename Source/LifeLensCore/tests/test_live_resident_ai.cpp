#include <cassert>
#include <string>

#include "lifelens/Simulation.h"
#include "lifelens/WebClientBridge.h"

using namespace lifelens;

int main()
{
    // Browser-to-Core submission never accepts invented actors, intents,
    // world identity, oversized delays, illegal targets or duplicate minutes.
    WebClientBridge bridge;
    assert(bridge.newGame("874213954","",CurrentWorldGenerationVersion));
    const std::string actor="1";
    const std::string worldSeed="874213954";
    assert(!bridge.submitLocalCognitiveProposal(
        actor,480,"another-world","ImproveFoodSecurity",0.75,"","hash"));
    assert(!bridge.submitLocalCognitiveProposal(
        actor,480,worldSeed,"InventFood",0.75,"","hash"));
    assert(!bridge.submitLocalCognitiveProposal(
        actor,480,worldSeed,"CooperateWithResident",0.75,"999999","hash"));
    assert(!bridge.submitLocalCognitiveProposal(
        actor,480,worldSeed,"ImproveFoodSecurity",2.0,"","hash"));
    assert(bridge.submitLocalCognitiveProposal(
        actor,480,worldSeed,"ImproveFoodSecurity",0.75,"","hash"));
    assert(!bridge.submitLocalCognitiveProposal(
        actor,480,worldSeed,"ImproveFoodSecurity",0.75,"","hash"));

    bridge.runMinutes(97);
    assert(!bridge.submitLocalCognitiveProposal(
        actor,480,worldSeed,"ImproveWaterSecurity",0.75,"","hash"));
    WebClientBridge empty;
    assert(!empty.submitLocalCognitiveProposal(
        actor,480,worldSeed,"ImproveFoodSecurity",0.75,"","hash"));

    // AI can choose WHOM to approach in real Core planning, despite stronger
    // optional competitors. The two simulations differ by one accepted,
    // serialized proposal; no resources, Needs or position are directly edited.
    Simulation withAI(417123);
    withAI.setupNewGame();
    SimulationStateSnapshot initial=withAI.captureSnapshot();
    assert(initial.world.minute==480);
    assert(initial.world.characters.size()==4);
    const CharacterId self=initial.world.characters[0].id;
    const CharacterId nearby=initial.world.characters[1].id;
    const CharacterId preferred=initial.world.characters[2].id;
    for(auto& c:initial.world.characters)
        c.needs={0.01,0.01,0.01,0.01,0.01};

    Relationship& strongest=initial.relationships.getOrCreate(self,nearby);
    strongest.affection=0.96;
    strongest.trust=0.96;
    strongest.comfort=0.96;
    strongest.familiarity=0.96;
    Relationship& desired=initial.relationships.getOrCreate(self,preferred);
    desired.affection=0.55;
    desired.trust=0.55;
    desired.comfort=0.55;
    desired.familiarity=0.55;

    Simulation baseline(417123);
    std::string error;
    assert(baseline.restoreSnapshot(initial,&error));
    assert(withAI.restoreSnapshot(initial,&error));

    const CognitiveRequest request=withAI.observeCognitiveRequest(
        self,CognitiveTriggerKind::Reflection);
    CognitiveProposal proposal;
    proposal.actor=self;
    proposal.intent=CognitiveIntentKind::CooperateWithResident;
    proposal.targetResident=preferred;
    proposal.priority=0.90;
    assert(withAI.acceptCognitiveProposal(
        request,proposal,"local-shared-model","local-001"));

    // Accepted intent is persisted as Core input and changes actual planning.
    Simulation restored(417123);
    assert(restored.restoreSnapshot(withAI.captureSnapshot(),&error));
    withAI.step();
    restored.step();
    baseline.step();

    const auto selected=withAI.observePendingContextAction(self);
    const auto replayed=restored.observePendingContextAction(self);
    assert(selected.active);
    assert(selected.kind==ContextActionKind::Social);
    assert(selected.targetResident==preferred);
    assert(replayed.active);
    assert(replayed.targetResident==selected.targetResident);
    assert(replayed.token==selected.token);

    const auto ordinary=baseline.observePendingContextAction(self);
    assert(!ordinary.active
           ||ordinary.kind!=ContextActionKind::Social
           ||ordinary.targetResident!=preferred);
    assert(withAI.world().characters[0].needs.hunger
           ==baseline.world().characters[0].needs.hunger);
    return 0;
}
