#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "Character.h"
#include "Parenting.h"
#include "Relationship.h"
#include "World.h"

namespace lifelens {

enum class CognitiveTriggerKind {
    None,
    RepeatedFailure,
    ResourceScarcity,
    SocialConflict,
    MajorLifeEvent,
    Discovery,
    MigrationPressure,
    LeadershipDecision,
    Reflection
};

inline constexpr std::size_t DefaultCognitiveMemoryContextLimit=8;
inline constexpr std::size_t DefaultCognitiveBeliefContextLimit=8;
inline constexpr std::size_t DefaultCognitiveRelationshipContextLimit=8;

enum class CognitiveIntentKind {
    None,
    ImproveFoodSecurity,
    ImproveWaterSecurity,
    ImproveShelter,
    ImproveSanitation,
    AcquireMaterials,
    CraftUsefulTools,
    ExpandCultivation,
    ExploreOpportunity,
    CooperateWithResident,
    ResolveConflict,
    TeachKnowledge,
    TradeWithResident,
    CareForDependent,
    MigrateHousehold
};

inline const char* cognitiveTriggerName(CognitiveTriggerKind trigger)
{
    switch(trigger){
        case CognitiveTriggerKind::RepeatedFailure: return "RepeatedFailure";
        case CognitiveTriggerKind::ResourceScarcity: return "ResourceScarcity";
        case CognitiveTriggerKind::SocialConflict: return "SocialConflict";
        case CognitiveTriggerKind::MajorLifeEvent: return "MajorLifeEvent";
        case CognitiveTriggerKind::Discovery: return "Discovery";
        case CognitiveTriggerKind::MigrationPressure: return "MigrationPressure";
        case CognitiveTriggerKind::LeadershipDecision: return "LeadershipDecision";
        case CognitiveTriggerKind::Reflection: return "Reflection";
        case CognitiveTriggerKind::None:
        default: return "None";
    }
}

inline const char* cognitiveIntentName(CognitiveIntentKind intent)
{
    switch(intent){
        case CognitiveIntentKind::ImproveFoodSecurity: return "ImproveFoodSecurity";
        case CognitiveIntentKind::ImproveWaterSecurity: return "ImproveWaterSecurity";
        case CognitiveIntentKind::ImproveShelter: return "ImproveShelter";
        case CognitiveIntentKind::ImproveSanitation: return "ImproveSanitation";
        case CognitiveIntentKind::AcquireMaterials: return "AcquireMaterials";
        case CognitiveIntentKind::CraftUsefulTools: return "CraftUsefulTools";
        case CognitiveIntentKind::ExpandCultivation: return "ExpandCultivation";
        case CognitiveIntentKind::ExploreOpportunity: return "ExploreOpportunity";
        case CognitiveIntentKind::CooperateWithResident: return "CooperateWithResident";
        case CognitiveIntentKind::ResolveConflict: return "ResolveConflict";
        case CognitiveIntentKind::TeachKnowledge: return "TeachKnowledge";
        case CognitiveIntentKind::TradeWithResident: return "TradeWithResident";
        case CognitiveIntentKind::CareForDependent: return "CareForDependent";
        case CognitiveIntentKind::MigrateHousehold: return "MigrateHousehold";
        case CognitiveIntentKind::None:
        default: return "None";
    }
}

inline std::vector<CognitiveIntentKind> cognitiveStrategicIntentCatalog()
{
    return {
        CognitiveIntentKind::ImproveFoodSecurity,
        CognitiveIntentKind::ImproveWaterSecurity,
        CognitiveIntentKind::ImproveShelter,
        CognitiveIntentKind::ImproveSanitation,
        CognitiveIntentKind::AcquireMaterials,
        CognitiveIntentKind::CraftUsefulTools,
        CognitiveIntentKind::ExpandCultivation,
        CognitiveIntentKind::ExploreOpportunity,
        CognitiveIntentKind::CooperateWithResident,
        CognitiveIntentKind::ResolveConflict,
        CognitiveIntentKind::TeachKnowledge,
        CognitiveIntentKind::TradeWithResident,
        CognitiveIntentKind::CareForDependent,
        CognitiveIntentKind::MigrateHousehold
    };
}

inline bool cognitiveIntentTargetsResident(CognitiveIntentKind intent)
{
    switch(intent){
        case CognitiveIntentKind::CooperateWithResident:
        case CognitiveIntentKind::ResolveConflict:
        case CognitiveIntentKind::TeachKnowledge:
        case CognitiveIntentKind::TradeWithResident:
        case CognitiveIntentKind::CareForDependent:
            return true;
        default:
            return false;
    }
}

struct CognitiveMemoryEvidence {
    CharacterId who=0;
    int minute=0;
    double recallScore=0.0;
    double confidence=0.0;
    double importance=0.0;
    double emotionValence=0.0;
    double emotionIntensity=0.0;
    std::string what;
    std::string where;
    std::vector<std::string> tags;
};

struct CognitiveBeliefEvidence {
    CharacterId subject=0;
    std::string proposition;
    double stance=0.0;
    double confidence=0.0;
    int lastUpdatedMinute=0;
};

struct CognitiveRelationshipEvidence {
    CharacterId target=0;
    double socialBond=0.0;
    double affection=0.0;
    double trust=0.0;
    double respect=0.0;
    double conflict=0.0;
    double fear=0.0;
    double grudge=0.0;
};

struct CognitiveRequest {
    CharacterId actor=0;
    int minute=0;
    CognitiveTriggerKind trigger=CognitiveTriggerKind::None;
    Needs needs{};
    Personality personality{};
    EmotionState emotion{};
    std::vector<CognitiveMemoryEvidence> memories;
    std::vector<CognitiveBeliefEvidence> beliefs;
    std::vector<CognitiveRelationshipEvidence> relationships;
    std::vector<CognitiveIntentKind> allowedIntents;
};

struct CognitiveProposal {
    CharacterId actor=0;
    CognitiveIntentKind intent=CognitiveIntentKind::None;
    double priority=0.0;
    CharacterId targetResident=0;
    std::string rationale;
};

enum class CognitiveValidationIssue {
    None,
    MissingTrigger,
    ActorMismatch,
    ActorUnavailable,
    InvalidIntent,
    IntentNotAllowed,
    InvalidPriority,
    MissingTarget,
    UnexpectedTarget,
    InvalidTarget
};

struct CognitiveValidationResult {
    bool accepted=false;
    CognitiveValidationIssue issue=CognitiveValidationIssue::InvalidIntent;
};

inline bool cognitiveIntentAllowed(
    const CognitiveRequest& request,
    CognitiveIntentKind intent)
{
    return std::find(
        request.allowedIntents.begin(),
        request.allowedIntents.end(),
        intent)!=request.allowedIntents.end();
}

inline const Character* cognitiveCharacter(
    const World& world,
    CharacterId id)
{
    if(id==0) return nullptr;
    for(const Character& character:world.characters){
        if(character.id==id) return &character;
    }
    return nullptr;
}

inline CognitiveValidationResult validateCognitiveProposal(
    const World& world,
    const CognitiveRequest& request,
    const CognitiveProposal& proposal)
{
    const auto reject=[](CognitiveValidationIssue issue){
        return CognitiveValidationResult{false,issue};
    };

    if(request.trigger==CognitiveTriggerKind::None)
        return reject(CognitiveValidationIssue::MissingTrigger);
    if(proposal.actor==0 || proposal.actor!=request.actor)
        return reject(CognitiveValidationIssue::ActorMismatch);

    const Character* actor=cognitiveCharacter(world,request.actor);
    if(actor==nullptr || !actor->alive || requiresDirectCare(actor->lifeStage))
        return reject(CognitiveValidationIssue::ActorUnavailable);

    if(proposal.intent==CognitiveIntentKind::None)
        return reject(CognitiveValidationIssue::InvalidIntent);
    if(!cognitiveIntentAllowed(request,proposal.intent))
        return reject(CognitiveValidationIssue::IntentNotAllowed);
    if(!std::isfinite(proposal.priority)
       || proposal.priority<0.0
       || proposal.priority>1.0)
        return reject(CognitiveValidationIssue::InvalidPriority);

    const bool requiresTarget=cognitiveIntentTargetsResident(proposal.intent);
    if(requiresTarget && proposal.targetResident==0)
        return reject(CognitiveValidationIssue::MissingTarget);
    if(!requiresTarget && proposal.targetResident!=0)
        return reject(CognitiveValidationIssue::UnexpectedTarget);
    if(requiresTarget){
        const Character* target=cognitiveCharacter(world,proposal.targetResident);
        if(target==nullptr || !target->alive || target->id==actor->id)
            return reject(CognitiveValidationIssue::InvalidTarget);
    }

    return {true,CognitiveValidationIssue::None};
}

inline CognitiveRequest buildCognitiveRequest(
    const Character& actor,
    const RelationshipBook& relationships,
    int currentMinute,
    CognitiveTriggerKind trigger,
    std::vector<CognitiveIntentKind> allowedIntents,
    std::size_t memoryLimit,
    std::size_t beliefLimit,
    std::size_t relationshipLimit)
{
    CognitiveRequest request;
    request.actor=actor.id;
    request.minute=currentMinute;
    request.trigger=trigger;
    request.needs=actor.needs;
    request.personality=actor.personality;
    request.emotion=actor.emotion;
    request.allowedIntents=std::move(allowedIntents);

    std::vector<CognitiveMemoryEvidence> memories;
    memories.reserve(actor.memory.entries.size());
    for(const MemoryRecord& memory:actor.memory.entries){
        CognitiveMemoryEvidence evidence;
        evidence.who=memory.who;
        evidence.minute=memory.minute;
        evidence.recallScore=memory.recallScore(currentMinute);
        evidence.confidence=memory.effectiveConfidence(currentMinute);
        evidence.importance=memory.importance;
        evidence.emotionValence=memory.emotionValence;
        evidence.emotionIntensity=memory.emotionIntensity;
        evidence.what=memory.what;
        evidence.where=memory.where;
        evidence.tags=memory.tags;
        memories.push_back(std::move(evidence));
    }
    std::sort(memories.begin(),memories.end(),
        [](const CognitiveMemoryEvidence& a,const CognitiveMemoryEvidence& b){
            if(a.recallScore!=b.recallScore) return a.recallScore>b.recallScore;
            if(a.minute!=b.minute) return a.minute>b.minute;
            if(a.who!=b.who) return a.who<b.who;
            return a.what<b.what;
        });
    if(memories.size()>memoryLimit) memories.resize(memoryLimit);
    request.memories=std::move(memories);

    std::vector<CognitiveBeliefEvidence> beliefs;
    beliefs.reserve(actor.beliefs.beliefs.size());
    for(const BeliefRecord& belief:actor.beliefs.beliefs){
        CognitiveBeliefEvidence evidence;
        evidence.subject=belief.subject;
        evidence.proposition=belief.proposition;
        evidence.stance=belief.stance;
        evidence.confidence=belief.confidence;
        evidence.lastUpdatedMinute=belief.lastUpdatedMinute;
        beliefs.push_back(std::move(evidence));
    }
    std::sort(beliefs.begin(),beliefs.end(),
        [](const CognitiveBeliefEvidence& a,const CognitiveBeliefEvidence& b){
            const double as=a.confidence*std::abs(a.stance);
            const double bs=b.confidence*std::abs(b.stance);
            if(as!=bs) return as>bs;
            if(a.lastUpdatedMinute!=b.lastUpdatedMinute)
                return a.lastUpdatedMinute>b.lastUpdatedMinute;
            if(a.subject!=b.subject) return a.subject<b.subject;
            return a.proposition<b.proposition;
        });
    if(beliefs.size()>beliefLimit) beliefs.resize(beliefLimit);
    request.beliefs=std::move(beliefs);

    std::vector<CognitiveRelationshipEvidence> relationEvidence;
    for(const Relationship& relationship:relationships.all()){
        if(relationship.from!=actor.id || relationship.to==0) continue;
        CognitiveRelationshipEvidence evidence;
        evidence.target=relationship.to;
        evidence.socialBond=relationship.socialBond();
        evidence.affection=relationship.affection;
        evidence.trust=relationship.trust;
        evidence.respect=relationship.respect;
        evidence.conflict=relationship.conflict;
        evidence.fear=relationship.fear;
        evidence.grudge=relationship.grudge;
        relationEvidence.push_back(evidence);
    }
    std::sort(relationEvidence.begin(),relationEvidence.end(),
        [](const CognitiveRelationshipEvidence& a,const CognitiveRelationshipEvidence& b){
            const double as=std::max(a.socialBond,std::max({a.conflict,a.fear,a.grudge}));
            const double bs=std::max(b.socialBond,std::max({b.conflict,b.fear,b.grudge}));
            if(as!=bs) return as>bs;
            return a.target<b.target;
        });
    if(relationEvidence.size()>relationshipLimit)
        relationEvidence.resize(relationshipLimit);
    request.relationships=std::move(relationEvidence);

    return request;
}

} // namespace lifelens
