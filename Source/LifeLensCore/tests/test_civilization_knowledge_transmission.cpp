#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/CivilizationKnowledgeTransmission.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/SocialKnowledgeSnapshotCodec.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static bool patchLittleEndianU32(
    std::vector<std::uint8_t>& bytes,
    std::size_t offset,
    std::uint32_t value)
{
    if(bytes.size()<offset+4) return false;
    bytes[offset]=static_cast<std::uint8_t>(value&0xffu);
    bytes[offset+1]=static_cast<std::uint8_t>((value>>8)&0xffu);
    bytes[offset+2]=static_cast<std::uint8_t>((value>>16)&0xffu);
    bytes[offset+3]=static_cast<std::uint8_t>((value>>24)&0xffu);
    return true;
}

static bool sameSocialKnowledge(
    const SocialKnowledgeBook& a,
    const SocialKnowledgeBook& b)
{
    if(a.facts().size()!=b.facts().size() || a.receipts().size()!=b.receipts().size()) return false;
    for(std::size_t i=0;i<a.facts().size();++i){
        const SocialFact& x=a.facts()[i];
        const SocialFact& y=b.facts()[i];
        if(x.id!=y.id || x.subject!=y.subject || x.proposition!=y.proposition ||
           x.where!=y.where || x.eventMinute!=y.eventMinute || x.supports!=y.supports ||
           x.importance!=y.importance || x.confidence!=y.confidence ||
           x.emotionValence!=y.emotionValence || x.emotionIntensity!=y.emotionIntensity) return false;
    }
    for(std::size_t i=0;i<a.receipts().size();++i){
        const KnowledgeReceipt& x=a.receipts()[i];
        const KnowledgeReceipt& y=b.receipts()[i];
        if(x.factId!=y.factId || x.holder!=y.holder || x.originWitness!=y.originWitness ||
           x.immediateSource!=y.immediateSource || x.subject!=y.subject || x.supports!=y.supports ||
           x.confidence!=y.confidence || x.learnedMinute!=y.learnedMinute || x.source!=y.source ||
           x.transmissionPath!=y.transmissionPath) return false;
    }
    return true;
}

static Character makeLearner(CharacterId id)
{
    Character c;
    c.id=id;
    c.name="Learner"+std::to_string(id);
    c.civilization.character=id;
    c.civilization.learningSkill=1.0;
    c.personality.curiosity=1.0;
    c.personality.openness=1.0;
    c.personality.sociability=0.8;
    return c;
}

int main()
{
    constexpr std::uint64_t seed=515151;

    Character teacher=makeLearner(1);
    teacher.name="Teacher";
    teacher.civilization.knowledge.learn(
        TechniqueId::SharpFlake,KnowledgeLevel::Mastered,0.98);
    teacher.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Flint,20,0.5,1.0});

    Character observer=makeLearner(2);
    observer.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Flint,4,0.5,1.0});

    SocialKnowledgeBook book;
    const KnowledgeReceipt* origin=registerTechniqueOrigin(
        book,teacher,TechniqueId::SharpFlake,600,
        CivilizationEventType::Crafted,seed);
    CHECK(origin!=nullptr);
    CHECK(book.facts().size()==1);
    CHECK(book.receipts().size()==1);
    CHECK(origin->source==MemorySource::DirectWitness);

    const SocialFact* fact=book.findFact(origin->factId);
    CHECK(fact!=nullptr);
    const SocialFactId factId=fact->id;
    const TechniqueTransmissionOutcome witnessed=applyTechniqueWitness(
        book,*fact,teacher,observer,seed,601);
    CHECK(witnessed.receiptAccepted);
    CHECK(observer.civilization.knowledge.knowsAtLeast(
        TechniqueId::SharpFlake,KnowledgeLevel::Observed));
    CHECK(!observer.civilization.knowledge.knowsAtLeast(
        TechniqueId::SharpFlake,KnowledgeLevel::Reproducible));
    CHECK(book.findReceipt(observer.id,factId)!=nullptr);
    CHECK(observer.memory.entries.back().source==MemorySource::DirectWitness);

    Character taught=makeLearner(3);
    taught.civilization.knowledge.learn(
        TechniqueId::SharpFlake,KnowledgeLevel::Understood,0.60);
    taught.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Flint,4,0.5,1.0});

    RelationshipBook relationships;
    Relationship& taughtToTeacher=relationships.getOrCreate(taught.id,teacher.id);
    taughtToTeacher.trust=1.0;
    taughtToTeacher.respect=1.0;
    taughtToTeacher.familiarity=1.0;
    taughtToTeacher.comfort=1.0;

    TechniqueTransmissionOutcome taughtOutcome;
    bool foundTeachingSuccess=false;
    for(std::uint64_t epoch=0;epoch<1000 && !foundTeachingSuccess;++epoch){
        SocialKnowledgeBook attemptBook=book;
        Character attemptLearner=taught;
        const TechniqueTransmissionOutcome candidate=teachTechnique(
            attemptBook,teacher,attemptLearner,TechniqueId::SharpFlake,
            relationships,seed,610,epoch);
        if(candidate.result==TechniqueTeachingResult::Advanced &&
           candidate.after==KnowledgeLevel::Reproducible){
            book=std::move(attemptBook);
            taught=std::move(attemptLearner);
            taughtOutcome=candidate;
            foundTeachingSuccess=true;
        }
    }
    CHECK(foundTeachingSuccess);
    CHECK(taughtOutcome.receiptAccepted);
    CHECK(taught.civilization.knowledge.knowsAtLeast(
        TechniqueId::SharpFlake,KnowledgeLevel::Reproducible));
    const KnowledgeReceipt* taughtReceipt=book.findReceipt(taught.id,factId);
    CHECK(taughtReceipt!=nullptr);
    CHECK(taughtReceipt->source==MemorySource::ToldByOther);
    CHECK(taughtReceipt->immediateSource==teacher.id);
    CHECK(taughtReceipt->transmissionPath.size()==2);

    SocialKnowledgeBook weakBook;
    Character weakTeacher=teacher;
    Character weakLearner=makeLearner(4);
    weakLearner.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Flint,4,0.5,1.0});
    CHECK(registerTechniqueOrigin(
        weakBook,weakTeacher,TechniqueId::SharpFlake,700,
        CivilizationEventType::Crafted,seed)!=nullptr);
    RelationshipBook weakRelationships;
    const TechniqueTransmissionOutcome weak=teachTechnique(
        weakBook,weakTeacher,weakLearner,TechniqueId::SharpFlake,
        weakRelationships,seed,701,1);
    CHECK(weak.result==TechniqueTeachingResult::TooWeak ||
          weak.result==TechniqueTeachingResult::ComprehensionFailed);
    CHECK(!weakLearner.civilization.knowledge.knowsAtLeast(
        TechniqueId::SharpFlake,KnowledgeLevel::Reproducible));

    const TechniqueTransmissionOutcome duplicate=teachTechnique(
        book,teacher,taught,TechniqueId::SharpFlake,
        relationships,seed,620,999);
    CHECK(duplicate.result==TechniqueTeachingResult::AlreadyKnown ||
          duplicate.result==TechniqueTeachingResult::NoFact ||
          duplicate.result==TechniqueTeachingResult::DuplicateOrLoop);

    Simulation sim(seed);
    sim.setupNewGame();
    Character& simTeacher=sim.world().characters[0];
    Character& simLearner=sim.world().characters[1];
    simTeacher.civilization.knowledge.learn(
        TechniqueId::FiberCordage,KnowledgeLevel::Mastered,0.97);
    const KnowledgeReceipt* simOrigin=registerTechniqueOrigin(
        sim.socialKnowledge(),simTeacher,TechniqueId::FiberCordage,
        sim.world().minute,CivilizationEventType::Crafted,seed);
    CHECK(simOrigin!=nullptr);
    const SocialFact* simFact=sim.socialKnowledge().findFact(simOrigin->factId);
    CHECK(simFact!=nullptr);
    CHECK(applyTechniqueWitness(
        sim.socialKnowledge(),*simFact,simTeacher,simLearner,
        seed,sim.world().minute).receiptAccepted);

    const SimulationStateSnapshot snapshot=sim.captureSnapshot();
    CHECK(sameSocialKnowledge(sim.socialKnowledge(),snapshot.socialKnowledge));

    std::string error;
    std::vector<std::uint8_t> bytes;
    CHECK(encodeSimulationSnapshot(snapshot,bytes,&error));
    CHECK(error.empty());
    CHECK(bytes.size()>32);
    CHECK(bytes[8]==static_cast<std::uint8_t>(SimulationSnapshotBinaryFormatVersion)
        && bytes[9]==0 && bytes[10]==0 && bytes[11]==0);

    SimulationStateSnapshot decoded;
    CHECK(decodeSimulationSnapshot(bytes,decoded,&error));
    CHECK(error.empty());
    CHECK(sameSocialKnowledge(snapshot.socialKnowledge,decoded.socialKnowledge));

    Simulation restored(1);
    CHECK(restored.restoreSnapshot(decoded,&error));
    CHECK(error.empty());
    CHECK(sameSocialKnowledge(sim.socialKnowledge(),restored.socialKnowledge()));

    const auto marker=std::find_end(
        bytes.begin()+12,bytes.end(),
        SocialKnowledgeSnapshotExtensionMagic,
        SocialKnowledgeSnapshotExtensionMagic+sizeof(SocialKnowledgeSnapshotExtensionMagic));
    CHECK(marker!=bytes.end());
    std::vector<std::uint8_t> v2(bytes.begin(),marker);
    CHECK(patchLittleEndianU32(v2,8,2));
    SimulationStateSnapshot migratedV2;
    CHECK(decodeSimulationSnapshot(v2,migratedV2,&error));
    CHECK(error.empty());
    CHECK(migratedV2.socialKnowledge.facts().empty());
    CHECK(migratedV2.socialKnowledge.receipts().empty());
    CHECK(migratedV2.world.characters.size()==snapshot.world.characters.size());

    sim.runMinutes(240);
    restored.runMinutes(240);
    std::vector<std::uint8_t> futureA,futureB;
    CHECK(encodeSimulationSnapshot(sim.captureSnapshot(),futureA,&error));
    CHECK(encodeSimulationSnapshot(restored.captureSnapshot(),futureB,&error));
    CHECK(futureA==futureB);

    sim.setupNewGame();
    CHECK(sim.socialKnowledge().facts().empty());
    CHECK(sim.socialKnowledge().receipts().empty());

    std::cout << "civilization witness/imitation/teaching + provenance persistence passed\n";
    return 0;
}
