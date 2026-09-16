#pragma once

#include <algorithm>
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
    Parenting
};

struct PendingContextAction {
    std::uint64_t token=0;
    ContextActionKind kind=ContextActionKind::None;
    int issuedMinute=-1;

    SocialUtilityDecision social{};
    CivilizationUtilityDecision civilization{};
    CharacterId parentingTarget=0;
    ParentingAction parentingAction=ParentingAction::Comfort;

    bool hasSpatialTarget=false;
    GridPos targetPos{};
    SanitationSiteId sanitationSiteId=0;

    bool active() const
    {
        return token!=0 && kind!=ContextActionKind::None;
    }

    void clear()
    {
        *this=PendingContextAction{};
    }
};

struct PendingContextActionObservation {
    CharacterId actor=0;
    bool active=false;
    std::uint64_t token=0;
    ContextActionKind kind=ContextActionKind::None;
    int issuedMinute=-1;

    SocialIntent socialIntent=SocialIntent::None;
    CharacterId targetResident=0;

    CivilizationIntent civilizationIntent=CivilizationIntent::None;
    MaterialKind material=MaterialKind::Unknown;
    ItemKind item=ItemKind::RawMaterial;
    TechniqueId technique=TechniqueId::None;
    int quantity=0;
    ResourceNodeId resourceNode=0;
    StorageId storage=0;

    ParentingAction parentingAction=ParentingAction::Comfort;

    bool hasSpatialTarget=false;
    GridPos targetPos{};
    SanitationSiteId sanitationSiteId=0;
};

inline int contextActionTimeoutMinutes(ContextActionKind kind)
{
    switch(kind){
        case ContextActionKind::Social: return 45;
        case ContextActionKind::Parenting: return 45;
        case ContextActionKind::Civilization: return 120;
        case ContextActionKind::None:
        default: return 0;
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
            return false;
        case CivilizationIntent::Craft:
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
            || decision.experiment==ExperimentKind::DigSanitationPit;
    }
    if(decision.intent==CivilizationIntent::Craft){
        return decision.technique==TechniqueId::DesignatedSanitationArea
            || decision.technique==TechniqueId::DugSanitationPit;
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
            break;
        case ContextActionKind::Parenting:
            result.parentingAction=pending.parentingAction;
            result.targetResident=pending.parentingTarget;
            break;
        case ContextActionKind::None:
        default:
            break;
    }
    return result;
}

} // namespace lifelens
