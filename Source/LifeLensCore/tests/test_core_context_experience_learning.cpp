#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>

#include "lifelens/ResidentContextOutcomeLearning.h"
#include "lifelens/ResidentCognitivePreference.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

static const Character* findResident(const World& world,CharacterId id)
{
    for(const auto& resident:world.characters)
        if(resident.id==id) return &resident;
    return nullptr;
}

int main()
{
    Simulation sim(790205);
    sim.setupNewGame();
    const int initialMinute=sim.world().minute;
    const CharacterId actor=sim.world().characters[0].id;
    const CharacterId target=sim.world().characters[1].id;
    const CharacterId alternate=sim.world().characters[2].id;
    const auto request=sim.observeCognitiveRequest(
        actor,CognitiveTriggerKind::Reflection);
    CognitiveProposal thought;
    thought.actor=actor;
    thought.intent=CognitiveIntentKind::TradeWithResident;
    thought.targetResident=target;
    thought.priority=1.0;
    thought.rationale="Try locally available exchange";
    assert(sim.acceptCognitiveProposal(request,thought,"local-test","outcome-learning"));

    auto& resident=sim.world().characters[0];
    const double neutral=residentCognitiveIntentPreference(
        sim.world(),sim.acceptedCognitiveDecisions(),actor,
        CognitiveIntentKind::TradeWithResident,target);
    assert(std::abs(neutral-0.12)<1e-12);
    const std::size_t originalMemories=resident.memory.entries.size();

    // A disappearing token and journey return do not forge trade outcomes.
    rememberCoreContextExperience(resident,{actor,target,101,initialMinute,
        ContextActionKind::Trade,CoreContextOutcomeCode::TradeReturned});
    assert(resident.memory.entries.size()==originalMemories);
    // Inconsistent action kind/code is discarded instead of teaching fiction.
    rememberCoreContextExperience(resident,{actor,target,102,initialMinute,
        ContextActionKind::Trade,CoreContextOutcomeCode::TeachingAdvanced});
    assert(resident.memory.entries.size()==originalMemories);

    for(int minute=1;minute<=4;++minute){
        sim.world().minute=initialMinute+minute;
        const CoreContextOutcomeEvent failed{
            actor,target,static_cast<std::uint64_t>(200+minute),sim.world().minute,
            ContextActionKind::Trade,CoreContextOutcomeCode::TradeExchangeRejected
        };
        rememberCoreContextExperience(resident,failed);
        rememberCoreContextExperience(resident,failed); // duplicate ACK
    }
    const auto* belief=resident.beliefs.find(
        0,actionExperienceProposition("CoreContext:Trade"));
    assert(belief &&belief->contradictionCount==4);
    assert(resident.memory.entries.size()==originalMemories+4);
    assert(learnedActionDisposition(resident,"CoreContext:Trade")<1.0);
    assert(residentCognitivePartnerExperienceFactor(
        resident,target,"CoreContext:Trade",sim.world().minute)==0.0);
    // Failing with one actual person must NOT count as failure against a
    // different person or a different activity such as teaching.
    assert(residentCognitivePartnerExperienceFactor(
        resident,alternate,"CoreContext:Trade",sim.world().minute)==1.0);
    assert(residentCognitivePartnerExperienceFactor(
        resident,target,"CoreContext:Teaching",sim.world().minute)==1.0);
    const double discouraged=residentCognitiveIntentPreference(
        sim.world(),sim.acceptedCognitiveDecisions(),actor,
        CognitiveIntentKind::TradeWithResident,target);
    assert(discouraged==0.0);
    // The accepted thought remains in the journal; only its optional
    // preference for a repeatedly failing partner is temporarily released.
    assert(sim.acceptedCognitiveDecisions().size()==1);

    // Same Core intent can still be considered after failure; real later
    // success repairs confidence rather than permanently banning exchange.
    for(int minute=5;minute<=10;++minute){
        sim.world().minute=initialMinute+minute;
        rememberCoreContextExperience(resident,{actor,target,
            static_cast<std::uint64_t>(200+minute),sim.world().minute,
            ContextActionKind::Trade,CoreContextOutcomeCode::TradeExchanged});
    }
    assert(belief->supportCount==6);
    assert(resident.memory.entries.size()<=originalMemories+4);
    assert(learnedActionDisposition(resident,"CoreContext:Trade")>1.0);
    assert(std::abs(residentCognitivePartnerExperienceFactor(
        resident,target,"CoreContext:Trade",sim.world().minute)-1.04)<1e-12);
    assert(std::abs(residentCognitiveIntentPreference(
        sim.world(),sim.acceptedCognitiveDecisions(),actor,
        CognitiveIntentKind::TradeWithResident,target)-0.12)<1e-12);

    bool hasVerifiablePersonalSource=false;
    for(const auto& memory:resident.memory.entries){
        if(memory.tags.size()!=3 ||memory.tags[1]!="CoreContext:Trade")
            continue;
        assert(memory.source==MemorySource::DirectWitness);
        assert(memory.sourceCharacter==actor);
        assert(memory.who==target);
        assert(memory.where.find("core-context-token:")==0);
        hasVerifiablePersonalSource=true;
    }
    assert(hasVerifiablePersonalSource);

    // Distinct partners keep their own evidence without a new persistent
    // AI or per-partner belief key. Failed B does not erase succeeded A.
    Character alternateHistory=resident;
    rememberCoreContextExperience(alternateHistory,{
        actor,alternate,1005,initialMinute+11,
        ContextActionKind::Trade,CoreContextOutcomeCode::TradeExchangeRejected
    });
    assert(std::abs(residentCognitivePartnerExperienceFactor(
        alternateHistory,target,"CoreContext:Trade",initialMinute+11)-1.04)<1e-12);
    assert(std::abs(residentCognitivePartnerExperienceFactor(
        alternateHistory,alternate,"CoreContext:Trade",initialMinute+11)-0.92)<1e-12);

    // An outcome must be directly witnessed and tied to a real Core token.
    Character forged=alternateHistory;
    for(auto& memory:forged.memory.entries){
        if(memory.who==alternate &&memory.tags.size()==3
           &&memory.tags[1]=="CoreContext:Trade")
            memory.source=MemorySource::ToldByOther;
    }
    assert(residentCognitivePartnerExperienceFactor(
        forged,alternate,"CoreContext:Trade",initialMinute+11)==1.0);
    for(auto& memory:forged.memory.entries){
        if(memory.who==alternate &&memory.tags.size()==3
           &&memory.tags[1]=="CoreContext:Trade"){
            memory.source=MemorySource::DirectWitness;
            memory.where="model-claimed-core-context-token:1005";
        }
    }
    assert(residentCognitivePartnerExperienceFactor(
        forged,alternate,"CoreContext:Trade",initialMinute+11)==1.0);
    // Same-minute success wins in either iteration order, preventing
    // non-deterministic conclusions from a reordered saved memory list.
    Character sameMinute=alternateHistory;
    for(const auto& memory:alternateHistory.memory.entries){
        if(memory.who==alternate &&memory.tags.size()==3
           &&memory.tags[1]=="CoreContext:Trade"){
            auto success=memory;
            success.tags[2]="success";
            success.where="core-context-token:1006";
            sameMinute.memory.add(success);
            break;
        }
    }
    assert(std::abs(residentCognitivePartnerExperienceFactor(
        sameMinute,alternate,"CoreContext:Trade",initialMinute+11)-1.04)<1e-12);
    std::reverse(sameMinute.memory.entries.begin(),sameMinute.memory.entries.end());
    assert(std::abs(residentCognitivePartnerExperienceFactor(
        sameMinute,alternate,"CoreContext:Trade",initialMinute+11)-1.04)<1e-12);
    assert(residentCognitivePartnerExperienceFactor(
        alternateHistory,alternate,"CoreContext:Trade",initialMinute+1452)==1.0);

    // Care evidence is observed but never used to suppress essential care
    // preference; only optional teaching and trade get a bounded adjustment.
    sim.world().minute=initialMinute+11;
    rememberCoreContextExperience(resident,{actor,target,999,sim.world().minute,
        ContextActionKind::Parenting,CoreContextOutcomeCode::ParentingRejected});
    assert(learnedActionDisposition(resident,"CoreContext:Parenting")<1.0);

    std::vector<std::uint8_t> before,after;
    std::string error;
    assert(encodeSimulationSnapshot(sim.captureSnapshot(),before,&error));
    SimulationStateSnapshot snapshot;
    assert(decodeSimulationSnapshot(before,snapshot,&error));
    assert(encodeSimulationSnapshot(snapshot,after,&error));
    assert(after==before);
    const auto* loaded=findResident(snapshot.world,actor);
    assert(loaded!=nullptr);
    assert(std::abs(learnedActionDisposition(*loaded,"CoreContext:Trade")
        -learnedActionDisposition(resident,"CoreContext:Trade"))<1e-12);
    assert(std::abs(residentCognitivePartnerExperienceFactor(
        *loaded,target,"CoreContext:Trade",sim.world().minute)
        -residentCognitivePartnerExperienceFactor(
            resident,target,"CoreContext:Trade",sim.world().minute))<1e-12);
    assert(std::abs(learnedActionDisposition(*loaded,"CoreContext:Parenting")
        -learnedActionDisposition(resident,"CoreContext:Parenting"))<1e-12);
    return 0;
}
