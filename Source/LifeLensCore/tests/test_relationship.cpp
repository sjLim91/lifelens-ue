#include <cassert>
#include <cmath>
#include "lifelens/Relationship.h"

using namespace lifelens;

static bool near(double a,double b,double eps=1e-9){return std::fabs(a-b)<=eps;}

int main()
{
    RelationshipBook book;
    Relationship& aToB=book.getOrCreate(10,20);
    Relationship& bToA=book.getOrCreate(20,10);

    assert(book.size()==2);
    assert(aToB.from==10 && aToB.to==20);
    assert(bToA.from==20 && bToA.to==10);
    assert(book.find(10,20)!=nullptr);
    assert(book.find(10,30)==nullptr);

    // Social/family interactions may create other relationships while a caller
    // holds both directional records. Growth must not invalidate either one.
    const Relationship* original=book.find(10,20);
    for(CharacterId id=100;id<1100;++id) book.getOrCreate(id,id+1);
    assert(&aToB==original);
    assert(book.find(10,20)==original);
    assert(aToB.from==10 && aToB.to==20);
    assert(bToA.from==20 && bToA.to==10);
    assert(&book.getOrCreate(10,20)==original);
    assert(book.size()==1002);
    assert(book.all().front().from==10 && book.all().back().from==1099);

    aToB.apply(relationshipDeltaFor(RelationshipEvent::SharedPositiveExperience));
    assert(aToB.affection>0.0);
    assert(aToB.comfort>0.0);
    assert(aToB.familiarity>0.0);
    assert(near(bToA.affection,0.0)); // directional state must stay independent

    aToB.apply(relationshipDeltaFor(RelationshipEvent::Intimacy,0.5));
    const double romanceBeforeBetrayal=aToB.romancePotential();
    assert(aToB.romanticInterest>0.0);
    assert(aToB.attraction>0.0);
    assert(aToB.sexualAttraction>0.0);

    aToB.trust=0.80;
    aToB.affection=0.75;
    aToB.apply(relationshipDeltaFor(RelationshipEvent::Betrayal));
    assert(aToB.trust<0.80);
    assert(aToB.affection<0.75);
    assert(aToB.conflict>0.0);
    assert(aToB.grudge>0.0);
    assert(aToB.romancePotential()<romanceBeforeBetrayal || aToB.conflict>0.0);

    const double conflictBeforeApology=aToB.conflict;
    const double grudgeBeforeApology=aToB.grudge;
    aToB.apply(relationshipDeltaFor(RelationshipEvent::Apology));
    assert(aToB.conflict<conflictBeforeApology);
    assert(aToB.grudge<grudgeBeforeApology);

    Relationship saturated;
    saturated.from=1; saturated.to=2;
    saturated.affection=0.99;
    saturated.trust=0.01;
    saturated.apply(RelationshipDelta{0.50,-0.50,0,0,0,0,0,0,0,0,0,0,0});
    assert(near(saturated.affection,1.0));
    assert(near(saturated.trust,0.0));

    return 0;
}
