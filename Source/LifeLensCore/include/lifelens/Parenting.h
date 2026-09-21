#pragma once

#include <algorithm>
#include <array>
#include <limits>

#include "Character.h"
#include "LifeStage.h"
#include "Relationship.h"

namespace lifelens {

enum class ParentingAction {
    Feed,
    PutToSleep,
    Bathe,
    ToiletAssist,
    Hold,
    Play,
    Educate,
    Discipline,
    Comfort,
    HealthCare
};

struct ParentingContext {
    double timeAvailable=1.0;
    double resources=1.0;
    double caregiverStress=0.0;
    double warmth=0.75;
    double consistency=0.70;
    double harshness=0.10;
    bool foodAvailable=true;
    bool waterAvailable=true;
};

struct ParentingDecision {
    ParentingAction action=ParentingAction::Comfort;
    double utility=0.0;
    bool valid=false;
};

enum class ParentingResult {
    Invalid,
    NotParent,
    NotAllowedForStage,
    UnavailableResources,
    Performed
};

inline bool isDependentStage(LifeStage stage)
{
    return stage==LifeStage::Baby || stage==LifeStage::Toddler ||
           stage==LifeStage::Child || stage==LifeStage::Teen;
}

inline bool requiresDirectCare(LifeStage stage)
{
    return stage==LifeStage::Baby || stage==LifeStage::Toddler;
}

inline bool isParentOf(const Character& caregiver,const Character& child)
{
    return std::find(child.parentIds.begin(),child.parentIds.end(),caregiver.id)!=child.parentIds.end();
}

inline bool parentingActionAllowed(ParentingAction action,LifeStage stage)
{
    if(!isDependentStage(stage)) return false;
    switch(action){
        case ParentingAction::Feed:
        case ParentingAction::PutToSleep:
        case ParentingAction::Bathe:
        case ParentingAction::Play:
        case ParentingAction::Comfort:
        case ParentingAction::HealthCare:
            return true;
        case ParentingAction::ToiletAssist:
            return stage==LifeStage::Baby || stage==LifeStage::Toddler || stage==LifeStage::Child;
        case ParentingAction::Hold:
            return stage==LifeStage::Baby || stage==LifeStage::Toddler;
        case ParentingAction::Educate:
            return stage==LifeStage::Toddler || stage==LifeStage::Child || stage==LifeStage::Teen;
        case ParentingAction::Discipline:
            return stage==LifeStage::Toddler || stage==LifeStage::Child || stage==LifeStage::Teen;
    }
    return false;
}

inline double parentingUtility(
    ParentingAction action,
    const Character& caregiver,
    const Character& child,
    const Relationship& caregiverToChild,
    const ParentingContext& context)
{
    if(!parentingActionAllowed(action,child.lifeStage)) return -1.0;

    const double available=clampDevelopment(context.timeAvailable);
    const double resources=clampDevelopment(context.resources);
    const double caregiverCapacity=clampDevelopment(
        0.45*available+0.30*(1.0-clampDevelopment(context.caregiverStress))+
        0.15*clampDevelopment(caregiver.personality.patience)+
        0.10*clampDevelopment(caregiver.personality.empathy));
    const double bond=clampRelationship(
        0.55*caregiverToChild.affection+0.25*caregiverToChild.commitment+0.20*caregiverToChild.comfort);

    double urgency=0.0;
    switch(action){
        case ParentingAction::Feed: {
            if(!context.foodAvailable && !context.waterAvailable) return -1.0;
            const double foodWeight=context.foodAvailable ? 0.65 : 0.0;
            const double waterWeight=context.waterAvailable ? 0.35 : 0.0;
            const double totalWeight=foodWeight+waterWeight;
            urgency=totalWeight>0.0
                ? (foodWeight*child.needs.hunger+waterWeight*child.needs.thirst)/totalWeight
                : 0.0;
            break;
        }
        case ParentingAction::PutToSleep:
            urgency=child.needs.sleep;
            break;
        case ParentingAction::Bathe:
            urgency=child.needs.hygiene;
            break;
        case ParentingAction::ToiletAssist:
            urgency=child.needs.bladder;
            break;
        case ParentingAction::Hold:
            urgency=0.55*(1.0-child.development.attachment)+
                    0.25*child.development.stress+
                    0.20*child.emotion.anxiety;
            break;
        case ParentingAction::Play:
            urgency=0.36*(1.0-child.development.socialSkill)+
                    0.28*(1.0-child.development.confidence)+
                    0.20*(1.0-child.development.attachment)+
                    0.16*clampDevelopment(child.personality.curiosity);
            break;
        case ParentingAction::Educate:
            urgency=0.48*(1.0-child.development.learningSupport)+
                    0.22*clampDevelopment(child.personality.curiosity)+
                    0.15*(1.0-child.development.confidence)+
                    0.15*lifeStageProfile(child.lifeStage).skillLearningRate/1.45;
            break;
        case ParentingAction::Discipline:
            urgency=0.48*(1.0-child.development.disciplineInternalization)+
                    0.32*clampDevelopment(child.personality.impulsiveness)+
                    0.20*(1.0-clampDevelopment(caregiver.personality.patience));
            break;
        case ParentingAction::Comfort:
            urgency=0.46*child.development.stress+
                    0.20*child.emotion.sadness+
                    0.18*child.emotion.anxiety+
                    0.16*child.emotion.fear;
            break;
        case ParentingAction::HealthCare:
            urgency=0.78*(1.0-child.development.health)+0.22*child.development.stress;
            break;
    }

    const double resourceNeed=action==ParentingAction::HealthCare ? resources : 1.0;
    return clampDevelopment((0.72*clampDevelopment(urgency)+0.18*caregiverCapacity+0.10*bond)*resourceNeed);
}

inline ParentingDecision chooseParentingAction(
    const Character& caregiver,
    const Character& child,
    const Relationship& caregiverToChild,
    const ParentingContext& context)
{
    ParentingDecision best;
    best.utility=-std::numeric_limits<double>::infinity();

    constexpr std::array<ParentingAction,10> actions={
        ParentingAction::Feed,
        ParentingAction::PutToSleep,
        ParentingAction::Bathe,
        ParentingAction::ToiletAssist,
        ParentingAction::Hold,
        ParentingAction::Play,
        ParentingAction::Educate,
        ParentingAction::Discipline,
        ParentingAction::Comfort,
        ParentingAction::HealthCare
    };

    for(ParentingAction action:actions){
        const double score=parentingUtility(action,caregiver,child,caregiverToChild,context);
        if(score>best.utility){
            best.action=action;
            best.utility=score;
            best.valid=score>=0.0;
        }
    }
    return best;
}

inline void applyPositiveCareBond(
    Relationship& caregiverToChild,
    Relationship& childToCaregiver,
    double intensity)
{
    caregiverToChild.apply(relationshipDeltaFor(RelationshipEvent::Helped,intensity));
    childToCaregiver.apply(relationshipDeltaFor(RelationshipEvent::Comforted,intensity));
}

inline ParentingResult applyParentingAction(
    Character& caregiver,
    Character& child,
    Relationship& caregiverToChild,
    Relationship& childToCaregiver,
    ParentingAction action,
    const ParentingContext& context,
    bool authorizedFallbackCaregiver=false)
{
    if(caregiver.id==0 || child.id==0 || caregiver.id==child.id) return ParentingResult::Invalid;
    if(!caregiver.alive || !child.alive) return ParentingResult::Invalid;
    if(!isParentOf(caregiver,child) && !authorizedFallbackCaregiver)
        return ParentingResult::NotParent;
    if(!parentingActionAllowed(action,child.lifeStage)) return ParentingResult::NotAllowedForStage;
    if(action==ParentingAction::Feed && !context.foodAvailable && !context.waterAvailable)
        return ParentingResult::UnavailableResources;

    const double warmth=clampDevelopment(context.warmth);
    const double consistency=clampDevelopment(context.consistency);
    const double harshness=clampDevelopment(context.harshness);
    const double careQuality=clampDevelopment(
        0.42*warmth+0.26*consistency+
        0.18*clampDevelopment(caregiver.personality.empathy)+
        0.14*clampDevelopment(caregiver.personality.patience)-
        0.30*harshness);

    switch(action){
        case ParentingAction::Feed:
            // Production consumes provisions only when the corresponding need
            // is materially non-zero. Mirror that threshold here so care can
            // never create a Need effect without consuming the matching item.
            child.needs.apply({
                context.foodAvailable && child.needs.hunger>0.05 ? -0.58 : 0.0,
                context.waterAvailable && child.needs.thirst>0.05 ? -0.42 : 0.0,
                0.0,0.0,0.0});
            child.development.attachment+=0.018*careQuality;
            child.development.emotionalSecurity+=0.012*careQuality;
            applyPositiveCareBond(caregiverToChild,childToCaregiver,0.35*careQuality);
            break;
        case ParentingAction::PutToSleep:
            child.needs.apply({0.0,0.0,-0.68,0.0,0.0});
            child.development.attachment+=0.018*careQuality;
            child.development.stress-=0.030*careQuality;
            applyPositiveCareBond(caregiverToChild,childToCaregiver,0.30*careQuality);
            break;
        case ParentingAction::Bathe:
            child.needs.apply({0.0,0.0,0.0,0.0,-0.72});
            child.development.attachment+=0.010*careQuality;
            applyPositiveCareBond(caregiverToChild,childToCaregiver,0.22*careQuality);
            break;
        case ParentingAction::ToiletAssist:
            child.needs.apply({0.0,0.0,0.0,-0.72,child.lifeStage==LifeStage::Baby ? 0.018 : 0.008});
            child.development.attachment+=0.012*careQuality;
            child.development.stress-=0.025*careQuality;
            applyPositiveCareBond(caregiverToChild,childToCaregiver,0.24*careQuality);
            break;
        case ParentingAction::Hold:
            child.development.attachment+=0.060*careQuality;
            child.development.emotionalSecurity+=0.055*careQuality;
            child.development.stress-=0.100*careQuality;
            applyEmotionEvent(child.emotion,EmotionEventType::Comfort,0.80*careQuality);
            applyPositiveCareBond(caregiverToChild,childToCaregiver,0.60*careQuality);
            break;
        case ParentingAction::Play:
            child.development.attachment+=0.035*careQuality;
            child.development.confidence+=0.030*careQuality;
            child.development.socialSkill+=0.040*careQuality;
            child.development.stress-=0.045*careQuality;
            child.personality.sociability=clampDevelopment(child.personality.sociability+0.006*careQuality);
            child.personality.curiosity=clampDevelopment(child.personality.curiosity+0.005*careQuality);
            applyEmotionEvent(child.emotion,EmotionEventType::PositiveSocial,0.65*careQuality);
            caregiverToChild.apply(relationshipDeltaFor(RelationshipEvent::SharedPositiveExperience,0.45*careQuality));
            childToCaregiver.apply(relationshipDeltaFor(RelationshipEvent::SharedPositiveExperience,0.55*careQuality));
            break;
        case ParentingAction::Educate:
            child.development.learningSupport+=0.050*careQuality;
            child.development.confidence+=0.024*careQuality;
            child.development.socialSkill+=0.012*careQuality;
            child.personality.curiosity=clampDevelopment(child.personality.curiosity+0.008*careQuality);
            child.personality.conscientiousness=clampDevelopment(child.personality.conscientiousness+0.006*careQuality);
            applyEmotionEvent(child.emotion,EmotionEventType::Success,0.35*careQuality);
            applyPositiveCareBond(caregiverToChild,childToCaregiver,0.30*careQuality);
            break;
        case ParentingAction::Discipline: {
            const double supportive=clampDevelopment(consistency*warmth*(1.0-harshness));
            child.development.disciplineInternalization+=0.060*supportive;
            child.development.stress+=0.080*harshness-0.020*supportive;
            child.development.confidence-=0.045*harshness;
            child.development.emotionalSecurity-=0.040*harshness;
            child.personality.conscientiousness=clampDevelopment(
                child.personality.conscientiousness+0.008*supportive-0.003*harshness);
            child.personality.emotionalStability=clampDevelopment(
                child.personality.emotionalStability+0.004*supportive-0.010*harshness);
            if(harshness>0.55){
                childToCaregiver.apply(relationshipDeltaFor(RelationshipEvent::Conflict,0.45*harshness));
                applyEmotionEvent(child.emotion,EmotionEventType::Conflict,0.45*harshness);
            }else{
                childToCaregiver.apply(relationshipDeltaFor(RelationshipEvent::Helped,0.24*supportive));
            }
            caregiverToChild.familiarity=clampRelationship(caregiverToChild.familiarity+0.010);
            break;
        }
        case ParentingAction::Comfort:
            child.development.attachment+=0.045*careQuality;
            child.development.emotionalSecurity+=0.060*careQuality;
            child.development.stress-=0.160*careQuality;
            child.development.confidence+=0.018*careQuality;
            applyEmotionEvent(child.emotion,EmotionEventType::Comfort,0.95*careQuality);
            caregiverToChild.apply(relationshipDeltaFor(RelationshipEvent::Comforted,0.55*careQuality));
            childToCaregiver.apply(relationshipDeltaFor(RelationshipEvent::Comforted,0.70*careQuality));
            break;
        case ParentingAction::HealthCare:
            child.development.health+=0.180*clampDevelopment(context.resources)*careQuality;
            child.development.stress-=0.040*careQuality;
            child.development.emotionalSecurity+=0.020*careQuality;
            applyPositiveCareBond(caregiverToChild,childToCaregiver,0.38*careQuality);
            break;
    }

    child.development.normalize();
    caregiver.needs.apply({0.0,0.0,0.018,0.008,0.005});
    caregiver.development.stress=clampDevelopment(caregiver.development.stress+0.006);
    applyEmotionEvent(caregiver.emotion,EmotionEventType::PositiveSocial,0.18*careQuality);
    return ParentingResult::Performed;
}

} // namespace lifelens
