#include <iostream>
#include <string>

#include "lifelens/CivilizationObserverReadModel.h"
#include "lifelens/Simulation.h"

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

static const CivilizationTechniqueObservation* findTechnique(
    const ResidentCivilizationObservation& observation,
    TechniqueId technique)
{
    for(const auto& item:observation.techniques) if(item.technique==technique) return &item;
    return nullptr;
}

int main()
{
    constexpr std::uint64_t seed=606060;
    Simulation sim(seed);
    sim.setupNewGame();
    CHECK(sim.world().characters.size()==4);

    Character& discoverer=sim.world().characters[0];
    Character& witness=sim.world().characters[1];
    const int eventMinute=sim.world().minute;

    ResourceNode* flint=findResource(sim.world(),MaterialKind::Flint);
    CHECK(flint!=nullptr);
    const int resourceBefore=flint->quantity;
    const CivilizationEvent gathered=gatherResource(discoverer.civilization,*flint,4);
    CHECK(gathered.quantity>0);
    CHECK(flint->quantity<resourceBefore);

    discoverer.civilization.knowledge.learn(
        TechniqueId::SharpFlake,KnowledgeLevel::Mastered,0.98);
    const KnowledgeReceipt* origin=registerTechniqueOrigin(
        sim.socialKnowledge(),discoverer,TechniqueId::SharpFlake,
        eventMinute,CivilizationEventType::Discovered,seed);
    CHECK(origin!=nullptr);
    const SocialFact* discoveryFact=sim.socialKnowledge().findFact(origin->factId);
    CHECK(discoveryFact!=nullptr);

    const TechniqueTransmissionOutcome witnessed=applyTechniqueWitness(
        sim.socialKnowledge(),*discoveryFact,discoverer,witness,
        seed,eventMinute);
    CHECK(witnessed.receiptAccepted);

    // Add a crafted demonstration fact to ensure the world read model does not
    // mistake repeated crafting for a new major discovery. Snapshot timestamps
    // must never be in the future relative to world.minute.
    const KnowledgeReceipt* crafted=registerTechniqueOrigin(
        sim.socialKnowledge(),discoverer,TechniqueId::FiberCordage,
        eventMinute,CivilizationEventType::Crafted,seed);
    CHECK(crafted!=nullptr);

    sim.world().storageSites[0].inventory.add(
        {ItemKind::RawMaterial,MaterialKind::Wood,3,0.5,1.0});

    const ResidentCivilizationObservation discovererRead=
        sim.observeResidentCivilization(discoverer.id);
    CHECK(discovererRead.residentId==discoverer.id);
    CHECK(discovererRead.totalInventoryUnits>0);
    CHECK(discovererRead.knownTechniqueCount>=1);
    CHECK(discovererRead.reproducibleTechniqueCount>=1);
    const CivilizationTechniqueObservation* discovererTechnique=
        findTechnique(discovererRead,TechniqueId::SharpFlake);
    CHECK(discovererTechnique!=nullptr);
    CHECK(discovererTechnique->hasProvenance);
    CHECK(discovererTechnique->source==CivilizationKnowledgeSource::SelfDiscovery);
    CHECK(discovererTechnique->originResidentId==discoverer.id);
    CHECK(discovererTechnique->immediateSourceId==discoverer.id);

    const ResidentCivilizationObservation witnessRead=
        sim.observeResidentCivilization(witness.id);
    const CivilizationTechniqueObservation* witnessTechnique=
        findTechnique(witnessRead,TechniqueId::SharpFlake);
    CHECK(witnessTechnique!=nullptr);
    CHECK(witnessTechnique->hasProvenance);
    CHECK(witnessTechnique->source==CivilizationKnowledgeSource::DirectWitness);
    CHECK(witnessTechnique->originResidentId==discoverer.id);
    CHECK(witnessTechnique->immediateSourceId==witness.id);
    CHECK(witnessTechnique->level>=KnowledgeLevel::Observed);

    const ResidentCivilizationObservation missing=sim.observeResidentCivilization(999999);
    CHECK(missing.residentId==0);
    CHECK(missing.inventory.empty());
    CHECK(missing.techniques.empty());

    const CivilizationWorldObservation worldRead=sim.observeCivilizationWorld();
    CHECK(worldRead.resourceNodeCount==7);
    CHECK(worldRead.storageSiteCount==1);
    CHECK(worldRead.totalStoredUnits==3);
    CHECK(worldRead.techniqueFactCount==2);
    CHECK(worldRead.transmissionReceiptCount==3);
    CHECK(worldRead.uniqueKnownTechniqueTypes>=1);
    CHECK(worldRead.uniqueReproducibleTechniqueTypes>=1);
    CHECK(worldRead.recentDiscoveries.size()==1);
    CHECK(worldRead.recentDiscoveries[0].technique==TechniqueId::SharpFlake);
    CHECK(worldRead.recentDiscoveries[0].discovererId==discoverer.id);
    CHECK(worldRead.recentDiscoveries[0].discovererName==discoverer.name);
    CHECK(worldRead.recentDiscoveries[0].recipientCount==2);
    CHECK(worldRead.recentDiscoveries[0].livingKnowerCount>=2);

    // Read models must be rebuilt from the restored authoritative Core snapshot.
    // They must not depend on transient Unreal/legacy projection state.
    const SimulationStateSnapshot snapshot=sim.captureSnapshot();
    Simulation restored(1);
    std::string error;
    CHECK(restored.restoreSnapshot(snapshot,&error));
    CHECK(error.empty());

    const auto restoredResident=restored.observeResidentCivilization(discoverer.id);
    const auto restoredWorld=restored.observeCivilizationWorld();
    const auto* restoredTechnique=findTechnique(restoredResident,TechniqueId::SharpFlake);
    CHECK(restoredTechnique!=nullptr);
    CHECK(restoredTechnique->source==CivilizationKnowledgeSource::SelfDiscovery);
    CHECK(restoredTechnique->factId==discovererTechnique->factId);
    CHECK(restoredResident.totalInventoryUnits==discovererRead.totalInventoryUnits);
    CHECK(restoredWorld.totalResourceUnits==worldRead.totalResourceUnits);
    CHECK(restoredWorld.totalStoredUnits==worldRead.totalStoredUnits);
    CHECK(restoredWorld.techniqueFactCount==worldRead.techniqueFactCount);
    CHECK(restoredWorld.transmissionReceiptCount==worldRead.transmissionReceiptCount);
    CHECK(restoredWorld.recentDiscoveries.size()==worldRead.recentDiscoveries.size());

    std::cout << "civilization observer read model passed\n";
    return 0;
}
