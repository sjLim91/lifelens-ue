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
