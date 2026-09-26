#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "lifelens/PrimitiveFireProgression.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

int main()
{
    SimulationRuleset rules=DefaultSimulationRuleset;
    rules.needs.hungerPerMinute=0.00105;
    rules.utilityAI.idleScore=0.036;
    rules.utilityAI.secondChoiceProbability=0.04;
    CHECK(validSimulationRuleset(rules));

    Simulation source(777331,0,CurrentWorldGenerationVersion,rules);
    source.setupNewGame();
    source.runMinutes(5000);

    // This is a codec fixture, not an autonomous-family integration test.
    // Long-running simulation may legitimately create relationship/family state
    // before we seed representative records below. Reset those books so codec
    // coverage remains deterministic and independent of decision-loop changes.
    source.romances()=RomanceBook{};
    source.households()=HouseholdBook{};
    source.genealogy()=GenealogyBook{};
    source.pregnancies()=PregnancyBook{};
    source.births()=BirthBook{};

    // Seed representative family/history state so book codecs are non-empty.
    Character& a=source.world().characters[0];
    Character& b=source.world().characters[2];
    Relationship& ab=source.relationships().getOrCreate(a.id,b.id);
    Relationship& ba=source.relationships().getOrCreate(b.id,a.id);
    ab.affection=0.83; ab.trust=0.72; ab.attraction=0.68; ab.romanticInterest=0.74; ab.commitment=0.61;
    ba.affection=0.79; ba.trust=0.75; ba.attraction=0.70; ba.romanticInterest=0.71; ba.commitment=0.64;
    CHECK(source.romances().startDating(a.id,b.id,a.id,source.world().minute-1000));
    CHECK(source.households().create(901,{a.id,b.id},9,2500.0));
    CHECK(source.genealogy().linkSpouses(a.id,b.id));
    PregnancyState* pregnancy=source.pregnancies().start(b.id,a.id,source.world().minute-100);
    CHECK(pregnancy!=nullptr);
    pregnancy->fatigue=0.31;
    pregnancy->stress=0.22;

    MemoryRecord memory;
    memory.who=b.id;
    memory.what="persistent-memory";
    memory.where="shared-home";
    memory.minute=source.world().minute-25;
    memory.importance=0.91;
    memory.tags={"save","family"};
    a.memory.add(memory);
    a.beliefs.getOrCreate(b.id,"trusted").applyEvidence(true,0.88,source.world().minute);

    // Seed an actively burning FirePit so the binary codec must preserve all
    // authoritative runtime fields, not merely the physical facility shell.
    auto& facilities=source.world().facilities;
    facilities.erase(
        std::remove_if(facilities.begin(),facilities.end(),[](const ConstructedFacility& facility){
            return facility.kind==FacilityKind::FirePit;
        }),
        facilities.end());
    a.civilization.knowledge.learn(
        TechniqueId::FireMaking,KnowledgeLevel::Reproducible,0.9);
    a.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Stone,5,0.5,1.0});
    a.civilization.inventory.add({ItemKind::RawMaterial,MaterialKind::Wood,4,0.5,1.0});
    GridPos fireActivityAnchor{};
    CHECK(source.runtimePosition(a.id,fireActivityAnchor));
    const PrimitiveFirePitSiteOpportunity fireSite=
        choosePrimitiveFirePitSite(source.world(),a.id,fireActivityAnchor);
    CHECK(fireSite.available);
    ConstructedFacility* firePit=establishPrimitiveFirePitProject(source.world(),a,fireSite.pos);
    CHECK(firePit!=nullptr);
    const FacilityId firePitId=firePit->id;
    CHECK(deliverFacilityMaterial(*firePit,a.civilization.inventory,MaterialKind::Stone,5)==5);
    CHECK(deliverFacilityMaterial(*firePit,a.civilization.inventory,MaterialKind::Wood,2)==2);
    PrimitiveFirePitWorkResult fireWork;
    while(!fireWork.completed){
        fireWork=workOnPrimitiveFirePit(source.world(),a,2.0);
        CHECK(fireWork.worked);
    }
    CHECK(fuelPrimitiveFirePit(source.world(),a,firePitId,2)==2);
    CHECK(ignitePrimitiveFirePit(source.world(),a,firePitId));
    for(int i=0;i<7;++i){
        ++source.world().minute;
        advancePrimitiveFireOneMinute(source.world());
    }
    firePit=primitiveFirePitProject(source.world());
    CHECK(firePit!=nullptr && firePit->lit);
    CHECK(firePit->fuelUnits==2);
    CHECK(firePit->burnMinutesRemaining==PrimitiveFireBurnMinutesPerWoodUnit-7);
    const int savedBurnMinutes=firePit->burnMinutesRemaining;
    const double savedHeat=firePit->heatLevel;

    {
        // Explicit non-default pending context fixture. Headless locomotion now
        // keeps social/parenting/teaching actions alive while the actor walks,
        // so the binary codec must preserve that in-flight authority.
        SimulationStateSnapshot pendingSnapshot=source.captureSnapshot();
        auto actorRuntime=pendingSnapshot.runtime.find(a.id);
        auto targetRuntime=pendingSnapshot.runtime.find(b.id);
        CHECK(actorRuntime!=pendingSnapshot.runtime.end());
        CHECK(targetRuntime!=pendingSnapshot.runtime.end());

        PendingContextAction pending;
        pending.token=987654321ULL;
        pending.kind=ContextActionKind::Social;
        pending.issuedMinute=pendingSnapshot.world.minute;
        pending.social.intent=SocialIntent::Approach;
        pending.social.target=b.id;
        pending.social.utility=0.73;
        pending.hasSpatialTarget=true;
        pending.targetPos=targetRuntime->second.pos;
        actorRuntime->second.pendingContext=pending;

        std::vector<std::uint8_t> pendingBytes;
        std::string pendingError;
        CHECK(encodeSimulationSnapshot(
            pendingSnapshot,pendingBytes,&pendingError));
        CHECK(pendingError.empty());

        SimulationStateSnapshot pendingDecoded;
        CHECK(decodeSimulationSnapshot(
            pendingBytes,pendingDecoded,&pendingError));
        CHECK(pendingError.empty());

        const auto decodedRuntime=pendingDecoded.runtime.find(a.id);
        CHECK(decodedRuntime!=pendingDecoded.runtime.end());
        const PendingContextAction& decodedPending=
            decodedRuntime->second.pendingContext;
        CHECK(decodedPending.token==pending.token);
        CHECK(decodedPending.kind==pending.kind);
        CHECK(decodedPending.issuedMinute==pending.issuedMinute);
        CHECK(decodedPending.social.intent==pending.social.intent);
        CHECK(decodedPending.social.target==pending.social.target);
        CHECK(decodedPending.social.utility==pending.social.utility);
        CHECK(decodedPending.hasSpatialTarget);
        CHECK(decodedPending.targetPos.x==pending.targetPos.x);
        CHECK(decodedPending.targetPos.y==pending.targetPos.y);

        std::vector<std::uint8_t> pendingReencoded;
        CHECK(encodeSimulationSnapshot(
            pendingDecoded,pendingReencoded,&pendingError));
        CHECK(pendingBytes==pendingReencoded);
    }

    const SimulationStateSnapshot snapshot=source.captureSnapshot();
    CHECK(snapshot.version==SimulationSnapshotVersion);
    CHECK(sameSimulationRuleset(snapshot.ruleset,rules));
    std::vector<std::uint8_t> bytes;
    std::string error;
    CHECK(encodeSimulationSnapshot(snapshot,bytes,&error));
    CHECK(error.empty());
    CHECK(bytes.size()>64);

    SimulationStateSnapshot decoded;
    CHECK(decodeSimulationSnapshot(bytes,decoded,&error));
    CHECK(error.empty());
    CHECK(sameSimulationRuleset(decoded.ruleset,rules));
    const ConstructedFacility* decodedFirePit=nullptr;
    for(const auto& facility:decoded.world.facilities){
        if(facility.id==firePitId){ decodedFirePit=&facility; break; }
    }
    CHECK(decodedFirePit!=nullptr);
    CHECK(decodedFirePit->kind==FacilityKind::FirePit);
    CHECK(decodedFirePit->lit);
    CHECK(decodedFirePit->fuelUnits==2);
    CHECK(decodedFirePit->burnMinutesRemaining==savedBurnMinutes);
    CHECK(decodedFirePit->heatLevel==savedHeat);

    // Canonical codec contract: decode+encode must reproduce identical bytes.
    std::vector<std::uint8_t> reencoded;
    CHECK(encodeSimulationSnapshot(decoded,reencoded,&error));
    CHECK(bytes==reencoded);

    Simulation restored(
        decoded.world.seed,
        decoded.world.populationSeed,
        decoded.world.generationVersion,
        decoded.ruleset);
    CHECK(restored.restoreSnapshot(decoded,&error));
    CHECK(error.empty());

    // A simulation created under different immutable rules must not silently
    // accept state that was produced under another ruleset.
    Simulation wrongRuleset(decoded.world.seed);
    CHECK(!wrongRuleset.restoreSnapshot(decoded,&error));
    CHECK(!error.empty());

    // Restored Core must serialize identically immediately.
    std::vector<std::uint8_t> restoredBytes;
    CHECK(encodeSimulationSnapshot(restored.captureSnapshot(),restoredBytes,&error));
    CHECK(restoredBytes==bytes);

    // And must continue into the exact same future, proving RNG/runtime/ruleset state.
    source.runMinutes(10000);
    restored.runMinutes(10000);
    std::vector<std::uint8_t> futureA,futureB;
    CHECK(encodeSimulationSnapshot(source.captureSnapshot(),futureA,&error));
    CHECK(encodeSimulationSnapshot(restored.captureSnapshot(),futureB,&error));
    CHECK(futureA==futureB);

    // Corruption/format mismatches are rejected cleanly.
    std::vector<std::uint8_t> corrupted=bytes;
    corrupted[0]^=0xff;
    SimulationStateSnapshot rejected;
    CHECK(!decodeSimulationSnapshot(corrupted,rejected,&error));
    CHECK(!error.empty());

    std::vector<std::uint8_t> truncated(bytes.begin(),bytes.begin()+bytes.size()/2);
    CHECK(!decodeSimulationSnapshot(truncated,rejected,&error));

    std::vector<std::uint8_t> trailing=bytes;
    trailing.push_back(0x42);
    CHECK(!decodeSimulationSnapshot(trailing,rejected,&error));

    std::cout << "snapshot binary codec roundtrip + ruleset continuation passed\n";
    return 0;
}
