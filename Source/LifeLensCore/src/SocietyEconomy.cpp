#include "lifelens/Simulation.h"
#include "lifelens/SocietyEconomy.h"

#include <algorithm>
#include <cmath>
#include <set>
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

    const auto commitExchange=
        [&](Character& first,
            Character& second,
            SocietyExchangePlan exchange,
            const char* prefix)->bool
        {
            if(!exchange.valid()) return false;
            if(hasSocietyTradePartnership(
                    socialKnowledge_,first.id,second.id)){
                exchange.score=societyClamp01(exchange.score+0.08);
            }
            if(residentInstitutionMember(
                   socialKnowledge_,first.id,
                   SocietyInstitutionKind::ExchangeNetwork)
               && residentInstitutionMember(
                   socialKnowledge_,second.id,
                   SocietyInstitutionKind::ExchangeNetwork)){
                exchange.score=societyClamp01(exchange.score+0.05);
            }

            constexpr double ExchangeThreshold=0.66;
            if(exchange.score<ExchangeThreshold
               || !executeMutualExchange(first,second,exchange)){
                return false;
            }

            registerSocietyExchangeFact(
                socialKnowledge_,first,second,exchange,
                world_.minute,world_.seed);
            const bool hadPartnership=
                hasSocietyTradePartnership(
                    socialKnowledge_,first.id,second.id);
            const SocialFact* partnership=
                registerSocietyTradePartnershipIfQualified(
                    socialKnowledge_,first,second,
                    world_.minute,world_.seed);

            relationships_.getOrCreate(first.id,second.id).apply(
                relationshipDeltaFor(
                    RelationshipEvent::SharedPositiveExperience,0.35));
            relationships_.getOrCreate(second.id,first.id).apply(
                relationshipDeltaFor(
                    RelationshipEvent::SharedPositiveExperience,0.35));

            std::ostringstream log;
            log<<prefix<<first.name<<" exchanged "
               <<materialName(exchange.firstGives)
               <<" with "<<second.name<<" for "
               <<materialName(exchange.secondGives);
            if(partnership!=nullptr && !hadPartnership){
                log<<" -> trade partnership";
            }
            emit(log.str());
            return true;
        };

    // Keep C5's immediate exchange path for residents who are already
    // physically together. C6-C adds travel only when no useful local exchange
    // was completed; it never turns inventories into remote RPC endpoints.
    SocietyExchangePlan bestLocal;
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

            const int distance=manhattan(
                firstRuntime->second.pos,
                secondRuntime->second.pos);
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
            if(candidate.score>bestLocal.score+1e-12){
                bestLocal=candidate;
            }
        }
    }

    if(bestLocal.valid()){
        Character* first=nullptr;
        Character* second=nullptr;
        for(auto& resident:world_.characters){
            if(resident.id==bestLocal.first) first=&resident;
            if(resident.id==bestLocal.second) second=&resident;
        }
        if(first!=nullptr && second!=nullptr
           && commitExchange(*first,*second,bestLocal,"")){
            return;
        }
    }

    // Only one long-range trade journey is scheduled at a time for now. This
    // prevents a tiny founding population from all abandoning routine survival
    // simultaneously while still allowing a durable route to emerge through
    // repeated real trips.
    for(const auto& entry:runtime_){
        if(entry.second.pendingContext.active()
           && entry.second.pendingContext.kind==ContextActionKind::Trade){
            return;
        }
    }

    const SettlementPopulation population=settlementPopulation();
    const SettlementNetworkObservation network=
        lifelens::observeSettlementNetwork(world_,&population);
    if(network.activeSettlementCount<2) return;

    std::set<CharacterId> eligible;
    for(const Character& resident:world_.characters){
        if(!resident.alive || requiresDirectCare(resident.lifeStage)) continue;
        const auto runtimeIt=runtime_.find(resident.id);
        if(runtimeIt==runtime_.end()) continue;
        const Runtime& runtime=runtimeIt->second;
        if(runtime.pendingContext.active()
           || runtime.socialActive
           || !runtime.plan.empty()){
            continue;
        }
        const double urgentNeed=std::max({
            resident.needs.hunger,
            resident.needs.thirst,
            resident.needs.sleep,
            resident.needs.bladder,
            resident.needs.hygiene
        });
        if(urgentNeed>=0.80) continue;
        eligible.insert(resident.id);
    }
    if(eligible.size()<2) return;

    const InterSettlementTradeMission mission=
        bestInterSettlementTradeMission(
            world_,network,population,
            relationships_,socialKnowledge_,&eligible);
    if(!mission.available
       || mission.score<InterSettlementTradeMissionThreshold){
        return;
    }

    auto travelerRuntime=runtime_.find(mission.traveler);
    auto partnerRuntime=runtime_.find(mission.partner);
    if(travelerRuntime==runtime_.end()
       || partnerRuntime==runtime_.end()){
        return;
    }
    Character* traveler=nullptr;
    Character* partner=nullptr;
    for(auto& resident:world_.characters){
        if(resident.id==mission.traveler) traveler=&resident;
        if(resident.id==mission.partner) partner=&resident;
    }
    if(traveler==nullptr || partner==nullptr) return;

    Runtime& runtime=travelerRuntime->second;
    clearNavigation(runtime);
    runtime.plan.clear();
    runtime.actionIndex=0;
    runtime.announced=false;
    runtime.socialActive=false;
    runtime.socialIntent=SocialIntent::None;
    runtime.socialTarget=0;
    runtime.civilizationActive=false;

    PendingContextAction trade;
    trade.token=issueContextActionToken();
    trade.kind=ContextActionKind::Trade;
    trade.issuedMinute=world_.minute;
    trade.social.target=mission.partner;
    trade.social.utility=mission.score;
    trade.hasSpatialTarget=true;
    trade.targetPos=partnerRuntime->second.pos;
    runtime.pendingContext=trade;

    std::ostringstream log;
    log<<traveler->name<<" departed settlement "
       <<mission.originSettlement<<" to trade with "
       <<partner->name<<" at settlement "
       <<mission.destinationSettlement
       <<" (distance "<<mission.distanceGrid
       <<", trade "<<std::fixed<<std::setprecision(2)
       <<mission.score<<")";
    emit(log.str());
}

void Simulation::advanceSocietyRecordkeeping()
{
    if(world_.minute<=0 || world_.minute%360!=0) return;
    const SocietyWorldObservation society=
        buildSocietyWorldObservation(
            world_,socialKnowledge_,&households_);
    if(static_cast<int>(society.recordStage)
       <static_cast<int>(CollectiveRecordStage::ProtoRecordkeeping)){
        return;
    }

    const SocialFact* target=
        bestSocietyDurableRecordCandidate(socialKnowledge_);
    if(target==nullptr) return;

    Character* recorder=nullptr;
    double bestScore=-1.0;
    MaterialKind medium=MaterialKind::Unknown;
    for(Character& resident:world_.characters){
        if(!resident.alive || !lifeStageProfile(resident.lifeStage).canWork){
            continue;
        }
        const bool organizationMember=
            residentInstitutionMember(
                socialKnowledge_,resident.id,
                SocietyInstitutionKind::LearningCircle)
            || residentInstitutionMember(
                socialKnowledge_,resident.id,
                SocietyInstitutionKind::InquiryCircle)
            || residentInstitutionMember(
                socialKnowledge_,resident.id,
                SocietyInstitutionKind::StorageCommons);
        if(!organizationMember) continue;

        MaterialKind candidateMedium=MaterialKind::Unknown;
        if(resident.civilization.inventory.count(
                ItemKind::RawMaterial,MaterialKind::Clay)
           >residentExchangeReserve(resident,MaterialKind::Clay)){
            candidateMedium=MaterialKind::Clay;
        }else if(resident.civilization.inventory.count(
                ItemKind::RawMaterial,MaterialKind::Wood)
           >residentExchangeReserve(resident,MaterialKind::Wood)){
            candidateMedium=MaterialKind::Wood;
        }
        if(candidateMedium==MaterialKind::Unknown) continue;

        const ResidentSocietyStatus status=
            observeResidentSocietyStatus(resident);
        const double roleBonus=
            status.role==SocietyRole::Educator ? 0.24
            : status.role==SocietyRole::Storekeeper ? 0.18
            : status.role==SocietyRole::Craftsperson ? 0.14
            : 0.0;
        const double score=
            roleBonus
            +0.34*resident.civilization.learningSkill
            +0.22*resident.personality.conscientiousness
            +0.20*resident.personality.orderliness;
        if(score>bestScore+1e-12
           || (std::abs(score-bestScore)<=1e-12
               && (recorder==nullptr || resident.id<recorder->id))){
            recorder=&resident;
            bestScore=score;
            medium=candidateMedium;
        }
    }
    if(recorder==nullptr || medium==MaterialKind::Unknown) return;

    if(!recorder->civilization.inventory.remove(
            ItemKind::RawMaterial,medium,1)){
        return;
    }
    recorder->civilization.inventory.add(
        {ItemKind::RecordTablet,medium,1,0.72,0.92});

    const SocialFact* record=registerSocietyDurableRecordFact(
        socialKnowledge_,*recorder,*target,medium,
        world_.minute,world_.seed);
    if(record==nullptr){
        recorder->civilization.inventory.remove(
            ItemKind::RecordTablet,medium,1);
        recorder->civilization.inventory.add(
            {ItemKind::RawMaterial,medium,1,0.5,1.0});
        return;
    }

    std::ostringstream log;
    log<<recorder->name<<" preserved social knowledge on "
       <<(medium==MaterialKind::Clay ? "clay" : "wood")
       <<" record media";
    emit(log.str());
}

void Simulation::advanceSocietyInstitutions()
{
    if(world_.minute<=0 || world_.minute%360!=0) return;

    const SocietyWorldObservation observation=
        buildSocietyWorldObservation(
            world_,socialKnowledge_,&households_);
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
