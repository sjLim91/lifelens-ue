#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>

#include "Character.h"
#include "Relationship.h"
#include "WitnessRumor.h"

namespace lifelens {

enum class TechniqueTransmissionMode {
    SelfDiscovery,
    DirectWitness,
    Imitation,
    Teaching
};

enum class TechniqueTeachingResult {
    Invalid,
    NoFact,
    TooWeak,
    DuplicateOrLoop,
    ComprehensionFailed,
    Advanced,
    AlreadyKnown
};

struct TechniqueTransmissionOutcome {
    TechniqueTeachingResult result=TechniqueTeachingResult::Invalid;
    SocialFactId factId=0;
    TechniqueId technique=TechniqueId::None;
    KnowledgeLevel before=KnowledgeLevel::Unknown;
    KnowledgeLevel after=KnowledgeLevel::Unknown;
    double confidence=0.0;
    double chance=0.0;
    double roll=1.0;
    bool receiptAccepted=false;
    bool knowledgeAdvanced=false;
};

inline std::string techniqueFactProposition(TechniqueId technique)
{
    return std::string("demonstrated civilization technique:")+
        std::to_string(static_cast<int>(technique));
}

inline SocialFactId civilizationTechniqueFactId(
    std::uint64_t worldSeed,
    CharacterId actor,
    TechniqueId technique,
    int minute,
    CivilizationEventType eventType)
{
    std::uint64_t value=mixKnowledge64(worldSeed ^ 0x434956544543484Eull);
    value=mixKnowledge64(value ^ actor);
    value=mixKnowledge64(value ^ (static_cast<std::uint64_t>(technique)+1ULL)*0x9E3779B97F4A7C15ull);
    value=mixKnowledge64(value ^ (static_cast<std::uint64_t>(eventType)+1ULL)*0xBF58476D1CE4E5B9ull);
    value=mixKnowledge64(value ^ static_cast<std::uint64_t>(std::max(0,minute)));
    return value==0 ? 1 : value;
}

inline SocialFact makeCivilizationTechniqueFact(
    std::uint64_t worldSeed,
    CharacterId actor,
    TechniqueId technique,
    int minute,
    CivilizationEventType eventType)
{
    SocialFact fact;
    fact.id=civilizationTechniqueFactId(worldSeed,actor,technique,minute,eventType);
    fact.subject=actor;
    fact.proposition=techniqueFactProposition(technique);
    fact.where="civilization-worksite";
    fact.eventMinute=minute;
    fact.supports=true;
    fact.importance=eventType==CivilizationEventType::Discovered ? 0.92 : 0.72;
    fact.confidence=eventType==CivilizationEventType::Discovered ? 0.98 : 0.94;
    fact.emotionValence=0.32;
    fact.emotionIntensity=eventType==CivilizationEventType::Discovered ? 0.74 : 0.46;
    return fact;
}

inline bool factRepresentsTechnique(const SocialFact& fact,TechniqueId technique)
{
    return technique!=TechniqueId::None && fact.proposition==techniqueFactProposition(technique);
}

inline double techniqueMasteryFactor(KnowledgeLevel level)
{
    switch(level){
        case KnowledgeLevel::Observed: return 0.12;
        case KnowledgeLevel::Hypothesized: return 0.24;
        case KnowledgeLevel::Understood: return 0.42;
        case KnowledgeLevel::Reproducible: return 0.64;
        case KnowledgeLevel::Practiced: return 0.82;
        case KnowledgeLevel::Mastered: return 1.00;
        default: return 0.0;
    }
}

inline bool techniquePrerequisiteContextSatisfied(const Character& learner,TechniqueId technique)
{
    if(technique==TechniqueId::None) return false;
    if(technique==TechniqueId::ChippedStoneTool &&
       !learner.civilization.knowledge.knowsAtLeast(
           TechniqueId::SharpFlake,KnowledgeLevel::Reproducible)) return false;
    if(technique==TechniqueId::DugSanitationPit &&
       !learner.civilization.knowledge.knowsAtLeast(
           TechniqueId::DesignatedSanitationArea,KnowledgeLevel::Reproducible)) return false;
    if(technique==TechniqueId::CopperSmelting &&
       (!learner.civilization.knowledge.knowsAtLeast(
            TechniqueId::FireMaking,KnowledgeLevel::Reproducible)
        || !learner.civilization.knowledge.knowsAtLeast(
            TechniqueId::StoneHammer,KnowledgeLevel::Reproducible)
        || !learner.civilization.knowledge.knowsAtLeast(
            TechniqueId::SimpleContainer,KnowledgeLevel::Reproducible))) return false;
    return true;
}

inline bool techniqueMaterialContextSatisfied(const Character& learner,TechniqueId technique)
{
    const TechniqueRecipe recipe=techniqueRecipe(technique);
    return recipe.technique!=TechniqueId::None &&
        hasIngredients(learner.civilization.inventory,recipe.inputs);
}

inline bool techniqueImitationContextSatisfied(const Character& learner,TechniqueId technique)
{
    return techniquePrerequisiteContextSatisfied(learner,technique) &&
        techniqueMaterialContextSatisfied(learner,technique);
}

inline const SocialFact* bestTechniqueFactForTeaching(
    const SocialKnowledgeBook& book,
    CharacterId teacher,
    CharacterId learner,
    TechniqueId technique)
{
    const SocialFact* best=nullptr;
    double bestConfidence=-1.0;
    int bestMinute=-1;
    for(const SocialFact& fact:book.facts()){
        if(!factRepresentsTechnique(fact,technique)) continue;
        const KnowledgeReceipt* teacherReceipt=book.findReceipt(teacher,fact.id);
        if(teacherReceipt==nullptr) continue;
        if(book.hasReceipt(learner,fact.id)) continue;
        if(std::find(
            teacherReceipt->transmissionPath.begin(),
            teacherReceipt->transmissionPath.end(),
            learner)!=teacherReceipt->transmissionPath.end()) continue;
        if(teacherReceipt->confidence>bestConfidence ||
           (teacherReceipt->confidence==bestConfidence && fact.eventMinute>bestMinute)){
            best=&fact;
            bestConfidence=teacherReceipt->confidence;
            bestMinute=fact.eventMinute;
        }
    }
    return best;
}

inline const KnowledgeReceipt* registerTechniqueOrigin(
    SocialKnowledgeBook& book,
    Character& actor,
    TechniqueId technique,
    int minute,
    CivilizationEventType eventType,
    std::uint64_t worldSeed)
{
    if(actor.id==0 || technique==TechniqueId::None) return nullptr;
    const SocialFact fact=makeCivilizationTechniqueFact(
        worldSeed,actor.id,technique,minute,eventType);
    if(!book.registerFact(fact)) return nullptr;
    return book.recordDirectWitness(
        fact.id,actor.id,actor.memory,actor.beliefs,minute);
}

inline TechniqueTransmissionOutcome applyTechniqueWitness(
    SocialKnowledgeBook& book,
    const SocialFact& fact,
    const Character& demonstrator,
    Character& observer,
    std::uint64_t worldSeed,
    int minute)
{
    TechniqueTransmissionOutcome outcome;
    outcome.factId=fact.id;
    if(observer.id==0 || demonstrator.id==0 || observer.id==demonstrator.id) return outcome;

    TechniqueId technique=TechniqueId::None;
    // Sanitation remains a protected subset of the now-extended range. Legacy
    // structural contract marker: raw<=static_cast<int>(TechniqueId::DugSanitationPit)
    for(int raw=static_cast<int>(TechniqueId::SharpFlake);
        raw<=static_cast<int>(TechniqueId::CopperSmelting);++raw){
        const TechniqueId candidate=static_cast<TechniqueId>(raw);
        if(factRepresentsTechnique(fact,candidate)){ technique=candidate; break; }
    }
    if(technique==TechniqueId::None) return outcome;
    outcome.technique=technique;
    outcome.before=observer.civilization.knowledge.level(technique);

    const KnowledgeReceipt* receipt=book.recordDirectWitness(
        fact.id,observer.id,observer.memory,observer.beliefs,minute);
    if(receipt==nullptr) return outcome;
    outcome.receiptAccepted=true;

    const double observedConfidence=std::max(0.30,std::min(0.78,receipt->confidence*0.72));
    observer.civilization.knowledge.learn(
        technique,KnowledgeLevel::Observed,observedConfidence);

    if(techniqueImitationContextSatisfied(observer,technique)){
        const double demonstratorMastery=techniqueMasteryFactor(
            demonstrator.civilization.knowledge.level(technique));
        outcome.chance=std::max(0.0,std::min(0.92,
            0.10
            +0.28*observer.civilization.learningSkill
            +0.18*observer.personality.curiosity
            +0.10*observer.personality.openness
            +0.18*demonstratorMastery));
        outcome.roll=deterministicKnowledgeUnit(
            worldSeed ^ 0x494D49544154454Bull,
            fact.id,demonstrator.id,observer.id,
            static_cast<std::uint64_t>(std::max(0,minute/15)));
        if(outcome.roll<outcome.chance){
            observer.civilization.knowledge.learn(
                technique,KnowledgeLevel::Understood,
                std::max(observedConfidence,0.48+0.28*(1.0-outcome.roll)));
            outcome.knowledgeAdvanced=true;
        }
    }

    outcome.after=observer.civilization.knowledge.level(technique);
    outcome.confidence=observer.civilization.knowledge.confidence(technique);
    outcome.result=outcome.knowledgeAdvanced
        ? TechniqueTeachingResult::Advanced
        : TechniqueTeachingResult::AlreadyKnown;
    return outcome;
}

inline double learnerTrustInTeacher(
    const RelationshipBook& relationships,
    CharacterId learner,
    CharacterId teacher)
{
    const Relationship* relation=relationships.find(learner,teacher);
    if(relation==nullptr) return 0.08;
    return std::max(0.0,std::min(1.0,
        relation->trust*0.62+
        relation->respect*0.18+
        relation->familiarity*0.12+
        relation->comfort*0.08));
}

inline TechniqueTransmissionOutcome teachTechnique(
    SocialKnowledgeBook& book,
    Character& teacher,
    Character& learner,
    TechniqueId technique,
    const RelationshipBook& relationships,
    std::uint64_t worldSeed,
    int minute,
    std::uint64_t epoch)
{
    TechniqueTransmissionOutcome outcome;
    outcome.technique=technique;
    outcome.before=learner.civilization.knowledge.level(technique);
    outcome.after=outcome.before;

    if(teacher.id==0 || learner.id==0 || teacher.id==learner.id ||
       technique==TechniqueId::None ||
       !teacher.civilization.knowledge.knowsAtLeast(
           technique,KnowledgeLevel::Reproducible)){
        outcome.result=TechniqueTeachingResult::Invalid;
        return outcome;
    }
    if(learner.civilization.knowledge.knowsAtLeast(
        technique,KnowledgeLevel::Reproducible)){
        outcome.result=TechniqueTeachingResult::AlreadyKnown;
        return outcome;
    }

    const SocialFact* fact=bestTechniqueFactForTeaching(
        book,teacher.id,learner.id,technique);
    if(fact==nullptr){
        outcome.result=TechniqueTeachingResult::NoFact;
        return outcome;
    }
    outcome.factId=fact->id;

    const auto statement=book.makeStatement(fact->id,teacher.id,minute,worldSeed);
    if(!statement){
        outcome.result=TechniqueTeachingResult::NoFact;
        return outcome;
    }

    const double trust=learnerTrustInTeacher(relationships,learner.id,teacher.id);
    const StatementReceptionOutcome reception=book.receiveStatement(
        *statement,learner.id,trust,
        learner.memory,learner.beliefs,minute,worldSeed);
    if(reception.result==StatementReceptionResult::Duplicate ||
       reception.result==StatementReceptionResult::LoopSuppressed){
        outcome.result=TechniqueTeachingResult::DuplicateOrLoop;
        return outcome;
    }
    if(reception.result==StatementReceptionResult::TooWeak){
        outcome.result=TechniqueTeachingResult::TooWeak;
        return outcome;
    }
    if(reception.result!=StatementReceptionResult::Accepted){
        outcome.result=TechniqueTeachingResult::Invalid;
        return outcome;
    }
    outcome.receiptAccepted=true;

    const KnowledgeLevel teacherLevel=teacher.civilization.knowledge.level(technique);
    const double mastery=techniqueMasteryFactor(teacherLevel);
    outcome.chance=std::max(0.0,std::min(0.97,
        0.08
        +0.24*trust
        +0.24*mastery
        +0.24*learner.civilization.learningSkill
        +0.12*learner.personality.curiosity
        +0.08*learner.personality.openness));
    outcome.roll=deterministicKnowledgeUnit(
        worldSeed ^ 0x54454143484C4C31ull,
        fact->id,teacher.id,learner.id,epoch);

    if(outcome.roll>=outcome.chance){
        if(outcome.before==KnowledgeLevel::Unknown){
            learner.civilization.knowledge.learn(
                technique,KnowledgeLevel::Observed,
                std::max(0.25,reception.acceptedConfidence*0.62));
        }
        outcome.after=learner.civilization.knowledge.level(technique);
        outcome.confidence=learner.civilization.knowledge.confidence(technique);
        outcome.result=TechniqueTeachingResult::ComprehensionFailed;
        return outcome;
    }

    KnowledgeLevel target=KnowledgeLevel::Understood;
    if(outcome.before==KnowledgeLevel::Unknown || outcome.before==KnowledgeLevel::Observed){
        target=KnowledgeLevel::Hypothesized;
    }else if(outcome.before==KnowledgeLevel::Hypothesized){
        target=KnowledgeLevel::Understood;
    }else if(outcome.before==KnowledgeLevel::Understood &&
             static_cast<int>(teacherLevel)>=static_cast<int>(KnowledgeLevel::Practiced) &&
             techniqueImitationContextSatisfied(learner,technique)){
        target=KnowledgeLevel::Reproducible;
    }

    const double learnedConfidence=std::max(
        reception.acceptedConfidence,
        std::min(0.94,0.42+0.30*mastery+0.18*learner.civilization.learningSkill));
    learner.civilization.knowledge.learn(technique,target,learnedConfidence);
    outcome.after=learner.civilization.knowledge.level(technique);
    outcome.confidence=learner.civilization.knowledge.confidence(technique);
    outcome.knowledgeAdvanced=static_cast<int>(outcome.after)>static_cast<int>(outcome.before);
    outcome.result=outcome.knowledgeAdvanced
        ? TechniqueTeachingResult::Advanced
        : TechniqueTeachingResult::AlreadyKnown;
    return outcome;
}

} // namespace lifelens
