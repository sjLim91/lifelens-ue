#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

#include "Character.h"
#include "Ids.h"
#include "Relationship.h"

namespace lifelens {

inline double clampHousehold(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

struct HouseholdResponsibilities {
    double cooking=0.0;
    double cleaning=0.0;
    double shopping=0.0;
    double maintenance=0.0;
    double caregiving=0.0;
};

struct HouseholdMember {
    CharacterId characterId=0;
    HouseholdResponsibilities responsibilities;
    double contributionWeight=1.0;
};

struct Household {
    HouseholdId id=0;
    std::vector<HouseholdMember> members;
    ObjectId home=0;
    double resources=0.0;
    double sharedMoney=0.0;
    std::vector<ObjectId> sharedObjects;

    bool contains(CharacterId characterId) const
    {
        for(const auto& member:members){
            if(member.characterId==characterId) return true;
        }
        return false;
    }
};

class HouseholdBook {
public:
    Household* find(HouseholdId id)
    {
        for(auto& household:items_) if(household.id==id) return &household;
        return nullptr;
    }

    const Household* find(HouseholdId id) const
    {
        for(const auto& household:items_) if(household.id==id) return &household;
        return nullptr;
    }

    Household* householdOf(CharacterId characterId)
    {
        for(auto& household:items_) if(household.contains(characterId)) return &household;
        return nullptr;
    }

    const Household* householdOf(CharacterId characterId) const
    {
        for(const auto& household:items_) if(household.contains(characterId)) return &household;
        return nullptr;
    }

    bool create(HouseholdId id,const std::vector<CharacterId>& members,ObjectId home=0,double sharedMoney=0.0)
    {
        if(id==0 || find(id)!=nullptr) return false;
        for(CharacterId member:members){
            if(member==0 || householdOf(member)!=nullptr) return false;
        }

        Household household;
        household.id=id;
        household.home=home;
        household.sharedMoney=std::max(0.0,sharedMoney);
        for(CharacterId member:members){
            household.members.push_back(HouseholdMember{member,{ },1.0});
        }
        items_.push_back(household);
        return true;
    }

    bool addMember(HouseholdId id,CharacterId characterId)
    {
        Household* target=find(id);
        if(target==nullptr || characterId==0 || householdOf(characterId)!=nullptr) return false;
        target->members.push_back(HouseholdMember{characterId,{ },1.0});
        return true;
    }

    bool removeMember(CharacterId characterId)
    {
        for(auto& household:items_){
            auto it=std::find_if(household.members.begin(),household.members.end(),[&](const HouseholdMember& member){
                return member.characterId==characterId;
            });
            if(it!=household.members.end()){
                household.members.erase(it);
                return true;
            }
        }
        return false;
    }

    bool moveMember(CharacterId characterId,HouseholdId destinationId)
    {
        Household* destination=find(destinationId);
        if(destination==nullptr || characterId==0) return false;
        Household* current=householdOf(characterId);
        if(current!=nullptr && current->id==destinationId) return true;

        HouseholdMember preserved;
        bool hadCurrent=false;
        if(current!=nullptr){
            for(const auto& member:current->members){
                if(member.characterId==characterId){
                    preserved=member;
                    hadCurrent=true;
                    break;
                }
            }
            removeMember(characterId);
        }
        if(!hadCurrent) preserved=HouseholdMember{characterId,{ },1.0};
        destination=find(destinationId);
        if(destination==nullptr) return false;
        destination->members.push_back(preserved);
        return true;
    }

    bool formSharedHousehold(
        HouseholdId newId,
        CharacterId first,
        CharacterId second,
        ObjectId home=0,
        double sharedMoney=0.0)
    {
        if(newId==0 || first==0 || second==0 || first==second || find(newId)!=nullptr) return false;

        const Household* firstCurrent=householdOf(first);
        const Household* secondCurrent=householdOf(second);
        if(firstCurrent!=nullptr && secondCurrent!=nullptr && firstCurrent->id==secondCurrent->id) return false;

        HouseholdMember firstMember{first,{ },1.0};
        HouseholdMember secondMember{second,{ },1.0};
        if(firstCurrent!=nullptr){
            for(const auto& member:firstCurrent->members) if(member.characterId==first) firstMember=member;
        }
        if(secondCurrent!=nullptr){
            for(const auto& member:secondCurrent->members) if(member.characterId==second) secondMember=member;
        }

        removeMember(first);
        removeMember(second);

        Household household;
        household.id=newId;
        household.home=home;
        household.sharedMoney=std::max(0.0,sharedMoney);
        household.members.push_back(firstMember);
        household.members.push_back(secondMember);
        items_.push_back(household);
        pruneEmpty();
        return true;
    }

    void pruneEmpty()
    {
        items_.erase(std::remove_if(items_.begin(),items_.end(),[](const Household& household){
            return household.members.empty();
        }),items_.end());
    }

    const std::vector<Household>& all() const { return items_; }

private:
    std::vector<Household> items_;
};

struct CohabitationContext {
    double housingReadiness=0.5;
    double financialReadiness=0.5;
    double lifeGoalAlignment=0.5;
    double scheduleCompatibility=0.5;
    double currentHousingPressure=0.0;
    double externalStress=0.0;
    bool available=true;
};

struct CohabitationEvaluation {
    double score=0.0;
    bool ready=false;
};

inline CohabitationEvaluation evaluateCohabitationInterest(
    const Character& actor,
    const Relationship& towardOther,
    const CohabitationContext& context,
    bool romanticPartners,
    double threshold)
{
    if(!context.available) return {};

    double score=
        0.20*towardOther.socialBond()+
        0.11*towardOther.comfort+
        0.10*towardOther.trust+
        0.08*towardOther.familiarity+
        0.08*towardOther.commitment+
        0.10*clampHousehold(context.lifeGoalAlignment)+
        0.08*clampHousehold(context.housingReadiness)+
        0.06*clampHousehold(context.financialReadiness)+
        0.06*clampHousehold(context.scheduleCompatibility)+
        0.04*clampHousehold(actor.personality.patience)+
        0.03*clampHousehold(actor.personality.conscientiousness)+
        0.03*clampHousehold(context.currentHousingPressure)+
        (romanticPartners ? 0.08 : 0.0)-
        0.12*towardOther.conflict-
        0.10*towardOther.fear-
        0.08*towardOther.grudge-
        0.08*clampHousehold(context.externalStress);

    CohabitationEvaluation result;
    result.score=clampHousehold(score);
    result.ready=result.score>=clampHousehold(threshold);
    return result;
}

enum class CohabitationProposalResult {
    Invalid,
    AlreadyCohabiting,
    Unavailable,
    NotReady,
    Rejected,
    Accepted
};

struct CohabitationProposalOutcome {
    CohabitationProposalResult result=CohabitationProposalResult::Invalid;
    double proposerScore=0.0;
    double recipientScore=0.0;
};

inline CohabitationProposalOutcome evaluateCohabitationProposal(
    const Character& proposer,
    const Character& recipient,
    const Relationship& proposerToRecipient,
    const Relationship& recipientToProposer,
    const CohabitationContext& proposerContext,
    const CohabitationContext& recipientContext,
    const HouseholdBook& households,
    bool romanticPartners,
    double proposalThreshold=0.62,
    double acceptanceThreshold=0.58)
{
    CohabitationProposalOutcome outcome;
    if(proposer.id==0 || recipient.id==0 || proposer.id==recipient.id) return outcome;

    const Household* proposerHome=households.householdOf(proposer.id);
    const Household* recipientHome=households.householdOf(recipient.id);
    if(proposerHome!=nullptr && recipientHome!=nullptr && proposerHome->id==recipientHome->id){
        outcome.result=CohabitationProposalResult::AlreadyCohabiting;
        return outcome;
    }

    if(!proposerContext.available || !recipientContext.available){
        outcome.result=CohabitationProposalResult::Unavailable;
        return outcome;
    }

    const CohabitationEvaluation proposerEval=evaluateCohabitationInterest(
        proposer,proposerToRecipient,proposerContext,romanticPartners,proposalThreshold);
    outcome.proposerScore=proposerEval.score;
    if(!proposerEval.ready){
        outcome.result=CohabitationProposalResult::NotReady;
        return outcome;
    }

    const CohabitationEvaluation recipientEval=evaluateCohabitationInterest(
        recipient,recipientToProposer,recipientContext,romanticPartners,acceptanceThreshold);
    outcome.recipientScore=recipientEval.score;
    outcome.result=recipientEval.ready ? CohabitationProposalResult::Accepted : CohabitationProposalResult::Rejected;
    return outcome;
}

inline CohabitationProposalOutcome applyCohabitationProposal(
    Character& proposer,
    Character& recipient,
    Relationship& proposerToRecipient,
    Relationship& recipientToProposer,
    const CohabitationContext& proposerContext,
    const CohabitationContext& recipientContext,
    HouseholdBook& households,
    HouseholdId newHouseholdId,
    bool romanticPartners,
    ObjectId home=0,
    double sharedMoney=0.0,
    double proposalThreshold=0.62,
    double acceptanceThreshold=0.58)
{
    CohabitationProposalOutcome outcome=evaluateCohabitationProposal(
        proposer,recipient,proposerToRecipient,recipientToProposer,
        proposerContext,recipientContext,households,romanticPartners,
        proposalThreshold,acceptanceThreshold);

    if(outcome.result==CohabitationProposalResult::Rejected){
        proposerToRecipient.apply(relationshipDeltaFor(RelationshipEvent::Rejection,0.35));
        applyEmotionEvent(proposer.emotion,EmotionEventType::Rejection,0.35);
        return outcome;
    }

    if(outcome.result!=CohabitationProposalResult::Accepted) return outcome;

    if(!households.formSharedHousehold(newHouseholdId,proposer.id,recipient.id,home,sharedMoney)){
        outcome.result=CohabitationProposalResult::Invalid;
        return outcome;
    }

    const RelationshipEvent positiveEvent=romanticPartners
        ? RelationshipEvent::CommitmentMade
        : RelationshipEvent::SharedPositiveExperience;
    proposerToRecipient.apply(relationshipDeltaFor(positiveEvent,0.50));
    recipientToProposer.apply(relationshipDeltaFor(positiveEvent,0.50));
    applyEmotionEvent(proposer.emotion,EmotionEventType::PositiveSocial,0.45);
    applyEmotionEvent(recipient.emotion,EmotionEventType::PositiveSocial,0.45);
    return outcome;
}

} // namespace lifelens
