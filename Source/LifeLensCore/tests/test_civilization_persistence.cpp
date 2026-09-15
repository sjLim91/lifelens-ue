#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/CivilizationSnapshotCodec.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static ResourceNode* findResource(World& world,MaterialKind material)
{
    for(auto& node:world.resourceNodes) if(node.material==material) return &node;
    return nullptr;
}

static const TechniqueKnowledge* findKnowledge(const KnowledgeState& state,TechniqueId technique)
{
    for(const auto& record:state.all()) if(record.technique==technique) return &record;
    return nullptr;
}

static bool sameInventory(const Inventory& a,const Inventory& b)
{
    if(a.stacks().size()!=b.stacks().size()) return false;
    for(std::size_t i=0;i<a.stacks().size();++i){
        const auto& x=a.stacks()[i]; const auto& y=b.stacks()[i];
        if(x.kind!=y.kind || x.material!=y.material || x.quantity!=y.quantity ||
           x.quality!=y.quality || x.durability!=y.durability) return false;
    }
    return true;
}

static bool sameCivilization(const IndividualCivilizationState& a,const IndividualCivilizationState& b)
{
    if(a.character!=b.character || a.gatheringSkill!=b.gatheringSkill ||
       a.craftingSkill!=b.craftingSkill || a.learningSkill!=b.learningSkill ||
       !sameInventory(a.inventory,b.inventory) || a.knowledge.all().size()!=b.knowledge.all().size()) return false;
    for(std::size_t i=0;i<a.knowledge.all().size();++i){
        const auto& x=a.knowledge.all()[i]; const auto& y=b.knowledge.all()[i];
        if(x.technique!=y.technique || x.level!=y.level || x.confidence!=y.confidence ||
           x.successfulUses!=y.successfulUses) return false;
    }
    return true;
}

static bool sameCivilizationWorld(const World& a,const World& b)
{
    if(a.characters.size()!=b.characters.size() || a.resourceNodes.size()!=b.resourceNodes.size() ||
       a.storageSites.size()!=b.storageSites.size()) return false;
    for(std::size_t i=0;i<a.characters.size();++i){
        if(a.characters[i].id!=b.characters[i].id ||
           !sameCivilization(a.characters[i].civilization,b.characters[i].civilization)) return false;
    }
    for(std::size_t i=0;i<a.resourceNodes.size();++i){
        const auto& x=a.resourceNodes[i]; const auto& y=b.resourceNodes[i];
        if(x.id!=y.id || x.material!=y.material || x.quantity!=y.quantity ||
           x.maxQuantity!=y.maxQuantity || x.renewable!=y.renewable ||
           x.regenerationPerDay!=y.regenerationPerDay) return false;
    }
    for(std::size_t i=0;i<a.storageSites.size();++i){
        if(a.storageSites[i].id!=b.storageSites[i].id ||
           !sameInventory(a.storageSites[i].inventory,b.storageSites[i].inventory)) return false;
    }
    return true;
}

static bool patchLittleEndianU32(std::vector<std::uint8_t>& bytes,std::size_t offset,std::uint32_t value)
{
    if(bytes.size()<offset+4) return false;
    bytes[offset]=static_cast<std::uint8_t>(value&0xffu);
    bytes[offset+1]=static_cast<std::uint8_t>((value>>8)&0xffu);
    bytes[offset+2]=static_cast<std::uint8_t>((value>>16)&0xffu);
    bytes[offset+3]=static_cast<std::uint8_t>((value>>24)&0xffu);
    return true;
}

int main()
{
    Simulation source(909090);
    source.setupNewGame();
    CHECK(source.world().characters.size()==4);
    CHECK(!source.world().resourceNodes.empty());
    CHECK(source.world().generatedNaturalChunks.size()==1);
    CHECK(source.world().storageSites.empty());

    Character& discoverer=source.world().characters[0];
    CHECK(discoverer.civilization.character==discoverer.id);
    CHECK(discoverer.civilization.knowledge.all().empty());

    ResourceNode* flint=findResource(source.world(),MaterialKind::Flint);
    CHECK(flint!=nullptr && flint->material==MaterialKind::Flint);
    const int flintBefore=flint->quantity;
    const CivilizationEvent gathered=gatherResource(discoverer.civilization,*flint,4);
    CHECK(gathered.quantity>0);
    CHECK(flint->quantity==flintBefore-gathered.quantity);
    CHECK(discoverer.civilization.inventory.count(ItemKind::RawMaterial,MaterialKind::Flint)==gathered.quantity);

    discoverer.civilization.knowledge.learn(TechniqueId::SharpFlake,KnowledgeLevel::Reproducible,0.81);
    discoverer.civilization.knowledge.recordSuccessfulUse(TechniqueId::SharpFlake);
    discoverer.civilization.knowledge.recordSuccessfulUse(TechniqueId::SharpFlake);
    const TechniqueKnowledge* beforeKnowledge=findKnowledge(discoverer.civilization.knowledge,TechniqueId::SharpFlake);
    CHECK(beforeKnowledge!=nullptr && beforeKnowledge->successfulUses==2);

    StorageSite persistedStorage;
    persistedStorage.id=7001;
    source.world().storageSites.push_back(persistedStorage);
    source.world().storageSites[0].inventory.add({ItemKind::RawMaterial,MaterialKind::Wood,7,0.52,0.91});
    source.world().storageSites[0].inventory.add({ItemKind::Cordage,MaterialKind::Fiber,2,0.77,0.88});

    const SimulationStateSnapshot snapshot=source.captureSnapshot();
    CHECK(sameCivilizationWorld(source.world(),snapshot.world));

    std::string error;
    Simulation restored(1);
    CHECK(restored.restoreSnapshot(snapshot,&error));
    CHECK(error.empty());
    CHECK(sameCivilizationWorld(source.world(),restored.world()));

    std::vector<std::uint8_t> bytes;
    CHECK(encodeSimulationSnapshot(snapshot,bytes,&error));
    CHECK(error.empty());
    CHECK(bytes.size()>32);
    CHECK(bytes[8]==static_cast<std::uint8_t>(SimulationSnapshotBinaryFormatVersion)
        && bytes[9]==0 && bytes[10]==0 && bytes[11]==0);

    SimulationStateSnapshot decoded;
    CHECK(decodeSimulationSnapshot(bytes,decoded,&error));
    CHECK(error.empty());
    CHECK(sameCivilizationWorld(snapshot.world,decoded.world));

    std::vector<std::uint8_t> reencoded;
    CHECK(encodeSimulationSnapshot(decoded,reencoded,&error));
    CHECK(bytes==reencoded);

    // Reconstruct an authentic legacy-v1-shaped payload by removing all modern
    // extensions at the civilization marker and changing only the format header.
    const auto marker=std::find_end(
        bytes.begin()+12,bytes.end(),
        CivilizationSnapshotExtensionMagic,
        CivilizationSnapshotExtensionMagic+sizeof(CivilizationSnapshotExtensionMagic));
    CHECK(marker!=bytes.end());
    std::vector<std::uint8_t> legacy(bytes.begin(),marker);
    CHECK(patchLittleEndianU32(legacy,8,1));

    SimulationStateSnapshot migrated;
    CHECK(decodeSimulationSnapshot(legacy,migrated,&error));
    CHECK(error.empty());
    CHECK(migrated.world.characters.size()==snapshot.world.characters.size());
    CHECK(migrated.world.resourceNodes.size()==7);
    CHECK(migrated.world.storageSites.size()==1);
    CHECK(migrated.socialKnowledge.facts().empty());
    CHECK(migrated.socialKnowledge.receipts().empty());
    for(const Character& character:migrated.world.characters){
        CHECK(character.civilization.character==character.id);
        CHECK(character.civilization.inventory.stacks().empty());
        CHECK(character.civilization.knowledge.all().empty());
    }

    std::vector<std::uint8_t> migratedCurrent;
    CHECK(encodeSimulationSnapshot(migrated,migratedCurrent,&error));
    CHECK(!migratedCurrent.empty()
        && migratedCurrent[8]==static_cast<std::uint8_t>(SimulationSnapshotBinaryFormatVersion));

    SimulationStateSnapshot bad=snapshot;
    bad.world.characters[0].civilization.character=999999;
    Simulation untouched(123);
    untouched.setupNewGame();
    const auto untouchedBefore=untouched.captureSnapshot();
    CHECK(!untouched.restoreSnapshot(bad,&error));
    CHECK(!error.empty());
    CHECK(untouched.world().seed==untouchedBefore.world.seed);

    bad=snapshot;
    bad.world.resourceNodes.push_back(bad.world.resourceNodes[0]);
    CHECK(!untouched.restoreSnapshot(bad,&error));

    Simulation continuation(1);
    CHECK(continuation.restoreSnapshot(decoded,&error));
    source.runMinutes(2500);
    continuation.runMinutes(2500);
    std::vector<std::uint8_t> futureA,futureB;
    CHECK(encodeSimulationSnapshot(source.captureSnapshot(),futureA,&error));
    CHECK(encodeSimulationSnapshot(continuation.captureSnapshot(),futureB,&error));
    CHECK(futureA==futureB);

    std::cout << "civilization runtime snapshot persistence + v1 migration passed\n";
    return 0;
}
