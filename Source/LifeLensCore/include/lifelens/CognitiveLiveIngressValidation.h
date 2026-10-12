#pragma once

#include <cstdint>
#include <cstring>
#include <string>

#include "Simulation.h"

// COG-B2a: pure, read-only Core admission gate for *candidate* strategic
// proposals. Admission here is NOT an accepted/persisted event and cannot
// execute or bias simulation. One Core journal (future COG-B2b) must be the
// sole writer of accepted input after all live checks pass.
namespace lifelens {

struct LiveCognitiveProposal {
    WorldSeed worldSeed=0;
    PopulationSeed populationSeed=0;
    WorldGenerationVersion generationVersion=0;
    CognitiveRequest request{};
    CognitiveProposal proposal{};
    std::uint64_t claimedContextFingerprint=0;
};

enum class LiveCognitiveValidationIssue {
    None,
    WrongWorld,
    StaleTime,
    ActorUnavailable,
    ForgedContext,
    InvalidProposal
};

struct LiveCognitiveValidation {
    bool valid=false;
    LiveCognitiveValidationIssue issue=
        LiveCognitiveValidationIssue::InvalidProposal;
    // Digest of *current* Core-owned request (not client supplied data).
    std::uint64_t currentContextFingerprint=0;
};

class CognitiveRequestHashV1 {
public:
    void word(std::uint64_t value){
        for(int i=0;i<8;++i){
            byte(static_cast<unsigned char>(value>>(i*8)));
        }
    }
    void number(double value){
        static_assert(sizeof(double)==sizeof(std::uint64_t)
            && std::numeric_limits<double>::is_iec559,
            "deterministic IEEE-754 binary64 required");
        std::uint64_t bits=0;
        std::memcpy(&bits,&value,sizeof(bits));
        word(bits);
    }
    void string(const std::string& value){
        word(static_cast<std::uint64_t>(value.size()));
        for(unsigned char c:value) byte(c);
    }
    std::uint64_t result() const{return hash_;}
private:
    void byte(unsigned char value){
        hash_^=value;
        hash_*=1099511628211ULL;
    }
    std::uint64_t hash_=14695981039346656037ULL;
};

// Stable World-bound digest of *all* bounded Core CognitionRequest fields.
// It detects stale/forged input only when compared to a re-observed request;
// this is not authentication or cryptographic integrity.
inline std::uint64_t liveCognitiveRequestFingerprint(
    const World& world,const CognitiveRequest& request)
{
    CognitiveRequestHashV1 h;
    h.word(0x434f47494e475631ULL); // COGINGV1
    h.word(world.seed);
    h.word(world.populationSeed);
    h.word(world.generationVersion);
    h.word(request.actor);
    h.word(static_cast<std::uint64_t>(request.minute));
    h.word(static_cast<std::uint64_t>(request.trigger));
    for(double value:{
        request.needs.hunger,request.needs.thirst,request.needs.sleep,
        request.needs.bladder,request.needs.hygiene,
        request.personality.introversion,request.personality.conscientiousness,
        request.personality.openness,request.personality.agreeableness,
        request.personality.emotionalStability,request.personality.empathy,
        request.personality.impulsiveness,request.personality.riskTolerance,
        request.personality.ambition,request.personality.patience,
        request.personality.sociability,request.personality.curiosity,
        request.personality.orderliness,request.personality.adaptability,
        request.emotion.joy,request.emotion.sadness,request.emotion.anger,
        request.emotion.fear,request.emotion.embarrassment,
        request.emotion.pride,request.emotion.jealousy,
        request.emotion.affection,request.emotion.anxiety,
        request.emotion.relief,request.emotion.grief,
        request.emotion.valence,request.emotion.arousal
    }) h.number(value);

    h.word(request.allowedIntents.size());
    for(const auto intent:request.allowedIntents){
        h.word(static_cast<std::uint64_t>(intent));
    }
    h.word(request.memories.size());
    for(const auto& memory:request.memories){
        h.word(memory.who);
        h.word(static_cast<std::uint64_t>(memory.minute));
        h.number(memory.recallScore);
        h.number(memory.confidence);
        h.number(memory.importance);
        h.number(memory.emotionValence);
        h.number(memory.emotionIntensity);
        h.string(memory.what);
        h.string(memory.where);
        h.word(memory.tags.size());
        for(const auto& tag:memory.tags)h.string(tag);
    }
    h.word(request.beliefs.size());
    for(const auto& belief:request.beliefs){
        h.word(belief.subject);
        h.string(belief.proposition);
        h.number(belief.stance);
        h.number(belief.confidence);
        h.word(static_cast<std::uint64_t>(belief.lastUpdatedMinute));
    }
    h.word(request.relationships.size());
    for(const auto& relation:request.relationships){
        h.word(relation.target);
        h.number(relation.socialBond);
        h.number(relation.affection);
        h.number(relation.trust);
        h.number(relation.respect);
        h.number(relation.conflict);
        h.number(relation.fear);
        h.number(relation.grudge);
    }
    return h.result();
}

inline LiveCognitiveValidation validateLiveCognitiveProposal(
    const Simulation& simulation,const LiveCognitiveProposal& candidate)
{
    const World& world=simulation.world();
    const auto reject=[](LiveCognitiveValidationIssue issue,
                         std::uint64_t current=0){
        return LiveCognitiveValidation{false,issue,current};
    };
    if(candidate.worldSeed!=world.seed
        ||candidate.populationSeed!=world.populationSeed
        ||candidate.generationVersion!=world.generationVersion)
        return reject(LiveCognitiveValidationIssue::WrongWorld);

    const CognitiveRequest& offered=candidate.request;
    if(offered.minute!=world.minute
        ||offered.minute<0
        ||offered.trigger==CognitiveTriggerKind::None)
        return reject(LiveCognitiveValidationIssue::StaleTime);
    if(offered.actor==0)
        return reject(LiveCognitiveValidationIssue::ActorUnavailable);

    const CognitiveRequest actual=simulation.observeCognitiveRequest(
        offered.actor,offered.trigger);
    if(actual.actor!=offered.actor)
        return reject(LiveCognitiveValidationIssue::ActorUnavailable);
    const std::uint64_t truth=liveCognitiveRequestFingerprint(world,actual);
    if(candidate.claimedContextFingerprint==0
        ||candidate.claimedContextFingerprint!=truth
        ||liveCognitiveRequestFingerprint(world,offered)!=truth)
        return reject(LiveCognitiveValidationIssue::ForgedContext,truth);

    if(!validateCognitiveProposal(world,actual,candidate.proposal).accepted)
        return reject(LiveCognitiveValidationIssue::InvalidProposal,truth);

    return {true,LiveCognitiveValidationIssue::None,truth};
}

} // namespace lifelens
