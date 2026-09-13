#include <cassert>

#include "lifelens/Household.h"

using namespace lifelens;

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
    assert(households.create(10,{a.id},101,250.0));
    assert(households.create(20,{b.id},202,400.0));
    assert(households.householdOf(a.id)->id==10);
    assert(households.householdOf(b.id)->id==20);

    // Preserve member responsibilities when moving into a newly formed shared household.
    Household* aOld=households.find(10);
    assert(aOld!=nullptr);
    aOld->members[0].responsibilities.cooking=0.75;
    aOld->members[0].contributionWeight=0.65;

    CohabitationProposalOutcome accepted=applyCohabitationProposal(
        a,b,aToB,bToA,readyContext(),readyContext(),households,
        30,true,303,900.0);
    assert(accepted.result==CohabitationProposalResult::Accepted);
    assert(accepted.proposerScore>=0.62);
    assert(accepted.recipientScore>=0.58);

    const Household* shared=households.find(30);
    assert(shared!=nullptr);
    assert(shared->members.size()==2);
    assert(shared->contains(a.id));
    assert(shared->contains(b.id));
    assert(shared->home==303);
    assert(shared->sharedMoney==900.0);
    assert(households.find(10)==nullptr); // empty origin households are pruned
    assert(households.find(20)==nullptr);
    assert(households.householdOf(a.id)->id==30);
    assert(households.householdOf(b.id)->id==30);

    bool foundPreserved=false;
    for(const auto& member:shared->members){
        if(member.characterId==a.id){
            foundPreserved=true;
            assert(member.responsibilities.cooking==0.75);
            assert(member.contributionWeight==0.65);
        }
    }
    assert(foundPreserved);

    // A pair already sharing a household does not repeatedly propose cohabitation.
    CohabitationProposalOutcome already=evaluateCohabitationProposal(
        a,b,aToB,bToA,readyContext(),readyContext(),households,true);
    assert(already.result==CohabitationProposalResult::AlreadyCohabiting);

    // One household per character is enforced.
    assert(households.create(40,{3},404,100.0));
    assert(!households.addMember(40,a.id));
    assert(!households.create(50,{b.id},505,100.0));

    // Friends can also choose to cohabit; romance is a bonus, not a hard requirement.
    Character c=makeCharacter(3,"C");
    Character d=makeCharacter(4,"D");
    Relationship cToD=strongBond(c.id,d.id);
    Relationship dToC=strongBond(d.id,c.id);
    HouseholdBook friendHouseholds;
    CohabitationProposalOutcome friends=evaluateCohabitationProposal(
        c,d,cToD,dToC,readyContext(),readyContext(),friendHouseholds,false);
    assert(friends.result==CohabitationProposalResult::Accepted);

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
    assert(rejected.result==CohabitationProposalResult::Rejected);
    assert(rejected.proposerScore>=0.62);
    assert(rejected.recipientScore<0.58);
    assert(rejectedBook.find(60)==nullptr);
    assert(cToD.trust<=trustBefore);
    assert(c.emotion.sadness>0.0);

    return 0;
}
