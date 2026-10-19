#include <cassert>
#include <algorithm>
#include <string>

#include "lifelens/Simulation.h"
#include "lifelens/WebClientBridge.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

static std::string receipt(WebClientBridge& bridge,const std::string& actor){
    const auto json=bridge.cognitiveRequestJson(actor,"Reflection");
    const std::string key="\"contextFingerprint\":\"";
    const auto begin=json.find(key);
    if(begin==std::string::npos)return {};
    const auto value=begin+key.size();
    return json.substr(value,json.find('"',value)-value);
}

int main()
{
    // Browser-to-Core submission never accepts invented actors, intents,
    // world identity, oversized delays, illegal targets or duplicate minutes.
    WebClientBridge bridge;
    assert(bridge.newGame("874213954","",CurrentWorldGenerationVersion));
    const std::string actor="1";
    const std::string worldSeed="874213954";
    assert(!bridge.submitLocalCognitiveProposal(
        actor,480,"another-world","ImproveFoodSecurity",0.75,"","hash","Reflection",receipt(bridge,actor)));
    assert(!bridge.submitLocalCognitiveProposal(
        actor,480,worldSeed,"InventFood",0.75,"","hash","Reflection",receipt(bridge,actor)));
    assert(!bridge.submitLocalCognitiveProposal(
        actor,480,worldSeed,"CooperateWithResident",0.75,"999999","hash","Reflection",receipt(bridge,actor)));
    assert(!bridge.submitLocalCognitiveProposal(
        actor,480,worldSeed,"ImproveFoodSecurity",2.0,"","hash","Reflection",receipt(bridge,actor)));
    assert(bridge.submitLocalCognitiveProposal(
        actor,480,worldSeed,"ImproveFoodSecurity",0.75,"","hash","Reflection",receipt(bridge,actor)));
    assert(!bridge.submitLocalCognitiveProposal(
        actor,480,worldSeed,"ImproveFoodSecurity",0.75,"","hash","Reflection",receipt(bridge,actor)));

    bridge.runMinutes(97);
    assert(!bridge.submitLocalCognitiveProposal(
        actor,480,worldSeed,"ImproveWaterSecurity",0.75,"","hash","Reflection",receipt(bridge,actor)));
    WebClientBridge empty;
    assert(!empty.submitLocalCognitiveProposal(
        actor,480,worldSeed,"ImproveFoodSecurity",0.75,"","hash","Reflection",receipt(bridge,actor)));

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

    // Only a real completed action may become personal firsthand evidence.
    // The model's target and Core-validated context fingerprint are linked
    // to the same pending token; no model text can forge a trust result.
    const auto beforeExecution=withAI.captureSnapshot();
    const auto pendingIt=beforeExecution.runtime.find(self);
    assert(pendingIt!=beforeExecution.runtime.end());
    const auto& link=pendingIt->second.pendingContext.autonomousGoal;
    assert(link.active());
    assert(link.token==selected.token);
    assert(link.actor==self &&link.target==preferred);
    assert(link.contextFingerprint==cognitiveRequestFingerprint(request));
    assert(link.intent==SocialIntent::Approach);
    for(const auto& m:withAI.world().characters[0].memory.entries){
        assert(m.tags.empty() ||m.tags[0]!="goal_action_feedback");
    }

    // Persistence before execution retains the exact observation token.
    std::vector<std::uint8_t> bytes;
    assert(encodeSimulationSnapshot(beforeExecution,bytes,&error));
    SimulationStateSnapshot loaded;
    assert(decodeSimulationSnapshot(bytes,loaded,&error));
    Simulation persisted(417123);
    assert(persisted.restoreSnapshot(loaded,&error));
    const auto persistedLink=
        persisted.captureSnapshot().runtime.at(self).pendingContext.autonomousGoal;
    assert(persistedLink.token==link.token);
    assert(persistedLink.contextFingerprint==link.contextFingerprint);

    withAI.setExternalPhysicalExecution(true);
    persisted.setExternalPhysicalExecution(true);
    assert(withAI.completeExternalContextAction(
        self,selected.token,selected.targetPos));
    assert(persisted.completeExternalContextAction(
        self,selected.token,selected.targetPos));

    const Character& experienced=withAI.world().characters[0];
    const Character& replayExperience=persisted.world().characters[0];
    bool observed=false;
    for(const auto& m:experienced.memory.entries){
        if(m.tags.size()==3 &&m.tags[0]=="goal_action_feedback"
           &&m.tags[1]=="self_trust" &&m.tags[2]=="Approach"
           &&m.who==preferred &&m.source==MemorySource::DirectWitness){
            observed=true;
            assert(m.witnessed);
            bool same=false;
            for(const auto& rm:replayExperience.memory.entries)
                if(rm.tags==m.tags &&rm.who==m.who &&rm.what==m.what)
                    same=true;
            assert(same);
        }
    }
    assert(observed);
    const auto nextThought=withAI.observeCognitiveRequest(
        self,CognitiveTriggerKind::Reflection);
    bool personallyLearned=false;
    for(const auto& memory:nextThought.memories)
        if(memory.source==MemorySource::DirectWitness
           && memory.who==preferred
           &&std::find(memory.tags.begin(),memory.tags.end(),
                         "goal_action_feedback")!=memory.tags.end())
            personallyLearned=true;
    assert(personallyLearned);
    return 0;
}
