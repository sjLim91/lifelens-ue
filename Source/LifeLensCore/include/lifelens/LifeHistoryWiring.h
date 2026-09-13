#pragma once

#include <random>
#include <string>
#include <vector>

#include "Birth.h"
#include "Death.h"
#include "Marriage.h"
#include "Pregnancy.h"
#include "Romance.h"

namespace lifelens {

inline DatingProposalOutcome applyDatingProposalTracked(
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
    auto outcome=applyDatingProposal(
        proposer,recipient,proposerToRecipient,recipientToProposer,
        proposerContext,recipientContext,romances,minute,
        proposalThreshold,acceptanceThreshold);
    if(outcome.result==DatingProposalResult::Accepted){
        recordLifeEvent(proposer.lifeHistory,LifeEventType::DatingStarted,minute,{recipient.id});
        recordLifeEvent(recipient.lifeHistory,LifeEventType::DatingStarted,minute,{proposer.id});
    }
    return outcome;
}

inline EngagementProposalOutcome applyEngagementProposalTracked(
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
    auto outcome=applyEngagementProposal(
        proposer,recipient,proposerToRecipient,recipientToProposer,
        proposerContext,recipientContext,romances,minute,
        proposalThreshold,acceptanceThreshold);
    if(outcome.result==EngagementProposalResult::Accepted){
        recordLifeEvent(proposer.lifeHistory,LifeEventType::Engaged,minute,{recipient.id});
        recordLifeEvent(recipient.lifeHistory,LifeEventType::Engaged,minute,{proposer.id});
    }
    return outcome;
}

inline MarriageDecisionOutcome applyMarriageDecisionTracked(
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
    auto outcome=applyMarriageDecision(
        first,second,firstToSecond,secondToFirst,
        firstContext,secondContext,romances,households,minute,
        newHouseholdId,home,sharedMoney,firstThreshold,secondThreshold);
    if(outcome.result==MarriageDecisionResult::Married){
        recordLifeEvent(first.lifeHistory,LifeEventType::Married,minute,{second.id});
        recordLifeEvent(second.lifeHistory,LifeEventType::Married,minute,{first.id});
    }
    return outcome;
}

inline PregnancyAttemptOutcome applyPregnancyAttemptTracked(
    Character& gestationalParent,
    Character& partner,
    const ReproductiveProfile& gestationalProfile,
    const ReproductiveProfile& partnerProfile,
    const Relationship& gestationalToPartner,
    const Relationship& partnerToGestational,
    const PregnancyContext& context,
    PregnancyBook& pregnancies,
    int currentMinute,
    double deterministicRoll,
    double readinessThreshold=0.62)
{
    auto outcome=applyPregnancyAttempt(
        gestationalParent,partner,gestationalProfile,partnerProfile,
        gestationalToPartner,partnerToGestational,context,pregnancies,
        currentMinute,deterministicRoll,readinessThreshold);
    if(outcome.result==PregnancyAttemptResult::Conceived){
        recordLifeEvent(gestationalParent.lifeHistory,LifeEventType::PregnancyStarted,currentMinute,{partner.id});
        recordLifeEvent(partner.lifeHistory,LifeEventType::PregnancyStarted,currentMinute,{gestationalParent.id});
    }
    return outcome;
}

inline BirthOutcome performBirthTracked(
    Character& gestationalParent,
    Character& geneticPartner,
    CharacterId childId,
    const std::string& childName,
    PregnancyBook& pregnancies,
    HouseholdBook& households,
    BirthBook& births,
    std::mt19937_64& rng,
    int currentMinute,
    double geneticVariation=0.08,
    GenealogyBook* genealogy=nullptr)
{
    auto outcome=performBirth(
        gestationalParent,geneticPartner,childId,childName,
        pregnancies,households,births,rng,currentMinute,
        geneticVariation,genealogy);
    if(outcome.result==BirthResult::Success){
        recordLifeEvent(gestationalParent.lifeHistory,LifeEventType::ChildBorn,currentMinute,{childId,geneticPartner.id});
        recordLifeEvent(geneticPartner.lifeHistory,LifeEventType::ChildBorn,currentMinute,{childId,gestationalParent.id});
    }
    return outcome;
}

inline DeathOutcome applyDeathTracked(
    Character& deceased,
    int minute,
    DeathCause cause,
    const std::vector<Character*>& residents,
    RelationshipBook& relationships,
    RomanceBook& romances)
{
    auto outcome=applyDeath(deceased,minute,cause,residents,relationships,romances);
    if(outcome.died && outcome.survivingPartner!=0 && outcome.romanceClosed){
        for(Character* resident:residents){
            if(resident!=nullptr && resident->id==outcome.survivingPartner && resident->alive){
                recordLifeEvent(resident->lifeHistory,LifeEventType::PartnerWidowed,minute,{deceased.id});
                break;
            }
        }
    }
    return outcome;
}

} // namespace lifelens
