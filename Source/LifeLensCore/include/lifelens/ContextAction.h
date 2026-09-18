#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>

#include "CivilizationDecision.h"
#include "CivilizationSpatial.h"
#include "Parenting.h"
#include "SocialUtility.h"

namespace lifelens {

enum class ContextActionKind {
    None,
    Social,
    Civilization,
    Parenting,
    KnowledgeTeaching
};

struct PendingContextAction {
    std::uint64_t token=0;
    ContextActionKind kind=ContextActionKind::None;
    int issuedMinute=-1;

    SocialUtilityDecision social{};
    CivilizationUtilityDecision civilization{};
    CharacterId parentingTarget=0;
    ParentingAction parentingAction=ParentingAction::Comfort;
    ParentingContext parentingContext{};

    CharacterId knowledgeTeachingTarget=0;
    TechniqueId knowledgeTeachingTechnique=TechniqueId::None;
    double knowledgeTeachingScore=0.0;

    bool hasSpatialTarget=false;
    GridPos targetPos{};
    SanitationSiteId sanitationSiteId=0;

    bool active() const { return token!=0 && kind!=ContextActionKind::None; }
    void clear() { *this=PendingContextAction{}; }
};

struct PendingContextActionObservation {
    CharacterId actor=0;
    bool active=false;
    std::uint64_t token=0;
    ContextActionKind kind=ContextActionKind::None;
    int issuedMinute=-1;
    int durationTicks=0;

    SocialIntent socialIntent=SocialIntent::None;
    CharacterId targetResident=0;

    CivilizationIntent civilizationIntent=CivilizationIntent::None;
    MaterialKind material=MaterialKind::Unknown;
    ItemKind item=ItemKind::RawMaterial;
    TechniqueId technique=TechniqueId::None;
    int quantity=0;
    ResourceNodeId resourceNode=0;
    StorageId storage=0;
    FacilityBuildAction facilityAction=FacilityBuildAction::None;
    FacilityId facility=0;
    FacilityKind facilityKind=FacilityKind::PrimitiveStorage;

    ParentingAction parentingAction=ParentingAction::Comfort;

    bool hasSpatialTarget=false;
    GridPos targetPos{};
    SanitationSiteId sanitationSiteId=0;
};

inline std::uint64_t nextContextActionToken()
{
    static std::atomic<std::uint64_t> next{1};
    std::uint64_t token=next.fetch_add(1,std::memory_order_relaxed);
    if(token==0) token=next.fetch_add(1,std::memory_order_relaxed);
    return token;
}

inline int contextActionTimeoutMinutes(ContextActionKind kind)
{
    switch(kind){
        case ContextActionKind::Social: return 45;
        case ContextActionKind::Parenting: return 45;
        case ContextActionKind::KnowledgeTeaching: return 45;
        case ContextActionKind::Civilization: return 120;
        case ContextActionKind::None:
        default: return 0;
    }
}

inline int contextActionDurationTicks(const PendingContextAction& action)
{
    switch(action.kind){
        case ContextActionKind::Social:
            return action.social.intent==SocialIntent::Avoid ? 1 : 3;
        case ContextActionKind::KnowledgeTeaching:
            return 6;
        case ContextActionKind::Parenting:
            switch(action.parentingAction){
                case ParentingAction::Feed: return 3;
                case ParentingAction::PutToSleep: return 4;
                case ParentingAction::Bathe: return 5;
                case ParentingAction::ToiletAssist: return 4;
                case ParentingAction::Hold: return 3;
                case ParentingAction::Play: return 5;
                case ParentingAction::Educate: return 6;
                case ParentingAction::Discipline: return 3;
                case ParentingAction::Comfort: return 4;
                case ParentingAction::HealthCare: return 6;
            }
            return 3;
        case ContextActionKind::Civilization:
            if(action.civilization.intent==CivilizationIntent::Craft
               && action.civilization.facilityAction!=FacilityBuildAction::None){
                switch(action.civilization.facilityAction){
                    case FacilityBuildAction::Plan: return 4;
                    case FacilityBuildAction::DeliverMaterial: return 4;
                    case FacilityBuildAction::Work: return 8;
                    case FacilityBuildAction::Repair: return 7;
                    case FacilityBuildAction::Fuel: return 4;
                    case FacilityBuildAction::Ignite: return 6;
                    case FacilityBuildAction::CollectCharcoal: return 4;
                    case FacilityBuildAction::LoadSmeltCharge: return 5;
                    case FacilityBuildAction::CollectMetal: return 4;
                    case FacilityBuildAction::None:
                    default: return 1;
                }
            }
            switch(action.civilization.intent){
                case CivilizationIntent::Gather: return 5;
                case CivilizationIntent::Store: return 3;
                case CivilizationIntent::Experiment: return 7;
                case CivilizationIntent::Craft: return 8;
                case CivilizationIntent::None:
                default: return 1;
            }
        case ContextActionKind::None:
        default:
            return 1;
    }
}

inline bool contextActionExpired(const PendingContextAction& action,int currentMinute)
{
    if(!action.active()) return false;
    const int timeout=contextActionTimeoutMinutes(action.kind);
    return timeout>0 && currentMinute-action.issuedMinute>=timeout;
}

inline bool contextActionNearTarget(GridPos resolved,GridPos target,int maxGridDelta=1)
{
    return std::abs(resolved.x-target.x)<=std::max(0,maxGridDelta)
        && std::abs(resolved.y-target.y)<=std::max(0,maxGridDelta);
}

inline bool resolveCivilizationContextTarget(
    const World& world,
    const Character& actor,
    const CivilizationUtilityDecision& decision,
    GridPos& outTarget,
    SanitationSiteId& outSanitationSiteId)
{
    outTarget={};
    outSanitationSiteId=0;

    switch(decision.intent){
        case CivilizationIntent::Gather:
            return resolveCivilizationResourceGridPosition(world,decision.resourceNode,outTarget);
        case CivilizationIntent::Store:
            return resolveCivilizationStorageGridPosition(world,decision.storage,outTarget);
        case CivilizationIntent::Experiment:
            if(decision.experiment==ExperimentKind::DesignateSanitationArea){
                const PrimitiveSanitationOpportunity opportunity=evaluatePrimitiveSanitationOpportunity(
                    world.seed,actor,world.environmentalResidues,world.minute,
                    civilizationSanitationReferencePosition(world));
                if(!opportunity.siteAvailable) return false;
                outTarget=opportunity.suggestedSite;
                return true;
            }
            if(decision.experiment==ExperimentKind::DigSanitationPit){
                const DugSanitationPitOpportunity opportunity=evaluateDugSanitationPitOpportunity(
                    actor,world.environmentalResidues,world.primitiveSanitationSites);
                if(!opportunity.candidateAvailable) return false;
                outTarget=opportunity.pos;
                outSanitationSiteId=opportunity.siteId;
                return true;
            }
            if(decision.experiment==ExperimentKind::SmeltCopperOre){
                const ConstructedFacility* furnace=primitiveFurnaceProject(world);
                if(furnace==nullptr || furnace->state!=FacilityState::Operational || !furnace->active) return false;
                outTarget=furnace->pos;
                return true;
            }
            return false;
        case CivilizationIntent::Craft:
            if(decision.facilityKind==FacilityKind::WorkSurface
               && decision.facilityAction==FacilityBuildAction::None
               && decision.facility!=0
               && decision.hasFacilityTarget){
                for(const auto& facility:world.facilities){
                    if(facility.id!=decision.facility
                       || facility.kind!=FacilityKind::WorkSurface
                       || !facilityOperationalAndActive(facility)) continue;
                    if(facility.pos.x!=decision.facilityTargetPos.x
                       || facility.pos.y!=decision.facilityTargetPos.y) return false;
                    outTarget=facility.pos;
                    return true;
                }
                return false;
            }
            if(isSettlementFoundationFacility(decision.facilityKind)
               && decision.facilityAction!=FacilityBuildAction::None){
                if(!decision.hasFacilityTarget) return false;
                if(decision.facilityAction==FacilityBuildAction::Plan){
                    const SettlementFacilitySiteOpportunity opportunity=
                        chooseSettlementFacilitySite(
                            world,actor.id,decision.facilityKind);
                    if(!opportunity.available
                       || opportunity.pos.x!=decision.facilityTargetPos.x
                       || opportunity.pos.y!=decision.facilityTargetPos.y) return false;
                    outTarget=decision.facilityTargetPos;
                    return true;
                }
                for(const auto& facility:world.facilities){
                    if(facility.id!=decision.facility
                       || facility.kind!=decision.facilityKind
                       || !isSettlementFoundationFacility(facility.kind)) continue;
                    if(facility.pos.x!=decision.facilityTargetPos.x
                       || facility.pos.y!=decision.facilityTargetPos.y) return false;

                    if(decision.facilityAction==FacilityBuildAction::Repair){
                        if(!settlementFacilityNeedsMaintenance(facility)) return false;
                        outTarget=facility.pos;
                        return true;
                    }

                    if(facility.state==FacilityState::Operational
                       || facility.state==FacilityState::Ruined) return false;
                    if(decision.facilityAction!=FacilityBuildAction::DeliverMaterial
                       && decision.facilityAction!=FacilityBuildAction::Work) return false;
                    outTarget=facility.pos;
                    return true;
                }
                return false;
            }
            if(decision.technique==TechniqueId::PrimitiveStorage){
                if(!decision.hasFacilityTarget) return false;
                if(decision.facilityAction==FacilityBuildAction::Plan){
                    const PrimitiveStorageSiteOpportunity opportunity=choosePrimitiveStorageSite(world,actor.id);
                    if(!opportunity.available
                       || opportunity.pos.x!=decision.facilityTargetPos.x
                       || opportunity.pos.y!=decision.facilityTargetPos.y) return false;
                    outTarget=decision.facilityTargetPos;
                    return true;
                }
                for(const auto& facility:world.facilities){
                    if(facility.id!=decision.facility
                       || facility.kind!=FacilityKind::PrimitiveStorage
                       || facility.state==FacilityState::Operational
                       || facility.state==FacilityState::Ruined) continue;
                    if(facility.pos.x!=decision.facilityTargetPos.x
                       || facility.pos.y!=decision.facilityTargetPos.y) return false;
                    outTarget=facility.pos;
                    return true;
                }
                return false;
            }
            if(decision.technique==TechniqueId::FireMaking
               && decision.facilityKind==FacilityKind::FirePit){
                if(!decision.hasFacilityTarget) return false;
                if(decision.facilityAction==FacilityBuildAction::Plan){
                    const PrimitiveFirePitSiteOpportunity opportunity=choosePrimitiveFirePitSite(world,actor.id);
                    if(!opportunity.available
                       || opportunity.pos.x!=decision.facilityTargetPos.x
                       || opportunity.pos.y!=decision.facilityTargetPos.y) return false;
                    outTarget=decision.facilityTargetPos;
                    return true;
                }
                for(const auto& facility:world.facilities){
                    if(facility.id!=decision.facility
                       || facility.kind!=FacilityKind::FirePit
                       || facility.state==FacilityState::Ruined) continue;
                    if(facility.pos.x!=decision.facilityTargetPos.x
                       || facility.pos.y!=decision.facilityTargetPos.y) return false;
                    const bool operational=facility.state==FacilityState::Operational && facility.active;
                    if((decision.facilityAction==FacilityBuildAction::DeliverMaterial
                        || decision.facilityAction==FacilityBuildAction::Work) && operational) return false;
                    if((decision.facilityAction==FacilityBuildAction::Fuel
                        || decision.facilityAction==FacilityBuildAction::Ignite
                        || decision.facilityAction==FacilityBuildAction::CollectCharcoal) && !operational) return false;
                    outTarget=facility.pos;
                    return true;
                }
                return false;
            }
            if(decision.facilityKind==FacilityKind::Furnace
               && (decision.technique==TechniqueId::FireMaking
                   || decision.technique==TechniqueId::CopperSmelting)){
                if(!decision.hasFacilityTarget) return false;
                if(decision.facilityAction==FacilityBuildAction::Plan){
                    const PrimitiveFurnaceSiteOpportunity opportunity=choosePrimitiveFurnaceSite(world,actor.id);
                    if(!opportunity.available
                       || opportunity.pos.x!=decision.facilityTargetPos.x
                       || opportunity.pos.y!=decision.facilityTargetPos.y) return false;
                    outTarget=decision.facilityTargetPos;
                    return true;
                }
                for(const auto& facility:world.facilities){
                    if(facility.id!=decision.facility
                       || facility.kind!=FacilityKind::Furnace
                       || facility.state==FacilityState::Ruined) continue;
                    if(facility.pos.x!=decision.facilityTargetPos.x
                       || facility.pos.y!=decision.facilityTargetPos.y) return false;
                    const bool operational=facility.state==FacilityState::Operational && facility.active;
                    if((decision.facilityAction==FacilityBuildAction::DeliverMaterial
                        || decision.facilityAction==FacilityBuildAction::Work) && operational) return false;
                    if((decision.facilityAction==FacilityBuildAction::LoadSmeltCharge
                        || decision.facilityAction==FacilityBuildAction::Ignite
                        || decision.facilityAction==FacilityBuildAction::CollectMetal) && !operational) return false;
                    outTarget=facility.pos;
                    return true;
                }
                return false;
            }
            if(decision.technique==TechniqueId::DesignatedSanitationArea){
                const PrimitiveSanitationOpportunity opportunity=evaluateDesignatedSanitationSiteCreationOpportunity(
                    world.seed,actor,world.environmentalResidues,world.minute,
                    civilizationSanitationReferencePosition(world));
                if(!opportunity.siteAvailable) return false;
                outTarget=opportunity.suggestedSite;
                return true;
            }
            if(decision.technique==TechniqueId::DugSanitationPit){
                const PrimitiveSanitationSite* site=activePrimitiveSanitationSite(world.primitiveSanitationSites);
                if(site==nullptr) return false;
                outTarget=site->pos;
                outSanitationSiteId=site->id;
                return true;
            }
            return false;
        case CivilizationIntent::None:
        default:
            return false;
    }
}

inline bool civilizationContextRequiresSpatialTarget(const CivilizationUtilityDecision& decision)
{
    if(decision.intent==CivilizationIntent::Gather || decision.intent==CivilizationIntent::Store) return true;
    if(decision.intent==CivilizationIntent::Experiment){
        return decision.experiment==ExperimentKind::DesignateSanitationArea
            || decision.experiment==ExperimentKind::DigSanitationPit
            || decision.experiment==ExperimentKind::SmeltCopperOre;
    }
    if(decision.intent==CivilizationIntent::Craft){
        return (decision.facilityKind==FacilityKind::WorkSurface
                && decision.facilityAction==FacilityBuildAction::None
                && decision.facility!=0
                && decision.hasFacilityTarget)
            || (isSettlementFoundationFacility(decision.facilityKind)
                && decision.facilityAction!=FacilityBuildAction::None)
            || decision.technique==TechniqueId::DesignatedSanitationArea
            || decision.technique==TechniqueId::DugSanitationPit
            || decision.technique==TechniqueId::PrimitiveStorage
            || (decision.technique==TechniqueId::FireMaking
                && decision.facilityKind==FacilityKind::FirePit
                && decision.facilityAction!=FacilityBuildAction::None)
            || (decision.facilityKind==FacilityKind::Furnace
                && (decision.technique==TechniqueId::FireMaking
                    || decision.technique==TechniqueId::CopperSmelting)
                && decision.facilityAction!=FacilityBuildAction::None);
    }
    return false;
}

inline PendingContextActionObservation observePendingContextAction(
    CharacterId actor,
    const PendingContextAction& pending)
{
    PendingContextActionObservation result;
    result.actor=actor;
    if(!pending.active()) return result;

    result.active=true;
    result.token=pending.token;
    result.kind=pending.kind;
    result.issuedMinute=pending.issuedMinute;
    result.durationTicks=contextActionDurationTicks(pending);
    result.hasSpatialTarget=pending.hasSpatialTarget;
    result.targetPos=pending.targetPos;
    result.sanitationSiteId=pending.sanitationSiteId;

    switch(pending.kind){
        case ContextActionKind::Social:
            result.socialIntent=pending.social.intent;
            result.targetResident=pending.social.target;
            break;
        case ContextActionKind::Civilization:
            result.civilizationIntent=pending.civilization.intent;
            result.material=pending.civilization.material;
            result.item=pending.civilization.item;
            result.technique=pending.civilization.technique;
            result.quantity=pending.civilization.quantity;
            result.resourceNode=pending.civilization.resourceNode;
            result.storage=pending.civilization.storage;
            result.facilityAction=pending.civilization.facilityAction;
            result.facility=pending.civilization.facility;
            result.facilityKind=pending.civilization.facilityKind;
            break;
        case ContextActionKind::Parenting:
            result.parentingAction=pending.parentingAction;
            result.targetResident=pending.parentingTarget;
            break;
        case ContextActionKind::KnowledgeTeaching:
            result.targetResident=pending.knowledgeTeachingTarget;
            result.technique=pending.knowledgeTeachingTechnique;
            break;
        case ContextActionKind::None:
        default:
            break;
    }
    return result;
}

} // namespace lifelens
