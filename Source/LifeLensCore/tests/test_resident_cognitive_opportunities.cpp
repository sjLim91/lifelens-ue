#include <cassert>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "lifelens/ResidentCognitivePreference.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

static int units(const Simulation& sim,CharacterId id,MaterialKind material)
{
    const auto* resident=cognitiveCharacter(sim.world(),id);
    assert(resident!=nullptr);
    return resident->civilization.inventory.count(ItemKind::RawMaterial,material);
}

static void choose(Simulation& sim,CharacterId actor,
                   CognitiveIntentKind intent,CharacterId target=0)
{
    const auto request=sim.observeCognitiveRequest(
        actor,CognitiveTriggerKind::Reflection);
    CognitiveProposal proposal;
    proposal.actor=actor;
    proposal.intent=intent;
    proposal.priority=1.0;
    proposal.targetResident=target;
    proposal.rationale="prefer an existing legal Core opportunity";
    assert(sim.acceptCognitiveProposal(
        request,proposal,"local-test","preference-regression"));
}

static SimulationStateSnapshot binaryRoundTrip(const Simulation& sim)
{
    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(sim.captureSnapshot(),bytes,&error));
    SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));
    std::vector<std::uint8_t> second;
    assert(encodeSimulationSnapshot(decoded,second,&error));
    assert(second==bytes);
    return decoded;
}

static void preferenceGuards()
{
    Simulation sim(790101);
    sim.setupNewGame();
    const auto a=sim.world().characters[0].id;
    const auto b=sim.world().characters[1].id;
    choose(sim,a,CognitiveIntentKind::TradeWithResident,b);
    const auto journal=sim.acceptedCognitiveDecisions();
    assert(residentCognitiveTradePreference(sim.world(),journal,a,b)==0.12);
    assert(residentCognitiveIntentPreference(sim.world(),journal,a,
        CognitiveIntentKind::MigrateHousehold)==0.0);

    auto invalid=journal;
    invalid.front().priority=std::numeric_limits<double>::quiet_NaN();
    assert(residentCognitiveTradePreference(sim.world(),invalid,a,b)==0.0);
    invalid=journal;
    invalid.front().priority=0.54;
    assert(residentCognitiveTradePreference(sim.world(),invalid,a,b)==0.0);
    invalid=journal;
    invalid.front().minute=sim.world().minute+1;
    assert(residentCognitiveTradePreference(sim.world(),invalid,a,b)==0.0);
    invalid=journal;
    invalid.front().contextFingerprint=0;
    assert(residentCognitiveTradePreference(sim.world(),invalid,a,b)==0.0);

    World later=sim.world();
    later.minute+=ResidentAgencyGoalHorizonMinutes+1;
    assert(residentCognitiveTradePreference(later,journal,a,b)==0.0);
    World dead=sim.world();
    dead.characters[1].alive=false;
    assert(residentCognitiveTradePreference(dead,journal,a,b)==0.0);
    dead=sim.world();
    dead.characters[0].alive=false;
    assert(residentCognitiveTradePreference(dead,journal,a,b)==0.0);

    sim.world().minute++;
    choose(sim,a,CognitiveIntentKind::ImproveFoodSecurity);
    assert(residentCognitiveTradePreference(
        sim.world(),sim.acceptedCognitiveDecisions(),a,b)==0.0);
}

static void localTradeChoice()
{
    Simulation sim(790102);
    sim.setupNewGame();
    auto snapshot=sim.captureSnapshot();
    assert(snapshot.world.characters.size()==4);
    const auto a=snapshot.world.characters[0].id;
    const auto b=snapshot.world.characters[1].id;
    const auto requested=snapshot.world.characters[2].id;
    const GridPos home=snapshot.runtime.at(a).pos;
    snapshot.world.minute=59;
    for(auto& resident:snapshot.world.characters){
        resident.needs={0.02,0.02,0.02,0.02,0.02};
        resident.civilization.inventory=Inventory{};
        auto& runtime=snapshot.runtime.at(resident.id);
        runtime.goal=Goal::Idle;
        runtime.plan.clear();
        runtime.pendingContext.clear();
        runtime.penaltyUntilMinute=10000;
        runtime.socialActive=false;
        runtime.pos=home;
        runtime.navigationRoute.clear();
        runtime.navigationRouteIndex=0;
        runtime.navigationHasTarget=false;
        runtime.navigationArrived=false;
        runtime.navigationRouteFailed=false;
    }
    snapshot.world.characters[0].civilization.inventory.add({
        ItemKind::RawMaterial,MaterialKind::Wood,10,0.8,1.0});
    for(std::size_t i=1;i<3;++i)
        snapshot.world.characters[i].civilization.inventory.add({
            ItemKind::RawMaterial,MaterialKind::Stone,10,0.8,1.0});
    for(const auto target:{b,requested}){
        snapshot.relationships.getOrCreate(a,target).trust=0.95;
        snapshot.relationships.getOrCreate(target,a).trust=0.95;
    }

    Simulation ordinary(790102);
    assert(ordinary.restoreSnapshot(snapshot));
    ordinary.step();
    assert(units(ordinary,b,MaterialKind::Wood)==1);
    assert(units(ordinary,requested,MaterialKind::Wood)==0);

    Simulation preferred(790102);
    assert(preferred.restoreSnapshot(snapshot));
    choose(preferred,a,CognitiveIntentKind::TradeWithResident,requested);
    // Accepted intent alone cannot exchange anything.
    assert(units(preferred,a,MaterialKind::Stone)==0);
    const auto saved=binaryRoundTrip(preferred);
    Simulation replay(790102);
    assert(replay.restoreSnapshot(saved));
    preferred.step();
    replay.step();
    assert(units(preferred,a,MaterialKind::Stone)==1);
    assert(units(preferred,a,MaterialKind::Wood)==9);
    assert(units(preferred,requested,MaterialKind::Wood)==1);
    assert(units(preferred,requested,MaterialKind::Stone)==9);
    assert(units(preferred,b,MaterialKind::Wood)==0);
    assert(societyExchangeFactCount(preferred.socialKnowledge())==1);
    assert(units(replay,requested,MaterialKind::Wood)==1);
    std::vector<std::uint8_t> left,right;
    assert(encodeSimulationSnapshot(preferred.captureSnapshot(),left));
    assert(encodeSimulationSnapshot(replay.captureSnapshot(),right));
    assert(left==right);

    // Validated thought cannot authorize missing stock or a remote local swap.
    auto empty=saved;
    empty.world.characters[2].civilization.inventory=Inventory{};
    Simulation noGoods(790102);
    assert(noGoods.restoreSnapshot(empty));
    noGoods.step();
    assert(units(noGoods,requested,MaterialKind::Wood)==0);
    assert(units(noGoods,b,MaterialKind::Wood)==1);

    auto distant=saved;
    distant.runtime.at(requested).pos={home.x+SettlementServiceRadiusGrid*4,home.y};
    Simulation noRemote(790102);
    assert(noRemote.restoreSnapshot(distant));
    noRemote.step();
    assert(units(noRemote,requested,MaterialKind::Wood)==0);
    assert(units(noRemote,b,MaterialKind::Wood)==1);
}

int main()
{
    preferenceGuards();
    localTradeChoice();
    return 0;
}
