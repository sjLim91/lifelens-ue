#pragma once

#include <algorithm>
#include <cstddef>
#include <string>

#include "Character.h"

namespace lifelens {

struct SanitationProblemRecognitionResult {
    bool recognized=false;
    bool becameRecognized=false;
    std::size_t qualifyingMemories=0;
    double evidenceWeight=0.0;
    double strongestEvidence=0.0;
    double beliefConfidence=0.0;
    int latestEvidenceMinute=0;
};

inline const std::string& sanitationProblemBeliefProposition()
{
    static const std::string Proposition=
        "human-waste contamination is a recurring sanitation problem";
    return Proposition;
}

inline bool sanitationRecognitionMemoryHasTag(
    const MemoryRecord& memory,
    const std::string& tag)
{
    return std::find(memory.tags.begin(),memory.tags.end(),tag)!=memory.tags.end();
}

inline bool isDirectSanitationProblemEvidence(const MemoryRecord& memory)
{
    return memory.witnessed
        && memory.source==MemorySource::DirectWitness
        && sanitationRecognitionMemoryHasTag(memory,"contamination")
        && sanitationRecognitionMemoryHasTag(memory,"human_waste")
        && sanitationRecognitionMemoryHasTag(memory,"sanitation");
}

inline double sanitationProblemEvidenceStrength(
    const MemoryRecord& memory,
    int currentMinute)
{
    if(!isDirectSanitationProblemEvidence(memory) || memory.minute>currentMinute) return 0.0;

    const double confidence=memory.effectiveConfidence(currentMinute);
    const double salience=MemoryRecord::clamp01(
        0.45
        +0.35*MemoryRecord::clamp01(memory.importance)
        +0.20*MemoryRecord::clamp01(memory.emotionIntensity));
    return MemoryRecord::clamp01(confidence*salience);
}

inline bool isEstablishedSanitationProblemBelief(const BeliefRecord* belief)
{
    return belief!=nullptr
        && belief->stance>=0.50
        && belief->confidence>=0.55;
}

inline SanitationProblemRecognitionResult recognizeSanitationProblem(
    Character& character,
    int currentMinute)
{
    SanitationProblemRecognitionResult result;

    for(const auto& memory:character.memory.entries){
        const double strength=sanitationProblemEvidenceStrength(memory,currentMinute);
        if(strength<=0.0) continue;
        ++result.qualifyingMemories;
        result.evidenceWeight+=strength;
        result.strongestEvidence=std::max(result.strongestEvidence,strength);
        result.latestEvidenceMinute=std::max(result.latestEvidenceMinute,memory.minute);
    }

    const std::string& proposition=sanitationProblemBeliefProposition();
    const BeliefRecord* existing=character.beliefs.find(0,proposition);
    const bool wasRecognized=isEstablishedSanitationProblemBelief(existing);

    // Repetition is the normal path. A single exceptionally salient firsthand
    // event may also be enough to identify a recurring sanitation class of problem,
    // but ordinary low-grade exposure cannot create civilization knowledge at once.
    const bool enoughRepeatedEvidence=
        result.qualifyingMemories>=2 && result.evidenceWeight>=1.25;
    const bool enoughSingleStrongEvidence=
        result.qualifyingMemories>=1 && result.strongestEvidence>=0.90;
    const bool evidenceSupportsRecognition=
        enoughRepeatedEvidence || enoughSingleStrongEvidence;

    if(evidenceSupportsRecognition){
        BeliefRecord& belief=character.beliefs.getOrCreate(0,proposition);

        // Recompute from current memories and raise the durable Belief to at least
        // that evidence level. Never += the same memories again on later planning
        // ticks: recognition must be idempotent rather than evidence inflation.
        belief.supportWeight=std::max(belief.supportWeight,result.evidenceWeight);
        belief.supportCount=std::max(belief.supportCount,result.qualifyingMemories);
        belief.lastUpdatedMinute=std::max(belief.lastUpdatedMinute,result.latestEvidenceMinute);
        belief.refresh();

        result.beliefConfidence=belief.confidence;
        result.recognized=isEstablishedSanitationProblemBelief(&belief);
        result.becameRecognized=!wasRecognized && result.recognized;
        return result;
    }

    if(existing){
        result.beliefConfidence=existing->confidence;
        result.recognized=wasRecognized;
    }
    return result;
}

inline bool hasRecognizedSanitationProblem(const Character& character)
{
    return isEstablishedSanitationProblemBelief(
        character.beliefs.find(0,sanitationProblemBeliefProposition()));
}

} // namespace lifelens
