#include <cassert>
#include <algorithm>
#include <string>

#include "lifelens/Simulation.h"
#include "lifelens/WebClientBridge.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/ResidentCivilizationOutcomeLearning.h"
#include "lifelens/ResidentContextOutcomeLearning.h"

using namespace lifelens;

static std::string receipt(WebClientBridge& bridge,const std::string& actor,
    const std::string& trigger="Reflection"){
    const auto json=bridge.cognitiveRequestJson(actor,trigger);
    const std::string key="\"contextFingerprint\":\"";
    const auto begin=json.find(key);
    if(begin==std::string::npos)return {};
    const auto value=begin+key.size();
    return json.substr(value,json.find('"',value)-value);
}

static std::string encodedSnapshot(const SimulationStateSnapshot& snapshot)
{
    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(snapshot,bytes,&error));
    constexpr char digits[]=
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    for(std::size_t i=0;i<bytes.size();i+=3){
        const auto word=(static_cast<std::uint32_t>(bytes[i])<<16)
            |(i+1<bytes.size()?static_cast<std::uint32_t>(bytes[i+1])<<8:0)
            |(i+2<bytes.size()?bytes[i+2]:0);
        encoded.push_back(digits[(word>>18)&63]);
        encoded.push_back(digits[(word>>12)&63]);
        encoded.push_back(i+1<bytes.size()?digits[(word>>6)&63]:'=');
        encoded.push_back(i+2<bytes.size()?digits[word&63]:'=');
    }
    return encoded;
}

static bool submitRepeatedFailure(const SimulationStateSnapshot& snapshot,
    const std::string& receiptTrigger="RepeatedFailure")
{
    WebClientBridge bridge;
    assert(bridge.restoreSnapshotBase64(encodedSnapshot(snapshot)));
    const auto actor=std::to_string(snapshot.world.characters.front().id);
    return bridge.submitLocalCognitiveProposal(actor,snapshot.world.minute,
        std::to_string(snapshot.world.seed),"AcquireMaterials",0.75,"",
        "controlled-bridge-regression","RepeatedFailure",
        receipt(bridge,actor,receiptTrigger));
}

static void repeatedFailureBridgeRegression()
{
    // Controlled execution/bridge regression, NOT an unmodified New Game
    // witness or Qwen inference. A competitor really harvests a natural
    // source; two subsequent Core extraction attempts fail without inventory
    // grants or fabricated success. Their own outcomes authorize reflection.
    Simulation simulation(874213954);
    simulation.setupNewGame();
    auto state=simulation.captureSnapshot();
    assert(!submitRepeatedFailure(state));
    CivilizationUtilityDecision gather;
    gather.intent=CivilizationIntent::Gather;
    gather.material=MaterialKind::Stone;
    GridPos target{};
    for(const auto& node:state.world.resourceNodes){
        if(node.material==gather.material &&node.quantity>0
           &&resolveCivilizationResourceAccessGridPosition(state.world,node.id,target)){
            gather.resourceNode=node.id;
            gather.quantity=node.quantity;
            break;
        }
    }
    assert(gather.resourceNode!=0);
    auto& actor=state.world.characters.front();
    const auto depleted=executeCivilizationDecisionAtPosition(
        state.world,state.world.characters.at(1),gather,target);
    assert(depleted.executed &&depleted.success);
    gather.quantity=1;
    for(int attempt=0;attempt<2;++attempt){
        const auto result=executeCivilizationDecisionAtPosition(
            state.world,actor,gather,target);
        assert(!result.success &&result.event.quantity==0);
        rememberCivilizationAttemptOutcome(actor,gather,state.world.minute,
            result.success,diagnoseResidentGatherFailure(state.world,actor,gather,target));
        if(attempt==0){
            assert(!submitRepeatedFailure(state));
            ++state.world.minute;
        }
    }
    assert(submitRepeatedFailure(state));
    assert(!submitRepeatedFailure(state,"Reflection"));
    const auto outcome=[](const MemoryRecord& memory){
        return memory.tags.size()==3 &&memory.tags[0]=="action_experience"
            &&memory.tags[1]=="Civilization:Gather:Stone";
    };
    auto invalid=state;
    for(auto& memory:invalid.world.characters.front().memory.entries)
        if(outcome(memory)) memory.source=MemorySource::ToldByOther;
    assert(!submitRepeatedFailure(invalid));
    invalid=state;
    for(auto& memory:invalid.world.characters.front().memory.entries)
        if(outcome(memory)) memory.sourceCharacter=state.world.characters.at(1).id;
    assert(!submitRepeatedFailure(invalid));
    invalid=state;
    for(auto& memory:invalid.world.characters.front().memory.entries)
        if(outcome(memory)) memory.what="모델이 실패했다고 주장했다";
    assert(!submitRepeatedFailure(invalid));
    invalid=state;
    for(auto& memory:invalid.world.characters.front().memory.entries)
        if(outcome(memory)) memory.minute=invalid.world.minute;
    assert(!submitRepeatedFailure(invalid));
    invalid=state;
    invalid.world.minute+=1441;
    assert(!submitRepeatedFailure(invalid));

    // A real later successful extraction ends the action's failure streak.
    ++state.world.minute;
    bool recovered=false;
    for(const auto& node:state.world.resourceNodes){
        if(node.material!=gather.material ||node.quantity<=0
           ||!resolveCivilizationResourceAccessGridPosition(state.world,node.id,target))
            continue;
        gather.resourceNode=node.id;
        const auto result=executeCivilizationDecisionAtPosition(state.world,actor,gather,target);
        if(!result.success) continue;
        rememberCivilizationAttemptOutcome(actor,gather,state.world.minute,result.success);
        recovered=true;
        break;
    }
    assert(recovered &&!submitRepeatedFailure(state));

    // Typed Core context records use distinct tokens AND the same partner.
    // This is an isolated contract fixture, not simulated social success.
    state=simulation.captureSnapshot();
    auto& social=state.world.characters.front();
    CoreContextOutcomeEvent event;
    event.actor=social.id;
    event.target=state.world.characters.at(1).id;
    event.kind=ContextActionKind::KnowledgeTeaching;
    event.code=CoreContextOutcomeCode::TeachingRejected;
    event.minute=state.world.minute;
    event.token=111;
    rememberCoreContextExperience(social,event);
    event.minute=++state.world.minute;
    event.token=112;
    rememberCoreContextExperience(social,event);
    assert(submitRepeatedFailure(state));
    invalid=state;
    for(auto& memory:invalid.world.characters.front().memory.entries)
        if(memory.tags.size()==3 &&memory.tags[1]=="CoreContext:Teaching")
            memory.where="core-context-token:111";
    assert(!submitRepeatedFailure(invalid));
    event.code=CoreContextOutcomeCode::TeachingAdvanced;
    event.minute=++state.world.minute;
    event.token=113;
    rememberCoreContextExperience(social,event);
    assert(!submitRepeatedFailure(state));
}

int main()
{
    repeatedFailureBridgeRegression();
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
