#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "Genealogy.h"
#include "Household.h"
#include "ObserverReadModel.h"
#include "Pregnancy.h"
#include "Romance.h"

namespace lifelens {

struct EmotionObservation {
    double joy=0.0;
    double sadness=0.0;
    double anger=0.0;
    double fear=0.0;
    double embarrassment=0.0;
    double pride=0.0;
    double jealousy=0.0;
    double affection=0.0;
    double anxiety=0.0;
    double relief=0.0;
    double grief=0.0;
    double valence=0.0;
    double arousal=0.0;
    double intensity=0.0;
};

inline EmotionObservation makeEmotionObservation(const Character& character)
{
    EmotionObservation dto;
    dto.joy=character.emotion.joy;
    dto.sadness=character.emotion.sadness;
    dto.anger=character.emotion.anger;
    dto.fear=character.emotion.fear;
    dto.embarrassment=character.emotion.embarrassment;
    dto.pride=character.emotion.pride;
    dto.jealousy=character.emotion.jealousy;
    dto.affection=character.emotion.affection;
    dto.anxiety=character.emotion.anxiety;
    dto.relief=character.emotion.relief;
    dto.grief=character.emotion.grief;
    dto.valence=character.emotion.valence;
    dto.arousal=character.emotion.arousal;
    dto.intensity=character.emotion.intensity();
    return dto;
}

struct FamilyMemberObservation {
    CharacterId id=0;
    std::string name;
    KinshipType kinship=KinshipType::Unrelated;
    bool alive=true;
    LifeStage lifeStage=LifeStage::Adult;
};

struct FamilyObservation {
    CharacterId subjectId=0;
    HouseholdId householdId=0;

    bool hasRomanceHistory=false;
    bool hasActivePartner=false;
    CharacterId partnerId=0;
    std::string partnerName;
    RomanceStage partnerStage=RomanceStage::FormerPartners;
    bool cohabitingWithPartner=false;

    bool isGestationalParent=false;
    bool expectingChild=false;
    CharacterId pregnancyPartnerId=0;
    std::string pregnancyPartnerName;

    std::vector<FamilyMemberObservation> parents;
    std::vector<FamilyMemberObservation> children;
    std::vector<FamilyMemberObservation> siblings;
};

inline FamilyMemberObservation makeFamilyMemberObservation(
    const World& world,
    CharacterId id,
    KinshipType kinship)
{
    FamilyMemberObservation dto;
    dto.id=id;
    dto.kinship=kinship;
    if(const Character* character=findObservedCharacter(world,id)){
        dto.name=character->name;
        dto.alive=character->alive;
        dto.lifeStage=character->lifeStage;
    }
    return dto;
}

inline const RomancePair* latestRomanceFor(const RomanceBook& romances,CharacterId id)
{
    const auto& items=romances.all();
    for(auto it=items.rbegin();it!=items.rend();++it){
        if(it->contains(id)) return &(*it);
    }
    return nullptr;
}

inline FamilyObservation buildFamilyObservation(
    const World& world,
    const GenealogyBook& genealogy,
    const RomanceBook& romances,
    const HouseholdBook& households,
    const PregnancyBook& pregnancies,
    const Character& character)
{
    FamilyObservation dto;
    dto.subjectId=character.id;

    if(const Household* household=households.householdOf(character.id)){
        dto.householdId=household->id;
    }

    if(const RomancePair* pair=latestRomanceFor(romances,character.id)){
        dto.hasRomanceHistory=true;
        dto.partnerId=pair->partnerOf(character.id);
        dto.partnerStage=pair->stage;
        dto.hasActivePartner=pair->active();
        if(const Character* partner=findObservedCharacter(world,dto.partnerId)){
            dto.partnerName=partner->name;
        }
        if(dto.hasActivePartner && dto.householdId!=0){
            const Household* partnerHome=households.householdOf(dto.partnerId);
            dto.cohabitingWithPartner=partnerHome!=nullptr && partnerHome->id==dto.householdId;
        }
    }

    if(const PregnancyState* pregnancy=pregnancies.activeFor(character.id)){
        dto.isGestationalParent=true;
        dto.expectingChild=true;
        dto.pregnancyPartnerId=pregnancy->geneticPartner;
    } else {
        for(const auto& entry:pregnancies.all()){
            if(entry.active() && entry.geneticPartner==character.id){
                dto.expectingChild=true;
                dto.pregnancyPartnerId=entry.gestationalParent;
                break;
            }
        }
    }
    if(dto.pregnancyPartnerId!=0){
        if(const Character* partner=findObservedCharacter(world,dto.pregnancyPartnerId)){
            dto.pregnancyPartnerName=partner->name;
        }
    }

    for(CharacterId parentId:character.parentIds){
        dto.parents.push_back(makeFamilyMemberObservation(world,parentId,KinshipType::Parent));
    }
    for(CharacterId childId:character.childrenIds){
        dto.children.push_back(makeFamilyMemberObservation(world,childId,KinshipType::Child));
    }

    for(const auto& candidate:world.characters){
        if(candidate.id==character.id) continue;
        const KinshipType relation=genealogy.relationBetween(character.id,candidate.id);
        if(relation==KinshipType::Sibling || relation==KinshipType::HalfSibling){
            dto.siblings.push_back(makeFamilyMemberObservation(world,candidate.id,relation));
        }
    }

    return dto;
}

struct LifeStageCountsObservation {
    std::size_t baby=0;
    std::size_t toddler=0;
    std::size_t child=0;
    std::size_t teen=0;
    std::size_t youngAdult=0;
    std::size_t adult=0;
    std::size_t middleAge=0;
    std::size_t elderly=0;
};

struct WorldOverviewObservation {
    int minute=0;
    std::size_t totalResidents=0;
    std::size_t livingResidents=0;
    std::size_t deceasedResidents=0;
    LifeStageCountsObservation lifeStages;

    std::size_t households=0;
    std::size_t activeCouples=0;
    std::size_t datingCouples=0;
    std::size_t engagedCouples=0;
    std::size_t marriedCouples=0;
    std::size_t separatedCouples=0;
    std::size_t activePregnancies=0;
    std::size_t majorLifeEvents=0;
};

inline bool isMajorObserverLifeEvent(LifeEventType type)
{
    switch(type){
        case LifeEventType::Birth:
        case LifeEventType::DatingStarted:
        case LifeEventType::Engaged:
        case LifeEventType::Married:
        case LifeEventType::CohabitationStarted:
        case LifeEventType::PregnancyStarted:
        case LifeEventType::ChildBorn:
        case LifeEventType::Separated:
        case LifeEventType::Divorced:
        case LifeEventType::PartnerWidowed:
        case LifeEventType::Death:
        case LifeEventType::Bereavement:
            return true;
        case LifeEventType::LifeStageChanged:
        case LifeEventType::ParentingMilestone:
        case LifeEventType::HouseholdChanged:
            return false;
    }
    return false;
}

inline WorldOverviewObservation buildWorldOverviewObservation(
    const World& world,
    const HouseholdBook& households,
    const RomanceBook& romances,
    const PregnancyBook& pregnancies)
{
    WorldOverviewObservation dto;
    dto.minute=world.minute;
    dto.totalResidents=world.characters.size();

    for(const auto& character:world.characters){
        if(character.alive){
            ++dto.livingResidents;
            switch(character.lifeStage){
                case LifeStage::Baby: ++dto.lifeStages.baby; break;
                case LifeStage::Toddler: ++dto.lifeStages.toddler; break;
                case LifeStage::Child: ++dto.lifeStages.child; break;
                case LifeStage::Teen: ++dto.lifeStages.teen; break;
                case LifeStage::YoungAdult: ++dto.lifeStages.youngAdult; break;
                case LifeStage::Adult: ++dto.lifeStages.adult; break;
                case LifeStage::MiddleAge: ++dto.lifeStages.middleAge; break;
                case LifeStage::Elderly: ++dto.lifeStages.elderly; break;
            }
        } else {
            ++dto.deceasedResidents;
        }

        for(const auto& event:character.lifeHistory){
            if(isMajorObserverLifeEvent(event.type)) ++dto.majorLifeEvents;
        }
    }

    for(const auto& household:households.all()){
        if(!household.members.empty()) ++dto.households;
    }

    for(const auto& pair:romances.all()){
        if(pair.active()) ++dto.activeCouples;
        switch(pair.stage){
            case RomanceStage::Dating: ++dto.datingCouples; break;
            case RomanceStage::Engaged: ++dto.engagedCouples; break;
            case RomanceStage::Married: ++dto.marriedCouples; break;
            case RomanceStage::Separated: ++dto.separatedCouples; break;
            case RomanceStage::Divorced:
            case RomanceStage::Widowed:
            case RomanceStage::FormerPartners:
                break;
        }
    }

    for(const auto& pregnancy:pregnancies.all()){
        if(pregnancy.active()) ++dto.activePregnancies;
    }

    return dto;
}

inline const char* romanceStageName(RomanceStage stage)
{
    switch(stage){
        case RomanceStage::Dating: return "Dating";
        case RomanceStage::Engaged: return "Engaged";
        case RomanceStage::Married: return "Married";
        case RomanceStage::Separated: return "Separated";
        case RomanceStage::Divorced: return "Divorced";
        case RomanceStage::Widowed: return "Widowed";
        case RomanceStage::FormerPartners: return "FormerPartners";
    }
    return "Unknown";
}

} // namespace lifelens
