#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "Character.h"
#include "Relationship.h"
#include "Romance.h"

namespace lifelens {

enum class DeathCause {
    AgeRelated,
    Illness,
    Accident,
    Other
};

struct PopulationContinuity {
    std::size_t living=0;
    std::size_t deceased=0;
    std::size_t livingAdults=0;
    std::size_t livingMinors=0;

    bool needsAdultReplacement(std::size_t minimumLivingAdults=2) const
    {
        return livingAdults<minimumLivingAdults;
    }
};

inline bool isAdultStage(LifeStage stage)
{
    return stage==LifeStage::YoungAdult ||
           stage==LifeStage::Adult ||
           stage==LifeStage::MiddleAge ||
           stage==LifeStage::Elderly;
}

inline PopulationContinuity summarizePopulation(const std::vector<Character*>& residents)
{
    PopulationContinuity result;
    for(const Character* resident:residents){
        if(resident==nullptr) continue;
        if(!resident->alive){
            ++result.deceased;
            continue;
        }
        ++result.living;
        if(isAdultStage(resident->lifeStage)) ++result.livingAdults;
        else ++result.livingMinors;
    }
    return result;
}

inline bool containsCharacter(const std::vector<CharacterId>& ids,CharacterId id)
{
    return std::find(ids.begin(),ids.end(),id)!=ids.end();
}

inline bool isCloseFamily(const Character& a,const Character& b)
{
    return containsCharacter(a.parentIds,b.id) ||
           containsCharacter(a.childrenIds,b.id) ||
           containsCharacter(b.parentIds,a.id) ||
           containsCharacter(b.childrenIds,a.id);
}

struct DeathOutcome {
    bool died=false;
    CharacterId deceased=0;
    CharacterId survivingPartner=0;
    std::size_t bereavedResidents=0;
    bool romanceClosed=false;
    PopulationContinuity population;
};

inline DeathOutcome applyDeath(
    Character& deceased,
    int minute,
    DeathCause cause,
    const std::vector<Character*>& residents,
    RelationshipBook& relationships,
    RomanceBook& romances)
{
    DeathOutcome outcome;
    outcome.deceased=deceased.id;
    if(deceased.id==0 || !deceased.alive) return outcome;

    const int deathMinute=std::max(0,minute);

    CharacterId partnerId=0;
    RomanceStage partnerStage=RomanceStage::FormerPartners;
    if(const RomancePair* activePair=romances.activeFor(deceased.id)){
        partnerId=activePair->partnerOf(deceased.id);
        partnerStage=activePair->stage;
    }

    deceased.alive=false;
    deceased.deathMinute=deathMinute;
    deceased.lifeHistory.push_back(LifeHistoryEntry{
        LifeEventType::Death,
        deathMinute,
        {},
        static_cast<int>(cause)});
    outcome.died=true;

    if(partnerId!=0){
        outcome.survivingPartner=partnerId;
        if(partnerStage==RomanceStage::Married || partnerStage==RomanceStage::Separated){
            outcome.romanceClosed=romances.markWidowed(partnerId,deceased.id,deathMinute);
        } else if(partnerStage==RomanceStage::Dating || partnerStage==RomanceStage::Engaged){
            outcome.romanceClosed=romances.endDating(partnerId,deceased.id,deathMinute);
        }
    }

    for(Character* survivor:residents){
        if(survivor==nullptr || survivor->id==deceased.id || !survivor->alive) continue;

        const bool spouse=survivor->id==partnerId;
        const bool family=isCloseFamily(*survivor,deceased);
        const Relationship* towardDeceased=relationships.find(survivor->id,deceased.id);
        double bond=towardDeceased!=nullptr ? towardDeceased->socialBond() : 0.0;
        if(family) bond=std::max(bond,0.72);
        if(spouse) bond=std::max(bond,0.95);

        // Distant strangers do not receive a full bereavement event merely because a death occurred.
        if(bond<0.15 && !family && !spouse) continue;

        const double impact=std::min(1.35,0.45+0.90*bond);
        applyEmotionEvent(survivor->emotion,EmotionEventType::Loss,impact);

        MemoryRecord memory;
        memory.who=deceased.id;
        memory.what="death";
        memory.where="";
        memory.minute=deathMinute;
        memory.emotionValence=-1.0;
        memory.emotionIntensity=std::min(1.0,0.45+0.55*bond);
        memory.importance=std::min(1.0,0.50+0.50*bond);
        memory.confidence=1.0;
        memory.witnessed=true;
        memory.source=MemorySource::DirectWitness;
        memory.decayPerDay=0.004;
        memory.tags={"death","loss"};
        if(family) memory.tags.push_back("family");
        if(spouse) memory.tags.push_back("partner");
        survivor->memory.add(memory);

        survivor->lifeHistory.push_back(LifeHistoryEntry{
            LifeEventType::Bereavement,
            deathMinute,
            {deceased.id},
            static_cast<int>(cause)});
        ++outcome.bereavedResidents;
    }

    outcome.population=summarizePopulation(residents);
    return outcome;
}

} // namespace lifelens
