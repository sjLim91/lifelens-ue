#include <algorithm>
#include <iostream>
#include <string>

#include "lifelens/LifeStage.h"
#include "lifelens/Simulation.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character* findCharacter(Simulation& sim,CharacterId id)
{
    for(auto& character:sim.world().characters) if(character.id==id) return &character;
    return nullptr;
}

static int totalResourceQuantity(const World& world)
{
    int total=0;
    for(const auto& node:world.resourceNodes) total+=node.quantity;
    return total;
}

static int totalInventoryUnits(const Character& character)
{
    return inventoryUnitCount(character.civilization.inventory);
}

int main()
{
    // Social intent is visible while pending, but the actual social outcome is
    // not applied until the matching token is acknowledged near the target.
    {
        Simulation sim(777);
        sim.setupSocialDemo();
        sim.world().resourceNodes.clear();
        sim.world().storageSites.clear();
        for(auto& character:sim.world().characters){
            character.needs={0.05,0.05,0.05,0.05,0.05};
        }
        sim.setExternalPhysicalExecution(true);

        PendingContextActionObservation pending;
        for(int minute=0;minute<10 && !pending.active;++minute){
            sim.step();
            for(const auto& resident:sim.world().characters){
                const auto candidate=sim.observePendingContextAction(resident.id);
                if(candidate.active && candidate.kind==ContextActionKind::Social){
                    pending=candidate;
                    break;
                }
            }
        }
        CHECK(pending.active);
        CHECK(pending.token!=0);
        CHECK(pending.targetResident!=0);

        Character* actor=findCharacter(sim,pending.actor);
        Character* target=findCharacter(sim,pending.targetResident);
        CHECK(actor!=nullptr && target!=nullptr);
        const std::size_t actorMemoryBefore=actor->memory.entries.size();
        const std::size_t targetMemoryBefore=target->memory.entries.size();
        const EmotionState actorEmotionBefore=actor->emotion;

        GridPos targetPos{};
        CHECK(sim.runtimePosition(pending.targetResident,targetPos));
        GridPos completionPos=targetPos;
        if(pending.socialIntent==SocialIntent::Avoid) completionPos.x+=1;

        CHECK(!sim.completeExternalContextAction(
            pending.actor,pending.token+1,completionPos));
        CHECK(sim.observePendingContextAction(pending.actor).active);
        CHECK(actor->memory.entries.size()==actorMemoryBefore);
        CHECK(target->memory.entries.size()==targetMemoryBefore);

        // Unfinished authoritative movement/context must survive save/restore.
        const SimulationStateSnapshot pendingSnapshot=sim.captureSnapshot();
        Simulation restored(1);
        std::string restoreError;
        CHECK(restored.restoreSnapshot(pendingSnapshot,&restoreError));
        CHECK(restoreError.empty());
        const auto restoredPending=
            restored.observePendingContextAction(pending.actor);
        CHECK(restoredPending.active);
        CHECK(restoredPending.token==pending.token);
        CHECK(restoredPending.kind==pending.kind);

        CHECK(sim.completeExternalContextAction(
            pending.actor,pending.token,completionPos));
        CHECK(!sim.observePendingContextAction(pending.actor).active);
        if(pending.socialIntent==SocialIntent::Avoid){
            const bool emotionChanged=
                actor->emotion.fear!=actorEmotionBefore.fear
                || actor->emotion.anxiety!=actorEmotionBefore.anxiety
                || actor->emotion.relief!=actorEmotionBefore.relief;
            CHECK(emotionChanged);
        }else{
            CHECK(target->memory.entries.size()>targetMemoryBefore);
        }
    }

    // Gather owns a real Core target. Resource/inventory mutation occurs only
    // after arrival ACK, never at decision time.
    {
        Simulation sim(4242001);
        sim.setupNewGame();
        sim.setExternalPhysicalExecution(true);
        CHECK(!sim.world().resourceNodes.empty());

        Character& actor=sim.world().characters.front();
        actor.needs={0.05,0.05,0.05,0.05,0.05};
        actor.personality.curiosity=1.0;
        actor.personality.openness=1.0;
        actor.personality.adaptability=1.0;
        actor.personality.patience=1.0;
        actor.civilization.gatheringSkill=1.0;
        for(std::size_t i=1;i<sim.world().characters.size();++i){
            sim.world().characters[i].alive=false;
            sim.world().characters[i].deathMinute=sim.world().minute;
        }

        const int resourcesBefore=totalResourceQuantity(sim.world());
        const int inventoryBefore=totalInventoryUnits(actor);

        sim.step();
        const PendingContextActionObservation pending=
            sim.observePendingContextAction(actor.id);
        CHECK(pending.active);
        CHECK(pending.kind==ContextActionKind::Civilization);
        CHECK(pending.civilizationIntent==CivilizationIntent::Gather);
        CHECK(pending.resourceNode!=0);
        CHECK(pending.hasSpatialTarget);
        CHECK(totalResourceQuantity(sim.world())==resourcesBefore);
        CHECK(totalInventoryUnits(actor)==inventoryBefore);

        GridPos wrong=pending.targetPos;
        wrong.x+=3;
        CHECK(!sim.completeExternalContextAction(actor.id,pending.token,wrong));
        CHECK(sim.observePendingContextAction(actor.id).active);
        CHECK(totalResourceQuantity(sim.world())==resourcesBefore);
        CHECK(totalInventoryUnits(actor)==inventoryBefore);

        CHECK(sim.completeExternalContextAction(
            actor.id,pending.token,pending.targetPos));
        CHECK(!sim.observePendingContextAction(actor.id).active);
        CHECK(totalResourceQuantity(sim.world())<resourcesBefore);
        CHECK(totalInventoryUnits(actor)>inventoryBefore);
    }

    // Direct-care children wait for a real caregiver interaction. ToiletAssist
    // changes bladder state and deposits waste only after the caregiver arrives.
    {
        Simulation sim(20202);
        sim.setupNewGame();
        SimulationStateSnapshot snapshot=sim.captureSnapshot();
        const CharacterId parentA=snapshot.world.characters[0].id;
        const CharacterId parentB=snapshot.world.characters[2].id;

        Character child;
        child.id=99;
        child.name="ContextBaby";
        child.sex=Sex::Female;
        child.hasBirthMinute=true;
        child.birthMinute=snapshot.world.minute;
        child.lifeStage=LifeStage::Baby;
        child.parentIds={parentA,parentB};
        child.civilization.character=child.id;
        child.needs={0.01,0.01,0.01,0.99,0.01};
        child.development.attachment=0.90;
        child.development.confidence=0.80;
        child.development.socialSkill=0.20;
        child.development.emotionalSecurity=0.90;
        child.development.health=1.0;
        applyLifeStageProfile(child,LifeStage::Baby);
        snapshot.world.characters.push_back(child);

        SimulationRuntimeSnapshot childRuntime;
        childRuntime.pos=snapshot.runtime[parentB].pos;
        snapshot.runtime.emplace(child.id,childRuntime);
        CHECK(snapshot.genealogy.registerBirth(child.id,parentA,parentB));
        for(CharacterId parent:{parentA,parentB}){
            Relationship& toChild=snapshot.relationships.getOrCreate(parent,child.id);
            toChild.affection=0.95;
            toChild.commitment=0.95;
            toChild.comfort=0.95;
            Relationship& toParent=snapshot.relationships.getOrCreate(child.id,parent);
            toParent.affection=0.95;
            toParent.comfort=0.95;
        }

        std::string error;
        CHECK(sim.restoreSnapshot(snapshot,&error));
        CHECK(error.empty());
        sim.setExternalPhysicalExecution(true);
        const double bladderBefore=findCharacter(sim,child.id)->needs.bladder;
        const std::size_t residueBefore=sim.world().environmentalResidues.all().size();

        sim.step();
        PendingContextActionObservation pending;
        for(CharacterId parent:{parentA,parentB}){
            const auto candidate=sim.observePendingContextAction(parent);
            if(candidate.active && candidate.kind==ContextActionKind::Parenting){
                pending=candidate;
                break;
            }
        }
        CHECK(pending.active);
        CHECK(pending.parentingAction==ParentingAction::ToiletAssist);
        CHECK(findCharacter(sim,child.id)->needs.bladder>=bladderBefore);
        CHECK(sim.world().environmentalResidues.all().size()==residueBefore);

        GridPos childPos{};
        CHECK(sim.runtimePosition(child.id,childPos));
        CHECK(sim.completeExternalContextAction(
            pending.actor,pending.token,childPos));
        CHECK(findCharacter(sim,child.id)->needs.bladder<bladderBefore);
        CHECK(sim.world().environmentalResidues.all().size()==residueBefore+1);
        CHECK(!sim.observePendingContextAction(pending.actor).active);
    }

    std::cout << "context action completion acknowledgement passed\n";
    return 0;
}
