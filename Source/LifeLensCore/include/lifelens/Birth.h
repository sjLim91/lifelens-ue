#pragma once

#include <algorithm>
#include <random>
#include <string>
#include <vector>

#include "Character.h"
#include "Genetics.h"
#include "Genealogy.h"
#include "Household.h"
#include "Pregnancy.h"

namespace lifelens {

enum class BirthResult {
    Invalid,
    NoActivePregnancy,
    NotDue,
    WrongPartner,
    DuplicateChild,
    GenealogyConflict,
    Success
};

struct BirthRecord {
    CharacterId childId=0;
    CharacterId parentA=0;
    CharacterId parentB=0;
    int birthMinute=0;
    HouseholdId householdId=0;
};

class BirthBook {
public:
    const BirthRecord* find(CharacterId childId) const
    {
        for(const auto& record:items_) if(record.childId==childId) return &record;
        return nullptr;
    }

    bool add(const BirthRecord& record)
    {
        if(record.childId==0 || record.parentA==0 || record.parentB==0 ||
           record.parentA==record.parentB || find(record.childId)!=nullptr) return false;
        items_.push_back(record);
        return true;
    }

    const std::vector<BirthRecord>& all() const { return items_; }

private:
    std::vector<BirthRecord> items_;
};

struct BirthOutcome {
    BirthResult result=BirthResult::Invalid;
    Character child;
    HouseholdId householdId=0;
};

inline bool containsCharacterId(const std::vector<CharacterId>& ids,CharacterId id)
{
    return std::find(ids.begin(),ids.end(),id)!=ids.end();
}

inline BirthOutcome performBirth(
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
    BirthOutcome outcome;
    if(gestationalParent.id==0 || geneticPartner.id==0 || childId==0 ||
       gestationalParent.id==geneticPartner.id || childName.empty()) return outcome;
    if(childId==gestationalParent.id || childId==geneticPartner.id || births.find(childId)!=nullptr){
        outcome.result=BirthResult::DuplicateChild;
        return outcome;
    }
    if(genealogy!=nullptr && !genealogy->canRegisterBirth(childId,gestationalParent.id,geneticPartner.id)){
        outcome.result=BirthResult::GenealogyConflict;
        return outcome;
    }

    PregnancyState* pregnancy=pregnancies.activeFor(gestationalParent.id);
    if(pregnancy==nullptr){
        outcome.result=BirthResult::NoActivePregnancy;
        return outcome;
    }
    if(pregnancy->geneticPartner!=geneticPartner.id){
        outcome.result=BirthResult::WrongPartner;
        return outcome;
    }
    if(currentMinute<pregnancy->dueMinute){
        outcome.result=BirthResult::NotDue;
        return outcome;
    }

    Character child;
    child.id=childId;
    child.name=childName;
    child.hasBirthMinute=true;
    child.birthMinute=currentMinute;
    child.lifeStage=LifeStage::Baby;
    const LifeStageProfile newbornProfile=lifeStageProfile(LifeStage::Baby);
    child.metabolism=newbornProfile.metabolismMultiplier;
    child.sleepTendency=newbornProfile.sleepTendencyMultiplier;
    child.parentIds={gestationalParent.id,geneticPartner.id};
    child.genetics=inheritGenetics(
        gestationalParent.genetics,geneticPartner.genetics,rng,geneticVariation);
    child.civilization.character=childId;
    // A newborn starts without technique knowledge. Capacity develops from
    // inherited learning potential and later upbringing/practice rather than
    // automatically inheriting the parents' recipes.
    child.civilization.gatheringSkill=0.10;
    child.civilization.craftingSkill=0.08;
    child.civilization.learningSkill=std::max(0.12,std::min(0.55,
        0.12+child.genetics.learningPotential*0.35));
    child.lifeHistory.push_back(LifeHistoryEntry{
        LifeEventType::Birth,currentMinute,{gestationalParent.id,geneticPartner.id},0});

    HouseholdId householdId=0;
    const Household* home=households.householdOf(gestationalParent.id);
    if(home!=nullptr){
        householdId=home->id;
        if(!households.addMember(householdId,child.id)){
            outcome.result=BirthResult::Invalid;
            return outcome;
        }
    }

    BirthRecord record;
    record.childId=child.id;
    record.parentA=gestationalParent.id;
    record.parentB=geneticPartner.id;
    record.birthMinute=currentMinute;
    record.householdId=householdId;
    if(!births.add(record)){
        if(householdId!=0) households.removeMember(child.id);
        outcome.result=BirthResult::DuplicateChild;
        return outcome;
    }

    if(!pregnancies.complete(gestationalParent.id,currentMinute)){
        if(householdId!=0) households.removeMember(child.id);
        outcome.result=BirthResult::Invalid;
        return outcome;
    }

    if(!containsCharacterId(gestationalParent.childrenIds,child.id))
        gestationalParent.childrenIds.push_back(child.id);
    if(!containsCharacterId(geneticPartner.childrenIds,child.id))
        geneticPartner.childrenIds.push_back(child.id);

    if(genealogy!=nullptr){
        // Prevalidated above, so a successful birth cannot create a contradictory family edge.
        genealogy->registerBirth(child.id,gestationalParent.id,geneticPartner.id);
    }

    applyEmotionEvent(gestationalParent.emotion,EmotionEventType::PositiveSocial,0.8);
    applyEmotionEvent(geneticPartner.emotion,EmotionEventType::PositiveSocial,0.7);

    outcome.result=BirthResult::Success;
    outcome.child=child;
    outcome.householdId=householdId;
    return outcome;
}

} // namespace lifelens
