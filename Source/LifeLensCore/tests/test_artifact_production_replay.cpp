#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "lifelens/ArtifactInvention.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "fixtures/cognitive_causal_export.h"
using namespace lifelens;
static std::vector<std::uint8_t> encode(const Simulation& s){
    std::vector<std::uint8_t> b;std::string error;
    if(!encodeSimulationSnapshot(s.captureSnapshot(),b,&error))std::cerr<<error<<'\n';
    assert(!b.empty());return b;
}
int main(){
    SimulationRuleset rules=DefaultSimulationRuleset;
    rules.needs.hungerPerMinute=rules.needs.thirstPerMinute=rules.needs.sleepPerMinute=0;
    rules.needs.bladderPerMinute=rules.needs.hygienePerMinute=0;
    Simulation original(874213954,0,CurrentWorldGenerationVersion,rules);original.setupNewGame();
    for(auto& c:original.world().characters)c.needs={0.03,0.03,0.03,0.03,0.03};
    auto& actor=original.world().characters.front();const auto actorId=actor.id;
    actor.personality.curiosity=0.95;actor.civilization.craftingSkill=0.9;
    for(auto m:{MaterialKind::Stone,MaterialKind::Flint,MaterialKind::Wood,MaterialKind::Fiber,MaterialKind::Clay})
        actor.civilization.inventory.add({ItemKind::RawMaterial,m,12,0.6,1});
    const auto design=proposeArtifact(actor);assert(design);
    auto start=original.captureSnapshot();auto& runtime=start.runtime.at(actorId);
    runtime.plan.clear();runtime.actionIndex=0;runtime.goal=Goal::Idle;
    PendingContextAction pending;pending.token=start.nextContextActionToken++;
    pending.kind=ContextActionKind::Civilization;pending.issuedMinute=start.world.minute;
    pending.civilization.intent=CivilizationIntent::Experiment;
    pending.civilization.experiment=ExperimentKind::ComposeMaterials;
    pending.civilization.resourceNode=artifactDesignId(design->design);
    pending.civilization.material=design->design.parts.front().material;
    pending.civilization.quantity=static_cast<int>(std::ceil(measureArtifact(design->design).work));
    runtime.pendingContext=pending;
    std::string error;assert(original.restoreSnapshot(start,&error));
    const auto first=encode(original);SimulationStateSnapshot decoded;
    assert(decodeSimulationSnapshot(first,decoded,&error));
    Simulation replay(1,0,CurrentWorldGenerationVersion,rules);assert(replay.restoreSnapshot(decoded,&error));
    const int duration=contextActionDurationTicks(pending);
    for(int tick=0;tick<duration-1;++tick){original.step();replay.step();
        assert(original.world().characters.front().civilization.invention.ledger.empty());}
    bool attempted=false;
    for(int tick=0;tick<10;++tick){original.step();replay.step();
        if(!original.world().characters.front().civilization.invention.ledger.empty()){attempted=true;break;}}
    assert(attempted);assert(encode(original)==encode(replay));
    const auto& result=original.world().characters.front();
    assert(result.civilization.invention.techniques.size()==1);
    const auto event=result.civilization.invention.ledger.front();
    assert(event.design==artifactDesignId(design->design));assert(event.minute>=480+duration);
    assert(event.rawUnits==artifactRawCost(design->design));
    bool memory=false;
    for(const auto& m:result.memory.entries)for(const auto& tag:m.tags)
        if(tag.find("Civilization:Experiment:Artifact:")==0)memory=true;
    assert(memory);
    if(const char* directory=std::getenv("LIFELENS_ARTIFACT_FIXTURE_DIR")){
        std::filesystem::create_directories(directory);
        const auto put=[&](const char* name,const std::vector<std::uint8_t>& bytes){
            std::ofstream f(std::filesystem::path(directory)/name,std::ios::binary);
            f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());assert(f.good());};
        put("start.bin",first);put("result.bin",encode(original));
        std::ofstream ticks(std::filesystem::path(directory)/"minutes.txt");ticks<<original.world().minute-480;
        WebClientBridge bridge;assert(bridge.restoreSnapshotBase64(causalFixtureBase64(encode(original))));
        std::ofstream residents(std::filesystem::path(directory)/"residents.json");residents<<bridge.residentsJson();
    }
    // The completed physical vessel must actually improve a real gather
    // before its creator can demonstrate its use. All commands still pass
    // through ordinary production ticks, elapsed time and exact tokens.
    auto gather=original.captureSnapshot();auto& gr=gather.runtime.at(actorId);
    ResourceNode source;source.id=9009001;source.material=MaterialKind::PlantFood;
    source.quantity=20;source.maxQuantity=20;source.pos=gr.pos;
    gather.world.resourceNodes.push_back(source);
    gr.plan.clear();gr.actionIndex=0;gr.pendingContext.clear();
    gr.pendingContext.token=gather.nextContextActionToken++;
    gr.pendingContext.kind=ContextActionKind::Civilization;gr.pendingContext.issuedMinute=gather.world.minute;
    gr.pendingContext.civilization.intent=CivilizationIntent::Gather;
    gr.pendingContext.civilization.material=MaterialKind::PlantFood;
    gr.pendingContext.civilization.resourceNode=source.id;gr.pendingContext.civilization.quantity=2;
    assert(original.restoreSnapshot(gather,&error));
    original.step();
    assert(original.world().characters.front().civilization.inventory.artifacts.front().uses>0);
    const auto learnerId=original.world().characters[1].id;
    for(int meeting=0;meeting<2;++meeting){
        auto lesson=original.captureSnapshot();
        if(meeting)lesson.world.minute+=60; // controlled spacing, not offline catch-up
        auto& tr=lesson.runtime.at(actorId);auto& lr=lesson.runtime.at(learnerId);
        lr.pos=tr.pos;lr.plan={{ActionType::Idle,0,20}};lr.actionIndex=0;lr.pendingContext.clear();
        lesson.world.characters[1].personality.openness=0.9;
        lesson.world.characters[1].needs={0.03,0.03,0.03,0.03,0.03};
        auto& relation=lesson.relationships.getOrCreate(learnerId,actorId);
        relation.trust=relation.respect=relation.familiarity=relation.comfort=0.9;
        tr.plan.clear();tr.actionIndex=0;tr.pendingContext.clear();
        tr.pendingContext.token=lesson.nextContextActionToken++;
        tr.pendingContext.kind=ContextActionKind::KnowledgeTeaching;
        tr.pendingContext.issuedMinute=lesson.world.minute;
        tr.pendingContext.knowledgeTeachingTarget=learnerId;
        tr.pendingContext.civilization.experiment=ExperimentKind::ComposeMaterials;
        tr.pendingContext.civilization.resourceNode=lesson.world.characters.front().civilization.inventory.artifacts.front().id;
        tr.pendingContext.hasSpatialTarget=true;tr.pendingContext.targetPos=lr.pos;
        const auto lessonMinute=lesson.world.minute;
        assert(original.restoreSnapshot(lesson,&error));
        const auto lessonBinary=encode(original);
        assert(decodeSimulationSnapshot(lessonBinary,decoded,&error));
        assert(replay.restoreSnapshot(decoded,&error));
        for(int tick=0;tick<7;++tick){original.step();replay.step();
            if(tick<6)assert(original.world().characters[1].civilization.invention.techniques.size()==static_cast<std::size_t>(meeting));}
        assert(encode(original)==encode(replay));
        const auto& learned=original.world().characters[1].civilization.invention.techniques;
        assert(learned.size()==1&&learned.front().demonstrations==meeting+1);
        assert(learned.front().learnedMinute>=lessonMinute+6);
        assert(learned.front().reproducible()==(meeting==1));
    }
    assert(hasSocietyApprenticeship(original.captureSnapshot().socialKnowledge,actorId,learnerId));
    std::cout<<"PASS actual production gather use and two timed saved demonstrations -> apprenticeship\n";
    std::cout<<"PASS controlled production token="<<pending.token<<" duration="<<duration
        <<" actual="<<event.minute-480<<" paid="<<event.rawUnits<<" success="
        <<(event.kind==ArtifactEventKind::Created)<<" binary-replay=equal firsthand-memory=true\n";
}
