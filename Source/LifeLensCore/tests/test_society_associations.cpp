#include <cassert>
#include <iostream>

#include "lifelens/SocietyEconomy.h"

using namespace lifelens;

static Character resident(CharacterId id,LifeStage stage)
{
    Character c;
    c.id=id;
    c.name="Resident"+std::to_string(id);
    c.alive=true;
    c.lifeStage=stage;
    c.civilization.character=id;
    c.civilization.learningSkill=0.9;
    c.personality.patience=0.9;
    c.personality.empathy=0.8;
    c.personality.sociability=0.8;
    c.personality.curiosity=0.8;
    c.personality.openness=0.8;
    return c;
}

static void transmitFact(
    SocialKnowledgeBook& book,
    Character& teacher,
    Character& learner,
    SocialFactId factId,
    int minute,
    std::uint64_t seed)
{
    const auto statement=book.makeStatement(
        factId,teacher.id,minute,seed);
    assert(statement.has_value());
    const auto result=book.receiveStatement(
        *statement,
        learner.id,
        1.0,
        learner.memory,
        learner.beliefs,
        minute,
        seed);
    assert(result.result==StatementReceptionResult::Accepted);
}

int main()
{
    constexpr std::uint64_t seed=808080;

    Character teacher=resident(1,LifeStage::Adult);
    Character child=resident(2,LifeStage::Child);
    Character teen=resident(3,LifeStage::Teen);
    Character toddler=resident(4,LifeStage::Toddler);
    Character elder=resident(5,LifeStage::Elderly);

    assert(societyCanTeachTechnique(teacher));
    assert(societyCanTeachTechnique(elder));
    assert(!societyCanTeachTechnique(toddler));
    assert(societyCanLearnTechnique(child));
    assert(societyCanLearnTechnique(teen));
    assert(!societyCanLearnTechnique(toddler));
    assert(societyLearningRateMultiplier(child)
        >societyLearningRateMultiplier(teacher));

    teacher.civilization.knowledge.learn(
        TechniqueId::SharpFlake,KnowledgeLevel::Mastered,0.99);
    teacher.civilization.knowledge.learn(
        TechniqueId::FiberCordage,KnowledgeLevel::Mastered,0.99);

    SocialKnowledgeBook knowledge;
    const KnowledgeReceipt* sharpOrigin=registerTechniqueOrigin(
        knowledge,teacher,TechniqueId::SharpFlake,100,
        CivilizationEventType::Discovered,seed);
    assert(sharpOrigin!=nullptr);
    const SocialFactId sharpFactId=sharpOrigin->factId;

    const KnowledgeReceipt* cordageOrigin=registerTechniqueOrigin(
        knowledge,teacher,TechniqueId::FiberCordage,120,
        CivilizationEventType::Discovered,seed);
    assert(cordageOrigin!=nullptr);
    const SocialFactId cordageFactId=cordageOrigin->factId;

    transmitFact(
        knowledge,teacher,child,sharpFactId,200,seed);
    transmitFact(
        knowledge,teacher,child,cordageFactId,220,seed);
    assert(societyTeachingReceiptCountBetween(
        knowledge,teacher.id,child.id)==2);

    const SocialFact* apprenticeship=
        registerSocietyApprenticeshipIfQualified(
            knowledge,teacher,child,240,seed);
    assert(apprenticeship!=nullptr);
    assert(hasSocietyApprenticeship(
        knowledge,teacher.id,child.id));
    assert(knowledge.hasReceipt(
        teacher.id,apprenticeship->id));
    assert(knowledge.hasReceipt(
        child.id,apprenticeship->id));

    Character traderA=resident(10,LifeStage::Adult);
    Character traderB=resident(11,LifeStage::Adult);
    SocietyExchangePlan plan;
    plan.first=traderA.id;
    plan.second=traderB.id;
    plan.firstGives=MaterialKind::PlantFood;
    plan.secondGives=MaterialKind::Wood;
    plan.quantityEach=1;
    plan.score=0.90;

    assert(registerSocietyExchangeFact(
        knowledge,traderA,traderB,plan,300,seed)!=nullptr);
    assert(registerSocietyExchangeFact(
        knowledge,traderA,traderB,plan,360,seed)!=nullptr);
    assert(societyExchangePairCount(
        knowledge,traderA.id,traderB.id)==2);

    const SocialFact* partnership=
        registerSocietyTradePartnershipIfQualified(
            knowledge,traderA,traderB,380,seed);
    assert(partnership!=nullptr);
    assert(hasSocietyTradePartnership(
        knowledge,traderA.id,traderB.id));
    assert(knowledge.hasReceipt(
        traderA.id,partnership->id));
    assert(knowledge.hasReceipt(
        traderB.id,partnership->id));

    assert(registerSocietyInstitutionMembership(
        knowledge,teacher,
        SocietyInstitutionKind::LearningCircle,
        400,seed)!=nullptr);
    assert(registerSocietyInstitutionMembership(
        knowledge,child,
        SocietyInstitutionKind::LearningCircle,
        400,seed)!=nullptr);
    assert(residentInstitutionMember(
        knowledge,teacher.id,
        SocietyInstitutionKind::LearningCircle));
    assert(residentInstitutionMember(
        knowledge,child.id,
        SocietyInstitutionKind::LearningCircle));

    World world(seed);
    world.characters.clear();
    world.storageSites.clear();
    world.facilities.clear();
    world.characters={teacher,child,traderA,traderB};

    const SocietyWorldObservation observed=
        buildSocietyWorldObservation(world,knowledge);
    assert(observed.apprenticeshipCount==1);
    assert(observed.tradePartnershipCount==1);
    assert(observed.institutionMembershipCount==2);

    std::cout<<"society associations passed\n";
    return 0;
}
