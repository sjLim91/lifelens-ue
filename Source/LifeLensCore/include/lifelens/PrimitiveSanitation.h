#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "EnvironmentalExposure.h"
#include "SanitationProblemRecognition.h"

namespace lifelens {

using SanitationSiteId=std::uint64_t;

enum class PrimitiveSanitationSiteKind {
    DesignatedArea
};

struct PrimitiveSanitationSite {
    SanitationSiteId id=0;
    PrimitiveSanitationSiteKind kind=PrimitiveSanitationSiteKind::DesignatedArea;
    GridPos pos{};
    CharacterId establishedBy=0;
    int establishedMinute=0;
    bool active=true;
    int useCount=0;
};

inline bool validPrimitiveSanitationSite(const PrimitiveSanitationSite& site)
{
    return site.id!=0
        && site.kind==PrimitiveSanitationSiteKind::DesignatedArea
        && site.establishedBy!=0
        && site.establishedMinute>=0
        && site.useCount>=0;
}

inline const PrimitiveSanitationSite* findPrimitiveSanitationSite(
    const std::vector<PrimitiveSanitationSite>& sites,
    SanitationSiteId id)
{
    for(const auto& site:sites) if(site.id==id) return &site;
    return nullptr;
}

inline PrimitiveSanitationSite* findPrimitiveSanitationSite(
    std::vector<PrimitiveSanitationSite>& sites,
    SanitationSiteId id)
{
    for(auto& site:sites) if(site.id==id) return &site;
    return nullptr;
}

inline const PrimitiveSanitationSite* activeDesignatedSanitationSite(
    const std::vector<PrimitiveSanitationSite>& sites)
{
    const PrimitiveSanitationSite* best=nullptr;
    for(const auto& site:sites){
        if(!site.active || site.kind!=PrimitiveSanitationSiteKind::DesignatedArea) continue;
        if(best==nullptr || site.id<best->id) best=&site;
    }
    return best;
}

inline SanitationSiteId nextPrimitiveSanitationSiteId(
    const std::vector<PrimitiveSanitationSite>& sites)
{
    SanitationSiteId next=1;
    for(const auto& site:sites) next=std::max(next,site.id+1);
    return next;
}

struct PrimitiveSanitationOpportunity {
    bool problemRecognized=false;
    bool siteAvailable=false;
    GridPos suggestedSite{};
    double siteExposure=1.0;
    double problemConfidence=0.0;
};

inline constexpr double PrimitiveSanitationCleanSiteExposureLimit=0.08;

inline double recognizedSanitationProblemConfidence(const Character& character)
{
    const BeliefRecord* belief=character.beliefs.find(
        0,sanitationProblemBeliefProposition());
    return isEstablishedSanitationProblemBelief(belief)
        ? std::max(0.0,std::min(1.0,belief->confidence))
        : 0.0;
}

inline PrimitiveSanitationOpportunity evaluatePrimitiveSanitationOpportunity(
    std::uint64_t worldSeed,
    const Character& character,
    const EnvironmentalResidueField& field,
    int currentMinute)
{
    PrimitiveSanitationOpportunity result;
    result.problemRecognized=hasRecognizedSanitationProblem(character);
    if(!result.problemRecognized) return result;

    result.problemConfidence=recognizedSanitationProblemConfidence(character);
    result.suggestedSite=chooseLowExposureOutdoorReliefPosition(
        worldSeed,character,field,currentMinute);
    result.siteExposure=field.exposureAt(result.suggestedSite);
    result.siteAvailable=result.siteExposure<PrimitiveSanitationCleanSiteExposureLimit;
    return result;
}

inline PrimitiveSanitationOpportunity evaluateDesignatedSanitationSiteCreationOpportunity(
    std::uint64_t worldSeed,
    const Character& character,
    const EnvironmentalResidueField& field,
    int currentMinute)
{
    PrimitiveSanitationOpportunity result;
    result.problemRecognized=hasRecognizedSanitationProblem(character);
    result.problemConfidence=recognizedSanitationProblemConfidence(character);
    result.suggestedSite=chooseLowExposureOutdoorReliefPosition(
        worldSeed,character,field,currentMinute);
    result.siteExposure=field.exposureAt(result.suggestedSite);
    result.siteAvailable=result.siteExposure<PrimitiveSanitationCleanSiteExposureLimit;
    return result;
}

inline bool canEstablishDesignatedSanitationArea(
    std::uint64_t worldSeed,
    const Character& character,
    const EnvironmentalResidueField& field,
    const std::vector<PrimitiveSanitationSite>& sites,
    int currentMinute)
{
    if(!character.civilization.knowledge.knowsAtLeast(
        TechniqueId::DesignatedSanitationArea,KnowledgeLevel::Reproducible)) return false;
    if(activeDesignatedSanitationSite(sites)!=nullptr) return false;
    return evaluateDesignatedSanitationSiteCreationOpportunity(
        worldSeed,character,field,currentMinute).siteAvailable;
}

struct PrimitiveSanitationSiteCreationResult {
    bool established=false;
    SanitationSiteId siteId=0;
    GridPos pos{};
};

inline PrimitiveSanitationSiteCreationResult establishDesignatedSanitationArea(
    std::uint64_t worldSeed,
    Character& character,
    const EnvironmentalResidueField& field,
    std::vector<PrimitiveSanitationSite>& sites,
    int currentMinute)
{
    PrimitiveSanitationSiteCreationResult result;
    if(!canEstablishDesignatedSanitationArea(
        worldSeed,character,field,sites,currentMinute)) return result;

    const PrimitiveSanitationOpportunity opportunity=
        evaluateDesignatedSanitationSiteCreationOpportunity(
            worldSeed,character,field,currentMinute);
    if(!opportunity.siteAvailable) return result;

    PrimitiveSanitationSite site;
    site.id=nextPrimitiveSanitationSiteId(sites);
    site.kind=PrimitiveSanitationSiteKind::DesignatedArea;
    site.pos=opportunity.suggestedSite;
    site.establishedBy=character.id;
    site.establishedMinute=currentMinute;
    site.active=true;
    site.useCount=0;
    sites.push_back(site);

    result.established=true;
    result.siteId=site.id;
    result.pos=site.pos;
    return result;
}

enum class SanitationUseTargetKind {
    EmergencyOutdoor,
    DesignatedArea
};

struct SanitationUseTarget {
    SanitationUseTargetKind kind=SanitationUseTargetKind::EmergencyOutdoor;
    GridPos pos{};
    SanitationSiteId siteId=0;
};

inline SanitationUseTarget resolveSanitationUseTarget(
    std::uint64_t worldSeed,
    const Character& character,
    const EnvironmentalResidueField& field,
    const std::vector<PrimitiveSanitationSite>& sites,
    int currentMinute)
{
    if(const PrimitiveSanitationSite* site=activeDesignatedSanitationSite(sites)){
        return {SanitationUseTargetKind::DesignatedArea,site->pos,site->id};
    }
    return {
        SanitationUseTargetKind::EmergencyOutdoor,
        chooseLowExposureOutdoorReliefPosition(worldSeed,character,field,currentMinute),
        0};
}

inline bool recordDesignatedSanitationSiteUse(
    std::vector<PrimitiveSanitationSite>& sites,
    SanitationSiteId siteId,
    GridPos resolvedPosition)
{
    PrimitiveSanitationSite* site=findPrimitiveSanitationSite(sites,siteId);
    if(site==nullptr || !site->active
       || site->kind!=PrimitiveSanitationSiteKind::DesignatedArea
       || site->pos.x!=resolvedPosition.x || site->pos.y!=resolvedPosition.y) return false;
    ++site->useCount;
    return true;
}

inline int designatedSanitationUseDurationTicks()
{
    return 2;
}

inline NeedsDelta designatedSanitationUseEffectPerTick()
{
    return {0,0,0,-0.13,0.012};
}

} // namespace lifelens
