#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <set>
#include <vector>
#include "lifelens/ResidentMind.h"
#include "lifelens/ResidentCivilizationOutcomeLearning.h"
#include "lifelens/ResidentSensedResourceAgency.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/WebClientBridge.h"
#include "lifelens/MemoryAttribution.h"

using namespace lifelens;

int main(int argc,char** argv)
{
    if(argc>1){
        const int days=std::atoi(argv[1]);
        assert(days>=1 &&days<=1000);
        const auto seed=argc>2?std::strtoull(argv[2],nullptr,10):874213954ULL;
        Simulation sim(seed);sim.setupNewGame();
        for(int elapsed=1;elapsed<=days*1440;++elapsed){
            sim.step();
            if(elapsed%1440 ||(elapsed/1440!=1 &&elapsed/1440!=100
                &&elapsed/1440!=365 &&elapsed/1440!=1000 &&elapsed/1440!=days)) continue;
            std::vector<std::uint8_t> bytes,again;
            assert(encodeSimulationSnapshot(sim.captureSnapshot(),bytes));
            SimulationStateSnapshot saved;assert(decodeSimulationSnapshot(bytes,saved));
            Simulation replay(seed);assert(replay.restoreSnapshot(saved));
            assert(encodeSimulationSnapshot(replay.captureSnapshot(),again) &&bytes==again);
            for(const auto& c:sim.world().characters){
                std::set<std::uint64_t> ids;
                std::size_t sensory=0;
                for(const auto& m:c.memory.entries){
                    ids.insert(memoryRecordFingerprint(m));
                    if(!m.tags.empty() &&m.tags[0]=="mind_perception_v1")++sensory;
                }
                const auto mind=observeResidentMind(c,sim.world().minute);
                assert(mind.summaries.size()<=24 &&mind.places.size()<=12 &&sensory<=48);
                std::size_t summarized=mind.omittedSummaries;
                for(const auto& group:mind.summaries){
                    summarized+=group.episodes;
                    assert(group.evidenceIds.size()+group.omittedEvidence==group.episodes);
                    for(const auto id:group.evidenceIds) assert(ids.count(id));
                }
                assert(summarized==mind.eligibleEpisodes);
                if(c.alive)assert(cognitiveRequestFingerprint(sim.observeCognitiveRequest(c.id,CognitiveTriggerKind::Reflection))
                    ==cognitiveRequestFingerprint(replay.observeCognitiveRequest(c.id,CognitiveTriggerKind::Reflection)));
                std::cout << "{\"seed\":\"" << seed << "\",\"day\":" << elapsed/1440
                    << ",\"actor\":\"" << c.id << "\",\"alive\":" << (c.alive?"true":"false")
                    << ",\"episodes\":" << c.memory.entries.size() << ",\"sensory\":" << sensory
                    << ",\"summaries\":" << mind.summaries.size() << ",\"eligible\":" << mind.eligibleEpisodes
                    << ",\"curiosity\":" << mind.curiosity << ",\"habit\":" << mind.habit
                    << ",\"attachment\":" << mind.attachment << ",\"snapshotBytes\":" << bytes.size()
                    << ",\"binaryRoundtrip\":true,\"evidenceResolved\":true}\n" << std::flush;
            }
        }
        return 0;
    }
    World world(77);
    world.minute=480;
    Character actor;actor.id=1;actor.civilization.character=1;
    actor.needs={0.05,0.05,0.05,0.05,0.05};
    actor.personality.curiosity=0.5;
    Character other=actor;other.id=2;other.civilization.character=2;
    world.characters={actor,other};
    world.generatedNaturalChunks.clear();world.resourceNodes.clear();world.facilities.clear();
    world.resourceNodes={{101,MaterialKind::Water,10,10,false,0,{4,0}},
                         {102,MaterialKind::Stone,10,10,false,0,{1,1}},
                         {103,MaterialKind::Wood,10,10,false,0,{20,0}}};
    ConstructedFacility wall;
    wall.id=201;wall.kind=FacilityKind::Shelter;wall.state=FacilityState::Operational;
    wall.active=true;wall.pos={2,0};
    world.facilities.push_back(wall);
    assert(!residentHasLineOfSight(world,{0,0},{4,0}));
    assert(residentHasLineOfSight(world,{0,0},{1,1}));
    const auto local=observeLocalResourceProximity(world,1,{0,0},6);
    assert(local.nearbyResources.size()==1 &&local.nearbyResources[0].nodeId==102);
    ConstructedFacility fire=wall;
    fire.id=202;fire.kind=FacilityKind::FirePit;fire.pos={4,0};fire.lit=true;
    world.facilities.push_back(fire);
    const auto beforeRng=world.rng;
    auto facts=perceiveResidentEnvironment(world,1,{0,0},{{2,{1,0}}});
    assert(facts.size()==3);
    const auto sound=std::find_if(facts.begin(),facts.end(),[](const auto& f){return f.channel==PerceptionChannel::Hearing;});
    assert(sound!=facts.end() &&!sound->hasPosition &&sound->subjectId==0);
    assert(sound->position.x==0 &&sound->position.y==0);
    for(const auto& fact:facts)assert(fact.subjectId!=101 &&fact.subjectId!=103);
    assert(world.rng==beforeRng &&world.minute==480);
    rememberResidentPerceptions(world.characters[0],{0,0},480,facts);
    auto first=observeResidentMind(world.characters[0],480);
    assert(first.eligibleEpisodes==3 &&first.places.size()==2);
    assert(first.curiosity==0.5 &&first.habit==0.0);
    for(const auto& summary:first.summaries)assert(summary.successes==0 &&summary.failures==0);
    assert(world.characters[1].memory.entries.empty());
    rememberResidentPerceptions(world.characters[0],{0,0},480,facts);
    assert(world.characters[0].memory.entries.size()==3);
    world.minute=495;
    facts=perceiveResidentEnvironment(world,1,{0,0},{{2,{1,0}}});
    rememberResidentPerceptions(world.characters[0],{0,0},495,facts);
    assert(observeResidentMind(world.characters[0],495).attachment>first.attachment);
    world.facilities[0].active=false;
    assert(residentHasLineOfSight(world,{0,0},{4,0}));
    assert(observeLocalResourceProximity(world,1,{0,0},6).nearbyResources.size()==2);
    assert(perceiveResidentEnvironment(world,1,{INT_MIN,INT_MAX}).empty());

    // Real Core resource execution produces the learning receipt. No model
    // narrative can reward curiosity, habit or risk tolerance.
    auto& learned=world.characters[0];
    CivilizationUtilityDecision gather;
    gather.intent=CivilizationIntent::Gather;gather.material=MaterialKind::Stone;
    gather.resourceNode=102;gather.quantity=1;gather.item=ItemKind::RawMaterial;
    const auto actual=executeCivilizationDecisionAtPosition(world,learned,gather,{1,1});
    assert(actual.executed &&actual.success);
    rememberCivilizationAttemptOutcome(learned,gather,world.minute,true);
    const auto after=observeResidentMind(learned,world.minute);
    assert(after.curiosity>first.curiosity &&after.habit>0.0 &&after.riskTolerance>first.riskTolerance);
    assert(residentOptionalCuriosityMultiplier(learned,world.minute)>1.0);
    MemoryRecord told=learned.memory.entries.back();
    told.source=MemorySource::ToldByOther;told.sourceCharacter=2;told.witnessed=false;
    Character hearsay=actor;hearsay.memory.add(told);
    assert(observeResidentMind(hearsay,world.minute).curiosity==actor.personality.curiosity);
    MemoryRecord invented=told;
    invented.source=MemorySource::DirectWitness;invented.sourceCharacter=1;invented.witnessed=true;
    invented.what="나는 성공했다고 생각한다";
    Character narrative=actor;narrative.memory.add(invented);
    assert(observeResidentMind(narrative,world.minute).habit==0.0);
    // Distinct origin is never promoted when text, target and date coincide.
    learned.memory.add(told);learned.memory.add(invented);
    const auto beforeReverse=observeResidentMind(learned,world.minute);
    std::reverse(learned.memory.entries.begin(),learned.memory.entries.end());
    const auto reversed=observeResidentMind(learned,world.minute);
    assert(beforeReverse.curiosity==reversed.curiosity &&beforeReverse.habit==reversed.habit);
    assert(beforeReverse.summaries.size()==reversed.summaries.size());
    for(std::size_t i=0;i<reversed.summaries.size();++i)
        assert(beforeReverse.summaries[i].evidenceIds==reversed.summaries[i].evidenceIds);

    // Production tick creates dated observations and keeps one executor.
    // Derived hierarchy survives the EXISTING binary codec without a second
    // saved summary table, private mind or schema migration.
    Simulation sim(874213954);sim.setupNewGame();sim.runMinutes(120);
    const auto id=sim.world().characters.front().id;
    const auto request=sim.observeCognitiveRequest(id,CognitiveTriggerKind::Reflection);
    assert(request.hasMindObservation &&!request.mindObservation.summaries.empty());
    auto forged=request;forged.mindObservation.curiosity=1.0-request.mindObservation.curiosity;
    assert(cognitiveRequestFingerprint(request)!=cognitiveRequestFingerprint(forged));
    CognitiveProposal proposal;proposal.actor=id;proposal.intent=CognitiveIntentKind::AcquireMaterials;proposal.priority=0.8;
    assert(!sim.acceptCognitiveProposal(forged,proposal,"test-local","forged-mind"));
    std::vector<std::uint8_t> bytes,again;
    assert(encodeSimulationSnapshot(sim.captureSnapshot(),bytes));
    SimulationStateSnapshot saved;
    assert(decodeSimulationSnapshot(bytes,saved));
    Simulation replay(874213954);assert(replay.restoreSnapshot(saved));
    assert(cognitiveRequestFingerprint(request)==cognitiveRequestFingerprint(
        replay.observeCognitiveRequest(id,CognitiveTriggerKind::Reflection)));
    assert(encodeSimulationSnapshot(replay.captureSnapshot(),again) &&bytes==again);
    sim.runMinutes(120);replay.runMinutes(120);
    assert(encodeSimulationSnapshot(sim.captureSnapshot(),bytes));
    assert(encodeSimulationSnapshot(replay.captureSnapshot(),again) &&bytes==again);
    std::cout << "M1 sensory provenance, personal hierarchy, adapted disposition, one Core receipt and binary continuation PASS\n";
}
