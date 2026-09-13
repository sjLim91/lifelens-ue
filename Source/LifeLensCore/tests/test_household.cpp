#include <iostream>

#include "lifelens/Household.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character makeCharacter(CharacterId id,const char* name)
{
    Character c;
    c.id=id;
    c.name=name;
    c.personality.patience=0.72;
    c.personality.conscientiousness=0.70;
    return c;
}

static Relationship strongBond(CharacterId from,CharacterId to)
{
    Relationship r;
    r.from=from;
    r.to=to;
    r.affection=0.88;
    r.trust=0.86;
    r.respect=0.82;
    r.comfort=0.90;
    r.familiarity=0.88;
    r.commitment=0.72;
    r.conflict=0.03;
    r.fear=0.01;
    r.grudge=0.01;
    return r;
}

static CohabitationContext readyContext()
{
    CohabitationContext c;
    c.housingReadiness=0.92;
    c.financialReadiness=0.86;
    c.lifeGoalAlignment=0.90;
    c.scheduleCompatibility=0.84;
    c.currentHousingPressure=0.25;
    c.externalStress=0.05;
    return c;
}

int main()
{
    Character a=makeCharacter(1,"A");
    Character b=makeCharacter(2,"B");
    Relationship aToB=strongBond(a.id,b.id);
    Relationship bToA=strongBond(b.id,a.id);

    HouseholdBook households;
    CHECK(households.create(10,{a.id},101,250.0));
    CHECK(households.create(20,{b.id},202,400.0));
    CHECK(households.householdOf(a.id)!=nullptr);
    CHECK(households.householdOf(a.id)->id==10);
    CHECK(households.householdOf(b.id)!=nullptr);
    CHECK(households.householdOf(b.id)->id==20);

    // Preserve member responsibilities when moving into a newly formed shared household.
    Household* aOld=households.find(10);
    CHECK(aOld!=nullptr);
    CHECK(!aOld->members.empty());
    aOld->members[0].responsibilities.cooking=0.75;
    aOld->members[0].contributionWeight=0.65;

    CohabitationProposalOutcome accepted=applyCohabitationProposal(
        a,b,aToB,bToA,readyContext(),readyContext(),households,
        30,true,303,900.0);
    CHECK(accepted.result==CohabitationProposalResult::Accepted);
    CHECK(accepted.proposerScore>=0.62);
    CHECK(accepted.recipientScore>=0.58);

    const Household* shared=households.find(30);
    CHECK(shared!=nullptr);
    CHECK(shared->members.size()==2);
    CHECK(shared->contains(a.id));
    CHECK(shared->contains(b.id));
    CHECK(shared->home==303);
    CHECK(shared->sharedMoney==900.0);
    CHECK(households.find(10)==nullptr); // empty origin households are pruned
    CHECK(households.find(20)==nullptr);
    CHECK(households.householdOf(a.id)!=nullptr);
    CHECK(households.householdOf(a.id)->id==30);
    CHECK(households.householdOf(b.id)!=nullptr);
    CHECK(households.householdOf(b.id)->id==30);

    bool foundPreserved=false;
    for(const auto& member:shared->members){
        if(member.characterId==a.id){
            foundPreserved=true;
            CHECK(member.responsibilities.cooking==0.75);
            CHECK(member.contributionWeight==0.65);
        }
    }
    CHECK(foundPreserved);

    // A pair already sharing a household does not repeatedly propose cohabitation.
    CohabitationProposalOutcome already=evaluateCohabitationProposal(
        a,b,aToB,bToA,readyContext(),readyContext(),households,true);
    CHECK(already.result==CohabitationProposalResult::AlreadyCohabiting);

    // One household per character is enforced.
    CHECK(households.create(40,{3},404,100.0));
    CHECK(!households.addMember(40,a.id));
    CHECK(!households.create(50,{b.id},505,100.0));

    // Friends can also choose to cohabit; romance is a bonus, not a hard requirement.
    Character c=makeCharacter(3,"C");
    Character d=makeCharacter(4,"D");
    Relationship cToD=strongBond(c.id,d.id);
    Relationship dToC=strongBond(d.id,c.id);
    HouseholdBook friendHouseholds;
    CohabitationProposalOutcome friends=evaluateCohabitationProposal(
        c,d,cToD,dToC,readyContext(),readyContext(),friendHouseholds,false);
    CHECK(friends.result==CohabitationProposalResult::Accepted);

    // Recipient acceptance remains independent from proposer readiness.
    Relationship dToCWeak;
    dToCWeak.from=d.id;
    dToCWeak.to=c.id;
    dToCWeak.affection=0.15;
    dToCWeak.trust=0.18;
    dToCWeak.comfort=0.12;
    dToCWeak.familiarity=0.20;
    dToCWeak.commitment=0.05;
    dToCWeak.conflict=0.70;
    dToCWeak.fear=0.45;
    dToCWeak.grudge=0.35;

    HouseholdBook rejectedBook;
    const double trustBefore=cToD.trust;
    CohabitationProposalOutcome rejected=applyCohabitationProposal(
        c,d,cToD,dToCWeak,readyContext(),readyContext(),rejectedBook,
        60,false,606,500.0);
    CHECK(rejected.result==CohabitationProposalResult::Rejected);
    CHECK(rejected.proposerScore>=0.62);
    CHECK(rejected.recipientScore<0.58);
    CHECK(rejectedBook.find(60)==nullptr);
    CHECK(cToD.trust<=trustBefore);
    CHECK(c.emotion.sadness>0.0);

    return 0;
}
