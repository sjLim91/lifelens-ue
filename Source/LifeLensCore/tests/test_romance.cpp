#include <cassert>

#include "lifelens/Romance.h"

using namespace lifelens;

static Character makeCharacter(CharacterId id,const char* name)
{
    Character c;
    c.id=id;
    c.name=name;
    c.personality.riskTolerance=0.65;
    c.emotion.joy=0.30;
    c.emotion.affection=0.25;
    c.emotion.refreshSummary();
    return c;
}

static Relationship strongRomance(CharacterId from,CharacterId to)
{
    Relationship r;
    r.from=from;
    r.to=to;
    r.affection=0.88;
    r.trust=0.84;
    r.respect=0.75;
    r.comfort=0.82;
    r.familiarity=0.86;
    r.attraction=0.92;
    r.romanticInterest=0.91;
    r.sexualAttraction=0.80;
    r.commitment=0.45;
    r.conflict=0.03;
    r.jealousy=0.04;
    r.fear=0.01;
    r.grudge=0.01;
    return r;
}

static RomanceContext positiveContext()
{
    RomanceContext c;
    c.personalityCompatibility=0.90;
    c.sharedExperience=0.80;
    c.lifeGoalAlignment=0.85;
    c.pastRelationshipPenalty=0.05;
    c.available=true;
    return c;
}

int main()
{
    Character a=makeCharacter(1,"A");
    Character b=makeCharacter(2,"B");
    Character c=makeCharacter(3,"C");

    Relationship aToB=strongRomance(a.id,b.id);
    Relationship bToA=strongRomance(b.id,a.id);
    RomanceBook book;

    // Strong mutual state must allow a proposal, but the recipient is evaluated independently.
    DatingProposalOutcome accepted=applyDatingProposal(
        a,b,aToB,bToA,positiveContext(),positiveContext(),book,120);
    assert(accepted.result==DatingProposalResult::Accepted);
    assert(accepted.proposerScore>=0.64);
    assert(accepted.recipientScore>=0.60);
    assert(book.activeFor(a.id)!=nullptr);
    assert(book.activeFor(a.id)->partnerOf(a.id)==b.id);
    assert(book.activeFor(b.id)->partnerOf(b.id)==a.id);
    assert(book.findLatest(a.id,b.id)!=nullptr);
    assert(book.findLatest(b.id,a.id)!=nullptr);
    assert(aToB.commitment>0.45);
    assert(bToA.commitment>0.45);
    assert(a.emotion.affection>0.25);
    assert(b.emotion.affection>0.25);

    // Active partners are unavailable for another dating proposal.
    Relationship cToA=strongRomance(c.id,a.id);
    Relationship aToC=strongRomance(a.id,c.id);
    DatingProposalOutcome unavailable=evaluateDatingProposal(
        c,a,cToA,aToC,positiveContext(),positiveContext(),book);
    assert(unavailable.result==DatingProposalResult::Unavailable);

    // Ending a relationship keeps history but restores availability.
    assert(book.endDating(a.id,b.id,500));
    assert(book.activeFor(a.id)==nullptr);
    assert(book.activeFor(b.id)==nullptr);
    assert(book.isAvailable(a.id));
    const RomancePair* former=book.findLatest(a.id,b.id);
    assert(former!=nullptr);
    assert(former->stage==RomanceStage::FormerPartners);
    assert(former->startedMinute==120);
    assert(former->endedMinute==500);

    // One-sided attraction does not force dating: the recipient can reject independently.
    Character d=makeCharacter(4,"D");
    Relationship cToD=strongRomance(c.id,d.id);
    Relationship dToC;
    dToC.from=d.id;
    dToC.to=c.id;
    dToC.affection=0.18;
    dToC.trust=0.24;
    dToC.comfort=0.20;
    dToC.familiarity=0.22;
    dToC.attraction=0.12;
    dToC.romanticInterest=0.10;
    dToC.conflict=0.48;
    dToC.fear=0.25;
    dToC.grudge=0.18;

    const double proposerInterestBefore=cToD.romanticInterest;
    DatingProposalOutcome rejected=applyDatingProposal(
        c,d,cToD,dToC,positiveContext(),positiveContext(),book,700);
    assert(rejected.result==DatingProposalResult::Rejected);
    assert(rejected.proposerScore>=0.64);
    assert(rejected.recipientScore<0.60);
    assert(book.findLatest(c.id,d.id)==nullptr);
    assert(cToD.romanticInterest<proposerInterestBefore);
    assert(c.emotion.sadness>0.0);

    // High attraction alone is insufficient when trust/safety are poor.
    Relationship unsafe=strongRomance(a.id,c.id);
    unsafe.trust=0.08;
    unsafe.affection=0.25;
    unsafe.comfort=0.10;
    unsafe.familiarity=0.20;
    unsafe.conflict=0.90;
    unsafe.fear=0.85;
    unsafe.grudge=0.70;
    RomanceEvaluation unsafeEval=evaluateRomanceInterest(a,unsafe,positiveContext(),0.64);
    assert(!unsafeEval.ready);

    // Invalid self proposals are rejected without creating state.
    DatingProposalOutcome invalid=evaluateDatingProposal(
        a,a,aToB,aToB,positiveContext(),positiveContext(),book);
    assert(invalid.result==DatingProposalResult::Invalid);

    return 0;
}
