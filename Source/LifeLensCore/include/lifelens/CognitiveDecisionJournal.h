#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>
#include "CognitiveAgent.h"

namespace lifelens {

// An accepted thought is an auditable INPUT, not a command that can invent
// resources, residents, relationships or a physical action. COG-4 may later
// turn an accepted input into a bounded *candidate* for normal Core planning.
struct AcceptedCognitiveDecision {
    CharacterId actor=0;
    int minute=0;
    CognitiveTriggerKind trigger=CognitiveTriggerKind::None;
    std::uint64_t contextFingerprint=0;
    std::string backendIdentity;
    CognitiveIntentKind intent=CognitiveIntentKind::None;
    double priority=0.0;
    CharacterId target=0;
    std::string rationaleDigest;
    // The model may answer a previously issued Core view within a bounded
    // async window. Never relabel that old view as current firsthand context.
    int observedMinute=-1;
    std::uint64_t observedContextFingerprint=0;
};

inline constexpr std::size_t MaxAcceptedCognitiveDecisions=1024;
inline constexpr int ObservedCognitiveRequestMaxAgeMinutes=96;
inline constexpr std::size_t MaxCognitionIdentityBytes=128;

// Stable, platform-independent stream of IEEE754 binary64 values and bytes.
// This fingerprint is for freshness/determinism, not authentication.
inline std::uint64_t cognitiveRequestFingerprint(const CognitiveRequest& r)
{
    std::uint64_t h=14695981039346656037ULL;
    const auto byte=[&](unsigned char c) { h^=c; h*=1099511628211ULL; };
    const auto integer=[&](std::uint64_t n) {
        for(int i=0;i<8;++i) byte(static_cast<unsigned char>((n>>(i*8))&0xff));
    };
    const auto number=[&](double d) {
        std::uint64_t bits=0;
        static_assert(sizeof(d)==sizeof(bits),"Requires IEEE754 binary64");
        std::memcpy(&bits,&d,sizeof(bits));
        integer(bits);
    };
    const auto str=[&](const std::string& s) {
        integer(s.size());
        for(unsigned char c:s) byte(c);
    };
    integer(r.actor);
    integer(r.worldSeed);
    integer(r.populationSeed);
    integer(r.generationVersion);
    integer(static_cast<std::uint64_t>(r.minute));
    integer(static_cast<std::uint64_t>(r.trigger));
    for(double v:{r.needs.hunger,r.needs.thirst,r.needs.sleep,
                  r.needs.bladder,r.needs.hygiene}) number(v);
    const Personality& p=r.personality;
    for(double v:{p.introversion,p.conscientiousness,p.openness,
                  p.agreeableness,p.emotionalStability,p.empathy,
                  p.impulsiveness,p.riskTolerance,p.ambition,p.patience,
                  p.sociability,p.curiosity,p.orderliness,p.adaptability})
        number(v);
    const EmotionState& e=r.emotion;
    for(double v:{e.joy,e.sadness,e.anger,e.fear,e.embarrassment,e.pride,
                  e.jealousy,e.affection,e.anxiety,e.relief,e.grief,
                  e.valence,e.arousal}) number(v);
    integer(r.allowedIntents.size());
    for(CognitiveIntentKind intent:r.allowedIntents)
        integer(static_cast<std::uint64_t>(intent));
    integer(r.memories.size());
    for(const auto& m:r.memories){
        integer(m.who);integer(static_cast<std::uint64_t>(m.minute));
        // Same words with different origins must NOT share a context hash.
        integer(static_cast<std::uint64_t>(m.source));
        integer(m.witnessed ? 1ULL : 0ULL);
        number(m.recallScore);number(m.confidence);number(m.importance);
        number(m.emotionValence);number(m.emotionIntensity);
        str(m.what);str(m.where);
        integer(m.tags.size());
        for(const auto& tag:m.tags) str(tag);
    }
    if(r.localProximityObserved){
        // Domain separator preserves old fingerprints for legacy/shadow
        // requests that have no Core runtime sensor. Real new observations
        // are receipt-bound: an altered/moved/removed nearby resource makes
        // a previously issued model response stale rather than truthful.
        integer(0x4c4f43414c505258ULL);
        integer(static_cast<std::uint64_t>(
            static_cast<std::int64_t>(r.localObserverPosition.x)));
        integer(static_cast<std::uint64_t>(
            static_cast<std::int64_t>(r.localObserverPosition.y)));
        integer(static_cast<std::uint64_t>(r.localRadiusCells));
        integer(r.localResources.size());
        for(const auto& fact:r.localResources){
            integer(fact.nodeId);
            integer(static_cast<std::uint64_t>(fact.material));
            integer(static_cast<std::uint64_t>(
                static_cast<std::int64_t>(fact.position.x)));
            integer(static_cast<std::uint64_t>(
                static_cast<std::int64_t>(fact.position.y)));
            integer(static_cast<std::uint64_t>(fact.distanceCells));
            integer(static_cast<std::uint64_t>(fact.abundance));
        }
    }
    integer(r.beliefs.size());
    for(const auto& b:r.beliefs){
        integer(b.subject);str(b.proposition);
        number(b.stance);number(b.confidence);
        integer(static_cast<std::uint64_t>(b.lastUpdatedMinute));
    }
    integer(r.relationships.size());
    for(const auto& rel:r.relationships){
        integer(rel.target);number(rel.socialBond);number(rel.affection);
        number(rel.trust);number(rel.respect);number(rel.conflict);
        number(rel.fear);number(rel.grudge);
    }
    return h;
}

inline bool validAcceptedCognitiveDecision(
    const AcceptedCognitiveDecision& d,
    const World& world)
{
    if(d.actor==0 || d.minute<0 || d.minute>world.minute
       || d.trigger==CognitiveTriggerKind::None
       || d.intent==CognitiveIntentKind::None
       || d.contextFingerprint==0
       || !std::isfinite(d.priority)
       || d.priority<0.0 || d.priority>1.0
       || d.backendIdentity.empty()
       || d.backendIdentity.size()>MaxCognitionIdentityBytes
       || d.rationaleDigest.empty()
       || d.rationaleDigest.size()>MaxCognitionIdentityBytes)
        return false;
    if(d.observedMinute!=-1 ||d.observedContextFingerprint!=0){
        if(d.observedMinute<0 ||d.observedMinute>d.minute
           ||d.minute-d.observedMinute>ObservedCognitiveRequestMaxAgeMinutes
           ||d.observedContextFingerprint==0) return false;
    }
    switch(d.trigger){
        case CognitiveTriggerKind::RepeatedFailure:
        case CognitiveTriggerKind::ResourceScarcity:
        case CognitiveTriggerKind::SocialConflict:
        case CognitiveTriggerKind::MajorLifeEvent:
        case CognitiveTriggerKind::Discovery:
        case CognitiveTriggerKind::MigrationPressure:
        case CognitiveTriggerKind::LeadershipDecision:
        case CognitiveTriggerKind::Reflection: break;
        default: return false;
    }
    const auto catalog=cognitiveStrategicIntentCatalog();
    if(std::find(catalog.begin(),catalog.end(),d.intent)==catalog.end())
        return false;
    bool actorExists=false,targetExists=false;
    for(const Character& c:world.characters){
        if(c.id==d.actor) actorExists=true;
        if(c.id==d.target) targetExists=true;
    }
    if(!actorExists) return false;
    if(cognitiveIntentTargetsResident(d.intent))
        return d.target!=0 && d.target!=d.actor && targetExists;
    return d.target==0;
}

inline bool validCognitiveDecisionJournal(
    const std::vector<AcceptedCognitiveDecision>& events,
    const World& world)
{
    if(events.size()>MaxAcceptedCognitiveDecisions) return false;
    int previousMinute=-1;
    for(std::size_t i=0;i<events.size();++i){
        const auto& e=events[i];
        if(!validAcceptedCognitiveDecision(e,world)
           || e.minute<previousMinute) return false;
        for(std::size_t j=i;j>0;--j){
            const auto& prior=events[j-1];
            if(prior.minute!=e.minute) break;
            if(prior.actor==e.actor) return false;
        }
        previousMinute=e.minute;
    }
    return true;
}

inline bool appendAcceptedCognitiveDecision(
    std::vector<AcceptedCognitiveDecision>& events,
    const AcceptedCognitiveDecision& event,
    const World& world)
{
    if(!validAcceptedCognitiveDecision(event,world)
       || (!events.empty() && event.minute<events.back().minute))
        return false;
    for(auto it=events.rbegin();it!=events.rend();++it){
        if(it->minute!=event.minute) break;
        if(it->actor==event.actor) return false;
    }
    events.push_back(event);
    if(events.size()>MaxAcceptedCognitiveDecisions)
        events.erase(events.begin());
    return true;
}

} // namespace lifelens
