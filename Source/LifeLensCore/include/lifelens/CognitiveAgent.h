#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <string>
#include <string_view>
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

inline const char* cognitiveMemorySourceName(MemorySource source)
{
    switch(source){
        case MemorySource::DirectWitness: return "DirectWitness";
        case MemorySource::ToldByOther: return "ToldByOther";
        case MemorySource::Inferred: return "Inferred";
        default: return "Inferred";
    }
}

struct CognitiveMemoryEvidence {
    CharacterId who=0;
    // Provenance is a property of a resident's memory, not World truth.
    // Do not collapse firsthand observation, hearsay and inference.
    MemorySource source=MemorySource::Inferred;
    bool witnessed=false;
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
    WorldSeed worldSeed=0;
    PopulationSeed populationSeed=0;
    WorldGenerationVersion generationVersion=0;
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

// Transient read-only view used only while building a bounded request. Views
// never escape buildCognitiveRequest and never become persisted memories.
struct CognitiveMemoryCandidate {
    CharacterId who=0;
    CharacterId sourceCharacter=0;
    MemorySource source=MemorySource::Inferred;
    bool witnessed=false;
    int minute=0;
    double recallScore=0.0;
    double confidence=0.0;
    double importance=0.0;
    double emotionValence=0.0;
    double emotionIntensity=0.0;
    std::string_view what;
    std::string_view where;
    const std::vector<std::string>* tags=nullptr;
};

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

    std::vector<CognitiveMemoryCandidate> memories;
    memories.reserve(actor.memory.entries.size());
    for(const MemoryRecord& memory:actor.memory.entries){
        // The personal mind cannot recall an event from its own future,
        // even if an old snapshot or imported memory contains it. Reject
        // impossible timestamps without editing authoritative history.
        if(memory.minute<0 ||memory.minute>currentMinute) continue;
        CognitiveMemoryCandidate evidence;
        evidence.who=memory.who;
        evidence.sourceCharacter=memory.sourceCharacter;
        evidence.source=memory.source;
        evidence.witnessed=memory.witnessed;
        evidence.minute=memory.minute;
        evidence.recallScore=memory.recallScore(currentMinute);
        evidence.confidence=memory.effectiveConfidence(currentMinute);
        evidence.importance=memory.importance;
        evidence.emotionValence=memory.emotionValence;
        evidence.emotionIntensity=memory.emotionIntensity;
        evidence.what=memory.what;
        evidence.where=memory.where;
        evidence.tags=&memory.tags;
        memories.push_back(std::move(evidence));
    }
    std::sort(memories.begin(),memories.end(),
        [](const CognitiveMemoryCandidate& a,const CognitiveMemoryCandidate& b){
            if(a.recallScore!=b.recallScore) return a.recallScore>b.recallScore;
            if(a.minute!=b.minute) return a.minute>b.minute;
            if(a.who!=b.who) return a.who<b.who;
            return a.what<b.what;
        });
    // Reserve one of the existing bounded slots for the resident's MOST
    // RECENT firsthand outcome. Merely keeping any high-recall old outcome
    // can hide a newer, low-salience failed attempt from the next local AI
    // request. Never create an outcome or elevate hearsay/inference.
    const auto directActionEvidence=[&](const CognitiveMemoryCandidate& m){
        if(m.source!=MemorySource::DirectWitness ||!m.witnessed
           ||m.minute<0 ||m.minute>currentMinute
           ||currentMinute-m.minute>1440 ||m.tags->empty()) return false;
        return m.tags->front()=="action_experience"
            ||m.tags->front()=="goal_action_feedback"
            ||m.tags->front()=="social_self_outcome"
            // Current Core physical threat and contact injury outcomes are
            // personally witnessed experiences too, not background rumours.
            // Keep the most recent event visible to local reasoning without
            // increasing its bounded eight-memory budget or claiming that
            // a witnessed encounter proves an attack occurred.
            ||m.tags->front()=="predator_encounter"
            ||m.tags->front()=="predator_contact_injury"
            // A genuinely completed close-range care provision is a
            // firsthand social/economic outcome for BOTH giver and receiver.
            // Local reasoning can remember aid given or received without
            // converting a transfer into claimed nutrition or instant healing.
            ||m.tags->front()=="injury_care_provision";
    };
    if(memoryLimit>0 &&memories.size()>memoryLimit){
        const std::size_t selected=memoryLimit;
        const std::size_t missing=memories.size();
        std::size_t newest=missing;
        for(std::size_t i=0;i<memories.size();++i){
            if(!directActionEvidence(memories[i])) continue;
            if(newest==missing
               ||memories[i].minute>memories[newest].minute
               ||(memories[i].minute==memories[newest].minute
                  &&memories[i].recallScore>memories[newest].recallScore))
                newest=i;
        }
        // Retain the latest firsthand event even if vivid older memories
        // fill the normal context window (unchanged existing contract).
        std::vector<std::size_t> required;
        if(newest!=missing) required.push_back(newest);

        // Two independent Core failures of the SAME resident/action must
        // reach the one local mind together. Preserving a chronological
        // history in World alone is insufficient if the context truncates
        // the older failed attempt and RepeatedFailure never triggers.
        std::map<std::string,std::vector<std::size_t>> byAction;
        for(std::size_t i=0;i<memories.size();++i){
            const auto& m=memories[i];
            if(m.source!=MemorySource::DirectWitness
               ||!m.witnessed ||m.minute<0 ||m.minute>currentMinute
               ||currentMinute-m.minute>1440 ||m.tags->size()!=3
               ||(*m.tags)[0]!="action_experience"
               ||((*m.tags)[2]!="success" &&(*m.tags)[2]!="failure"))
                continue;
            const std::string& action=(*m.tags)[1];
            const bool civilization=m.who==actor.id
                &&action.rfind("Civilization:",0)==0;
            // Core execution results record the real partner as "who".
            // They are still this resident's direct experience only when
            // Core's source character and execution token prove the origin.
            const bool partnerContext=m.who!=0 &&m.who!=actor.id
                &&m.sourceCharacter==actor.id
                &&(action=="CoreContext:Trade"
                   ||action=="CoreContext:Teaching"
                   ||action=="CoreContext:Parenting")
                &&m.where.rfind("core-context-token:",0)==0
                &&m.where.size()>19
                &&std::all_of(m.where.begin()+19,m.where.end(),
                    [](char ch){return ch>='0' &&ch<='9';});
            if(!civilization &&!partnerContext) continue;
            // Two different residents do not constitute two failures
            // against the same partner. Keep each lived attempt distinct.
            const std::string group=partnerContext
                ?action+":target:"+std::to_string(m.who):action;
            byAction[group].push_back(i);
        }
        std::size_t firstFailure=missing,secondFailure=missing;
        for(auto& group:byAction){
            auto& attempts=group.second;
            std::sort(attempts.begin(),attempts.end(),
                [&](std::size_t a,std::size_t b){
                    if(memories[a].minute!=memories[b].minute)
                        return memories[a].minute>memories[b].minute;
                    // A success in the same minute defeats a failure
                    // regardless of source memory iteration order.
                    if((*memories[a].tags)[2]!=(*memories[b].tags)[2])
                        return (*memories[a].tags)[2]=="success";
                    return a<b;
                });
            const std::size_t newestAttempt=attempts.front();
            if((*memories[newestAttempt].tags)[2]!="failure") continue;
            std::size_t priorAttempt=missing;
            for(const std::size_t i:attempts)
                if(memories[i].minute<memories[newestAttempt].minute){
                    priorAttempt=i;
                    break;
                }
            if(priorAttempt==missing
               ||(*memories[priorAttempt].tags)[2]!="failure") continue;
            if(firstFailure==missing
               ||memories[newestAttempt].minute
                   >memories[firstFailure].minute){
                firstFailure=newestAttempt;
                secondFailure=priorAttempt;
            }
        }
        // With a tiny budget, never displace an unrelated freshest direct
        // event just to display half a pair: that would prove nothing.
        if(firstFailure!=missing &&selected>=2
           &&(selected>=3 ||newest==firstFailure
               ||newest==secondFailure)){
            required.push_back(firstFailure);
            required.push_back(secondFailure);
        }

        std::vector<std::size_t> indices;
        indices.reserve(selected);
        for(std::size_t i=0;i<selected;++i) indices.push_back(i);
        for(const std::size_t include:required){
            if(std::find(indices.begin(),indices.end(),include)
               !=indices.end()) continue;
            for(std::size_t slot=indices.size();slot>0;--slot){
                if(std::find(required.begin(),required.end(),
                             indices[slot-1])!=required.end())
                    continue;
                indices[slot-1]=include;
                break;
            }
        }
        // The input order remains ranked by deterministic recall score,
        // and the same 8-memory budget is preserved without fake events.
        std::sort(indices.begin(),indices.end());
        std::vector<CognitiveMemoryCandidate> chosen;
        chosen.reserve(indices.size());
        for(const auto i:indices)
            chosen.push_back(std::move(memories[i]));
        memories=std::move(chosen);
    }
    if(memories.size()>memoryLimit) memories.resize(memoryLimit);
    // Copy text/tags only for the selected bounded context. All original
    // memories, ranking, latest-outcome and repeated-failure rules stay intact.
    request.memories.reserve(memories.size());
    for(const auto& selected:memories){
        CognitiveMemoryEvidence evidence;
        evidence.who=selected.who;
        evidence.source=selected.source;
        evidence.witnessed=selected.witnessed;
        evidence.minute=selected.minute;
        evidence.recallScore=selected.recallScore;
        evidence.confidence=selected.confidence;
        evidence.importance=selected.importance;
        evidence.emotionValence=selected.emotionValence;
        evidence.emotionIntensity=selected.emotionIntensity;
        evidence.what=selected.what;
        evidence.where=selected.where;
        evidence.tags=*selected.tags;
        request.memories.push_back(std::move(evidence));
    }

    std::vector<CognitiveBeliefEvidence> beliefs;
    beliefs.reserve(actor.beliefs.beliefs.size());
    for(const BeliefRecord& belief:actor.beliefs.beliefs){
        // Beliefs may be wrong or based on hearsay, but a belief first
        // updated in the future cannot be known to this past cognition tick.
        if(belief.lastUpdatedMinute<0
           ||belief.lastUpdatedMinute>currentMinute) continue;
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
