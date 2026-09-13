#include <algorithm>
#include <iostream>
#include <random>

#include "lifelens/Birth.h"
#include "lifelens/Genealogy.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static bool hasId(const std::vector<CharacterId>& ids,CharacterId id)
{
    return std::find(ids.begin(),ids.end(),id)!=ids.end();
}

int main()
{
    GenealogyBook family;

    CHECK(family.registerBirth(3,1,2));
    CHECK(family.registerBirth(4,1,2));
    CHECK(family.registerBirth(5,1,6));
    CHECK(family.registerBirth(7,3,8));

    CHECK(family.relationBetween(1,3)==KinshipType::Parent);
    CHECK(family.relationBetween(3,1)==KinshipType::Child);
    CHECK(family.relationBetween(3,4)==KinshipType::Sibling);
    CHECK(family.relationBetween(3,5)==KinshipType::HalfSibling);
    CHECK(family.relationBetween(1,7)==KinshipType::Grandparent);
    CHECK(family.relationBetween(7,1)==KinshipType::Grandchild);

    CHECK(family.linkSpouses(3,9));
    CHECK(family.relationBetween(3,9)==KinshipType::Spouse);
    CHECK(family.relationBetween(9,1)==KinshipType::InLaw);
    CHECK(family.unlinkSpouses(3,9));
    CHECK(family.relationBetween(3,9)==KinshipType::Unrelated);

    const std::vector<CharacterId> ancestors=family.ancestors(7,4);
    CHECK(hasId(ancestors,3));
    CHECK(hasId(ancestors,1));
    CHECK(hasId(ancestors,2));
    const std::vector<CharacterId> descendants=family.descendants(1,4);
    CHECK(hasId(descendants,3));
    CHECK(hasId(descendants,4));
    CHECK(hasId(descendants,5));
    CHECK(hasId(descendants,7));

    // Conflicting parentage for an existing character is rejected.
    CHECK(!family.canRegisterBirth(3,10,11));
    CHECK(!family.registerBirth(3,10,11));

    // A real successful birth can synchronize genealogy in the same operation.
    Character parentA;
    parentA.id=100;
    parentA.name="A";
    parentA.lifeStage=LifeStage::Adult;
    Character parentB;
    parentB.id=101;
    parentB.name="B";
    parentB.lifeStage=LifeStage::Adult;

    PregnancyBook pregnancies;
    PregnancyState* pregnancy=pregnancies.start(parentA.id,parentB.id,0);
    CHECK(pregnancy!=nullptr);
    const int due=pregnancy->dueMinute;

    HouseholdBook households;
    CHECK(households.create(500,{parentA.id,parentB.id},900,1000.0));
    BirthBook births;
    GenealogyBook birthFamily;
    std::mt19937_64 rng(42);

    BirthOutcome born=performBirth(
        parentA,parentB,102,"Baby",pregnancies,households,births,rng,due,0.08,&birthFamily);
    CHECK(born.result==BirthResult::Success);
    CHECK(birthFamily.relationBetween(parentA.id,born.child.id)==KinshipType::Parent);
    CHECK(birthFamily.relationBetween(born.child.id,parentB.id)==KinshipType::Child);
    CHECK(hasId(parentA.childrenIds,born.child.id));
    CHECK(hasId(parentB.childrenIds,born.child.id));
    CHECK(households.householdOf(born.child.id)!=nullptr);

    // Pre-existing contradictory genealogy blocks the birth before mutation.
    Character otherA;
    otherA.id=200;
    Character otherB;
    otherB.id=201;
    PregnancyBook conflictingPregnancies;
    PregnancyState* conflicting=conflictingPregnancies.start(otherA.id,otherB.id,0);
    CHECK(conflicting!=nullptr);
    GenealogyBook conflictingFamily;
    CHECK(conflictingFamily.registerBirth(202,300,301));
    BirthBook conflictingBirths;
    HouseholdBook conflictingHouseholds;
    std::mt19937_64 rng2(7);
    BirthOutcome blocked=performBirth(
        otherA,otherB,202,"Conflict",conflictingPregnancies,conflictingHouseholds,
        conflictingBirths,rng2,conflicting->dueMinute,0.08,&conflictingFamily);
    CHECK(blocked.result==BirthResult::GenealogyConflict);
    CHECK(conflictingBirths.find(202)==nullptr);
    CHECK(otherA.childrenIds.empty());
    CHECK(otherB.childrenIds.empty());

    return 0;
}
