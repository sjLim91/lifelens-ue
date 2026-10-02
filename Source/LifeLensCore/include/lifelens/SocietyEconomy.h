#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "CivilizationKnowledgeTransmission.h"
#include "Household.h"
#include "Relationship.h"
#include "World.h"

namespace lifelens {

enum class SocietyRole : std::uint8_t {
    Generalist=0, Forager, Craftsperson, Farmer, Metallurgist,
    Educator, Caregiver, Storekeeper
};

enum class SocietyInstitutionKind : std::uint8_t {
    LearningCircle=0, ProductionNetwork, StorageCommons,
    CareNetwork, ExchangeNetwork, InquiryCircle
};

enum class CollectiveRecordStage : std::uint8_t {
    Ephemeral=0, OralTradition, RepeatedTradition, ProtoRecordkeeping
};

enum class SocietyCoordinationTask : std::uint8_t {
    None=0,
    ProvisionFood,
    ProvisionWater,
    MaterialSupply,
    ToolProduction,
    Cultivation,
    Metallurgy,
    SharedStorage,
    Education,
    Care,
    Inquiry
};

enum class SocietyResourceDisposition : std::uint8_t {
    PersonalReserve=0,
    HouseholdReserve,
    TradableSurplus,
    SharedSurplus
};

inline const char* societyCoordinationTaskName(SocietyCoordinationTask task)
{
    switch(task){
        case SocietyCoordinationTask::ProvisionFood: return "ProvisionFood";
        case SocietyCoordinationTask::ProvisionWater: return "ProvisionWater";
        case SocietyCoordinationTask::MaterialSupply: return "MaterialSupply";
        case SocietyCoordinationTask::ToolProduction: return "ToolProduction";
        case SocietyCoordinationTask::Cultivation: return "Cultivation";
        case SocietyCoordinationTask::Metallurgy: return "Metallurgy";
        case SocietyCoordinationTask::SharedStorage: return "SharedStorage";
        case SocietyCoordinationTask::Education: return "Education";
        case SocietyCoordinationTask::Care: return "Care";
        case SocietyCoordinationTask::Inquiry: return "Inquiry";
        case SocietyCoordinationTask::None:
        default: return "None";
    }
}

inline const char* societyResourceDispositionName(
    SocietyResourceDisposition disposition)
{
    switch(disposition){
        case SocietyResourceDisposition::HouseholdReserve: return "HouseholdReserve";
        case SocietyResourceDisposition::TradableSurplus: return "TradableSurplus";
        case SocietyResourceDisposition::SharedSurplus: return "SharedSurplus";
        case SocietyResourceDisposition::PersonalReserve:
        default: return "PersonalReserve";
    }
}

inline const char* societyRoleName(SocietyRole role)
{
    switch(role){
        case SocietyRole::Forager: return "Forager";
        case SocietyRole::Craftsperson: return "Craftsperson";
        case SocietyRole::Farmer: return "Farmer";
        case SocietyRole::Metallurgist: return "Metallurgist";
        case SocietyRole::Educator: return "Educator";
        case SocietyRole::Caregiver: return "Caregiver";
        case SocietyRole::Storekeeper: return "Storekeeper";
        default: return "Generalist";
    }
}

inline const char* societyInstitutionKindName(SocietyInstitutionKind kind)
{
    switch(kind){
        case SocietyInstitutionKind::LearningCircle: return "LearningCircle";
        case SocietyInstitutionKind::ProductionNetwork: return "ProductionNetwork";
        case SocietyInstitutionKind::StorageCommons: return "StorageCommons";
        case SocietyInstitutionKind::CareNetwork: return "CareNetwork";
        case SocietyInstitutionKind::ExchangeNetwork: return "ExchangeNetwork";
        case SocietyInstitutionKind::InquiryCircle: return "InquiryCircle";
    }
    return "LearningCircle";
}

inline const char* collectiveRecordStageName(CollectiveRecordStage stage)
{
    switch(stage){
        case CollectiveRecordStage::OralTradition: return "OralTradition";
        case CollectiveRecordStage::RepeatedTradition: return "RepeatedTradition";
        case CollectiveRecordStage::ProtoRecordkeeping: return "ProtoRecordkeeping";
        default: return "Ephemeral";
    }
}

inline double societyClamp01(double v)
{
    return std::max(0.0,std::min(1.0,v));
}

inline int societyTechniqueUses(const Character& resident,TechniqueId technique)
{
    for(const auto& record:resident.civilization.knowledge.all()){
        if(record.technique==technique) return std::max(0,record.successfulUses);
    }
    return 0;
}

inline double societyKnowledge01(const Character& resident,TechniqueId technique)
{
    return societyClamp01(
        static_cast<double>(static_cast<int>(
            resident.civilization.knowledge.level(technique)))/6.0);
}

struct ResidentSocietyStatus {
    CharacterId residentId=0;
    SocietyRole role=SocietyRole::Generalist;
    double roleStrength01=0.0;
    int practicedTechnologyCount=0;
    int successfulTechniqueUses=0;
};

inline ResidentSocietyStatus observeResidentSocietyStatus(const Character& resident)
{
    ResidentSocietyStatus out;
    out.residentId=resident.id;
    for(const auto& record:resident.civilization.knowledge.all()){
        out.successfulTechniqueUses+=std::max(0,record.successfulUses);
        if(static_cast<int>(record.level)>=static_cast<int>(KnowledgeLevel::Practiced)){
            ++out.practicedTechnologyCount;
        }
    }

    const double forager=societyClamp01(
        0.52*resident.civilization.gatheringSkill
        +0.20*resident.personality.curiosity
        +0.16*resident.personality.patience
        +0.12*resident.personality.adaptability);
    const double crafter=societyClamp01(
        0.46*resident.civilization.craftingSkill
        +0.24*societyClamp01(
            static_cast<double>(out.successfulTechniqueUses)/12.0)
        +0.18*resident.personality.conscientiousness
        +0.12*resident.personality.patience);
    const double farmer=societyClamp01(
        0.48*societyKnowledge01(resident,TechniqueId::Cultivation)
        +0.22*societyClamp01(
            static_cast<double>(societyTechniqueUses(
                resident,TechniqueId::Cultivation))/6.0)
        +0.18*resident.personality.conscientiousness
        +0.12*resident.personality.patience);
    const double metalKnowledge=std::max({
        societyKnowledge01(resident,TechniqueId::CopperSmelting),
        societyKnowledge01(resident,TechniqueId::TinSmelting),
        societyKnowledge01(resident,TechniqueId::BronzeAlloying)});
    const double metallurgist=societyClamp01(
        0.52*metalKnowledge
        +0.22*resident.civilization.craftingSkill
        +0.16*resident.personality.patience
        +0.10*resident.personality.conscientiousness);
    const double educator=societyClamp01(
        0.34*resident.civilization.learningSkill
        +0.25*societyClamp01(
            static_cast<double>(out.practicedTechnologyCount)/4.0)
        +0.16*resident.personality.patience
        +0.14*resident.personality.empathy
        +0.11*resident.personality.sociability);
    const double caregiver=societyClamp01(
        0.38*resident.health.careKnowledge01
        +0.25*resident.personality.empathy
        +0.20*resident.personality.agreeableness
        +0.17*resident.personality.conscientiousness);
    const double storekeeper=societyClamp01(
        0.38*societyKnowledge01(resident,TechniqueId::PrimitiveStorage)
        +0.28*resident.personality.orderliness
        +0.22*resident.personality.conscientiousness
        +0.12*resident.civilization.craftingSkill);

    struct Candidate { SocietyRole role; double score; };
    const std::array<Candidate,7> candidates={{
        {SocietyRole::Forager,forager},
        {SocietyRole::Craftsperson,crafter},
        {SocietyRole::Farmer,farmer},
        {SocietyRole::Metallurgist,metallurgist},
        {SocietyRole::Educator,educator},
        {SocietyRole::Caregiver,caregiver},
        {SocietyRole::Storekeeper,storekeeper}
    }};
    for(const auto& candidate:candidates){
        if(candidate.score>out.roleStrength01+1e-12){
            out.roleStrength01=candidate.score;
            out.role=candidate.role;
        }
    }
    if(out.roleStrength01<0.50) out.role=SocietyRole::Generalist;
    return out;
}

struct SocietyDemandSignal {
    MaterialKind material=MaterialKind::Unknown;
    int desiredUnits=0;
    int availableUnits=0;
    int deficitUnits=0;
    double demand01=0.0;
};

inline int societyWorldMaterialUnits(const World& world,MaterialKind material)
{
    int total=0;
    for(const auto& resident:world.characters){
        if(!resident.alive) continue;
        total+=resident.civilization.inventory.count(
            ItemKind::RawMaterial,material);
    }
    for(const auto& storage:world.storageSites){
        total+=storage.inventory.count(ItemKind::RawMaterial,material);
    }
    return total;
}

inline int societyFacilityMaterialNeed(const World& world,MaterialKind material)
{
    int total=0;
    for(const auto& facility:world.facilities){
        if(facility.state==FacilityState::Operational
           || facility.state==FacilityState::Ruined) continue;
        for(const auto& requirement:facility.requirements){
            if(requirement.material==material){
                total+=std::max(0,requirement.required-requirement.delivered);
            }
        }
    }
    return total;
}

inline SocietyDemandSignal observeSocietyMaterialDemand(
    const World& world,MaterialKind material)
{
    SocietyDemandSignal out;
    out.material=material;
    int living=0;
    for(const auto& resident:world.characters) if(resident.alive) ++living;
    living=std::max(1,living);
    const int build=societyFacilityMaterialNeed(world,material);
    switch(material){
        case MaterialKind::PlantFood:
        case MaterialKind::Water: out.desiredUnits=living*6; break;
        case MaterialKind::Wood: out.desiredUnits=living*3+build; break;
        case MaterialKind::Fiber:
        case MaterialKind::Clay: out.desiredUnits=living*2+build; break;
        case MaterialKind::Stone:
        case MaterialKind::Flint:
        case MaterialKind::CopperOre:
        case MaterialKind::TinOre:
        case MaterialKind::Charcoal: out.desiredUnits=living+build; break;
        case MaterialKind::Bronze:
        case MaterialKind::CopperMetal:
        case MaterialKind::TinMetal:
            out.desiredUnits=std::max(1,living/2)+build; break;
        default: out.desiredUnits=build; break;
    }
    out.availableUnits=societyWorldMaterialUnits(world,material);
    out.deficitUnits=std::max(0,out.desiredUnits-out.availableUnits);
    out.demand01=out.desiredUnits>0
        ? societyClamp01(static_cast<double>(out.deficitUnits)
            /static_cast<double>(out.desiredUnits))
        : 0.0;
    return out;
}

inline double societyMaterialDemand01(const World& world,MaterialKind material)
{
    return observeSocietyMaterialDemand(world,material).demand01;
}

inline double residentExchangeNeed01(
    const Character& resident,MaterialKind material)
{
    const SocietyRole role=observeResidentSocietyStatus(resident).role;
    const int owned=resident.civilization.inventory.count(
        ItemKind::RawMaterial,material);
    const double scarcity=owned==0 ? 1.0 : (owned==1 ? 0.45 : 0.0);
    if(material==MaterialKind::PlantFood){
        return societyClamp01(
            0.15+0.78*resident.needs.hunger+0.12*scarcity);
    }
    if(material==MaterialKind::Wood
       || material==MaterialKind::Fiber
       || material==MaterialKind::Clay
       || material==MaterialKind::Stone
       || material==MaterialKind::Flint){
        return societyClamp01(
            0.15+0.45*scarcity
            +((role==SocietyRole::Craftsperson
               || role==SocietyRole::Farmer
               || role==SocietyRole::Storekeeper) ? 0.34 : 0.0));
    }
    if(material==MaterialKind::CopperOre
       || material==MaterialKind::TinOre
       || material==MaterialKind::Charcoal
       || material==MaterialKind::CopperMetal
       || material==MaterialKind::TinMetal
       || material==MaterialKind::Bronze){
        return societyClamp01(
            0.12+0.45*scarcity
            +(role==SocietyRole::Metallurgist ? 0.48 : 0.0));
    }
    return 0.0;
}

inline int residentExchangeReserve(
    const Character& resident,MaterialKind material)
{
    if(material==MaterialKind::PlantFood){
        return resident.needs.hunger>=0.55 ? 3 : 2;
    }
    const SocietyRole role=observeResidentSocietyStatus(resident).role;
    if(role==SocietyRole::Metallurgist
       && (material==MaterialKind::CopperOre
           || material==MaterialKind::TinOre
           || material==MaterialKind::Charcoal
           || material==MaterialKind::CopperMetal
           || material==MaterialKind::TinMetal
           || material==MaterialKind::Bronze)) return 2;
    if((role==SocietyRole::Craftsperson || role==SocietyRole::Farmer)
       && (material==MaterialKind::Wood
           || material==MaterialKind::Fiber
           || material==MaterialKind::Clay
           || material==MaterialKind::Stone
           || material==MaterialKind::Flint)) return 2;
    return 1;
}

struct SocietyExchangePlan {
    CharacterId first=0;
    CharacterId second=0;
    MaterialKind firstGives=MaterialKind::Unknown;
    MaterialKind secondGives=MaterialKind::Unknown;
    int quantityEach=1;
    double score=0.0;
    bool valid() const
    {
        return first!=0 && second!=0 && first!=second
            && firstGives!=MaterialKind::Unknown
            && secondGives!=MaterialKind::Unknown
            && firstGives!=secondGives
            && quantityEach>0;
    }
};

inline constexpr std::array<MaterialKind,12> SocietyExchangeMaterials={{
    MaterialKind::PlantFood,MaterialKind::Stone,MaterialKind::Flint,
    MaterialKind::Wood,MaterialKind::Fiber,MaterialKind::Clay,
    MaterialKind::CopperOre,MaterialKind::TinOre,MaterialKind::Charcoal,
    MaterialKind::CopperMetal,MaterialKind::TinMetal,MaterialKind::Bronze
}};

inline SocietyExchangePlan bestMutualExchangePlan(
    const Character& first,
    const Character& second,
    const RelationshipBook& relationships)
{
    SocietyExchangePlan best;
    if(!first.alive || !second.alive || first.id==second.id) return best;
    const Relationship* a=relationships.find(first.id,second.id);
    const Relationship* b=relationships.find(second.id,first.id);
    const double trust=0.5*(a ? a->trust : 0.08)
        +0.5*(b ? b->trust : 0.08);

    for(const auto firstMaterial:SocietyExchangeMaterials){
        const int firstOwned=first.civilization.inventory.count(
            ItemKind::RawMaterial,firstMaterial);
        if(firstOwned<=residentExchangeReserve(first,firstMaterial)) continue;
        const double secondNeed=residentExchangeNeed01(second,firstMaterial);
        if(secondNeed<0.45) continue;

        for(const auto secondMaterial:SocietyExchangeMaterials){
            if(secondMaterial==firstMaterial) continue;
            const int secondOwned=second.civilization.inventory.count(
                ItemKind::RawMaterial,secondMaterial);
            if(secondOwned<=residentExchangeReserve(second,secondMaterial)) continue;
            const double firstNeed=residentExchangeNeed01(first,secondMaterial);
            if(firstNeed<0.45) continue;
            const double score=societyClamp01(
                0.42*firstNeed+0.42*secondNeed+0.16*trust);
            if(score>best.score+1e-12){
                best={first.id,second.id,firstMaterial,secondMaterial,1,score};
            }
        }
    }
    return best;
}

inline bool executeMutualExchange(
    Character& first,Character& second,const SocietyExchangePlan& plan)
{
    if(!plan.valid() || first.id!=plan.first || second.id!=plan.second){
        return false;
    }
    auto& a=first.civilization.inventory;
    auto& b=second.civilization.inventory;
    if(a.count(ItemKind::RawMaterial,plan.firstGives)<plan.quantityEach
       || b.count(ItemKind::RawMaterial,plan.secondGives)<plan.quantityEach){
        return false;
    }
    if(!a.transferTo(
        b,ItemKind::RawMaterial,plan.firstGives,plan.quantityEach)) return false;
    if(!b.transferTo(
        a,ItemKind::RawMaterial,plan.secondGives,plan.quantityEach)){
        b.transferTo(
            a,ItemKind::RawMaterial,plan.firstGives,plan.quantityEach);
        return false;
    }
    return true;
}

inline bool isSocietyExchangeFact(const SocialFact& fact)
{
    return fact.proposition.rfind("exchange:",0)==0;
}

inline SocialFactId societyExchangeFactId(
    std::uint64_t seed,CharacterId first,CharacterId second,
    MaterialKind firstGives,MaterialKind secondGives,int minute)
{
    std::uint64_t value=mixKnowledge64(seed^0x434545584348414Eull);
    value=mixKnowledge64(value^first);
    value=mixKnowledge64(value^(second<<1));
    value=mixKnowledge64(value^static_cast<std::uint64_t>(
        static_cast<int>(firstGives)+1));
    value=mixKnowledge64(value^(static_cast<std::uint64_t>(
        static_cast<int>(secondGives)+1)<<8));
    value=mixKnowledge64(value^static_cast<std::uint64_t>(
        std::max(0,minute)));
    return value==0 ? 1 : value;
}

inline const SocialFact* registerSocietyExchangeFact(
    SocialKnowledgeBook& book,
    Character& first,
    Character& second,
    const SocietyExchangePlan& plan,
    int minute,
    std::uint64_t seed)
{
    if(!plan.valid()) return nullptr;
    SocialFact fact;
    fact.id=societyExchangeFactId(
        seed,first.id,second.id,
        plan.firstGives,plan.secondGives,minute);
    fact.subject=first.id;
    fact.proposition=std::string("exchange:")
        +std::to_string(static_cast<int>(plan.firstGives))
        +":"+std::to_string(static_cast<int>(plan.secondGives));
    fact.where="settlement-exchange";
    fact.eventMinute=minute;
    fact.supports=true;
    fact.importance=0.58;
    fact.confidence=0.96;
    fact.emotionValence=0.22;
    fact.emotionIntensity=0.28;
    if(!book.registerFact(fact)) return nullptr;
    book.recordDirectWitness(
        fact.id,first.id,first.memory,first.beliefs,minute);
    book.recordDirectWitness(
        fact.id,second.id,second.memory,second.beliefs,minute);
    return book.findFact(fact.id);
}

inline bool isSocietyTechniqueFact(const SocialFact& fact)
{
    for(const auto& technology:TechnologyRegistry){
        if(factRepresentsTechnique(fact,technology.legacyTechnique)) return true;
    }
    return false;
}

inline int recentTeachingReceiptCount(
    const SocialKnowledgeBook& book,int minute,int window=7*24*60)
{
    const int start=std::max(0,minute-window);
    int count=0;
    for(const auto& receipt:book.receipts()){
        if(receipt.source!=MemorySource::ToldByOther
           || receipt.learnedMinute<start) continue;
        const SocialFact* fact=book.findFact(receipt.factId);
        if(fact!=nullptr && isSocietyTechniqueFact(*fact)) ++count;
    }
    return count;
}

inline int societyExchangeFactCount(const SocialKnowledgeBook& book)
{
    int count=0;
    for(const auto& fact:book.facts()){
        if(isSocietyExchangeFact(fact)) ++count;
    }
    return count;
}

inline bool societyCanTeachTechnique(const Character& resident)
{
    if(!resident.alive) return false;
    const LifeStageProfile stage=lifeStageProfile(resident.lifeStage);
    return stage.autonomy>=0.65
        || resident.lifeStage==LifeStage::Elderly;
}

inline bool societyCanLearnTechnique(const Character& resident)
{
    if(!resident.alive) return false;
    const LifeStageProfile stage=lifeStageProfile(resident.lifeStage);
    return stage.canAttendSchool
        || stage.autonomy>=0.45;
}

inline double societyLearningRateMultiplier(const Character& resident)
{
    return std::max(
        0.55,
        std::min(
            1.50,
            lifeStageProfile(resident.lifeStage).skillLearningRate));
}

inline int societyTeachingReceiptCountBetween(
    const SocialKnowledgeBook& book,
    CharacterId teacher,
    CharacterId learner)
{
    int count=0;
    for(const KnowledgeReceipt& receipt:book.receipts()){
        if(receipt.holder!=learner
           || receipt.immediateSource!=teacher
           || receipt.source!=MemorySource::ToldByOther) continue;
        const SocialFact* fact=book.findFact(receipt.factId);
        if(fact!=nullptr && isSocietyTechniqueFact(*fact)) ++count;
    }
    return count;
}

inline std::string societyApprenticeshipProposition(
    CharacterId teacher,
    CharacterId learner)
{
    return std::string("apprenticeship:")
        +std::to_string(teacher)
        +":"
        +std::to_string(learner);
}

inline bool hasSocietyApprenticeship(
    const SocialKnowledgeBook& book,
    CharacterId teacher,
    CharacterId learner)
{
    const std::string proposition=
        societyApprenticeshipProposition(teacher,learner);
    for(const SocialFact& fact:book.facts()){
        if(fact.proposition==proposition) return true;
    }
    return false;
}

inline SocialFactId societyAssociationFactId(
    std::uint64_t seed,
    std::uint64_t domain,
    CharacterId subject,
    CharacterId other,
    int discriminator)
{
    std::uint64_t value=mixKnowledge64(seed^domain);
    value=mixKnowledge64(value^subject);
    value=mixKnowledge64(value^(other<<1));
    value=mixKnowledge64(
        value^static_cast<std::uint64_t>(std::max(0,discriminator)));
    return value==0 ? 1 : value;
}

inline const SocialFact* registerSocietyApprenticeshipIfQualified(
    SocialKnowledgeBook& book,
    Character& teacher,
    Character& learner,
    int minute,
    std::uint64_t seed)
{
    if(teacher.id==0
       || learner.id==0
       || teacher.id==learner.id
       || !societyCanTeachTechnique(teacher)
       || !societyCanLearnTechnique(learner)) return nullptr;
    if(hasSocietyApprenticeship(book,teacher.id,learner.id)){
        const std::string proposition=
            societyApprenticeshipProposition(teacher.id,learner.id);
        for(const SocialFact& fact:book.facts()){
            if(fact.proposition==proposition) return &fact;
        }
    }
    if(societyTeachingReceiptCountBetween(
        book,teacher.id,learner.id)<2) return nullptr;

    SocialFact fact;
    fact.id=societyAssociationFactId(
        seed,0x41505052454E5449ull,
        teacher.id,learner.id,1);
    fact.subject=teacher.id;
    fact.proposition=
        societyApprenticeshipProposition(teacher.id,learner.id);
    fact.where="learning-network";
    fact.eventMinute=minute;
    fact.supports=true;
    fact.importance=0.76;
    fact.confidence=0.96;
    fact.emotionValence=0.24;
    fact.emotionIntensity=0.34;
    if(!book.registerFact(fact)) return nullptr;
    book.recordDirectWitness(
        fact.id,teacher.id,teacher.memory,teacher.beliefs,minute);
    book.recordDirectWitness(
        fact.id,learner.id,learner.memory,learner.beliefs,minute);
    return book.findFact(fact.id);
}

inline int societyExchangePairCount(
    const SocialKnowledgeBook& book,
    CharacterId first,
    CharacterId second)
{
    int count=0;
    for(const SocialFact& fact:book.facts()){
        if(!isSocietyExchangeFact(fact)) continue;
        if(book.hasReceipt(first,fact.id)
           && book.hasReceipt(second,fact.id)){
            ++count;
        }
    }
    return count;
}

inline std::string societyTradePartnershipProposition(
    CharacterId first,
    CharacterId second)
{
    const CharacterId low=std::min(first,second);
    const CharacterId high=std::max(first,second);
    return std::string("trade-partnership:")
        +std::to_string(low)
        +":"
        +std::to_string(high);
}

inline bool hasSocietyTradePartnership(
    const SocialKnowledgeBook& book,
    CharacterId first,
    CharacterId second)
{
    const std::string proposition=
        societyTradePartnershipProposition(first,second);
    for(const SocialFact& fact:book.facts()){
        if(fact.proposition==proposition) return true;
    }
    return false;
}

inline const SocialFact* registerSocietyTradePartnershipIfQualified(
    SocialKnowledgeBook& book,
    Character& first,
    Character& second,
    int minute,
    std::uint64_t seed)
{
    if(first.id==0 || second.id==0 || first.id==second.id) return nullptr;
    const std::string proposition=
        societyTradePartnershipProposition(first.id,second.id);
    for(const SocialFact& fact:book.facts()){
        if(fact.proposition==proposition) return &fact;
    }
    if(societyExchangePairCount(book,first.id,second.id)<2) return nullptr;

    const CharacterId low=std::min(first.id,second.id);
    const CharacterId high=std::max(first.id,second.id);
    SocialFact fact;
    fact.id=societyAssociationFactId(
        seed,0x5452414445504152ull,low,high,1);
    fact.subject=low;
    fact.proposition=proposition;
    fact.where="exchange-network";
    fact.eventMinute=minute;
    fact.supports=true;
    fact.importance=0.70;
    fact.confidence=0.97;
    fact.emotionValence=0.20;
    fact.emotionIntensity=0.28;
    if(!book.registerFact(fact)) return nullptr;

    Character& lowResident=first.id==low ? first : second;
    Character& highResident=first.id==high ? first : second;
    book.recordDirectWitness(
        fact.id,lowResident.id,
        lowResident.memory,lowResident.beliefs,minute);
    book.recordDirectWitness(
        fact.id,highResident.id,
        highResident.memory,highResident.beliefs,minute);
    return book.findFact(fact.id);
}

inline std::string societyInstitutionMembershipProposition(
    SocietyInstitutionKind kind)
{
    return std::string("institution:")
        +std::to_string(static_cast<int>(kind));
}

inline bool residentInstitutionMember(
    const SocialKnowledgeBook& book,
    CharacterId resident,
    SocietyInstitutionKind kind)
{
    const std::string proposition=
        societyInstitutionMembershipProposition(kind);
    for(const SocialFact& fact:book.facts()){
        if(fact.subject==resident
           && fact.proposition==proposition) return true;
    }
    return false;
}

inline const SocialFact* registerSocietyInstitutionMembership(
    SocialKnowledgeBook& book,
    Character& resident,
    SocietyInstitutionKind kind,
    int minute,
    std::uint64_t seed)
{
    const std::string proposition=
        societyInstitutionMembershipProposition(kind);
    for(const SocialFact& fact:book.facts()){
        if(fact.subject==resident.id
           && fact.proposition==proposition) return &fact;
    }

    SocialFact fact;
    fact.id=societyAssociationFactId(
        seed,0x494E535449545554ull,
        resident.id,0,static_cast<int>(kind)+1);
    fact.subject=resident.id;
    fact.proposition=proposition;
    fact.where="society-organization";
    fact.eventMinute=minute;
    fact.supports=true;
    fact.importance=0.62;
    fact.confidence=0.98;
    fact.emotionValence=0.12;
    fact.emotionIntensity=0.18;
    if(!book.registerFact(fact)) return nullptr;
    book.recordDirectWitness(
        fact.id,resident.id,
        resident.memory,resident.beliefs,minute);
    return book.findFact(fact.id);
}

struct SocietyCoordinationDirective {
    CharacterId residentId=0;
    SocietyCoordinationTask task=SocietyCoordinationTask::None;
    MaterialKind material=MaterialKind::Unknown;
    TechniqueId technique=TechniqueId::None;
    double priority01=0.0;
    bool institutionBacked=false;
    SocietyResourceDisposition resourceDisposition=
        SocietyResourceDisposition::PersonalReserve;
};

inline int societyHouseholdMemberCount(
    const HouseholdBook* households,
    CharacterId resident)
{
    if(households==nullptr) return 1;
    const Household* household=households->householdOf(resident);
    return household==nullptr
        ? 1
        : std::max(1,static_cast<int>(household->members.size()));
}

inline SocietyResourceDisposition observeSocietyResourceDisposition(
    const Character& resident,
    MaterialKind material,
    const SocialKnowledgeBook& book,
    const HouseholdBook* households=nullptr)
{
    if(material==MaterialKind::Unknown || material==MaterialKind::Water){
        return SocietyResourceDisposition::PersonalReserve;
    }
    const int owned=resident.civilization.inventory.count(
        ItemKind::RawMaterial,material);
    const int personalReserve=residentExchangeReserve(resident,material);
    if(owned<=personalReserve){
        return SocietyResourceDisposition::PersonalReserve;
    }

    const int householdMembers=
        societyHouseholdMemberCount(households,resident.id);
    int householdReserve=personalReserve;
    if(householdMembers>1){
        if(material==MaterialKind::PlantFood){
            householdReserve+=householdMembers-1;
        }else if(material==MaterialKind::Wood
                 || material==MaterialKind::Fiber
                 || material==MaterialKind::Clay){
            householdReserve+=std::min(2,householdMembers-1);
        }
    }
    if(owned<=householdReserve){
        return SocietyResourceDisposition::HouseholdReserve;
    }

    if(residentInstitutionMember(
        book,resident.id,SocietyInstitutionKind::StorageCommons)
       && owned>=householdReserve+2){
        return SocietyResourceDisposition::SharedSurplus;
    }
    return SocietyResourceDisposition::TradableSurplus;
}

inline SocietyCoordinationDirective observeSocietyCoordinationDirective(
    const World& world,
    const SocialKnowledgeBook& book,
    const Character& resident,
    const HouseholdBook* households=nullptr)
{
    SocietyCoordinationDirective out;
    out.residentId=resident.id;
    if(!resident.alive || !lifeStageProfile(resident.lifeStage).canWork){
        return out;
    }

    const ResidentSocietyStatus status=
        observeResidentSocietyStatus(resident);
    const double foodDemand=
        societyMaterialDemand01(world,MaterialKind::PlantFood);
    const double waterDemand=
        societyMaterialDemand01(world,MaterialKind::Water);

    const auto bestSupplyMaterial=[&](){
        constexpr std::array<MaterialKind,8> materials={{
            MaterialKind::Wood,MaterialKind::Fiber,MaterialKind::Clay,
            MaterialKind::Stone,MaterialKind::Flint,
            MaterialKind::CopperOre,MaterialKind::TinOre,
            MaterialKind::Charcoal
        }};
        MaterialKind best=MaterialKind::Wood;
        double bestDemand=-1.0;
        for(const MaterialKind material:materials){
            const double demand=societyMaterialDemand01(world,material);
            if(demand>bestDemand+1e-12){
                bestDemand=demand;
                best=material;
            }
        }
        return std::pair<MaterialKind,double>{best,std::max(0.0,bestDemand)};
    };
    const auto supply=bestSupplyMaterial();

    auto set=[&](
        SocietyCoordinationTask task,
        double priority,
        MaterialKind material=MaterialKind::Unknown,
        TechniqueId technique=TechniqueId::None,
        SocietyInstitutionKind institution=SocietyInstitutionKind::ProductionNetwork){
        out.task=task;
        out.priority01=societyClamp01(priority);
        out.material=material;
        out.technique=technique;
        out.institutionBacked=
            residentInstitutionMember(book,resident.id,institution);
    };

    switch(status.role){
        case SocietyRole::Forager:
            if(foodDemand>=waterDemand && foodDemand>=supply.second){
                set(SocietyCoordinationTask::ProvisionFood,
                    0.52+0.42*foodDemand,MaterialKind::PlantFood);
            }else if(waterDemand>=supply.second){
                set(SocietyCoordinationTask::ProvisionWater,
                    0.52+0.42*waterDemand,MaterialKind::Water);
            }else{
                set(SocietyCoordinationTask::MaterialSupply,
                    0.48+0.44*supply.second,supply.first);
            }
            break;
        case SocietyRole::Craftsperson:
            set(SocietyCoordinationTask::ToolProduction,
                0.56+0.20*status.roleStrength01);
            break;
        case SocietyRole::Farmer:
            set(SocietyCoordinationTask::Cultivation,
                0.58+0.34*foodDemand,
                MaterialKind::PlantFood,TechniqueId::Cultivation);
            break;
        case SocietyRole::Metallurgist:
            set(SocietyCoordinationTask::Metallurgy,
                0.58+0.28*std::max({
                    societyMaterialDemand01(world,MaterialKind::CopperOre),
                    societyMaterialDemand01(world,MaterialKind::TinOre),
                    societyMaterialDemand01(world,MaterialKind::Charcoal)}));
            break;
        case SocietyRole::Storekeeper:
            set(SocietyCoordinationTask::SharedStorage,
                0.58+0.22*status.roleStrength01,
                supply.first,TechniqueId::None,
                SocietyInstitutionKind::StorageCommons);
            break;
        case SocietyRole::Educator:
            set(SocietyCoordinationTask::Education,
                0.58+0.22*status.roleStrength01,
                MaterialKind::Unknown,TechniqueId::None,
                SocietyInstitutionKind::LearningCircle);
            break;
        case SocietyRole::Caregiver: {
            int living=0;
            int vulnerable=0;
            for(const Character& candidate:world.characters){
                if(!candidate.alive) continue;
                ++living;
                if(candidate.health.illnessSeverity>=0.18
                   || candidate.health.injurySeverity>=0.18) ++vulnerable;
            }
            set(SocietyCoordinationTask::Care,
                0.48+0.46*static_cast<double>(vulnerable)
                    /static_cast<double>(std::max(1,living)),
                MaterialKind::Unknown,TechniqueId::None,
                SocietyInstitutionKind::CareNetwork);
            break;
        }
        case SocietyRole::Generalist:
        default:
            if(foodDemand>=waterDemand && foodDemand>=supply.second){
                set(SocietyCoordinationTask::ProvisionFood,
                    0.40+0.36*foodDemand,MaterialKind::PlantFood);
            }else if(waterDemand>=supply.second){
                set(SocietyCoordinationTask::ProvisionWater,
                    0.40+0.36*waterDemand,MaterialKind::Water);
            }else{
                set(SocietyCoordinationTask::MaterialSupply,
                    0.38+0.38*supply.second,supply.first);
            }
            break;
    }

    if(residentInstitutionMember(
            book,resident.id,SocietyInstitutionKind::InquiryCircle)
       && resident.personality.curiosity>=0.62
       && out.priority01<0.70){
        set(SocietyCoordinationTask::Inquiry,
            0.62+0.18*resident.personality.curiosity,
            MaterialKind::Unknown,TechniqueId::None,
            SocietyInstitutionKind::InquiryCircle);
    }

    if(out.material!=MaterialKind::Unknown){
        out.resourceDisposition=observeSocietyResourceDisposition(
            resident,out.material,book,households);
    }
    return out;
}

inline bool isSocietySharedContributionFact(const SocialFact& fact)
{
    return fact.proposition.rfind("shared-contribution:",0)==0;
}

inline const SocialFact* registerSocietySharedContributionFact(
    SocialKnowledgeBook& book,
    Character& resident,
    MaterialKind material,
    int quantity,
    int minute,
    std::uint64_t seed)
{
    if(resident.id==0
       || material==MaterialKind::Unknown
       || quantity<=0) return nullptr;
    SocialFact fact;
    fact.id=societyAssociationFactId(
        seed,0x534841524544434Full,
        resident.id,
        static_cast<CharacterId>(static_cast<int>(material)+1),
        std::max(1,minute));
    fact.subject=resident.id;
    fact.proposition=std::string("shared-contribution:")
        +std::to_string(static_cast<int>(material))
        +":"+std::to_string(quantity);
    fact.where="storage-commons";
    fact.eventMinute=minute;
    fact.supports=true;
    fact.importance=0.55;
    fact.confidence=0.98;
    fact.emotionValence=0.16;
    fact.emotionIntensity=0.20;
    if(!book.registerFact(fact)) return nullptr;
    book.recordDirectWitness(
        fact.id,resident.id,resident.memory,resident.beliefs,minute);
    return book.findFact(fact.id);
}

inline int societyRecordTabletUnits(const World& world)
{
    int total=0;
    for(const Character& resident:world.characters){
        if(!resident.alive) continue;
        total+=resident.civilization.inventory.count(
            ItemKind::RecordTablet,MaterialKind::Unknown,true);
    }
    for(const StorageSite& storage:world.storageSites){
        total+=storage.inventory.count(
            ItemKind::RecordTablet,MaterialKind::Unknown,true);
    }
    return total;
}

inline bool isSocietyDurableRecordFact(const SocialFact& fact)
{
    return fact.proposition.rfind("durable-record:",0)==0;
}

inline bool societyFactAlreadyDurablyRecorded(
    const SocialKnowledgeBook& book,
    SocialFactId targetFact)
{
    const std::string proposition=
        std::string("durable-record:")+std::to_string(targetFact);
    for(const SocialFact& fact:book.facts()){
        if(fact.proposition==proposition) return true;
    }
    return false;
}

inline const SocialFact* bestSocietyDurableRecordCandidate(
    const SocialKnowledgeBook& book)
{
    const SocialFact* best=nullptr;
    for(const SocialFact& fact:book.facts()){
        if(isSocietyDurableRecordFact(fact)
           || fact.importance<0.58
           || societyFactAlreadyDurablyRecorded(book,fact.id)) continue;
        if(best==nullptr
           || fact.importance>best->importance+1e-12
           || (std::abs(fact.importance-best->importance)<=1e-12
               && fact.eventMinute<best->eventMinute)
           || (std::abs(fact.importance-best->importance)<=1e-12
               && fact.eventMinute==best->eventMinute
               && fact.id<best->id)){
            best=&fact;
        }
    }
    return best;
}

inline const SocialFact* registerSocietyDurableRecordFact(
    SocialKnowledgeBook& book,
    Character& recorder,
    const SocialFact& target,
    MaterialKind medium,
    int minute,
    std::uint64_t seed)
{
    if(recorder.id==0 || societyFactAlreadyDurablyRecorded(book,target.id)){
        return nullptr;
    }
    std::uint64_t value=mixKnowledge64(
        seed^0x44555241424C4552ull);
    value=mixKnowledge64(value^recorder.id);
    value=mixKnowledge64(value^target.id);
    value=mixKnowledge64(
        value^static_cast<std::uint64_t>(static_cast<int>(medium)+1));
    SocialFact fact;
    fact.id=value==0 ? 1 : value;
    fact.subject=recorder.id;
    fact.proposition=std::string("durable-record:")
        +std::to_string(target.id);
    fact.where="record-media";
    fact.eventMinute=minute;
    fact.supports=true;
    fact.importance=0.86;
    fact.confidence=0.99;
    fact.emotionValence=0.10;
    fact.emotionIntensity=0.18;
    if(!book.registerFact(fact)) return nullptr;
    book.recordDirectWitness(
        fact.id,recorder.id,recorder.memory,recorder.beliefs,minute);
    return book.findFact(fact.id);
}

inline int societyFactCountWithPrefix(
    const SocialKnowledgeBook& book,
    const std::string& prefix)
{
    int count=0;
    for(const SocialFact& fact:book.facts()){
        if(fact.proposition.rfind(prefix,0)==0) ++count;
    }
    return count;
}

inline CollectiveRecordStage observeCollectiveRecordStage(
    const World& world,const SocialKnowledgeBook& book)
{
    int techniqueFacts=0;
    int multiHop=0;
    for(const auto& fact:book.facts()){
        if(isSocietyTechniqueFact(fact)) ++techniqueFacts;
    }
    for(const auto& receipt:book.receipts()){
        if(receipt.hopCount()>=2) ++multiHop;
    }
    const int exchanges=societyExchangeFactCount(book);
    if(!world.storageSites.empty()
       && techniqueFacts>=6 && exchanges>=2 && multiHop>=2){
        return CollectiveRecordStage::ProtoRecordkeeping;
    }
    if(techniqueFacts>=4 && multiHop>=1){
        return CollectiveRecordStage::RepeatedTradition;
    }
    if(techniqueFacts>=2 || book.receipts().size()>=4){
        return CollectiveRecordStage::OralTradition;
    }
    return CollectiveRecordStage::Ephemeral;
}

struct SocietyInstitutionStatus {
    SocietyInstitutionKind kind=SocietyInstitutionKind::LearningCircle;
    bool active=false;
    double strength01=0.0;
    int evidenceCount=0;
};

struct SocietyWorldObservation {
    int livingResidentCount=0;
    int specializedResidentCount=0;
    int educatorCount=0;
    int producerCount=0;
    int caregiverCount=0;
    int storekeeperCount=0;
    int recentTeachingReceipts=0;
    int exchangeFactCount=0;
    int apprenticeshipCount=0;
    int tradePartnershipCount=0;
    int institutionMembershipCount=0;
    int activeInstitutionCount=0;
    int sharedContributionFactCount=0;
    int durableRecordFactCount=0;
    int recordMediaUnits=0;
    int coordinatedResidentCount=0;
    CollectiveRecordStage recordStage=CollectiveRecordStage::Ephemeral;
    std::vector<ResidentSocietyStatus> residents;
    std::vector<SocietyCoordinationDirective> coordination;
    std::vector<SocietyDemandSignal> demands;
    std::vector<SocietyInstitutionStatus> institutions;
};

inline SocietyWorldObservation buildSocietyWorldObservation(
    const World& world,const SocialKnowledgeBook& book)
{
    SocietyWorldObservation out;
    for(const auto& resident:world.characters){
        if(!resident.alive) continue;
        ++out.livingResidentCount;
        auto status=observeResidentSocietyStatus(resident);
        if(status.role!=SocietyRole::Generalist) ++out.specializedResidentCount;
        if(status.role==SocietyRole::Educator) ++out.educatorCount;
        if(status.role==SocietyRole::Caregiver) ++out.caregiverCount;
        if(status.role==SocietyRole::Storekeeper) ++out.storekeeperCount;
        if(status.role==SocietyRole::Forager
           || status.role==SocietyRole::Craftsperson
           || status.role==SocietyRole::Farmer
           || status.role==SocietyRole::Metallurgist) ++out.producerCount;
        out.residents.push_back(status);
    }
    out.recentTeachingReceipts=recentTeachingReceiptCount(book,world.minute);
    out.exchangeFactCount=societyExchangeFactCount(book);
    out.apprenticeshipCount=
        societyFactCountWithPrefix(book,"apprenticeship:");
    out.tradePartnershipCount=
        societyFactCountWithPrefix(book,"trade-partnership:");
    out.institutionMembershipCount=
        societyFactCountWithPrefix(book,"institution:");
    out.sharedContributionFactCount=
        societyFactCountWithPrefix(book,"shared-contribution:");
    out.durableRecordFactCount=
        societyFactCountWithPrefix(book,"durable-record:");
    out.recordMediaUnits=societyRecordTabletUnits(world);
    out.recordStage=observeCollectiveRecordStage(world,book);
    for(const Character& resident:world.characters){
        if(!resident.alive) continue;
        SocietyCoordinationDirective directive=
            observeSocietyCoordinationDirective(world,book,resident,nullptr);
        if(directive.task!=SocietyCoordinationTask::None){
            ++out.coordinatedResidentCount;
        }
        out.coordination.push_back(directive);
    }

    constexpr std::array<MaterialKind,9> materials={{
        MaterialKind::PlantFood,MaterialKind::Water,MaterialKind::Wood,
        MaterialKind::Fiber,MaterialKind::Clay,MaterialKind::CopperOre,
        MaterialKind::TinOre,MaterialKind::Charcoal,MaterialKind::Bronze
    }};
    for(const auto material:materials){
        out.demands.push_back(observeSocietyMaterialDemand(world,material));
    }

    auto push=[&](SocietyInstitutionKind kind,bool active,double strength,int evidence){
        SocietyInstitutionStatus status;
        status.kind=kind;
        status.active=active;
        status.strength01=societyClamp01(strength);
        status.evidenceCount=std::max(0,evidence);
        if(active) ++out.activeInstitutionCount;
        out.institutions.push_back(status);
    };
    const double living=static_cast<double>(std::max(1,out.livingResidentCount));
    push(
        SocietyInstitutionKind::LearningCircle,
        out.educatorCount>0 && out.recentTeachingReceipts>=2,
        0.45*static_cast<double>(out.educatorCount)/living
          +0.55*societyClamp01(
              static_cast<double>(out.recentTeachingReceipts)/8.0),
        out.recentTeachingReceipts);
    push(
        SocietyInstitutionKind::ProductionNetwork,
        out.producerCount>=2,
        static_cast<double>(out.producerCount)/living,
        out.producerCount);

    int stored=0;
    for(const auto& storage:world.storageSites){
        for(const auto& stack:storage.inventory.stacks()){
            stored+=std::max(0,stack.quantity);
        }
    }
    push(
        SocietyInstitutionKind::StorageCommons,
        !world.storageSites.empty() && stored>0,
        societyClamp01(static_cast<double>(stored)
            /static_cast<double>(std::max(1,out.livingResidentCount*8))),
        stored);

    int vulnerable=0;
    for(const auto& resident:world.characters){
        if(resident.alive
           && (resident.health.illnessSeverity>=0.18
               || resident.health.injurySeverity>=0.18)) ++vulnerable;
    }
    push(
        SocietyInstitutionKind::CareNetwork,
        out.caregiverCount>0 && vulnerable>0,
        0.55*static_cast<double>(out.caregiverCount)/living
          +0.45*static_cast<double>(vulnerable)/living,
        vulnerable);
    push(
        SocietyInstitutionKind::ExchangeNetwork,
        out.exchangeFactCount>=2,
        societyClamp01(static_cast<double>(out.exchangeFactCount)/8.0),
        out.exchangeFactCount);

    int discoveries=0;
    const int start=std::max(0,world.minute-14*24*60);
    for(const auto& fact:book.facts()){
        if(fact.eventMinute>=start
           && fact.importance>=0.90
           && isSocietyTechniqueFact(fact)) ++discoveries;
    }
    push(
        SocietyInstitutionKind::InquiryCircle,
        discoveries>=2 && (out.educatorCount>0 || out.producerCount>=2),
        societyClamp01(
            0.5*static_cast<double>(discoveries)/4.0
            +0.5*static_cast<double>(
                out.educatorCount+out.producerCount)/living),
        discoveries);
    return out;
}

} // namespace lifelens
