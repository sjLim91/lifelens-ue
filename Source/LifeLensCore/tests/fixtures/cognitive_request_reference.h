#pragma once
// Frozen semantic reference from main@ad7062c (before view-only optimization).
// Used to verify byte-identical request fingerprints and tie/pinning behavior.
#include "lifelens/CognitiveAgent.h"
namespace lifelens {
inline CognitiveRequest referenceBuildCognitiveRequest(
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
    // Reserve one of the existing bounded slots for the resident's MOST
    // RECENT firsthand outcome. Merely keeping any high-recall old outcome
    // can hide a newer, low-salience failed attempt from the next local AI
    // request. Never create an outcome or elevate hearsay/inference.
    const auto directActionEvidence=[&](const CognitiveMemoryEvidence& m){
        if(m.source!=MemorySource::DirectWitness ||!m.witnessed
           ||m.minute<0 ||m.minute>currentMinute
           ||currentMinute-m.minute>1440 ||m.tags.empty()) return false;
        return m.tags.front()=="action_experience"
            ||m.tags.front()=="goal_action_feedback"
            ||m.tags.front()=="social_self_outcome"
            // Current Core physical threat and contact injury outcomes are
            // personally witnessed experiences too, not background rumours.
            // Keep the most recent event visible to local reasoning without
            // increasing its bounded eight-memory budget or claiming that
            // a witnessed encounter proves an attack occurred.
            ||m.tags.front()=="predator_encounter"
            ||m.tags.front()=="predator_contact_injury"
            // A genuinely completed close-range care provision is a
            // firsthand social/economic outcome for BOTH giver and receiver.
            // Local reasoning can remember aid given or received without
            // converting a transfer into claimed nutrition or instant healing.
            ||m.tags.front()=="injury_care_provision";
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
            if(m.who!=actor.id ||m.source!=MemorySource::DirectWitness
               ||!m.witnessed ||m.minute<0 ||m.minute>currentMinute
               ||currentMinute-m.minute>1440 ||m.tags.size()!=3
               ||m.tags[0]!="action_experience"
               ||m.tags[1].rfind("Civilization:",0)!=0
               ||(m.tags[2]!="success" &&m.tags[2]!="failure"))
                continue;
            byAction[m.tags[1]].push_back(i);
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
                    if(memories[a].tags[2]!=memories[b].tags[2])
                        return memories[a].tags[2]=="success";
                    return a<b;
                });
            const std::size_t newestAttempt=attempts.front();
            if(memories[newestAttempt].tags[2]!="failure") continue;
            std::size_t priorAttempt=missing;
            for(const std::size_t i:attempts)
                if(memories[i].minute<memories[newestAttempt].minute){
                    priorAttempt=i;
                    break;
                }
            if(priorAttempt==missing
               ||memories[priorAttempt].tags[2]!="failure") continue;
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
        std::vector<CognitiveMemoryEvidence> chosen;
        chosen.reserve(indices.size());
        for(const auto i:indices)
            chosen.push_back(std::move(memories[i]));
        memories=std::move(chosen);
    }
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
