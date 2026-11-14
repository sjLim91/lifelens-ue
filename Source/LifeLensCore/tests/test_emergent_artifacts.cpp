#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include "lifelens/ArtifactInvention.h"
#include "lifelens/ArtifactObservation.h"
#include "lifelens/ArtifactSnapshotCodec.h"
#include "lifelens/Simulation.h"
#include "lifelens/ResidentAIExistingAffordance.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/ToolEffectiveness.h"
#include "fixtures/cognitive_causal_export.h"
using namespace lifelens;

static Character worker(CharacterId id=1) {
    Character c;c.id=id;c.alive=true;c.lifeStage=LifeStage::Adult;
    c.civilization.character=id;c.civilization.craftingSkill=0.9;
    c.personality.curiosity=0.95;c.personality.openness=0.9;
    c.needs={0.05,0.05,0.05,0.05,0.05};return c;
}
static void raw(Character& c,MaterialKind m,int count) {
    c.civilization.inventory.add({ItemKind::RawMaterial,m,count,0.6,1});
}
static std::vector<std::uint8_t> binary(const Simulation& s) {
    std::vector<std::uint8_t> b;std::string e;
    if(!encodeSimulationSnapshot(s.captureSnapshot(),b,&e))std::cerr<<e<<'\n';
    assert(!b.empty());return b;
}
static PhysicalArtifact artifact(const ArtifactDesign& d,CharacterId creator,std::uint64_t id=123) {
    PhysicalArtifact a;a.id=id;a.creator=creator;a.createdMinute=480;a.design=d;
    a.durability=measureArtifact(d).durability;return a;
}
int main() {
    const ArtifactDesign flint{{{MaterialKind::Flint,ArtifactShape::Edge,ArtifactJoint::Integral,1},
        {MaterialKind::Wood,ArtifactShape::Rod,ArtifactJoint::Bound,2},
        {MaterialKind::Fiber,ArtifactShape::Cord,ArtifactJoint::Bound,1}}};
    auto bone=flint;bone.parts.front().material=MaterialKind::Bone;
    assert(validArtifactDesign(flint));assert(artifactDesignId(flint)!=artifactDesignId(bone));
    assert(measureArtifact(flint).cutting>measureArtifact(bone).cutting);
    auto loose=flint;loose.parts.pop_back();assert(measureArtifact(loose).cutting<measureArtifact(flint).cutting);
    const ArtifactDesign bowl{{{MaterialKind::Clay,ArtifactShape::Bowl,ArtifactJoint::Integral,2}}};
    auto porous=bowl;porous.parts.front().material=MaterialKind::Charcoal;
    assert(measureArtifact(bowl).carrying>=1);assert(measureArtifact(porous).carrying==0);
    const ArtifactDesign plate{{{MaterialKind::Stone,ArtifactShape::Plate,ArtifactJoint::Integral,1}}};
    assert(measureArtifact(plate).heating>0.8);
    auto invalid=flint;invalid.parts[0].size=0;assert(!validArtifactDesign(invalid));
    invalid=flint;invalid.parts[0].material=MaterialKind::Water;assert(!validArtifactDesign(invalid));
    std::cout<<"PASS M8-01: typed material/geometry/joint/reach/container/thermal constraints\n";

    Character inventor=worker();raw(inventor,MaterialKind::Flint,40);raw(inventor,MaterialKind::Wood,40);
    raw(inventor,MaterialKind::Fiber,40);raw(inventor,MaterialKind::Clay,40);
    assert(proposeArtifact(inventor));auto urgent=inventor;urgent.needs.thirst=0.9;
    assert(!proposeArtifact(urgent));
    World w(42);w.minute=480;w.characters={inventor,worker(2)};w.characters[1].personality.curiosity=0;
    const auto before=w.characters[0].civilization.inventory.stacks();
    assert(!executeArtifactTrial(w,w.characters[0],1).attempted);
    assert(w.characters[0].civilization.inventory.stacks().size()==before.size());
    int successes=0,failures=0,paid=0;std::set<std::uint64_t> designs;
    for(int attempt=0;attempt<20;++attempt){
        auto& c=w.characters[0];const auto proposal=proposeArtifact(c);if(!proposal)break;
        const auto id=artifactDesignId(proposal->design);designs.insert(id);
        int beforeRaw=0;for(const auto& s:c.civilization.inventory.stacks())beforeRaw+=s.quantity;
        for(const auto& owned:c.civilization.inventory.artifacts)
            assert(artifactDesignId(owned.design)!=id); // A fresh identical tool is not an improvement.
        const auto r=executeArtifactTrial(w,c,id);assert(r.attempted);
        int afterRaw=0;for(const auto& s:c.civilization.inventory.stacks())afterRaw+=s.quantity;
        assert(beforeRaw-afterRaw==artifactRawCost(proposal->design));paid+=beforeRaw-afterRaw;
        successes+=r.success;failures+=!r.success;
        assert(c.civilization.knowledge.all().empty());
        assert(w.characters[1].civilization.invention.techniques.empty());
        w.minute+=60;
    }
    assert(successes>0&&failures>0&&paid>0&&designs.size()>1);
    bool injured=false;
    for(std::uint64_t seed=1;seed<=128&&!injured;++seed){
        World risk(seed);risk.minute=480;auto experimenter=worker(9);
        raw(experimenter,MaterialKind::Flint,2);
        risk.characters={experimenter};const auto hypothesis=proposeArtifact(risk.characters.front());assert(hypothesis);
        const auto trial=executeArtifactTrial(risk,risk.characters.front(),artifactDesignId(hypothesis->design));
        if(risk.characters.front().health.injurySeverity>0){
            assert(trial.attempted&&!trial.success);injured=true;
        }
    }
    assert(injured);

    assert(validArtifactWorld(w));
    std::cout<<"PASS M8-03/04: distinct structural trials, measured success/failure, raw loss, private learning\n";

    Character user=worker(3);user.civilization.inventory.artifacts.push_back(artifact(flint,user.id));
    auto& inv=user.civilization.inventory;
    const auto use=inspectGatherTool(inv,MaterialKind::Fiber);assert(use.artifactId==123);
    assert(!residentAIToolOutputNeeded(user,TechniqueId::ChippedStoneTool));
    const int boosted=gatheringQuantityWithTool(inv,MaterialKind::Fiber,3);assert(boosted>3);
    raw(user,MaterialKind::Fiber,12);const double pristine=inv.artifacts.front().durability;
    while(inv.artifacts.front().durability>0.02){
        GatherToolUseProfile used;assert(consumeGatherToolUse(inv,MaterialKind::Fiber,&used));
        rememberArtifactUse(user,used.artifactId,480,boosted,used.broken);
    }
    assert(!inspectGatherTool(inv,MaterialKind::Fiber).available);
    const int fiber=inv.count(ItemKind::RawMaterial,MaterialKind::Fiber);
    assert(repairArtifact(user,123,481));assert(inv.count(ItemKind::RawMaterial,MaterialKind::Fiber)==fiber-1);
    assert(inv.artifacts.front().durability>0&&inv.artifacts.front().durability<pristine);
    assert(inspectGatherTool(inv,MaterialKind::Fiber).available);
    assert(!repairArtifact(user,999,482));
    Character carrier=worker(4);carrier.civilization.inventory.artifacts.push_back(artifact(bowl,4));
    raw(carrier,MaterialKind::Water,1);assert(portableWaterCount(carrier.civilization.inventory)==1);
    assert(!residentAIToolOutputNeeded(carrier,TechniqueId::SimpleContainer));
    assert(consumePortableWater(carrier.civilization.inventory));assert(rawWaterUnitCount(carrier.civilization.inventory)==0);
    assert(simpleContainerCount(carrier.civilization.inventory)==1);
    carrier.civilization.inventory.artifacts.push_back(artifact(plate,4,124));
    raw(carrier,MaterialKind::Water,1);assert(!heatWaterWithArtifact(carrier,124,480));
    carrier.civilization.knowledge.learn(TechniqueId::FireMaking,KnowledgeLevel::Reproducible,1);
    assert(!heatWaterWithArtifact(carrier,124,480));raw(carrier,MaterialKind::Wood,1);
    assert(heatWaterWithArtifact(carrier,124,480));
    assert(carrier.civilization.inventory.count(ItemKind::RawMaterial,MaterialKind::Wood)==0);
    assert(rawWaterUnitCount(carrier.civilization.inventory)==1);
    assert(carrier.civilization.inventory.artifacts.back().uses==1);
    std::cout<<"PASS M8-05: physical gather boost/wear/break/repair and fuel-bounded heating, vessel reuse\n";

    auto teacher=worker(5),learner=worker(6),rejecter=worker(7);
    teacher.civilization.inventory.artifacts.push_back(artifact(flint,teacher.id));
    teacher.civilization.inventory.artifacts.front().uses=3;
    auto* t=rememberArtifactDesign(teacher,flint,480,5,5,ArtifactKnowledgeSource::Experiment);
    t->trials=1;t->successes=1;t->uses=3;t->confidence=0.7;
    assert(!demonstrateArtifact(teacher,rejecter,480,0.1));assert(rejecter.civilization.invention.techniques.empty());
    assert(demonstrateArtifact(teacher,learner,480,0.9));
    assert(!learner.civilization.invention.techniques.front().reproducible());
    assert(!demonstrateArtifact(teacher,learner,480,0.9));
    assert(demonstrateArtifact(teacher,learner,540,0.9));
    assert(learner.civilization.invention.techniques.front().reproducible());
    assert(!recordArtifactTechnique(teacher,artifactDesignId(flint),540));
    teacher.civilization.inventory.add({ItemKind::RecordTablet,MaterialKind::Clay,1,0.6,1});
    assert(recordArtifactTechnique(teacher,artifactDesignId(flint),540));
    assert(!transferArtifactRecord(teacher,rejecter,artifactDesignId(flint),{0,0},{10,0}));
    assert(transferArtifactRecord(teacher,rejecter,artifactDesignId(flint),{0,0},{1,0}));
    assert(teacher.civilization.inventory.artifactRecords.empty());
    assert(readArtifactRecord(rejecter,600));assert(!readArtifactRecord(rejecter,600));
    assert(readArtifactRecord(rejecter,660));assert(rejecter.civilization.invention.techniques.front().reproducible());
    std::cout<<"PASS M8-06: personal two-meeting learning, rejection, paid durable record and local handover\n";

    World history(7);history.minute=700;history.characters={teacher,learner,rejecter};
    auto cultures=observeArtifactCultures(history);assert(cultures.size()==1&&cultures[0].livingKnowers==3);
    for(auto& c:history.characters)c.alive=false;
    cultures=observeArtifactCultures(history);assert(cultures[0].livingKnowers==0&&cultures[0].historicalKnowers==3);
    assert(artifactDesignLost(history,artifactDesignId(flint)));
    history.characters[1].alive=true;
    assert(!artifactDesignLost(history,artifactDesignId(flint)));
    std::cout<<"PASS M8-07: culture follows retained personal knowledge/actual use, death causes loss\n";
    std::set<std::uint64_t> paths;int failedTrials=0,rediscoveries=0;
    for(std::uint64_t seed=1;seed<=8;++seed){
        World branch(seed);branch.minute=800;
        auto fresh=worker(11);raw(fresh,MaterialKind::Flint,20);raw(fresh,MaterialKind::Wood,20);
        raw(fresh,MaterialKind::Fiber,20);
        // All branches start with identical raw inventory and no personal
        // method. Only deterministic experimental outcome varies with seed.
        const auto hypothesis=proposeArtifact(fresh);assert(hypothesis);
        auto former=worker(12);former.alive=false;
        auto* lost=rememberArtifactDesign(former,hypothesis->design,480,12,12,ArtifactKnowledgeSource::Experiment);
        lost->trials=1;lost->successes=1;
        branch.characters={fresh,former};
        for(int attempt=0;attempt<6;++attempt){
            const auto next=proposeArtifact(branch.characters.front());if(!next)break;
            const auto id=artifactDesignId(next->design);paths.insert(id);
            const bool previouslyLost=artifactDesignLost(branch,id);
            const auto r=executeArtifactTrial(branch,branch.characters.front(),id);
            assert(r.attempted);
            if(r.success){
                assert(branch.characters.front().civilization.invention.ledger.back().kind
                    ==(previouslyLost?ArtifactEventKind::Rediscovered:ArtifactEventKind::Created));
                rediscoveries+=previouslyLost;
            }else ++failedTrials;
            branch.minute+=60;
        }
    }
    assert(rediscoveries>0&&failedTrials>0&&paths.size()>1);
    std::cout<<"PASS M8-08: eight same-inventory seeds diverge on actual loss/retry/re-discovery, no era timer\n";


    Simulation sim(874213954);sim.setupNewGame();auto& c=sim.world().characters.front();
    c.civilization.inventory.artifacts.push_back(artifact(flint,c.id));
    c.civilization.inventory.artifactRecords.push_back({flint,c.id,480});
    auto* known=rememberArtifactDesign(c,flint,480,c.id,c.id,ArtifactKnowledgeSource::Experiment);
    known->trials=1;known->successes=1;
    const auto bytes=binary(sim);SimulationStateSnapshot decoded;std::string error;
    assert(decodeSimulationSnapshot(bytes,decoded,&error));Simulation replay(1);
    assert(replay.restoreSnapshot(decoded,&error));assert(binary(replay)==bytes);
    const auto beforeObservation=binary(replay);
    WebClientBridge bridge;assert(bridge.restoreSnapshotBase64(causalFixtureBase64(bytes)));
    const auto bridgeBefore=bridge.saveSnapshotBase64();
    assert(bridge.residentsJson().find("\"designId\":\""+std::to_string(artifactDesignId(flint))+"\"")!=std::string::npos);
    assert(bridge.civilizationWorldJson().find("inventionCultures")!=std::string::npos);
    assert(bridge.saveSnapshotBase64()==bridgeBefore);
    assert(binary(replay)==beforeObservation);
    auto forged=decoded;forged.world.characters.front().civilization.inventory.artifacts.front().durability=2;
    assert(!replay.restoreSnapshot(forged,&error));std::vector<std::uint8_t> out;
    assert(!encodeSimulationSnapshot(forged,out,&error));assert(binary(replay)==beforeObservation);
    auto duplicate=decoded;
    duplicate.world.characters.back().civilization.inventory.artifacts.push_back(duplicate.world.characters.front().civilization.inventory.artifacts.front());
    assert(!replay.restoreSnapshot(duplicate,&error));
    auto truncated=bytes;truncated.pop_back();assert(!decodeSimulationSnapshot(truncated,forged,&error));
    auto length=bytes;length[length.size()-12]=255;length[length.size()-11]=255;
    assert(!decodeSimulationSnapshot(length,forged,&error));
    Simulation legacy(7);legacy.setupNewGame();const auto old=binary(legacy);
    assert(decodeSimulationSnapshot(old,decoded,&error));assert(!hasArtifactState(decoded.world));
    assert(replay.restoreSnapshot(decoded,&error));assert(binary(replay)==old);
    for(auto technique:{TechniqueId::SharpFlake,TechniqueId::DiggingStick,TechniqueId::SimpleContainer}){
        Character classic=worker();classic.civilization.knowledge.learn(technique,KnowledgeLevel::Reproducible,1);
        const auto recipe=techniqueRecipe(technique);
        for(const auto& input:recipe.inputs)classic.civilization.inventory.add({input.kind,input.material,input.quantity,0.6,1});
        const auto adapter=inspectTechniqueCapability(technique,classic.civilization.inventory,classic.civilization.knowledge);
        assert(adapter.inputsAvailable&&adapter.knowledgeQualified);
        assert(reproduceTechnique(classic.id,technique,classic.civilization.inventory,classic.civilization.knowledge).success);
    }
    auto cognitive=worker(77);
    for(auto material:{MaterialKind::Flint,MaterialKind::Wood,MaterialKind::Fiber,MaterialKind::Clay})raw(cognitive,material,12);
    for(const auto& technology:TechnologyRegistry){
        if(technology.legacyTechnique==TechniqueId::None)continue;
        cognitive.civilization.knowledge.learn(technology.legacyTechnique,KnowledgeLevel::Reproducible,1);
        const auto recipe=techniqueRecipe(technology.legacyTechnique);
        if(recipe.producesItem)cognitive.civilization.inventory.add({recipe.outputKind,recipe.outputMaterial,12,0.5,1});
    }
    AcceptedCognitiveDecision thought;thought.actor=cognitive.id;thought.minute=w.minute;
    thought.intent=CognitiveIntentKind::CraftUsefulTools;thought.priority=0.9;thought.contextFingerprint=991;
    const auto selected=chooseResidentAIExistingCivilizationAffordance(w,cognitive,{0,0},thought);
    assert(selected&&selected->intent==CivilizationIntent::Experiment
        &&selected->experiment==ExperimentKind::ComposeMaterials);
    std::cout<<"PASS same resident cognition can select a novel measured tool trial without a parallel planner\n";
    std::cout<<"PASS M8-02/08: legacy craft adapter, exact unique artifact/knowledge save, old v8 byte equality, hostile tail rejection\n";
}
