#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "EnvironmentalExposure.h"
#include "SanitationProblemRecognition.h"

namespace lifelens {

using SanitationSiteId=std::uint64_t;

enum class PrimitiveSanitationSiteKind {
    DesignatedArea,
    DugPit
};

inline constexpr double DugSanitationPitWorkRequired=4.5;
inline constexpr double DugSanitationPitContainmentIntensityFactor=0.45;
inline constexpr int DugSanitationPitContainmentRadiusTiles=1;

struct PrimitiveSanitationSite {
    SanitationSiteId id=0;
    PrimitiveSanitationSiteKind kind=PrimitiveSanitationSiteKind::DesignatedArea;
    GridPos pos{};
    CharacterId establishedBy=0;
    int establishedMinute=0;
    bool active=true;
    int useCount=0;
    double improvementWork=0.0;
    CharacterId improvedBy=0;
    int improvedMinute=-1;
};

inline bool validPrimitiveSanitationSiteKind(PrimitiveSanitationSiteKind kind)
{
    return kind==PrimitiveSanitationSiteKind::DesignatedArea
        || kind==PrimitiveSanitationSiteKind::DugPit;
}

inline bool validPrimitiveSanitationSite(const PrimitiveSanitationSite& site)
{
    if(site.id==0 || !validPrimitiveSanitationSiteKind(site.kind)
       || site.establishedBy==0 || site.establishedMinute<0 || site.useCount<0
       || site.improvementWork<0.0 || site.improvementWork>DugSanitationPitWorkRequired+1e-9){
        return false;
    }
    if(site.kind==PrimitiveSanitationSiteKind::DesignatedArea){
        return site.improvedBy==0 && site.improvedMinute==-1
            && site.improvementWork<DugSanitationPitWorkRequired;
    }
    return site.improvedBy!=0
        && site.improvedMinute>=site.establishedMinute
        && site.improvementWork>=DugSanitationPitWorkRequired-1e-9;
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

inline const PrimitiveSanitationSite* activePrimitiveSanitationSite(
    const std::vector<PrimitiveSanitationSite>& sites)
{
    const PrimitiveSanitationSite* best=nullptr;
    for(const auto& site:sites){
        if(!site.active || !validPrimitiveSanitationSiteKind(site.kind)) continue;
        if(best==nullptr || site.id<best->id) best=&site;
    }
    return best;
}

inline PrimitiveSanitationSite* activePrimitiveSanitationSite(
    std::vector<PrimitiveSanitationSite>& sites)
{
    PrimitiveSanitationSite* best=nullptr;
    for(auto& site:sites){
        if(!site.active || !validPrimitiveSanitationSiteKind(site.kind)) continue;
        if(best==nullptr || site.id<best->id) best=&site;
    }
    return best;
}

inline const PrimitiveSanitationSite* activeDesignatedSanitationSite(
    const std::vector<PrimitiveSanitationSite>& sites)
{
    const PrimitiveSanitationSite* site=activePrimitiveSanitationSite(sites);
    return site!=nullptr && site->kind==PrimitiveSanitationSiteKind::DesignatedArea
        ? site
        : nullptr;
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
    if(activePrimitiveSanitationSite(sites)!=nullptr) return false;
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

struct DugSanitationPitOpportunity {
    bool candidateAvailable=false;
    SanitationSiteId siteId=0;
    GridPos pos{};
    int useCount=0;
    double siteExposure=0.0;
    double problemConfidence=0.0;
};

inline DugSanitationPitOpportunity evaluateDugSanitationPitOpportunity(
    const Character& character,
    const EnvironmentalResidueField& field,
    const std::vector<PrimitiveSanitationSite>& sites)
{
    DugSanitationPitOpportunity result;
    const PrimitiveSanitationSite* site=activePrimitiveSanitationSite(sites);
    if(site==nullptr || site->kind!=PrimitiveSanitationSiteKind::DesignatedArea) return result;
    if(!character.civilization.knowledge.knowsAtLeast(
        TechniqueId::DesignatedSanitationArea,KnowledgeLevel::Reproducible)) return result;

    result.siteId=site->id;
    result.pos=site->pos;
    result.useCount=site->useCount;
    result.siteExposure=field.exposureAt(site->pos);
    result.problemConfidence=recognizedSanitationProblemConfidence(character);
    result.candidateAvailable=hasRecognizedSanitationProblem(character)
        || site->useCount>=2
        || result.siteExposure>=0.12;
    return result;
}

inline bool canWorkOnDugSanitationPit(
    const Character& character,
    const std::vector<PrimitiveSanitationSite>& sites)
{
    const PrimitiveSanitationSite* site=activePrimitiveSanitationSite(sites);
    return site!=nullptr
        && site->kind==PrimitiveSanitationSiteKind::DesignatedArea
        && character.civilization.knowledge.knowsAtLeast(
            TechniqueId::DugSanitationPit,KnowledgeLevel::Reproducible);
}

inline double diggingToolWorkBonus(const Inventory& inventory)
{
    double best=0.0;
    for(const ItemStack& stack:inventory.stacks()){
        if(stack.quantity<=0 || itemCapability(stack.kind)!=ToolCapability::Dig) continue;
        best=std::max(best,0.90*stack.quality*stack.durability);
    }
    return best;
}

inline double dugSanitationPitWorkContribution(const Character& character)
{
    return 0.90
        +0.30*clampCivilization01(character.civilization.craftingSkill)
        +0.20*clampCivilization01(character.personality.patience)
        +diggingToolWorkBonus(character.civilization.inventory);
}

struct DugSanitationPitWorkResult {
    bool worked=false;
    bool completed=false;
    SanitationSiteId siteId=0;
    GridPos pos{};
    double workBefore=0.0;
    double workAfter=0.0;
};

inline DugSanitationPitWorkResult workOnDugSanitationPit(
    Character& character,
    EnvironmentalResidueField& field,
    std::vector<PrimitiveSanitationSite>& sites,
    int currentMinute)
{
    DugSanitationPitWorkResult result;
    if(!canWorkOnDugSanitationPit(character,sites)) return result;

    PrimitiveSanitationSite* site=activePrimitiveSanitationSite(sites);
    if(site==nullptr || site->kind!=PrimitiveSanitationSiteKind::DesignatedArea) return result;

    result.worked=true;
    result.siteId=site->id;
    result.pos=site->pos;
    result.workBefore=site->improvementWork;
    site->improvementWork=std::min(
        DugSanitationPitWorkRequired,
        site->improvementWork+dugSanitationPitWorkContribution(character));
    result.workAfter=site->improvementWork;

    if(site->improvementWork>=DugSanitationPitWorkRequired-1e-9){
        site->improvementWork=DugSanitationPitWorkRequired;
        site->kind=PrimitiveSanitationSiteKind::DugPit;
        site->improvedBy=character.id;
        site->improvedMinute=currentMinute;
        field.containHumanWasteAt(
            site->pos,currentMinute,
            DugSanitationPitContainmentIntensityFactor,
            DugSanitationPitContainmentRadiusTiles);
        result.workAfter=site->improvementWork;
        result.completed=true;
    }
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
    if(const PrimitiveSanitationSite* site=activePrimitiveSanitationSite(sites)){
        return {SanitationUseTargetKind::DesignatedArea,site->pos,site->id};
    }
    return {
        SanitationUseTargetKind::EmergencyOutdoor,
        chooseLowExposureOutdoorReliefPosition(worldSeed,character,field,currentMinute),
        0};
}

inline bool recordPrimitiveSanitationSiteUse(
    std::vector<PrimitiveSanitationSite>& sites,
    SanitationSiteId siteId,
    GridPos resolvedPosition)
{
    PrimitiveSanitationSite* site=findPrimitiveSanitationSite(sites,siteId);
    if(site==nullptr || !site->active
       || !validPrimitiveSanitationSiteKind(site->kind)
       || site->pos.x!=resolvedPosition.x || site->pos.y!=resolvedPosition.y) return false;
    ++site->useCount;
    return true;
}

inline bool recordDesignatedSanitationSiteUse(
    std::vector<PrimitiveSanitationSite>& sites,
    SanitationSiteId siteId,
    GridPos resolvedPosition)
{
    return recordPrimitiveSanitationSiteUse(sites,siteId,resolvedPosition);
}

inline int designatedSanitationUseDurationTicks()
{
    return 2;
}

inline int primitiveSanitationUseDurationTicks(PrimitiveSanitationSiteKind /*kind*/)
{
    return 2;
}

inline NeedsDelta designatedSanitationUseEffectPerTick()
{
    return {0,0,0,-0.13,0.012};
}

inline NeedsDelta primitiveSanitationUseEffectPerTick(PrimitiveSanitationSiteKind kind)
{
    if(kind==PrimitiveSanitationSiteKind::DugPit) return {0,0,0,-0.14,0.004};
    return designatedSanitationUseEffectPerTick();
}

inline double primitiveSanitationResidueIntensity(PrimitiveSanitationSiteKind kind)
{
    return kind==PrimitiveSanitationSiteKind::DugPit ? 0.16 : 0.42;
}

inline int primitiveSanitationResidueRadiusTiles(PrimitiveSanitationSiteKind kind)
{
    return kind==PrimitiveSanitationSiteKind::DugPit ? 1 : 3;
}

inline double primitiveSanitationHygieneBurden(PrimitiveSanitationSiteKind kind)
{
    return kind==PrimitiveSanitationSiteKind::DugPit ? 0.008 : 0.025;
}

} // namespace lifelens
