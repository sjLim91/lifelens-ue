#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "Character.h"
#include "EnvironmentalResidue.h"
#include "SanitationProblemRecognition.h"

namespace lifelens {

struct EnvironmentalExposureResult {
    double exposure=0.0;
    double hygieneBurden=0.0;
    bool perceived=false;
    bool memoryRecorded=false;
    bool sanitationProblemRecognized=false;
    bool sanitationProblemNewlyRecognized=false;
    double sanitationProblemConfidence=0.0;
};

inline std::string environmentalGridLabel(GridPos pos)
{
    std::ostringstream out;
    out<<"grid:"<<pos.x<<","<<pos.y;
    return out.str();
}

inline bool hasMemoryTag(const MemoryRecord& memory,const std::string& tag)
{
    return std::find(memory.tags.begin(),memory.tags.end(),tag)!=memory.tags.end();
}

inline double rememberedContaminationAt(
    const Character& character,
    GridPos pos,
    int currentMinute)
{
    const std::string where=environmentalGridLabel(pos);
    double strongest=0.0;
    const std::vector<std::string> queryTags={"environment","contamination","avoidance"};
    for(const auto& memory:character.memory.entries){
        if(memory.where!=where || !hasMemoryTag(memory,"contamination")) continue;
        strongest=std::max(strongest,memory.recallScore(currentMinute,0,queryTags));
    }
    return strongest;
}

inline bool recentlyRememberedContaminationAt(
    const Character& character,
    GridPos pos,
    int currentMinute,
    int dedupeWindowMinutes=180)
{
    const std::string where=environmentalGridLabel(pos);
    for(const auto& memory:character.memory.entries){
        if(memory.where!=where || !hasMemoryTag(memory,"contamination")) continue;
        if(currentMinute>=memory.minute && currentMinute-memory.minute<=dedupeWindowMinutes) return true;
    }
    return false;
}

inline EnvironmentalExposureResult perceiveEnvironmentalContamination(
    Character& character,
    const EnvironmentalResidueField& field,
    GridPos pos,
    int currentMinute)
{
    EnvironmentalExposureResult result;
    result.exposure=field.exposureAt(pos);

    // Ignore trace background values. Perception is intentionally distinct from
    // future disease/pathogen modelling; v1 represents immediate sanitation
    // discomfort, hygiene burden and experiential memory only.
    if(result.exposure<0.08) return result;

    result.perceived=true;
    result.hygieneBurden=0.002+result.exposure*0.018;
    character.needs.hygiene=Needs::clamp01(character.needs.hygiene+result.hygieneBurden);

    EmotionDelta discomfort;
    discomfort.sadness=0.008*result.exposure;
    discomfort.anxiety=0.028*result.exposure;
    character.emotion.apply(discomfort);

    if(result.exposure>=0.18
       && !recentlyRememberedContaminationAt(character,pos,currentMinute)){
        MemoryRecord memory;
        memory.who=0;
        memory.sourceCharacter=character.id;
        memory.what="experienced unsanitary surroundings";
        memory.where=environmentalGridLabel(pos);
        memory.minute=currentMinute;
        memory.emotionValence=-std::min(0.92,0.22+0.66*result.exposure);
        memory.emotionIntensity=std::min(1.0,0.28+0.70*result.exposure);
        memory.importance=std::min(1.0,0.34+0.52*result.exposure);
        memory.confidence=1.0;
        memory.witnessed=true;
        memory.source=MemorySource::DirectWitness;
        memory.decayPerDay=0.012;
        memory.tags={"environment","contamination","human_waste","avoidance","sanitation"};
        character.memory.add(std::move(memory));
        result.memoryRecorded=true;
    }

    const SanitationProblemRecognitionResult recognition=
        recognizeSanitationProblem(character,currentMinute);
    result.sanitationProblemRecognized=recognition.recognized;
    result.sanitationProblemNewlyRecognized=recognition.becameRecognized;
    result.sanitationProblemConfidence=recognition.beliefConfidence;

    return result;
}

inline double outdoorReliefAvoidanceScore(
    const Character& character,
    const EnvironmentalResidueField& field,
    GridPos candidate,
    int currentMinute)
{
    const double physicalExposure=field.exposureAt(candidate);
    const double remembered=rememberedContaminationAt(character,candidate,currentMinute);
    return physicalExposure*1.35+remembered*0.85;
}

inline GridPos chooseLowExposureOutdoorReliefPosition(
    std::uint64_t worldSeed,
    const Character& character,
    const EnvironmentalResidueField& field,
    int currentMinute)
{
    static constexpr std::array<GridPos,8> Directions={{
        {1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1}
    }};

    // v1 uses Core grid origin as the settlement reference, matching the
    // existing emergency-sanitation contract. Future settlement/household
    // centers can replace this reference without changing the scoring model.
    const std::uint64_t mixed=environmentalMix(
        (worldSeed?worldSeed:1ULL)^environmentalMix(character.id));
    const int startDirection=static_cast<int>(mixed%Directions.size());
    const int startDistance=5+static_cast<int>((mixed>>8)%3ULL);

    GridPos best{};
    double bestScore=1.0e9;
    int stableOrder=0;
    int bestOrder=1000000;

    for(int distanceOffset=0;distanceOffset<3;++distanceOffset){
        const int distance=5+((startDistance-5+distanceOffset)%3);
        for(int directionOffset=0;directionOffset<static_cast<int>(Directions.size());++directionOffset){
            const int directionIndex=(startDirection+directionOffset)%static_cast<int>(Directions.size());
            const GridPos direction=Directions[static_cast<std::size_t>(directionIndex)];
            const GridPos candidate{direction.x*distance,direction.y*distance};
            const double score=outdoorReliefAvoidanceScore(
                character,field,candidate,currentMinute);

            if(score+1.0e-9<bestScore
               || (std::abs(score-bestScore)<=1.0e-9 && stableOrder<bestOrder)){
                best=candidate;
                bestScore=score;
                bestOrder=stableOrder;
            }
            ++stableOrder;
        }
    }

    return best;
}

} // namespace lifelens
