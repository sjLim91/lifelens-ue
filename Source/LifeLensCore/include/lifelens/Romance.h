#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "Character.h"
#include "Ids.h"
#include "Relationship.h"

namespace lifelens {

inline double clampRomance(double value)
{
    return std::max(0.0, std::min(1.0, value));
}

enum class RomanceStage {
    Dating,
    FormerPartners
};

struct RomancePair {
    CharacterId first=0;
    CharacterId second=0;
    CharacterId initiator=0;
    RomanceStage stage=RomanceStage::Dating;
    int startedMinute=0;
    int endedMinute=-1;

    bool contains(CharacterId id) const
    {
        return first==id || second==id;
    }

    bool matches(CharacterId a,CharacterId b) const
    {
        return (first==a && second==b) || (first==b && second==a);
    }

    CharacterId partnerOf(CharacterId id) const
    {
        if(first==id) return second;
        if(second==id) return first;
        return 0;
    }

    bool active() const
    {
        return stage==RomanceStage::Dating;
    }
};

class RomanceBook {
public:
    bool isAvailable(CharacterId id) const
    {
        if(id==0) return false;
        return activeFor(id)==nullptr;
    }

    const RomancePair* activeFor(CharacterId id) const
    {
        for(auto it=items_.rbegin();it!=items_.rend();++it){
            if(it->active() && it->contains(id)) return &(*it);
        }
        return nullptr;
    }

    const RomancePair* findLatest(CharacterId a,CharacterId b) const
    {
        for(auto it=items_.rbegin();it!=items_.rend();++it){
            if(it->matches(a,b)) return &(*it);
        }
        return nullptr;
    }

    bool startDating(CharacterId a,CharacterId b,CharacterId initiator,int minute)
    {
        if(a==0 || b==0 || a==b) return false;
        if(initiator!=a && initiator!=b) return false;
        if(!isAvailable(a) || !isAvailable(b)) return false;
        RomancePair pair;
        pair.first=a;
        pair.second=b;
        pair.initiator=initiator;
        pair.stage=RomanceStage::Dating;
        pair.startedMinute=std::max(0,minute);
        items_.push_back(pair);
        return true;
    }

    bool endDating(CharacterId a,CharacterId b,int minute)
    {
        for(auto it=items_.rbegin();it!=items_.rend();++it){
            if(it->active() && it->matches(a,b)){
                it->stage=RomanceStage::FormerPartners;
                it->endedMinute=std::max(it->startedMinute,minute);
                return true;
            }
        }
        return false;
    }

    const std::vector<RomancePair>& all() const { return items_; }

private:
    std::vector<RomancePair> items_;
};

struct RomanceContext {
    // SPEC 32 inputs that are not already represented by Relationship/Emotion.
    double personalityCompatibility=0.5;
    double sharedExperience=0.0;
    double lifeGoalAlignment=0.5;
    double pastRelationshipPenalty=0.0;
    bool available=true;
};

struct RomanceEvaluation {
    double score=0.0;
    bool ready=false;
};

inline RomanceEvaluation evaluateRomanceInterest(
    const Character& actor,
    const Relationship& towardTarget,
    const RomanceContext& context,
    double threshold)
{
    if(!context.available) return {};

    const double positiveValence=clampRomance((actor.emotion.valence+1.0)*0.5);
    const double score=
        0.18*towardTarget.attraction+
        0.17*towardTarget.romanticInterest+
        0.13*towardTarget.affection+
        0.12*towardTarget.trust+
        0.08*towardTarget.familiarity+
        0.07*towardTarget.comfort+
        0.08*clampRomance(context.personalityCompatibility)+
        0.05*clampRomance(context.sharedExperience)+
        0.05*clampRomance(context.lifeGoalAlignment)+
        0.04*positiveValence+
        0.03*clampRomance(actor.personality.riskTolerance)-
        0.14*towardTarget.conflict-
        0.12*towardTarget.fear-
        0.10*towardTarget.grudge-
        0.04*towardTarget.jealousy-
        0.10*clampRomance(context.pastRelationshipPenalty);

    RomanceEvaluation result;
    result.score=clampRomance(score);
    result.ready=result.score>=clampRomance(threshold);
    return result;
}

enum class DatingProposalResult {
    Invalid,
    Unavailable,
    NotReady,
    Rejected,
    Accepted
};

struct DatingProposalOutcome {
    DatingProposalResult result=DatingProposalResult::Invalid;
    double proposerScore=0.0;
    double recipientScore=0.0;
};

inline DatingProposalOutcome evaluateDatingProposal(
    const Character& proposer,
    const Character& recipient,
    const Relationship& proposerToRecipient,
    const Relationship& recipientToProposer,
    const RomanceContext& proposerContext,
    const RomanceContext& recipientContext,
    const RomanceBook& romances,
    double proposalThreshold=0.64,
    double acceptanceThreshold=0.60)
{
    DatingProposalOutcome outcome;
    if(proposer.id==0 || recipient.id==0 || proposer.id==recipient.id) return outcome;

    if(!romances.isAvailable(proposer.id) || !romances.isAvailable(recipient.id) ||
       !proposerContext.available || !recipientContext.available){
        outcome.result=DatingProposalResult::Unavailable;
        return outcome;
    }

    const RomanceEvaluation proposerEval=evaluateRomanceInterest(
        proposer,proposerToRecipient,proposerContext,proposalThreshold);
    outcome.proposerScore=proposerEval.score;
    if(!proposerEval.ready){
        outcome.result=DatingProposalResult::NotReady;
        return outcome;
    }

    const RomanceEvaluation recipientEval=evaluateRomanceInterest(
        recipient,recipientToProposer,recipientContext,acceptanceThreshold);
    outcome.recipientScore=recipientEval.score;
    outcome.result=recipientEval.ready ? DatingProposalResult::Accepted : DatingProposalResult::Rejected;
    return outcome;
}

inline DatingProposalOutcome applyDatingProposal(
    Character& proposer,
    Character& recipient,
    Relationship& proposerToRecipient,
    Relationship& recipientToProposer,
    const RomanceContext& proposerContext,
    const RomanceContext& recipientContext,
    RomanceBook& romances,
    int minute,
    double proposalThreshold=0.64,
    double acceptanceThreshold=0.60)
{
    DatingProposalOutcome outcome=evaluateDatingProposal(
        proposer,recipient,proposerToRecipient,recipientToProposer,
        proposerContext,recipientContext,romances,proposalThreshold,acceptanceThreshold);

    if(outcome.result==DatingProposalResult::Rejected){
        proposerToRecipient.apply(relationshipDeltaFor(RelationshipEvent::Rejection));
        applyEmotionEvent(proposer.emotion,EmotionEventType::Rejection);
        return outcome;
    }

    if(outcome.result!=DatingProposalResult::Accepted) return outcome;

    if(!romances.startDating(proposer.id,recipient.id,proposer.id,minute)){
        outcome.result=DatingProposalResult::Unavailable;
        return outcome;
    }

    proposerToRecipient.apply(relationshipDeltaFor(RelationshipEvent::CommitmentMade));
    recipientToProposer.apply(relationshipDeltaFor(RelationshipEvent::CommitmentMade));
    applyEmotionEvent(proposer.emotion,EmotionEventType::RomanticCloseness);
    applyEmotionEvent(recipient.emotion,EmotionEventType::RomanticCloseness);
    return outcome;
}

} // namespace lifelens
