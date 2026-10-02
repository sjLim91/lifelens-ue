#include "lifelens/Simulation.h"
#include "lifelens/SocietyEconomy.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace lifelens {
namespace {

int residentExchangeParticipationCount(
    const SocialKnowledgeBook& book,
    CharacterId resident)
{
    int count=0;
    for(const SocialFact& fact:book.facts()){
        if(isSocietyExchangeFact(fact)
           && book.hasReceipt(resident,fact.id)){
            ++count;
        }
    }
    return count;
}

bool residentHasSocietyAssociation(
    const SocialKnowledgeBook& book,
    CharacterId resident,
    const std::string& prefix)
{
    for(const SocialFact& fact:book.facts()){
        if(fact.proposition.rfind(prefix,0)==0
           && book.hasReceipt(resident,fact.id)){
            return true;
        }
    }
    return false;
}

bool institutionIsActive(
    const SocietyWorldObservation& observation,
    SocietyInstitutionKind kind)
{
    for(const SocietyInstitutionStatus& institution:observation.institutions){
        if(institution.kind==kind) return institution.active;
    }
    return false;
}

bool residentQualifiesForInstitution(
    const Character& resident,
    const SocialKnowledgeBook& book,
    SocietyInstitutionKind kind)
{
    const ResidentSocietyStatus society=
        observeResidentSocietyStatus(resident);
    switch(kind){
        case SocietyInstitutionKind::LearningCircle:
            return society.role==SocietyRole::Educator
                || society.practicedTechnologyCount>0
                || residentHasSocietyAssociation(
                    book,resident.id,"apprenticeship:");
        case SocietyInstitutionKind::ProductionNetwork:
            return society.role==SocietyRole::Forager
                || society.role==SocietyRole::Craftsperson
                || society.role==SocietyRole::Farmer
                || society.role==SocietyRole::Metallurgist;
        case SocietyInstitutionKind::StorageCommons:
            return society.role==SocietyRole::Storekeeper
                || resident.civilization.knowledge.knowsAtLeast(
                    TechniqueId::PrimitiveStorage,
                    KnowledgeLevel::Reproducible);
        case SocietyInstitutionKind::CareNetwork:
            return society.role==SocietyRole::Caregiver
                || resident.health.careKnowledge01>=0.35;
        case SocietyInstitutionKind::ExchangeNetwork:
            return residentExchangeParticipationCount(
                    book,resident.id)>=2
                || residentHasSocietyAssociation(
                    book,resident.id,"trade-partnership:");
        case SocietyInstitutionKind::InquiryCircle:
            return society.practicedTechnologyCount>0
                && (society.role==SocietyRole::Educator
                    || society.role==SocietyRole::Craftsperson
                    || society.role==SocietyRole::Metallurgist);
    }
    return false;
}

} // namespace

void Simulation::advanceSocietyExchange()
{
    if(world_.minute<=0 || world_.minute%60!=0) return;

    SocietyExchangePlan best;
    for(std::size_t i=0;i<world_.characters.size();++i){
        Character& first=world_.characters[i];
        if(!first.alive || requiresDirectCare(first.lifeStage)) continue;
        const auto firstRuntime=runtime_.find(first.id);
        if(firstRuntime==runtime_.end()
           || firstRuntime->second.pendingContext.active()
           || firstRuntime->second.socialActive) continue;

        for(std::size_t j=i+1;j<world_.characters.size();++j){
            Character& second=world_.characters[j];
            if(!second.alive || requiresDirectCare(second.lifeStage)) continue;
            const auto secondRuntime=runtime_.find(second.id);
            if(secondRuntime==runtime_.end()
               || secondRuntime->second.pendingContext.active()
               || secondRuntime->second.socialActive) continue;

            const int distance=
                std::abs(firstRuntime->second.pos.x-secondRuntime->second.pos.x)
                +std::abs(firstRuntime->second.pos.y-secondRuntime->second.pos.y);
            if(distance>1) continue;

            SocietyExchangePlan candidate=
                bestMutualExchangePlan(first,second,relationships_);
            if(candidate.valid()){
                if(hasSocietyTradePartnership(
                    socialKnowledge_,first.id,second.id)){
                    candidate.score=societyClamp01(candidate.score+0.08);
                }
                if(residentInstitutionMember(
                       socialKnowledge_,first.id,
                       SocietyInstitutionKind::ExchangeNetwork)
                   && residentInstitutionMember(
                       socialKnowledge_,second.id,
                       SocietyInstitutionKind::ExchangeNetwork)){
                    candidate.score=societyClamp01(candidate.score+0.05);
                }
            }
            if(candidate.score>best.score+1e-12){
                best=candidate;
            }
        }
    }

    constexpr double ExchangeThreshold=0.66;
    if(!best.valid() || best.score<ExchangeThreshold) return;

    Character* first=nullptr;
    Character* second=nullptr;
    for(auto& resident:world_.characters){
        if(resident.id==best.first) first=&resident;
        if(resident.id==best.second) second=&resident;
    }
    if(first==nullptr || second==nullptr) return;
    if(!executeMutualExchange(*first,*second,best)) return;

    registerSocietyExchangeFact(
        socialKnowledge_,*first,*second,best,world_.minute,world_.seed);
    const bool hadPartnership=
        hasSocietyTradePartnership(
            socialKnowledge_,first->id,second->id);
    const SocialFact* partnership=
        registerSocietyTradePartnershipIfQualified(
            socialKnowledge_,*first,*second,
            world_.minute,world_.seed);

    relationships_.getOrCreate(first->id,second->id).apply(
        relationshipDeltaFor(RelationshipEvent::SharedPositiveExperience,0.35));
    relationships_.getOrCreate(second->id,first->id).apply(
        relationshipDeltaFor(RelationshipEvent::SharedPositiveExperience,0.35));

    std::ostringstream log;
    log<<first->name<<" exchanged "<<materialName(best.firstGives)
       <<" with "<<second->name<<" for "<<materialName(best.secondGives);
    if(partnership!=nullptr && !hadPartnership){
        log<<" -> trade partnership";
    }
    emit(log.str());
}

void Simulation::advanceSocietyInstitutions()
{
    if(world_.minute<=0 || world_.minute%360!=0) return;

    const SocietyWorldObservation observation=
        buildSocietyWorldObservation(world_,socialKnowledge_);
    constexpr std::array<SocietyInstitutionKind,6> kinds={{
        SocietyInstitutionKind::LearningCircle,
        SocietyInstitutionKind::ProductionNetwork,
        SocietyInstitutionKind::StorageCommons,
        SocietyInstitutionKind::CareNetwork,
        SocietyInstitutionKind::ExchangeNetwork,
        SocietyInstitutionKind::InquiryCircle
    }};

    for(const SocietyInstitutionKind kind:kinds){
        if(!institutionIsActive(observation,kind)) continue;
        for(Character& resident:world_.characters){
            if(!resident.alive
               || !residentQualifiesForInstitution(
                    resident,socialKnowledge_,kind)) continue;
            if(residentInstitutionMember(
                socialKnowledge_,resident.id,kind)) continue;

            if(registerSocietyInstitutionMembership(
                socialKnowledge_,resident,kind,
                world_.minute,world_.seed)!=nullptr){
                std::ostringstream log;
                log<<resident.name<<" joined "
                   <<societyInstitutionKindName(kind);
                emit(log.str());
            }
        }
    }
}

} // namespace lifelens
