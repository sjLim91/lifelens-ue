#include <iostream>

#include "lifelens/Marriage.h"

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
    c.personality.conscientiousness=0.82;
    c.personality.patience=0.78;
    c.emotion.joy=0.30;
    c.emotion.affection=0.35;
    c.emotion.refreshSummary();
    return c;
}

static Relationship strongMarriageBond(CharacterId from,CharacterId to)
{
    Relationship r;
    r.from=from;
    r.to=to;
    r.affection=0.94;
    r.trust=0.92;
    r.respect=0.88;
    r.comfort=0.91;
    r.familiarity=0.93;
    r.attraction=0.86;
    r.romanticInterest=0.91;
    r.sexualAttraction=0.82;
    r.commitment=0.90;
    r.conflict=0.02;
    r.jealousy=0.03;
    r.fear=0.01;
    r.grudge=0.01;
    return r;
}

static MarriageContext readyContext()
{
    MarriageContext c;
    c.marriageIntent=0.94;
    c.lifeGoalAlignment=0.91;
    c.householdStability=0.86;
    c.financialReadiness=0.82;
    c.relationshipDuration=0.88;
    c.externalStress=0.04;
    c.available=true;
    c.mergeHouseholdsOnMarriage=true;
    return c;
}

int main()
{
    Character a=makeCharacter(1,"A");
    Character b=makeCharacter(2,"B");
    Relationship aToB=strongMarriageBond(a.id,b.id);
    Relationship bToA=strongMarriageBond(b.id,a.id);

    RomanceBook romances;
    CHECK(romances.startDating(a.id,b.id,a.id,100));

    EngagementProposalOutcome engaged=applyEngagementProposal(
        a,b,aToB,bToA,readyContext(),readyContext(),romances,1000);
    CHECK(engaged.result==EngagementProposalResult::Accepted);
    CHECK(engaged.proposerScore>=0.68);
    CHECK(engaged.recipientScore>=0.64);
    const RomancePair* pair=romances.findLatest(a.id,b.id);
    CHECK(pair!=nullptr);
    CHECK(pair->stage==RomanceStage::Engaged);
    CHECK(pair->engagedMinute==1000);
    CHECK(!romances.isAvailable(a.id));
    CHECK(!romances.isAvailable(b.id));

    HouseholdBook households;
    CHECK(households.create(10,{a.id},101,300.0));
    CHECK(households.create(20,{b.id},202,450.0));

    MarriageDecisionOutcome married=applyMarriageDecision(
        a,b,aToB,bToA,readyContext(),readyContext(),romances,households,
        2000,30,303,1200.0);
    CHECK(married.result==MarriageDecisionResult::Married);
    CHECK(married.firstScore>=0.72);
    CHECK(married.secondScore>=0.68);
    pair=romances.findLatest(a.id,b.id);
    CHECK(pair!=nullptr);
    CHECK(pair->stage==RomanceStage::Married);
    CHECK(pair->marriedMinute==2000);
    CHECK(households.find(10)==nullptr);
    CHECK(households.find(20)==nullptr);
    const Household* marriedHome=households.find(30);
    CHECK(marriedHome!=nullptr);
    CHECK(marriedHome->contains(a.id));
    CHECK(marriedHome->contains(b.id));
    CHECK(marriedHome->home==303);
    CHECK(marriedHome->sharedMoney==1200.0);

    // Separation preserves the legal relationship slot; divorce releases it.
    CHECK(romances.separate(a.id,b.id,3000));
    pair=romances.findLatest(a.id,b.id);
    CHECK(pair!=nullptr);
    CHECK(pair->stage==RomanceStage::Separated);
    CHECK(!romances.isAvailable(a.id));
    CHECK(!romances.isAvailable(b.id));
    CHECK(romances.divorce(a.id,b.id,3600));
    pair=romances.findLatest(a.id,b.id);
    CHECK(pair!=nullptr);
    CHECK(pair->stage==RomanceStage::Divorced);
    CHECK(pair->endedMinute==3600);
    CHECK(romances.isAvailable(a.id));
    CHECK(romances.isAvailable(b.id));

    // A willing proposer does not force engagement if the recipient is not ready.
    Character c=makeCharacter(3,"C");
    Character d=makeCharacter(4,"D");
    Relationship cToD=strongMarriageBond(c.id,d.id);
    Relationship dToC;
    dToC.from=d.id;
    dToC.to=c.id;
    dToC.affection=0.20;
    dToC.trust=0.18;
    dToC.comfort=0.16;
    dToC.familiarity=0.30;
    dToC.attraction=0.22;
    dToC.romanticInterest=0.18;
    dToC.commitment=0.12;
    dToC.conflict=0.65;
    dToC.fear=0.35;
    dToC.grudge=0.30;

    RomanceBook rejectionBook;
    CHECK(rejectionBook.startDating(c.id,d.id,c.id,100));
    const double proposerInterestBefore=cToD.romanticInterest;
    EngagementProposalOutcome rejected=applyEngagementProposal(
        c,d,cToD,dToC,readyContext(),readyContext(),rejectionBook,900);
    CHECK(rejected.result==EngagementProposalResult::Rejected);
    CHECK(rejected.proposerScore>=0.68);
    CHECK(rejected.recipientScore<0.64);
    const RomancePair* rejectedPair=rejectionBook.findLatest(c.id,d.id);
    CHECK(rejectedPair!=nullptr);
    CHECK(rejectedPair->stage==RomanceStage::Dating);
    CHECK(cToD.romanticInterest<proposerInterestBefore);
    CHECK(c.emotion.sadness>0.0);

    // Marriage can be deferred independently even after engagement.
    Character e=makeCharacter(5,"E");
    Character f=makeCharacter(6,"F");
    Relationship eToF=strongMarriageBond(e.id,f.id);
    Relationship fToE=strongMarriageBond(f.id,e.id);
    RomanceBook deferredBook;
    CHECK(deferredBook.startDating(e.id,f.id,e.id,10));
    CHECK(deferredBook.engage(e.id,f.id,20));
    MarriageContext notReady=readyContext();
    notReady.marriageIntent=0.05;
    notReady.lifeGoalAlignment=0.10;
    notReady.householdStability=0.10;
    notReady.financialReadiness=0.10;
    notReady.relationshipDuration=0.15;
    fToE.commitment=0.30;
    fToE.trust=0.45;
    fToE.conflict=0.45;
    MarriageDecisionOutcome deferred=evaluateMarriageDecision(
        e,f,eToF,fToE,readyContext(),notReady,deferredBook);
    CHECK(deferred.result==MarriageDecisionResult::Deferred);
    CHECK(deferredBook.findLatest(e.id,f.id)->stage==RomanceStage::Engaged);

    // Widowhood closes the active marriage and restores future availability.
    Character g=makeCharacter(7,"G");
    Character h=makeCharacter(8,"H");
    RomanceBook widowBook;
    CHECK(widowBook.startDating(g.id,h.id,g.id,10));
    CHECK(widowBook.engage(g.id,h.id,20));
    CHECK(widowBook.marry(g.id,h.id,30));
    CHECK(widowBook.markWidowed(g.id,h.id,4000));
    const RomancePair* widowPair=widowBook.findLatest(g.id,h.id);
    CHECK(widowPair!=nullptr);
    CHECK(widowPair->stage==RomanceStage::Widowed);
    CHECK(widowPair->endedMinute==4000);
    CHECK(widowBook.isAvailable(g.id));
    CHECK(widowBook.isAvailable(h.id));

    return 0;
}
