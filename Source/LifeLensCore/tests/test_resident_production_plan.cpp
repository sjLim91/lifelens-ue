#include <cassert>
#include <iostream>
#include "lifelens/Simulation.h"
#include "lifelens/CoreNavigation.h"
#include "lifelens/ResidentProductionPlanCodec.h"
#include "lifelens/SimulationSnapshotCodec.h"
using namespace lifelens;
static std::vector<std::uint8_t> encode(const Simulation& sim) {
    std::vector<std::uint8_t> b;std::string e;
    if(!encodeSimulationSnapshot(sim.captureSnapshot(),b,&e))std::cerr<<e<<'\n';
    assert(!b.empty());return b;
}
static Simulation fixture(int extraSources=0) {
    auto rules=DefaultSimulationRuleset;
    rules.needs.hungerPerMinute=rules.needs.thirstPerMinute=rules.needs.sleepPerMinute=0;
    rules.needs.bladderPerMinute=rules.needs.hygienePerMinute=0;
    Simulation sim(874213954,0,CurrentWorldGenerationVersion,rules);sim.setupNewGame();
    sim.world().wildlifePopulations.clear();sim.world().wildlifeEnabled=false;
    for(auto& c:sim.world().characters)c.needs={.03,.03,.03,.03,.03};
    auto& actor=sim.world().characters.front();actor.civilization.inventory=Inventory{};
    actor.civilization.knowledge=KnowledgeState{};
    actor.civilization.knowledge.learn(TechniqueId::DiggingStick,KnowledgeLevel::Reproducible,1);
    for(auto& n:sim.world().resourceNodes){n.quantity=0;for(auto& t:n.trees)t.remainingWood=0;}
    GridPos position;assert(sim.runtimePosition(actor.id,position));
    ResourceNode a;a.id=9000201;a.material=MaterialKind::Wood;a.quantity=a.maxQuantity=20;a.pos=position;a.renewable=false;
    ResourceNode b=a;b.id=9000202;bool reachable=false;
    for(int d=2;d<=5 &&!reachable;++d)for(auto off:{GridPos{d,0},GridPos{-d,0},GridPos{0,d},GridPos{0,-d}}) {
        b.pos={position.x+off.x,position.y+off.y};std::vector<GridPos> route;
        if(coreGroundTraversable(sim.world(),b.pos) &&buildCoreGroundRoute(sim.world(),position,b.pos,1,route)){reachable=true;break;}
    }
    assert(reachable);sim.world().resourceNodes.push_back(a);sim.world().resourceNodes.push_back(b);
    for(int i=0;i<extraSources;++i){auto extra=a;extra.id=9000203+i;sim.world().resourceNodes.push_back(extra);}
    const auto request=sim.observeCognitiveRequest(actor.id,CognitiveTriggerKind::Reflection);
    CognitiveProposal proposal;proposal.actor=actor.id;proposal.intent=CognitiveIntentKind::CraftUsefulTools;proposal.priority=.96;
    assert(sim.acceptCognitiveProposal(request,proposal,"controlled-native-test","source-loss-fixture"));
    return sim;
}
int main() {
    auto sim=fixture();const auto actor=sim.world().characters.front().id;
    sim.step();const auto* p=sim.residentProductionPlan(actor);assert(p &&p->steps.size()==1);
    assert(p->target==TechniqueId::DiggingStick &&p->steps.front().intent==CivilizationIntent::Gather);
    const auto goal=p->id;const auto deadline=p->expiresMinute;
    // Controlled competing depletion AFTER a genuine command was issued.
    // It is not counted as an untouched-world observation or fake receipt.
    const auto failedSource=p->steps.front().source;
    for(auto& n:sim.world().resourceNodes)if(n.id==failedSource){n.quantity=0;for(auto& tree:n.trees)tree.remainingWood=0;}
    const auto saved=encode(sim);SimulationStateSnapshot snapshot;std::string error;
    assert(decodeSimulationSnapshot(saved,snapshot,&error));
    Simulation replay(1,0,CurrentWorldGenerationVersion,sim.ruleset());assert(replay.restoreSnapshot(snapshot,&error));
    for(int i=0;i<120;++i) {
        sim.step();replay.step();assert(encode(sim)==encode(replay));
        if(const auto* plan=sim.residentProductionPlan(actor);plan &&!plan->open())break;
    }
    p=sim.residentProductionPlan(actor);assert(p &&p->id==goal &&p->expiresMinute==deadline);
    assert(p->state==ProductionPlanState::Completed &&p->steps.size()>=3 &&p->steps.size()<=6);
    assert(p->steps.front().result==CognitiveStepResult::Failed);
    assert(p->steps.front().reason==ProductionPlanReason::SourceExhausted);
    assert(p->steps[1].source!=failedSource &&p->steps[1].stockAfter>p->steps[1].stockBefore);
    assert(p->steps.back().intent==CivilizationIntent::Craft &&p->steps.back().stockAfter>=p->targetStock);
    assert(sim.world().characters.front().civilization.inventory.count(ItemKind::DiggingStick,MaterialKind::Wood)==1);
    const auto complete=sim.captureSnapshot();assert(validProductionPlans(complete));
    auto bad=complete;bad.runtime.at(actor).productionPlan.steps.back().token=goal;
    assert(!replay.restoreSnapshot(bad,&error));
    std::vector<std::uint8_t> bytes;assert(!encodeSimulationSnapshot(bad,bytes,&error));
    bad=complete;bad.runtime.at(actor).productionPlan.expiresMinute=deadline+1;
    assert(!replay.restoreSnapshot(bad,&error));
    bad=complete;bad.runtime.at(actor).productionPlan.cognitiveFingerprint++;
    bad.runtime.at(actor).productionPlan.state=ProductionPlanState::Active;
    bad.runtime.at(actor).productionPlan.finishedMinute=-1;
    assert(!replay.restoreSnapshot(bad,&error));
    auto truncated=encode(sim);truncated.pop_back();assert(!decodeSimulationSnapshot(truncated,snapshot,&error));
    // Old saves have no implicit production goal and no new envelope.
    Simulation old(42);old.setupNewGame();const auto legacy=encode(old);
    assert(decodeSimulationSnapshot(legacy,snapshot,&error));assert(!hasProductionPlans(snapshot));
    assert(old.restoreSnapshot(snapshot,&error));assert(encode(old)==legacy);
    // The original abstract tool thought now binds a CONCRETE output.
    // Another affordable tool must not silently replace the selected target
    // after its shared Wood ingredient arrives.
    auto committed=fixture();auto& maker=committed.world().characters.front();
    maker.civilization.knowledge.learn(TechniqueId::StoneHammer,KnowledgeLevel::Reproducible,1);
    maker.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Stone,2,.6,1});
    maker.civilization.inventory.add({ItemKind::Cordage,MaterialKind::Fiber,1,.6,1});
    committed.step();
    assert(committed.residentProductionPlan(actor) &&committed.residentProductionPlan(actor)->target==TechniqueId::DiggingStick);
    for(int i=0;i<119 &&committed.residentProductionPlan(actor)->open();++i)committed.step();
    assert(committed.residentProductionPlan(actor)->state==ProductionPlanState::Completed);
    assert(committed.world().characters.front().civilization.inventory.count(ItemKind::DiggingStick,MaterialKind::Wood)==1);
    assert(committed.world().characters.front().civilization.inventory.count(ItemKind::StoneHammer,MaterialKind::Stone)==0);
    // A newly accepted low-priority proposal is not a physical execution
    // decision. The already-issued real command finishes exactly once; its
    // actual result survives when the old objective is subsequently dropped.
    auto changed=fixture();changed.step();
    const auto originalToken=changed.residentProductionPlan(actor)->steps.front().token;
    const auto newerRequest=changed.observeCognitiveRequest(actor,CognitiveTriggerKind::Reflection);
    CognitiveProposal newer;newer.actor=actor;newer.intent=CognitiveIntentKind::ExploreOpportunity;newer.priority=.51;
    assert(changed.acceptCognitiveProposal(newerRequest,newer,"controlled-update","newer-proposal"));
    changed.step();
    const auto* oldPlan=changed.residentProductionPlan(actor);
    assert(oldPlan &&oldPlan->steps.front().token==originalToken &&oldPlan->steps.front().result==CognitiveStepResult::Succeeded);
    const auto rawAfter=changed.world().characters.front().civilization.inventory.count(ItemKind::RawMaterial,MaterialKind::Wood);
    assert(rawAfter==2);changed.step();
    oldPlan=changed.residentProductionPlan(actor);
    assert(oldPlan->state==ProductionPlanState::Abandoned &&oldPlan->reason==ProductionPlanReason::NewerGoal);
    assert(changed.world().characters.front().civilization.inventory.count(ItemKind::RawMaterial,MaterialKind::Wood)==rawAfter);
    assert(!encode(changed).empty());
    // Four actual failed sources, then two one-unit real harvests, exhaust
    // SIX owned commands while the concrete tool still needs a final craft.
    // A competitor only removes existing stock; no success/memory is injected.
    auto limited=fixture(6);bool budgetClosed=false;std::uint64_t budgetFp=0;
    for(int i=0;i<115 &&!budgetClosed;++i) {
        const auto state=limited.captureSnapshot();const auto& pending=state.runtime.at(actor).pendingContext;
        const auto* plan=limited.residentProductionPlan(actor);
        if(plan &&pending.active() &&pending.civilization.intent==CivilizationIntent::Gather) {
            const int leave=plan->steps.size()<=4?0:1;
            for(auto& node:limited.world().resourceNodes)if(node.id==pending.civilization.resourceNode) {
                node.quantity=std::min(node.quantity,leave);for(auto& tree:node.trees)tree.remainingWood=0;
            }
        }
        limited.step();plan=limited.residentProductionPlan(actor);
        if(plan &&plan->state==ProductionPlanState::Abandoned &&plan->reason==ProductionPlanReason::Budget) {
            assert(plan->steps.size()==MaxProductionSteps);
            assert(plan->steps.back().stockAfter==2);
            budgetFp=plan->cognitiveFingerprint;budgetClosed=true;
        }
    }
    assert(budgetClosed &&budgetFp);
    const auto commandsForOldThought=[&](){
        std::size_t count=0;for(const auto& trace:limited.cognitiveCausalJournal())
            if(trace.accepted &&trace.origin.actor==actor &&trace.origin.contextFingerprint==budgetFp)count+=trace.steps.size();
        return count;
    };
    assert(commandsForOldThought()==MaxProductionSteps);
    const int beforeLife=limited.world().minute;limited.runMinutes(20);
    assert(limited.world().minute==beforeLife+20 &&commandsForOldThought()==MaxProductionSteps);
    assert(limited.residentProductionPlan(actor)->state==ProductionPlanState::Abandoned);
    assert(limited.residentProductionPlan(actor)->steps.size()==MaxProductionSteps);
    assert(!encode(limited).empty());
    limited.runMinutes(140);
    assert(commandsForOldThought()==MaxProductionSteps);
    // A permanent survival emergency may consume time, never extend the
    // original promise or retain it forever beyond its deadline.
    auto expired=fixture();expired.step();const auto expires=expired.residentProductionPlan(actor)->expiresMinute;
    for(int i=0;i<150;++i) {
        expired.world().characters.front().needs.hunger=.99;
        expired.step();
        if(expired.world().minute>expires+1)break;
    }
    assert(!expired.residentProductionPlan(actor)->open());
    assert(expired.residentProductionPlan(actor)->reason==ProductionPlanReason::Deadline);
    assert(expired.residentProductionPlan(actor)->expiresMinute==expires);
    std::cout<<"PASS concrete tool goal: real source A failure -> movement/source B -> craft, exact saved replay, forged/torn/old saves, bounded deadline\n";
}
