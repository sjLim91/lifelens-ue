#include <cassert>
#include <cmath>
#include "lifelens/ResidentUnifiedAgency.h"

using namespace lifelens;

static Character resident(CharacterId id)
{
    Character c;
    c.id=id;
    c.civilization.character=id;
    c.alive=true;
    c.lifeStage=LifeStage::Adult;
    c.needs={0.10,0.10,0.10,0.10,0.10};
    return c;
}

int main()
{
    World world(993);
    world.minute=480;
    world.resourceNodes.clear();
    Character actor=resident(9),neighbor=resident(10);
    actor.personality.sociability=0.94;
    actor.personality.empathy=0.94;
    actor.personality.agreeableness=0.88;
    world.characters={actor,neighbor};
    RelationshipBook relations;
    auto& bond=relations.getOrCreate(9,10);
    bond.affection=0.88;
    bond.trust=0.88;
    bond.comfort=0.85;
    bond.familiarity=0.91;

    ResourceNode food;
    food.id=101;
    food.material=MaterialKind::PlantFood;
    food.quantity=12;
    food.maxQuantity=12;
    food.pos={0,0};
    world.resourceNodes.push_back(food);
    UnifiedUtilityDecision baseline;
    baseline.kind=UnifiedDecisionKind::Civilization;
    baseline.civilization.intent=CivilizationIntent::Explore;
    baseline.utility=0.42;

    const auto defaultDecision=chooseSingleResidentAgencyDecision(
        world,world.characters.front(),{0,0},relations,
        baseline,nullptr,nullptr,std::nullopt,false);
    assert(defaultDecision.authority==ResidentAgencyAuthority::EmbodiedFallback);
    assert(defaultDecision.decision.kind==baseline.kind);
    assert(defaultDecision.decision.civilization.intent==
           baseline.civilization.intent);

    AcceptedCognitiveDecision thought;
    thought.actor=9;
    thought.minute=480;
    thought.priority=0.85;
    thought.intent=CognitiveIntentKind::ImproveFoodSecurity;
    thought.contextFingerprint=884;
    const auto foodDecision=chooseSingleResidentAgencyDecision(
        world,world.characters.front(),{0,0},relations,
        baseline,&thought,nullptr,std::nullopt,false);
    assert(foodDecision.authority==ResidentAgencyAuthority::ResidentCognition);
    assert(foodDecision.cognitiveIntent==thought.intent);
    assert(foodDecision.contextFingerprint==884);
    assert(foodDecision.decision.kind==UnifiedDecisionKind::Civilization);
    assert(foodDecision.decision.civilization.intent==CivilizationIntent::Gather);
    assert(foodDecision.decision.civilization.resourceNode==101);

    // The exact SAME resident thought cannot also initiate an independent
    // social decision from a competing secondary model/loop.
    const auto same=chooseSingleResidentAgencyDecision(
        world,world.characters.front(),{0,0},relations,
        baseline,&thought,nullptr,GridPos{1,0},false);
    assert(same.decision.kind==UnifiedDecisionKind::Civilization);

    thought.intent=CognitiveIntentKind::CooperateWithResident;
    thought.target=10;
    const auto socialDecision=chooseSingleResidentAgencyDecision(
        world,world.characters.front(),{0,0},relations,
        baseline,&thought,nullptr,GridPos{1,0},false);
    assert(socialDecision.authority==ResidentAgencyAuthority::ResidentCognition);
    assert(socialDecision.decision.kind==UnifiedDecisionKind::Social);
    assert(socialDecision.decision.social.target==10);
    assert(socialDecision.decision.social.intent==SocialIntent::Approach);

    // Personal Core-completed self-mood is a REAL planning influence, not
    // merely an informational shadow proposal. Learn it in the SAME mind,
    // partner and actual approach lane, preserving Core physical authority.
    World welcomed=world;
    assert(rememberExperiencedSocialMoodOutcome(
        welcomed.characters[0],10,SocialIntent::Approach,479,0.10,0.18));
    const auto welcomedChoice=chooseSingleResidentAgencyDecision(
        welcomed,welcomed.characters.front(),{0,0},relations,
        baseline,&thought,nullptr,GridPos{1,0},false);
    assert(welcomedChoice.authority==ResidentAgencyAuthority::ResidentCognition);
    assert(welcomedChoice.decision.social.intent==SocialIntent::Approach);
    assert(welcomedChoice.decision.utility>socialDecision.decision.utility);

    World hurt=world;
    assert(rememberExperiencedSocialMoodOutcome(
        hurt.characters[0],10,SocialIntent::Approach,479,0.12,0.04));
    const auto hurtChoice=chooseSingleResidentAgencyDecision(
        hurt,hurt.characters.front(),{0,0},relations,
        baseline,&thought,nullptr,GridPos{1,0},false);
    assert(hurtChoice.authority==ResidentAgencyAuthority::ResidentCognition);
    assert(hurtChoice.decision.utility<socialDecision.decision.utility);
    assert(hurtChoice.decision.utility>0.0);
    assert(world.characters.front().beliefs.beliefs.empty());

    // A partner's own private mood cannot become the first resident's
    // firsthand learning. Reported memories alone cannot fabricate a
    // Core-proven self-mood belief either.
    World otherMind=world;
    assert(rememberExperiencedSocialMoodOutcome(
        otherMind.characters[1],9,SocialIntent::Approach,479,0.12,0.04));
    const auto independent=chooseSingleResidentAgencyDecision(
        otherMind,otherMind.characters.front(),{0,0},relations,
        baseline,&thought,nullptr,GridPos{1,0},false);
    assert(independent.decision.utility==socialDecision.decision.utility);
    World hearsayOnly=world;
    MemoryRecord rumour;
    rumour.who=10;
    rumour.minute=479;
    rumour.what="Someone said that meeting was harmful";
    rumour.source=MemorySource::ToldByOther;
    rumour.tags={"social_self_outcome","Approach","worsened","direct"};
    hearsayOnly.characters[0].memory.add(rumour);
    const auto rejectedRumour=chooseSingleResidentAgencyDecision(
        hearsayOnly,hearsayOnly.characters.front(),{0,0},relations,
        baseline,&thought,nullptr,GridPos{1,0},false);
    assert(rejectedRumour.decision.utility==socialDecision.decision.utility);

    // Genuinely urgent bodily survival outranks optional cognition while
    // the thinking identity remains that SAME resident.
    assert(chooseSingleResidentAgencyDecision(
        world,world.characters.front(),{0,0},relations,baseline,
        &thought,nullptr,GridPos{1,0},true).authority
        ==ResidentAgencyAuthority::EmbodiedFallback);

    auto comfort=baseline;
    comfort.kind=UnifiedDecisionKind::Social;
    comfort.social={SocialIntent::Comfort,10,0.92};
    const auto preserveCare=chooseSingleResidentAgencyDecision(
        world,world.characters.front(),{0,0},relations,comfort,
        &thought,nullptr,GridPos{1,0},false);
    assert(preserveCare.authority==ResidentAgencyAuthority::EmbodiedFallback);
    assert(preserveCare.decision.social.intent==SocialIntent::Comfort);

    auto avoid=comfort;
    avoid.social.intent=SocialIntent::Avoid;
    assert(chooseSingleResidentAgencyDecision(
        world,world.characters.front(),{0,0},relations,avoid,
        &thought,nullptr,GridPos{1,0},false).decision.social.intent
        ==SocialIntent::Avoid);

    assert(chooseSingleResidentAgencyDecision(
        world,world.characters.front(),{0,0},relations,baseline,
        &thought,nullptr,std::nullopt,false).authority
        ==ResidentAgencyAuthority::EmbodiedFallback);
    assert(chooseSingleResidentAgencyDecision(
        world,world.characters.front(),{0,0},relations,baseline,
        &thought,nullptr,GridPos{100000,0},false).authority
        ==ResidentAgencyAuthority::EmbodiedFallback);

    // Stale/implausible/foreign proposals have no authority. A "model"
    // never acquires food or moves a body without real Core execution.
    thought.minute=0;
    world.minute=ResidentAgencyGoalHorizonMinutes+1;
    assert(chooseSingleResidentAgencyDecision(
        world,world.characters.front(),{0,0},relations,baseline,
        &thought,nullptr,GridPos{1,0},false).authority
        ==ResidentAgencyAuthority::EmbodiedFallback);
    world.minute=480;
    thought.minute=480;
    // A single approved goal can still select a real social action hours
    // after cognition, without a second social/civilization decision engine.
    world.minute=1200;
    const auto continued=chooseSingleResidentAgencyDecision(
        world,world.characters.front(),{0,0},relations,baseline,
        &thought,nullptr,GridPos{1,0},false);
    assert(continued.authority==ResidentAgencyAuthority::ResidentCognition);
    assert(continued.decision.kind==UnifiedDecisionKind::Social);
    world.minute=480;
    // The single resident decision slot cannot be bypassed by a malformed
    // thought that would have been rejected by the accepted journal.
    thought.contextFingerprint=0;
    assert(chooseSingleResidentAgencyDecision(
        world,world.characters.front(),{0,0},relations,baseline,
        &thought,nullptr,GridPos{1,0},false).authority
        ==ResidentAgencyAuthority::EmbodiedFallback);
    thought.contextFingerprint=884;
    thought.priority=1.01;
    assert(chooseSingleResidentAgencyDecision(
        world,world.characters.front(),{0,0},relations,baseline,
        &thought,nullptr,GridPos{1,0},false).authority
        ==ResidentAgencyAuthority::EmbodiedFallback);
    thought.priority=0.85;
    thought.intent=CognitiveIntentKind::None;
    assert(chooseSingleResidentAgencyDecision(
        world,world.characters.front(),{0,0},relations,baseline,
        &thought,nullptr,GridPos{1,0},false).authority
        ==ResidentAgencyAuthority::EmbodiedFallback);
    thought.intent=CognitiveIntentKind::CooperateWithResident;
    thought.actor=100;
    assert(chooseSingleResidentAgencyDecision(
        world,world.characters.front(),{0,0},relations,baseline,
        &thought,nullptr,GridPos{1,0},false).authority
        ==ResidentAgencyAuthority::EmbodiedFallback);

    thought.actor=9;
    thought.priority=std::numeric_limits<double>::quiet_NaN();
    assert(chooseSingleResidentAgencyDecision(
        world,world.characters.front(),{0,0},relations,baseline,
        &thought,nullptr,GridPos{1,0},false).authority
        ==ResidentAgencyAuthority::EmbodiedFallback);

    world.characters.front().lifeStage=LifeStage::Baby;
    thought.priority=0.95;
    assert(chooseSingleResidentAgencyDecision(
        world,world.characters.front(),{0,0},relations,baseline,
        &thought,nullptr,GridPos{1,0},false).authority
        ==ResidentAgencyAuthority::EmbodiedFallback);
    world.characters.front().lifeStage=LifeStage::Adult;

    const auto rng=world.rng;
    const auto beforeItems=world.resourceNodes.front().quantity;
    const auto beforeNeed=world.characters.front().needs.hunger;
    for(int i=0;i<8;++i){
        const auto choice=chooseSingleResidentAgencyDecision(
            world,world.characters.front(),{0,0},relations,baseline,
            &thought,nullptr,GridPos{1,0},false);
        assert(choice.authority==ResidentAgencyAuthority::ResidentCognition);
    }
    assert(world.rng==rng);
    assert(world.resourceNodes.front().quantity==beforeItems);
    assert(world.characters.front().needs.hunger==beforeNeed);
    assert(world.minute==480);
    return 0;
}
