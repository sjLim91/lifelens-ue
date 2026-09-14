#pragma once

#include <algorithm>

#include "Character.h"
#include "Household.h"
#include "Relationship.h"
#include "Romance.h"

namespace lifelens {

inline double clampMarriage(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

struct MarriageContext {
    double marriageIntent=0.5;
    double lifeGoalAlignment=0.5;
    double householdStability=0.5;
    double financialReadiness=0.5;
    double relationshipDuration=0.5;
    double externalStress=0.0;
    bool available=true;
    bool mergeHouseholdsOnMarriage=true;
};

struct MarriageEvaluation {
    double score=0.0;
    bool ready=false;
};

inline MarriageEvaluation evaluateMarriageReadiness(
    const Character& actor,
    const Relationship& towardPartner,
    const MarriageContext& context,
    double threshold)
{
    if(!context.available) return {};

    const double score=
        0.16*towardPartner.commitment+
        0.14*towardPartner.trust+
        0.12*towardPartner.affection+
        0.12*towardPartner.socialBond()+
        0.10*towardPartner.romancePotential()+
        0.10*clampMarriage(context.marriageIntent)+
        0.07*clampMarriage(context.lifeGoalAlignment)+
        0.05*clampMarriage(context.householdStability)+
        0.04*clampMarriage(context.financialReadiness)+
        0.04*clampMarriage(context.relationshipDuration)+
        0.03*clampMarriage(actor.personality.conscientiousness)+
        0.03*clampMarriage(actor.personality.patience)-
        0.12*towardPartner.conflict-
        0.10*towardPartner.fear-
        0.09*towardPartner.grudge-
        0.05*towardPartner.jealousy-
        0.08*clampMarriage(context.externalStress);

    MarriageEvaluation result;
    result.score=clampMarriage(score);
    result.ready=result.score>=clampMarriage(threshold);
    return result;
}

enum class EngagementProposalResult {
    Invalid,
    WrongStage,
    Unavailable,
    NotReady,
    Rejected,
    Accepted
};

struct EngagementProposalOutcome {
    EngagementProposalResult result=EngagementProposalResult::Invalid;
    double proposerScore=0.0;
    double recipientScore=0.0;
};

inline EngagementProposalOutcome evaluateEngagementProposal(
    const Character& proposer,
    const Character& recipient,
    const Relationship& proposerToRecipient,
    const Relationship& recipientToProposer,
    const MarriageContext& proposerContext,
    const MarriageContext& recipientContext,
    const RomanceBook& romances,
    double proposalThreshold=0.68,
    double acceptanceThreshold=0.64)
{
    EngagementProposalOutcome outcome;
    if(proposer.id==0 || recipient.id==0 || proposer.id==recipient.id) return outcome;

    const RomancePair* pair=romances.findActivePair(proposer.id,recipient.id);
    if(pair==nullptr || pair->stage!=RomanceStage::Dating){
        outcome.result=EngagementProposalResult::WrongStage;
        return outcome;
    }

    if(!proposerContext.available || !recipientContext.available){
        outcome.result=EngagementProposalResult::Unavailable;
        return outcome;
    }

    const MarriageEvaluation proposerEval=evaluateMarriageReadiness(
        proposer,proposerToRecipient,proposerContext,proposalThreshold);
    outcome.proposerScore=proposerEval.score;
    if(!proposerEval.ready){
        outcome.result=EngagementProposalResult::NotReady;
        return outcome;
    }

    const MarriageEvaluation recipientEval=evaluateMarriageReadiness(
        recipient,recipientToProposer,recipientContext,acceptanceThreshold);
    outcome.recipientScore=recipientEval.score;
    outcome.result=recipientEval.ready ? EngagementProposalResult::Accepted : EngagementProposalResult::Rejected;
    return outcome;
}

inline EngagementProposalOutcome applyEngagementProposal(
    Character& proposer,
    Character& recipient,
    Relationship& proposerToRecipient,
    Relationship& recipientToProposer,
    const MarriageContext& proposerContext,
    const MarriageContext& recipientContext,
    RomanceBook& romances,
    int minute,
    double proposalThreshold=0.68,
    double acceptanceThreshold=0.64)
{
    EngagementProposalOutcome outcome=evaluateEngagementProposal(
        proposer,recipient,proposerToRecipient,recipientToProposer,
        proposerContext,recipientContext,romances,proposalThreshold,acceptanceThreshold);

    if(outcome.result==EngagementProposalResult::Rejected){
        proposerToRecipient.apply(relationshipDeltaFor(RelationshipEvent::Rejection,0.50));
        applyEmotionEvent(proposer.emotion,EmotionEventType::Rejection,0.50);
        return outcome;
    }
    if(outcome.result!=EngagementProposalResult::Accepted) return outcome;

    if(!romances.engage(proposer.id,recipient.id,minute)){
        outcome.result=EngagementProposalResult::WrongStage;
        return outcome;
    }

    proposerToRecipient.apply(relationshipDeltaFor(RelationshipEvent::CommitmentMade,0.75));
    recipientToProposer.apply(relationshipDeltaFor(RelationshipEvent::CommitmentMade,0.75));
    applyEmotionEvent(proposer.emotion,EmotionEventType::RomanticCloseness,0.70);
    applyEmotionEvent(recipient.emotion,EmotionEventType::RomanticCloseness,0.70);
    recordLifeEvent(proposer.lifeHistory,LifeEventType::Engaged,minute,{recipient.id});
    recordLifeEvent(recipient.lifeHistory,LifeEventType::Engaged,minute,{proposer.id});
    return outcome;
}

enum class MarriageDecisionResult {
    Invalid,
    WrongStage,
    Unavailable,
    NotReady,
    Deferred,
    Married
};

struct MarriageDecisionOutcome {
    MarriageDecisionResult result=MarriageDecisionResult::Invalid;
    double firstScore=0.0;
    double secondScore=0.0;
};

inline MarriageDecisionOutcome evaluateMarriageDecision(
    const Character& first,
    const Character& second,
    const Relationship& firstToSecond,
    const Relationship& secondToFirst,
    const MarriageContext& firstContext,
    const MarriageContext& secondContext,
    const RomanceBook& romances,
    double firstThreshold=0.72,
    double secondThreshold=0.68)
{
    MarriageDecisionOutcome outcome;
    if(first.id==0 || second.id==0 || first.id==second.id) return outcome;

    const RomancePair* pair=romances.findActivePair(first.id,second.id);
    if(pair==nullptr || pair->stage!=RomanceStage::Engaged){
        outcome.result=MarriageDecisionResult::WrongStage;
        return outcome;
    }

    if(!firstContext.available || !secondContext.available){
        outcome.result=MarriageDecisionResult::Unavailable;
        return outcome;
    }

    const MarriageEvaluation firstEval=evaluateMarriageReadiness(
        first,firstToSecond,firstContext,firstThreshold);
    const MarriageEvaluation secondEval=evaluateMarriageReadiness(
        second,secondToFirst,secondContext,secondThreshold);
    outcome.firstScore=firstEval.score;
    outcome.secondScore=secondEval.score;

    if(!firstEval.ready){
        outcome.result=MarriageDecisionResult::NotReady;
        return outcome;
    }
    outcome.result=secondEval.ready ? MarriageDecisionResult::Married : MarriageDecisionResult::Deferred;
    return outcome;
}

inline MarriageDecisionOutcome applyMarriageDecision(
    Character& first,
    Character& second,
    Relationship& firstToSecond,
    Relationship& secondToFirst,
    const MarriageContext& firstContext,
    const MarriageContext& secondContext,
    RomanceBook& romances,
    HouseholdBook& households,
    int minute,
    HouseholdId newHouseholdId=0,
    ObjectId home=0,
    double sharedMoney=0.0,
    double firstThreshold=0.72,
    double secondThreshold=0.68)
{
    MarriageDecisionOutcome outcome=evaluateMarriageDecision(
        first,second,firstToSecond,secondToFirst,
        firstContext,secondContext,romances,firstThreshold,secondThreshold);
    if(outcome.result!=MarriageDecisionResult::Married) return outcome;

    const Household* firstHome=households.householdOf(first.id);
    const Household* secondHome=households.householdOf(second.id);
    const bool alreadyShared=firstHome!=nullptr && secondHome!=nullptr && firstHome->id==secondHome->id;
    const bool shouldMerge=firstContext.mergeHouseholdsOnMarriage || secondContext.mergeHouseholdsOnMarriage;
    bool mergedHousehold=false;

    if(shouldMerge && !alreadyShared){
        if(newHouseholdId==0 || households.find(newHouseholdId)!=nullptr){
            outcome.result=MarriageDecisionResult::Invalid;
            return outcome;
        }
        if(!households.formSharedHousehold(newHouseholdId,first.id,second.id,home,sharedMoney)){
            outcome.result=MarriageDecisionResult::Invalid;
            return outcome;
        }
        mergedHousehold=true;
    }

    if(!romances.marry(first.id,second.id,minute)){
        outcome.result=MarriageDecisionResult::WrongStage;
        return outcome;
    }

    firstToSecond.apply(relationshipDeltaFor(RelationshipEvent::CommitmentMade));
    secondToFirst.apply(relationshipDeltaFor(RelationshipEvent::CommitmentMade));
    applyEmotionEvent(first.emotion,EmotionEventType::RomanticCloseness);
    applyEmotionEvent(second.emotion,EmotionEventType::RomanticCloseness);
    recordLifeEvent(first.lifeHistory,LifeEventType::Married,minute,{second.id});
    recordLifeEvent(second.lifeHistory,LifeEventType::Married,minute,{first.id});
    if(mergedHousehold){
        recordLifeEvent(first.lifeHistory,LifeEventType::CohabitationStarted,minute,{second.id});
        recordLifeEvent(second.lifeHistory,LifeEventType::CohabitationStarted,minute,{first.id});
    }
    return outcome;
}

} // namespace lifelens
