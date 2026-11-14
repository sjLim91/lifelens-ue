#pragma once

#include <optional>
#include "World.h"

namespace lifelens {
inline bool artifactSafeToWork(const Character& c) {
    return c.alive &&c.id &&lifeStageProfile(c.lifeStage).canWork
        &&std::max({c.needs.hunger,c.needs.thirst,c.needs.sleep,c.needs.bladder,
            c.needs.hygiene,c.health.injurySeverity})<0.48;
}
inline bool artifactSafeToLearn(const Character& c) {
    const auto stage=lifeStageProfile(c.lifeStage);
    return c.alive&&c.id&&(stage.canAttendSchool||stage.canWork)
        &&std::max({c.needs.hunger,c.needs.thirst,c.needs.sleep,c.needs.bladder,
            c.needs.hygiene,c.health.injurySeverity})<0.48;
}
inline int artifactRawCost(const ArtifactDesign& d) {
    int n=0;for(const auto& p:d.parts)n+=p.size;return n;
}
inline bool artifactInputsAvailable(const Inventory& inv,const ArtifactDesign& d) {
    if(!validArtifactDesign(d))return false;
    std::array<int,18> required{};
    for(const auto& p:d.parts)required[static_cast<unsigned>(p.material)]+=p.size;
    for(unsigned i=1;i<required.size();++i)
        if(inv.count(ItemKind::RawMaterial,static_cast<MaterialKind>(i))<required[i])return false;
    return true;
}
inline double artifactOwnedFunction(const Inventory& inv,ArtifactFunction f) {
    double best=0;
    for(const auto& a:inv.artifacts)if(a.durability>0.02)
        best=std::max(best,artifactFunctionValue(measureArtifact(a.design),f)
            *((f==ArtifactFunction::Cutting||f==ArtifactFunction::Digging)?a.durability:1.0));
    for(const auto& stack:inv.stacks()){
        if(stack.quantity<=0 ||stack.durability<=0.02)continue;
        const auto capability=itemCapability(stack.kind);
        const bool matches=(f==ArtifactFunction::Cutting &&(capability==ToolCapability::Cut||capability==ToolCapability::Chop))
            ||(f==ArtifactFunction::Digging&&capability==ToolCapability::Dig)
            ||(f==ArtifactFunction::Carrying&&capability==ToolCapability::Carry);
        if(matches)best=std::max(best,(0.4+0.5*stack.quality)*stack.durability);
    }
    return best;
}
struct ArtifactHypothesis { ArtifactDesign design; double score=0; };
inline std::optional<ArtifactHypothesis> proposeArtifact(const Character& c) {
    if(!artifactSafeToWork(c) ||c.civilization.inventory.artifacts.size()>=32)return {};
    const auto& inv=c.civilization.inventory;
    const auto& learning=c.civilization.invention;
    std::optional<ArtifactHypothesis> best;
    const auto consider=[&](ArtifactDesign d){
        if(!artifactInputsAvailable(inv,d))return;
        const auto p=measureArtifact(d);
        double utility=0;
        for(int i=0;i<4;++i){
            const auto f=static_cast<ArtifactFunction>(i);
            if(f==ArtifactFunction::Heating
               &&!c.civilization.knowledge.knowsAtLeast(TechniqueId::FireMaking,KnowledgeLevel::Reproducible))continue;
            const double value=artifactFunctionValue(p,f)
                *((f==ArtifactFunction::Cutting||f==ArtifactFunction::Digging)?p.durability:1.0);
            const double baseline=artifactOwnedFunction(inv,f);
            utility=std::max(utility,value-baseline-0.12);
        }
        if(utility<=0)return; // Familiarity cannot make an identical fresh tool an improvement.
        utility-=0.025*artifactRawCost(d);
        const auto id=artifactDesignId(d);
        if(learning.techniques.size()>=64 &&std::none_of(learning.techniques.begin(),learning.techniques.end(),
            [&](const ArtifactTechnique& t){return artifactDesignId(t.design)==id;}))return;
        for(const auto& t:learning.techniques)if(artifactDesignId(t.design)==id){
            utility+=t.reproducible()?0.12:0;
            utility-=0.09*(t.trials-t.successes);
        }
        if(utility>0 &&(!best ||utility>best->score+1e-12
            ||(std::abs(utility-best->score)<1e-12 &&id<artifactDesignId(best->design))))
            best=ArtifactHypothesis{std::move(d),utility};
    };
    // Known designs are compressed records of actual previous experiments/demos.
    for(const auto& t:learning.techniques)if(t.reproducible())consider(t.design);
    if(c.personality.curiosity<0.55)return best;
    for(int m=1;m<=static_cast<int>(MaterialKind::Bronze);++m){
        const auto material=static_cast<MaterialKind>(m);
        if(inv.count(ItemKind::RawMaterial,material)<1)continue;
        for(const auto shape:{ArtifactShape::Edge,ArtifactShape::Point,ArtifactShape::Bowl,ArtifactShape::Plate}){
            for(int size=1;size<=3;++size){
                ArtifactDesign single{{{material,shape,ArtifactJoint::Integral,size}}};
                consider(single);
                if(shape!=ArtifactShape::Edge &&shape!=ArtifactShape::Point)continue;
                // Enumerate actual material alternatives, not named tool recipes.
                for(const auto& grip:inv.stacks()){
                    if(grip.kind!=ItemKind::RawMaterial ||grip.quantity<1
                       ||grip.material==material)continue;
                    const auto gp=materialProperties(grip.material);
                    if(gp.hardness<0.2 ||gp.flexibility<0.2)continue;
                    auto joined=single;
                    joined.parts.push_back({grip.material,ArtifactShape::Rod,ArtifactJoint::Inserted,1});
                    consider(joined);
                    for(const auto binder:{MaterialKind::Fiber,MaterialKind::Hide}){
                        if(inv.count(ItemKind::RawMaterial,binder)<1)continue;
                        joined.parts[1].joint=ArtifactJoint::Bound;
                        auto bound=joined;
                        bound.parts.push_back({binder,ArtifactShape::Cord,ArtifactJoint::Bound,1});
                        consider(bound);
                    }
                }
            }
        }
    }
    return best;
}
inline ArtifactTechnique* rememberArtifactDesign(Character& c,const ArtifactDesign& d,
    int minute,CharacterId origin,CharacterId source,ArtifactKnowledgeSource provenance) {
    auto& l=c.civilization.invention;
    if(auto* known=l.find(artifactDesignId(d)))return known;
    if(l.techniques.size()>=64)return nullptr;
    ArtifactTechnique t;t.design=d;t.origin=origin;t.source=source;
    t.learnedMinute=minute;t.provenance=provenance;
    l.techniques.push_back(t);return &l.techniques.back();
}
inline bool artifactDesignLost(const World& world,std::uint64_t id) {
    bool history=false;
    for(const auto& c:world.characters)for(const auto& t:c.civilization.invention.techniques)
        if(artifactDesignId(t.design)==id &&t.reproducible()){
            if(c.alive)return false;history=true;
        }
    return history;
}
inline ExperimentResult executeArtifactTrial(World& world,Character& c,std::uint64_t expected) {
    ExperimentResult r;r.event.actor=c.id;r.event.technique=TechniqueId::None;
    const auto proposal=proposeArtifact(c);
    if(!proposal ||artifactDesignId(proposal->design)!=expected)return r;
    auto& learning=c.civilization.invention;
    if(!learning.ledger.empty() &&world.minute<learning.ledger.back().minute+30)return r;
    if(learning.sequence==UINT64_MAX ||(learning.techniques.size()>=64&&!learning.find(expected)))return r;
    const auto design=proposal->design;const auto perf=measureArtifact(design);
    Inventory paid=c.civilization.inventory;
    for(const auto& p:design.parts)
        if(!paid.remove(ItemKind::RawMaterial,p.material,p.size))return r;
    const bool rediscovered=artifactDesignLost(world,expected);
    auto* technique=rememberArtifactDesign(c,design,world.minute,c.id,c.id,ArtifactKnowledgeSource::Experiment);
    if(!technique)return r;
    c.civilization.inventory=std::move(paid);
    ++learning.sequence;++technique->trials;
    const auto hash=artifactMix(world.seed^artifactMix(c.id)^expected^artifactMix(learning.sequence));
    r.roll=static_cast<double>(hash>>11)*(1.0/9007199254740992.0);
    r.successChance=std::clamp(0.2+0.35*c.civilization.craftingSkill
        +0.3*perf.durability+0.15*technique->confidence,0.0,0.95);
    r.attempted=true;r.success=r.roll<r.successChance;
    r.event.material=design.parts.front().material;
    ArtifactEvent event;event.design=expected;event.minute=world.minute;
    event.rawUnits=artifactRawCost(design);event.work=perf.work;
    if(r.success){
        PhysicalArtifact a;a.id=artifactMix(hash^learning.sequence);if(!a.id)a.id=1;
        a.creator=c.id;a.createdMinute=world.minute;a.design=design;a.durability=perf.durability;
        c.civilization.inventory.artifacts.push_back(a);
        ++technique->successes;technique->confidence=std::min(1.0,technique->confidence+0.25);
        event.artifact=a.id;event.kind=rediscovered?ArtifactEventKind::Rediscovered:ArtifactEventKind::Created;
        event.effect=std::max({perf.cutting,perf.digging,perf.carrying,perf.heating});
        r.event.type=CivilizationEventType::Crafted;r.event.quantity=1;
    }else{
        technique->confidence=std::max(0.0,technique->confidence-0.08);
        event.kind=ArtifactEventKind::Failed;r.event.type=CivilizationEventType::ExperimentFailed;
        // Brittle sharp trials can actually hurt the worker; no invented injury event.
        if(r.roll>0.94 &&perf.cutting>0.4)
            c.health.injurySeverity=std::min(1.0,c.health.injurySeverity+0.035);
    }
    learning.record(event);
    c.needs.sleep=std::min(1.0,c.needs.sleep+perf.work*0.0002);
    c.civilization.craftingSkill=std::min(1.0,c.civilization.craftingSkill+(r.success?0.003:0.001));
    return r;
}
inline bool repairArtifact(Character& c,std::uint64_t id,int minute) {
    if(!artifactSafeToWork(c))return false;
    auto& inv=c.civilization.inventory;
    for(auto& a:inv.artifacts){
        if(a.id!=id ||a.durability>0.5 ||a.repairs>=6)continue;
        const auto p=measureArtifact(a.design);if(a.durability>=p.durability)return false;
        const MaterialKind input=a.design.parts.back().material;
        if(!inv.remove(ItemKind::RawMaterial,input,1))return false;
        a.durability=std::min(p.durability,a.durability+0.25);++a.repairs;
        c.civilization.invention.record({ArtifactEventKind::Repaired,a.id,artifactDesignId(a.design),0,minute,1,8,a.durability});
        c.needs.sleep=std::min(1.0,c.needs.sleep+0.002);return true;
    }return false;
}
inline bool recordArtifactTechnique(Character& c,std::uint64_t designId,int minute) {
    auto* t=c.civilization.invention.find(designId);
    auto& inv=c.civilization.inventory;
    if(!t ||!t->reproducible() ||inv.artifactRecords.size()>=32)return false;
    for(const auto& record:inv.artifactRecords)if(artifactDesignId(record.design)==designId)return false;
    if(!inv.remove(ItemKind::RecordTablet,MaterialKind::Unknown,1,true))return false;
    inv.artifactRecords.push_back({t->design,c.id,minute});
    c.civilization.invention.record({ArtifactEventKind::Recorded,0,designId,0,minute,0,6,1});return true;
}
// Called only after physical tool use; the design never implies successful use.
inline void rememberArtifactUse(Character& c,std::uint64_t id,int minute,double effect,bool broken,int rawUnits=0,double work=1) {
    for(auto& a:c.civilization.inventory.artifacts)if(a.id==id){
        if(auto* t=c.civilization.invention.find(artifactDesignId(a.design)))++t->uses;
        c.civilization.invention.record({broken?ArtifactEventKind::Broken:ArtifactEventKind::Used,
            id,artifactDesignId(a.design),0,minute,rawUnits,work,effect});return;
    }
}
// A demonstration transfers only an actually held, used design. Two successful
// paid meetings are needed to reproduce it; personal rejection remains possible.
inline bool demonstrateArtifact(Character& teacher,Character& learner,int minute,double trust,std::uint64_t expected=0) {
    if(!artifactSafeToWork(teacher) ||!artifactSafeToLearn(learner) ||teacher.id==learner.id)return false;
    for(const auto& a:teacher.civilization.inventory.artifacts){
        if(a.uses<1 ||a.durability<=0.02)continue;
        const auto id=artifactDesignId(a.design);
        if(expected &&expected!=a.id)continue;
        auto* known=learner.civilization.invention.find(id);
        if(known &&(known->reproducible() ||minute<known->learnedMinute+60))continue;
        if(trust<0.35 ||learner.personality.openness<0.2){
            learner.civilization.invention.record({ArtifactEventKind::Rejected,a.id,id,teacher.id,minute,0,0,trust});
            return false;
        }
        auto* t=rememberArtifactDesign(learner,a.design,minute,a.creator,teacher.id,ArtifactKnowledgeSource::Demonstration);
        if(!t)return false;
        ++t->demonstrations;t->learnedMinute=minute;t->confidence=std::min(1.0,t->confidence+0.2);
        learner.civilization.invention.record({ArtifactEventKind::Taught,a.id,id,teacher.id,minute,0,4,t->confidence});
        teacher.needs.sleep=std::min(1.0,teacher.needs.sleep+0.002);
        learner.needs.sleep=std::min(1.0,learner.needs.sleep+0.002);return true;
    }return false;
}
inline bool readArtifactRecord(Character& c,int minute,std::uint64_t expected=0) {
    if(!artifactSafeToWork(c))return false;
    for(const auto& record:c.civilization.inventory.artifactRecords){
        const auto id=artifactDesignId(record.design);
        if(expected&&expected!=id)continue;
        auto* t=c.civilization.invention.find(id);
        if(t &&(t->reproducible() ||minute<t->learnedMinute+60))continue;
        t=rememberArtifactDesign(c,record.design,minute,record.author,record.author,ArtifactKnowledgeSource::Record);
        if(!t)return false;
        ++t->demonstrations;t->learnedMinute=minute;t->confidence=std::min(1.0,t->confidence+0.2);
        c.civilization.invention.record({ArtifactEventKind::Taught,0,id,record.author,minute,0,4,t->confidence});
        c.needs.sleep=std::min(1.0,c.needs.sleep+0.002);return true;
    }return false;
}
inline bool heatWaterWithArtifact(Character& c,std::uint64_t id,int minute) {
    if(!artifactSafeToWork(c) ||!c.civilization.knowledge.knowsAtLeast(
        TechniqueId::FireMaking,KnowledgeLevel::Reproducible))return false;
    auto& inv=c.civilization.inventory;
    if(portableWaterCount(inv)<1 ||inv.count(ItemKind::RawMaterial,MaterialKind::Wood)<1)return false;
    for(auto& a:inv.artifacts){
        const auto p=measureArtifact(a.design);
        if(a.id!=id ||a.durability<=0.02 ||p.heating<0.5)continue;
        for(const auto& water:inv.stacks()){
            if(water.kind!=ItemKind::RawMaterial ||water.material!=MaterialKind::Water ||water.quality>=0.95)continue;
            ItemStack heated=water;heated.quantity=1;
            if(!inv.removeExactOne(heated))return false;
            inv.remove(ItemKind::RawMaterial,MaterialKind::Wood,1);
            // One actual fuel unit can heat one carried unit. No thermal gain
            // is generated by the plate itself and no water is created.
            heated.quality=std::min(1.0,heated.quality+0.5*p.heating);inv.add(heated);
            a.durability=std::max(0.0,a.durability-0.025);++a.uses;
            rememberArtifactUse(c,a.id,minute,0.5*p.heating,a.durability<=0.02,1,7);
            return true;
        }
    }return false;
}
// Records/tools retain their identity during real nearby handover. The caller
// must supply actual positions; this is not an inventory copy operation.
inline bool transferArtifactRecord(Character& from,Character& to,std::uint64_t id,
    GridPos fromPos,GridPos toPos) {
    if(!from.alive ||!to.alive ||from.id==to.id
       ||std::abs(fromPos.x-toPos.x)+std::abs(fromPos.y-toPos.y)>2)return false;
    auto& src=from.civilization.inventory.artifactRecords;
    auto& dst=to.civilization.inventory.artifactRecords;
    if(dst.size()>=32)return false;
    for(const auto& r:dst)if(artifactDesignId(r.design)==id)return false;
    for(auto it=src.begin();it!=src.end();++it)if(artifactDesignId(it->design)==id){
        dst.push_back(*it);src.erase(it);return true;
    }return false;
}
}
