#include <cassert>
#include <string>
#include <vector>

#include "lifelens/ResidentAIExistingAffordance.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

int main()
{
    // Use a known traversable seeded region with a real frontier. An empty
    // synthetic ocean-adjacent world does not guarantee an Explore option.
    World world(606002);
    world.minute=480;
    const GridPos camp=world.initialStartRegionCenterGrid();
    world.resourceNodes.clear();
    Character resident;
    resident.id=9;
    resident.alive=true;
    resident.civilization.character=9;
    resident.lifeStage=LifeStage::Adult;
    resident.needs={0.72,0.10,0.10,0.10,0.10};

    ResourceNode food;
    food.id=101;
    food.material=MaterialKind::PlantFood;
    food.quantity=20;
    food.maxQuantity=20;
    food.pos=camp;
    world.resourceNodes.push_back(food);

    AcceptedCognitiveDecision thought;
    thought.actor=9;
    thought.intent=CognitiveIntentKind::ImproveFoodSecurity;
    thought.minute=480;
    thought.priority=0.9;

    const auto original=chooseResidentAIExistingCivilizationAffordance(
        world,resident,camp,thought);
    assert(original && original->intent==CivilizationIntent::Gather);
    assert(original->material==MaterialKind::PlantFood);
    assert(!residentAIHasRecentRepeatedGatherFailure(
        resident,MaterialKind::PlantFood,world.minute));

    CivilizationUtilityDecision gather;
    gather.intent=CivilizationIntent::Gather;
    gather.material=MaterialKind::PlantFood;
    const std::string key=civilizationActionExperienceKey(gather);
    assert(key=="Civilization:Gather:PlantFood");

    for(int i=0;i<2;++i)
        rememberCivilizationAttemptOutcome(resident,gather,480+i,false);
    // The Core actually retains TWO different failure timestamps.
    // The model's RepeatedFailure trigger must not rely on synthetic tests
    // that add memories which live gameplay would have overwritten.
    int directFailures=0;
    for(const auto& memory:resident.memory.entries){
        if(memory.tags.size()==3
           &&memory.tags[0]=="action_experience"
           &&memory.tags[1]==key
           &&memory.tags[2]=="failure"){
            ++directFailures;
            assert(memory.source==MemorySource::DirectWitness);
            assert(memory.witnessed);
            assert(memory.who==resident.id);
            assert(memory.sourceCharacter==resident.id);
            assert(memory.minute==480 ||memory.minute==481);
        }
    }
    assert(directFailures==2);

    // Even among many emotionally vivid unrelated memories, the REAL
    // two-attempt history must reach the local resident's eight-slot mind.
    Character crowded=resident;
    for(int i=0;i<12;++i){
        MemoryRecord vivid;
        vivid.who=crowded.id;
        vivid.minute=482;
        vivid.what="unrelated vivid event "+std::to_string(i);
        vivid.importance=1.0;
        vivid.confidence=1.0;
        vivid.emotionIntensity=1.0;
        vivid.tags={"unrelated_event"};
        crowded.memory.add(vivid);
    }
    RelationshipBook relationships;
    auto context=buildCognitiveRequest(
        crowded,relationships,482,CognitiveTriggerKind::Reflection,
        {CognitiveIntentKind::ImproveFoodSecurity},8,8,8);
    assert(context.memories.size()==8);
    int visibleFailures=0;
    for(const auto& memory:context.memories){
        if(memory.tags.size()==3 &&memory.tags[1]==key
           &&memory.tags[2]=="failure"){
            ++visibleFailures;
            assert(memory.who==resident.id);
        }
    }
    assert(visibleFailures==2);
    const auto originalFingerprint=cognitiveRequestFingerprint(context);
    std::reverse(crowded.memory.entries.begin(),crowded.memory.entries.end());
    context=buildCognitiveRequest(
        crowded,relationships,482,CognitiveTriggerKind::Reflection,
        {CognitiveIntentKind::ImproveFoodSecurity},8,8,8);
    assert(cognitiveRequestFingerprint(context)==originalFingerprint);

    world.minute=482;
    thought.minute=482;
    assert(!residentAIHasRecentRepeatedGatherFailure(
        resident,MaterialKind::PlantFood,world.minute));
    // Two attempts do not suppress a perfectly real nearby food source.
    const auto twoFailures=chooseResidentAIExistingCivilizationAffordance(
        world,resident,camp,thought);
    assert(twoFailures && twoFailures->intent==CivilizationIntent::Gather);

    rememberCivilizationAttemptOutcome(resident,gather,482,false);
    assert(residentAIHasRecentRepeatedGatherFailure(
        resident,MaterialKind::PlantFood,world.minute));
    directFailures=0;
    for(const auto& memory:resident.memory.entries)
        if(memory.tags.size()==3 &&memory.tags[1]==key
           &&memory.tags[2]=="failure") ++directFailures;
    assert(directFailures==3);

    // A transplanted memory of another person's action is not firsthand
    // evidence even if a negative belief was copied alongside it.
    Character borrowed=resident;
    for(auto& memory:borrowed.memory.entries)
        if(memory.tags.size()==3 &&memory.tags[1]==key)
            memory.sourceCharacter=999;
    assert(!residentAIHasRecentRepeatedGatherFailure(
        borrowed,MaterialKind::PlantFood,world.minute));

    // A believable memory tag is not an executed Core outcome. Even when
    // an authentic failed-attempt belief exists, forged narration cannot
    // create a failure or a success that erases genuine learning.
    Character inventedFailures=resident;
    for(auto& memory:inventedFailures.memory.entries)
        if(memory.tags.size()==3 &&memory.tags[1]==key)
            memory.what="a model invented a Gather failure";
    assert(!residentAIHasRecentRepeatedGatherFailure(
        inventedFailures,MaterialKind::PlantFood,482));
    Character wrongSubject=resident;
    for(auto& memory:wrongSubject.memory.entries)
        if(memory.tags.size()==3 &&memory.tags[1]==key)
            memory.who=999;
    assert(!residentAIHasRecentRepeatedGatherFailure(
        wrongSubject,MaterialKind::PlantFood,482));
    Character fakeRecovery=resident;
    MemoryRecord inventedSuccess=fakeRecovery.memory.entries.front();
    inventedSuccess.minute=483;
    inventedSuccess.tags[2]="success";
    inventedSuccess.what="the model said the attempt succeeded";
    fakeRecovery.memory.entries.push_back(inventedSuccess);
    assert(residentAIHasRecentRepeatedGatherFailure(
        fakeRecovery,MaterialKind::PlantFood,483));

    // Core source memory distinguishes a repeatedly failing REAL node A
    // from a different living source B of the same material. Before personal
    // experience, equally stocked provision sources preserve vector order.
    // The native Core chooser intentionally preserves its stable baseline.
    // Personal experience affects ONLY a verified resident-goal preference
    // and the existing cognitive retry path, not an unrelated survival slot.
    World sources(606002);
    sources.minute=484;
    sources.resourceNodes.clear();
    ResourceNode sourceA=food;
    sourceA.id=301;
    sources.resourceNodes.push_back(sourceA);
    ResourceNode sourceB=food;
    sourceB.id=302;
    sourceB.pos={camp.x+4,camp.y};
    sources.resourceNodes.push_back(sourceB);
    Character searching=resident;
    searching.memory=MemoryState{};
    searching.beliefs=BeliefState{};
    CivilizationUtilityDecision attemptedA;
    attemptedA.intent=CivilizationIntent::Gather;
    attemptedA.material=MaterialKind::PlantFood;
    attemptedA.resourceNode=301;
    const auto neutralSource=bestGatherDecisionAtPosition(
        sources,searching,camp,nullptr,MaterialKind::PlantFood);
    assert(neutralSource.intent==CivilizationIntent::Gather
        &&neutralSource.resourceNode==301);
    for(int minute=481;minute<=483;++minute)
        rememberCivilizationAttemptOutcome(
            searching,attemptedA,minute,false,
            ResidentGatherFailureCause::ResourceAccessUnavailable);
    // Source removal/exhaustion must use a verified actor-owned Core
    // record. Later counterfeit "success" cannot reset confirmed loss.
    Character lostSource=searching;
    rememberCivilizationAttemptOutcome(
        lostSource,attemptedA,484,false,
        ResidentGatherFailureCause::SourceExhausted);
    assert(residentAIHasConfirmedLostGatherSource(
        lostSource,MaterialKind::PlantFood,484));
    MemoryRecord counterfeitSuccess=lostSource.memory.entries.back();
    counterfeitSuccess.minute=485;
    counterfeitSuccess.tags[2]="success";
    counterfeitSuccess.what="a made-up successful Gather result";
    lostSource.memory.entries.push_back(counterfeitSuccess);
    assert(residentAIHasConfirmedLostGatherSource(
        lostSource,MaterialKind::PlantFood,485));
    counterfeitSuccess.what="실제 문명 행동 "+key+" 수행에 성공했다";
    counterfeitSuccess.who=999; // someone else's actual record
    counterfeitSuccess.minute=486;
    lostSource.memory.entries.push_back(counterfeitSuccess);
    assert(residentAIHasConfirmedLostGatherSource(
        lostSource,MaterialKind::PlantFood,486));

    AcceptedCognitiveDecision sourceThought=thought;
    sourceThought.minute=480;
    const auto ranked=bestGatherDecisionAtPosition(
        sources,searching,camp,nullptr,MaterialKind::PlantFood);
    assert(ranked.intent==CivilizationIntent::Gather
        &&ranked.resourceNode==301);
    assert(ranked.utility==neutralSource.utility);
    const auto personalIndex=buildResidentResourceOutcomeIndex(
        searching,sources.minute);
    const auto personalRanked=bestGatherDecisionAtPosition(
        sources,searching,camp,nullptr,MaterialKind::PlantFood,
        nullptr,&personalIndex);
    assert(personalRanked.intent==CivilizationIntent::Gather
        &&personalRanked.resourceNode==302);
    // B has no personal failed-source evidence, so its utility stays neutral.
    // Learning about A must not add an unrelated material-wide penalty to B.
    assert(personalRanked.utility==neutralSource.utility);
    assert(residentAIHasRecentRepeatedGatherFailure(
        searching,MaterialKind::PlantFood,484));
    assert(residentAIRecentGatherSourceFailed(
        searching,MaterialKind::PlantFood,301,480,484));
    assert(residentAIHasRecentSourcedGatherFailure(
        searching,MaterialKind::PlantFood,480,484));
    assert(!residentAIRecentGatherSourceFailed(
        searching,MaterialKind::PlantFood,302,480,484));
    const auto changedSource=chooseResidentAIExistingCivilizationAffordance(
        sources,searching,camp,sourceThought);
    assert(changedSource
        &&changedSource->intent==CivilizationIntent::Gather
        &&changedSource->resourceNode==302);
    assert(changedSource->utility==ranked.utility); // no second penalty at the strategy layer
    // The next language-model turn cannot erase personal first-hand
    // failures. A new thought is NOT a new person or a clean memory.
    auto freshThought=sourceThought;
    freshThought.minute=484;
    const auto stillLearned=chooseResidentAIExistingCivilizationAffordance(
        sources,searching,camp,freshThought);
    assert(stillLearned
        &&stillLearned->intent==CivilizationIntent::Gather
        &&stillLearned->resourceNode==302);
    assert(stillLearned->utility==ranked.utility);
    assert(sources.resourceNodes[0].quantity==20
        &&sources.resourceNodes[1].quantity==20);
    assert(searching.civilization.inventory.count(
        ItemKind::RawMaterial,MaterialKind::PlantFood)==0);
    // A later VERIFIED success at A makes it eligible again. Do not
    // freeze an entire source based on historical failures forever.
    rememberCivilizationAttemptOutcome(searching,attemptedA,484,true);
    sources.minute=485;
    assert(!residentAIHasRecentSourcedGatherFailure(
        searching,MaterialKind::PlantFood,480,485));
    // Recovery of A cannot erase an independent CURRENT failure at B,
    // but a later real success at B resolves that source as well.
    auto attemptedB=attemptedA;
    attemptedB.resourceNode=302;
    rememberCivilizationAttemptOutcome(
        searching,attemptedB,485,false,
        ResidentGatherFailureCause::ResourceAccessUnavailable);
    assert(residentAIHasRecentSourcedGatherFailure(
        searching,MaterialKind::PlantFood,480,485));
    rememberCivilizationAttemptOutcome(searching,attemptedB,486,true);
    assert(!residentAIHasRecentSourcedGatherFailure(
        searching,MaterialKind::PlantFood,480,486));
    // A fabricated narrative with failure tags is NOT a new verified
    // execution record. The exact Core-written message is mandatory.
    Character narrated=searching;
    MemoryRecord claimed;
    claimed.who=narrated.id;
    claimed.sourceCharacter=narrated.id;
    claimed.source=MemorySource::DirectWitness;
    claimed.witnessed=true;
    claimed.minute=487;
    claimed.where="resource-node:303";
    claimed.what="model claimed this source failed";
    claimed.tags={"action_experience",key,"failure"};
    narrated.memory.add(claimed);
    assert(!residentAIHasRecentSourcedGatherFailure(
        narrated,MaterialKind::PlantFood,480,487));
    assert(!residentAIRecentGatherSourceFailed(
        narrated,MaterialKind::PlantFood,303,480,487));
    // A malicious or corrupted "direct" story at a previously successful
    // real source must not defeat the Core-verified success. The attempted
    // source ID and timestamp alone never make prose a physical outcome.
    MemoryRecord forgedAtA=claimed;
    forgedAtA.where="resource-node:301";
    forgedAtA.minute=488;
    narrated.memory.add(forgedAtA);
    assert(!residentAIRecentGatherSourceFailed(
        narrated,MaterialKind::PlantFood,301,480,488));
    // Even an exact Core failure phrase belongs to the actor who actually
    // executed it; a copied memory with an unrelated subject is not ours.
    MemoryRecord stolenAtA=claimed;
    stolenAtA.where="resource-node:301";
    stolenAtA.minute=489;
    stolenAtA.what="실제 문명 행동 "+key
        +" 수행을 시도했지만 실패했다 (직접 확인한 원인: access-unavailable)";
    stolenAtA.who=999;
    narrated.memory.add(stolenAtA);
    assert(!residentAIRecentGatherSourceFailed(
        narrated,MaterialKind::PlantFood,301,480,489));
    assert(!residentAIHasRecentSourcedGatherFailure(
        narrated,MaterialKind::PlantFood,480,489));
    const auto restoredSource=chooseResidentAIExistingCivilizationAffordance(
        sources,searching,camp,sourceThought);
    assert(restoredSource
        &&restoredSource->intent==CivilizationIntent::Gather
        &&restoredSource->resourceNode==301);

    // Actual food is STILL present. Core exploration pressure is zero; do
    // not invent a new frontier or block an accessible gather just because
    // prior attempts were bad.
    const auto stillAvailable=chooseResidentAIExistingCivilizationAffordance(
        world,resident,camp,thought);
    assert(stillAvailable &&stillAvailable->intent==CivilizationIntent::Gather);

    // Exhaustion of real nearby food changes the afforded strategy.
    world.resourceNodes.front().quantity=0;
    const auto unfiltered=bestResourceExplorationDecisionAtPosition(
        world,resident,camp);
    const auto explicitlyUnfiltered=bestResourceExplorationDecisionAtPosition(
        world,resident,camp,nullptr,MaterialKind::Unknown);
    assert(unfiltered.intent==explicitlyUnfiltered.intent);
    assert(unfiltered.material==explicitlyUnfiltered.material);
    assert(unfiltered.utility==explicitlyUnfiltered.utility);
    const auto alternative=bestResourceExplorationDecisionAtPosition(
        world,resident,camp,nullptr,MaterialKind::PlantFood);
    assert(alternative.intent==CivilizationIntent::Explore);
    assert(alternative.material==MaterialKind::PlantFood);
    const auto retry=chooseResidentAIExistingCivilizationAffordance(
        world,resident,camp,thought);
    assert(retry &&retry->intent==CivilizationIntent::Explore);
    assert(retry->material==MaterialKind::PlantFood);
    assert(retry->resourceNode==0);

    // A second resident has no private evidence. Hearsay alone does not
    // mutate their Core belief; they cannot acquire the first actor's bias.
    Character bystander=resident;
    bystander.id=10;
    bystander.civilization.character=10;
    bystander.memory=MemoryState{};
    bystander.beliefs=BeliefState{};
    assert(!residentAIHasRecentRepeatedGatherFailure(
        bystander,MaterialKind::PlantFood,world.minute));
    const auto noLearning=chooseResidentAIExistingCivilizationAffordance(
        world,bystander,camp,AcceptedCognitiveDecision{
            10,482,CognitiveTriggerKind::Reflection,1,"local",
            CognitiveIntentKind::ImproveFoodSecurity,0.9,0,"digest"});
    assert(!noLearning.has_value()); // depleted resource, no evidence

    // New firsthand success reverses the repetition penalty.
    rememberCivilizationAttemptOutcome(resident,gather,483,true);
    world.minute=483;
    thought.minute=483;
    assert(!residentAIHasRecentRepeatedGatherFailure(
        resident,MaterialKind::PlantFood,world.minute));

    // The action-specific trace is bounded even under persistent failure.
    // Crucially the last four true timestamps survive without unbounded
    // snapshot growth, and a recorded success is not mistaken for a failure.
    Character bounded=resident;
    for(int attempt=484;attempt<=490;++attempt)
        rememberCivilizationAttemptOutcome(bounded,gather,attempt,false);
    int retained=0;
    for(const auto& memory:bounded.memory.entries)
        if(memory.tags.size()==3 &&memory.tags[1]==key){
            ++retained;
            assert(memory.minute>=487 &&memory.minute<=490);
            assert(memory.sourceCharacter==bounded.id);
        }
    assert(retained==4);
    const auto beliefBeforeDuplicate=bounded.beliefs.find(
        0,actionExperienceProposition(key.c_str()))->contradictionCount;
    rememberCivilizationAttemptOutcome(bounded,gather,490,false);
    assert(bounded.beliefs.find(
        0,actionExperienceProposition(key.c_str()))->contradictionCount
        ==beliefBeforeDuplicate);
    assert(bounded.memory.entries.size()==4);

    // An old failure must not drive exploration forever.
    world.minute=1924;
    thought.minute=1924;
    assert(!residentAIHasRecentRepeatedGatherFailure(
        resident,MaterialKind::PlantFood,world.minute));

    // A forged/told-by-other failure without a matching firsthand memory
    // cannot trigger the alternate strategy even with negative belief.
    Character rumor=resident;
    for(auto& m:rumor.memory.entries)
        if(m.tags.size()==3 &&m.tags[2]=="failure"){
            m.source=MemorySource::ToldByOther;
            m.witnessed=false;
        }
    world.minute=483;
    assert(!residentAIHasRecentRepeatedGatherFailure(
        rumor,MaterialKind::PlantFood,world.minute));

    // Snapshot retains the actor's evidence without any new schema.
    Simulation simulation(1442);
    simulation.setupNewGame();
    simulation.runMinutes(3); // latest witnessed outcome is minute 483
    assert(simulation.world().minute==483);
    const auto id=simulation.world().characters.front().id;
    for(auto& c:simulation.world().characters)
        if(c.id==id){
            c.beliefs=resident.beliefs;
            c.memory=resident.memory;
            // The fixture's direct witness (id=9) becomes the actual
            // founder in this authoritative persistence scenario.
            for(auto& memory:c.memory.entries)
                if(memory.sourceCharacter==resident.id)
                    memory.sourceCharacter=id;
        }
    std::vector<std::uint8_t> bytes;
    std::string error;
    assert(encodeSimulationSnapshot(simulation.captureSnapshot(),bytes,&error));
    SimulationStateSnapshot restored;
    assert(decodeSimulationSnapshot(bytes,restored,&error));
    const Character* persisted=nullptr;
    for(const auto& c:restored.world.characters)
        if(c.id==id){persisted=&c;break;}
    assert(persisted);
    assert(persisted->beliefs.find(
        0,actionExperienceProposition(key.c_str()))!=nullptr);
    return 0;
}
