#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "lifelens/AutonomousSocialGoalRouting.h"
#include "lifelens/DecisionExecution.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

using namespace lifelens;

int main()
{
    World world(84017);
    world.minute=450;
    world.characters.clear();
    Character actor;
    actor.id=11;
    actor.personality.sociability=1.0;
    actor.personality.empathy=1.0;
    actor.personality.introversion=0.0;
    actor.needs.hunger=0.20;
    actor.needs.thirst=0.20;
    actor.needs.sleep=0.20;
    actor.needs.bladder=0.20;
    actor.needs.hygiene=0.20;
    Character target;
    target.id=22;
    Character baselineTarget;
    baselineTarget.id=33;
    world.characters={actor,target,baselineTarget};

    RelationshipBook relations;
    auto& approach=relations.getOrCreate(11,22);
    approach.trust=0.10;
    approach.affection=0.75;
    approach.comfort=0.65;
    approach.respect=0.45;
    auto& fallback=relations.getOrCreate(11,33);
    fallback.trust=0.75;

    const auto originalRng=world.rng;
    const double beforeScore=scoreApproachIntent(
        world,world.characters[0],22,relations);
    assert(beforeScore>0.18 &&beforeScore<0.80);
    const SocialUtilityDecision baseline{
        SocialIntent::Approach,33,beforeScore+0.10
    };
    // Without PERSONAL experience, the original choice still wins.
    assert(!preferAutonomousSocialGoalTarget(
        world,relations,world.characters[0],baseline));

    const double beforeMood=world.characters[0].emotion.valence;
    const auto execution=executeSocialDecision(
        world,relations,11,{SocialIntent::Approach,22,beforeScore},
        "simulation");
    assert(execution.socialExecuted &&execution.generatedSocialEvent);
    const double afterMood=world.characters[0].emotion.valence;
    assert(afterMood>beforeMood);
    // Core changed the target's private relationship to the initiator, not
    // the initiator's supposed omniscient trust in the target.
    assert(std::abs(relations.find(11,22)->trust-0.10)<1e-12);
    assert(world.characters[0].memory.entries.empty());

    // Commit AFTER actual successful execution, from self emotion ONLY.
    assert(rememberExperiencedSocialMoodOutcome(
        world.characters[0],22,SocialIntent::Approach,
        world.minute,beforeMood,afterMood));
    const auto* belief=world.characters[0].beliefs.find(
        22,socialMoodExperienceProposition(SocialIntent::Approach));
    assert(belief && belief->supportCount==1 &&belief->contradictionCount==0);
    assert(learnedSocialMoodDisposition(
        world.characters[0],22,SocialIntent::Approach)>0.0);
    assert(world.characters[0].memory.entries.size()==1);
    const auto& memory=world.characters[0].memory.entries[0];
    assert(memory.who==22 &&memory.sourceCharacter==11);
    assert(memory.source==MemorySource::DirectWitness &&memory.witnessed);
    assert(memory.tags.size()==4 &&memory.tags[0]=="social_self_outcome");
    assert(world.characters[1].beliefs.find(
        22,socialMoodExperienceProposition(SocialIntent::Approach))==nullptr);

    // Experience now breaks a CLOSE existing social tie; it never creates
    // another social slot or authorizes a social action on its own.
    const auto preferred=preferAutonomousSocialGoalTarget(
        world,relations,world.characters[0],baseline);
    assert(preferred && preferred->target==22);

    const auto restoredChoice=preferAutonomousSocialGoalTarget(
        world,relations,world.characters[0],baseline);
    assert(restoredChoice &&restoredChoice->target==preferred->target);
    const double originalSignal=learnedSocialMoodDisposition(
        world.characters[0],22,SocialIntent::Approach);
    assert(!rememberExperiencedSocialMoodOutcome(
        world.characters[0],22,SocialIntent::Approach,
        world.minute,beforeMood,afterMood));
    assert(belief->supportCount==1);
    assert(learnedSocialMoodDisposition(
        world.characters[0],22,SocialIntent::Approach)==originalSignal);
    assert(!rememberExperiencedSocialMoodOutcome(
        world.characters[0],22,SocialIntent::Approach,
        world.minute,afterMood,afterMood));
    assert(!rememberExperiencedSocialMoodOutcome(
        world.characters[0],33,SocialIntent::Avoid,
        world.minute,beforeMood,afterMood));

    // Repeated actual positive observations refresh one bounded personal
    // memory instead of appending one record for every social interaction.
    for(int i=0;i<100;++i){
        ++world.minute;
        assert(rememberExperiencedSocialMoodOutcome(
            world.characters[0],22,SocialIntent::Approach,
            world.minute,0.00,0.03));
    }
    assert(world.characters[0].memory.entries.size()==1);
    assert(belief->supportWeight+belief->contradictionWeight<14.0);
    assert(world.rng==originalRng);

    // Negative direct experience is retained separately, not mislabelled
    // as a drop in the OTHER person's invisible relationship trust.
    Character other=world.characters[0];
    other.id=44;
    other.memory.entries.clear();
    other.beliefs.beliefs.clear();
    assert(rememberExperiencedSocialMoodOutcome(
        other,22,SocialIntent::Repair,world.minute,0.25,0.05));
    assert(learnedSocialMoodDisposition(other,22,SocialIntent::Repair)<0);
    assert(learnedSocialMoodDisposition(other,22,SocialIntent::Approach)==0.0);

    // The existing authoritative snapshot serializes the learned beliefs
    // and bounded memories, without adding a new version or pending field.
    Simulation simulation(51515);
    simulation.setupNewGame();
    const CharacterId residentId=simulation.world().characters.front().id;
    for(auto& resident:simulation.world().characters){
        if(resident.id!=residentId) continue;
        resident.memory=world.characters[0].memory;
        resident.beliefs=world.characters[0].beliefs;
    }
    std::vector<std::uint8_t> encoded;
    std::string error;
    assert(encodeSimulationSnapshot(
        simulation.captureSnapshot(),encoded,&error));
    SimulationStateSnapshot snapshot;
    assert(decodeSimulationSnapshot(encoded,snapshot,&error));
    const Character* fromSnapshot=nullptr;
    for(const auto& resident:snapshot.world.characters){
        if(resident.id==residentId) fromSnapshot=&resident;
    }
    assert(fromSnapshot!=nullptr);
    assert(fromSnapshot->memory.entries.size()==1);
    assert(std::abs(learnedSocialMoodDisposition(
        *fromSnapshot,22,SocialIntent::Approach)
        -learnedSocialMoodDisposition(
            world.characters[0],22,SocialIntent::Approach))<1e-12);

    // Urgent physical survival always defeats learned social preferences.
    world.characters[0].needs.hunger=0.99;
    world.minute=465;
    assert(!preferAutonomousSocialGoalTarget(
        world,relations,world.characters[0],baseline));
    return 0;
}
