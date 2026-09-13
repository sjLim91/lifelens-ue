#include <iostream>

#include "lifelens/Parenting.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static Character makeParent(CharacterId id)
{
    Character c;
    c.id=id;
    c.name="Parent";
    c.lifeStage=LifeStage::Adult;
    c.personality.empathy=0.90;
    c.personality.patience=0.86;
    c.personality.conscientiousness=0.82;
    return c;
}

static Character makeChild(CharacterId id,CharacterId parentId,LifeStage stage)
{
    Character c;
    c.id=id;
    c.name="Child";
    c.lifeStage=stage;
    c.parentIds={parentId};
    c.development.attachment=0.35;
    c.development.confidence=0.40;
    c.development.socialSkill=0.20;
    c.development.emotionalSecurity=0.35;
    c.development.disciplineInternalization=0.10;
    c.development.learningSupport=0.15;
    c.development.health=1.0;
    c.personality.curiosity=0.65;
    c.personality.impulsiveness=0.55;
    return c;
}

static Relationship bond(CharacterId from,CharacterId to)
{
    Relationship r;
    r.from=from;
    r.to=to;
    r.affection=0.75;
    r.trust=0.70;
    r.comfort=0.65;
    r.familiarity=0.70;
    r.commitment=0.85;
    return r;
}

int main()
{
    Character parent=makeParent(1);
    Character baby=makeChild(2,parent.id,LifeStage::Baby);
    Relationship parentToBaby=bond(parent.id,baby.id);
    Relationship babyToParent=bond(baby.id,parent.id);
    ParentingContext context;

    // Immediate physical need should dominate the caregiving decision.
    baby.needs.hunger=0.95;
    baby.needs.thirst=0.70;
    baby.needs.sleep=0.10;
    baby.needs.hygiene=0.05;
    ParentingDecision hungryDecision=chooseParentingAction(parent,baby,parentToBaby,context);
    CHECK(hungryDecision.valid);
    CHECK(hungryDecision.action==ParentingAction::Feed);

    const double hungerBefore=baby.needs.hunger;
    const double attachmentBefore=baby.development.attachment;
    const double trustBefore=babyToParent.trust;
    CHECK(applyParentingAction(
        parent,baby,parentToBaby,babyToParent,ParentingAction::Feed,context)==ParentingResult::Performed);
    CHECK(baby.needs.hunger<hungerBefore);
    CHECK(baby.development.attachment>attachmentBefore);
    CHECK(babyToParent.trust>trustBefore);

    // Distress should cause comfort-seeking care and improve emotional security.
    baby.needs.hunger=0.05;
    baby.needs.thirst=0.05;
    baby.needs.sleep=0.05;
    baby.needs.hygiene=0.05;
    baby.development.stress=0.90;
    baby.emotion.sadness=0.85;
    baby.emotion.anxiety=0.75;
    baby.emotion.refreshSummary();
    ParentingDecision distressDecision=chooseParentingAction(parent,baby,parentToBaby,context);
    CHECK(distressDecision.valid);
    CHECK(distressDecision.action==ParentingAction::Comfort || distressDecision.action==ParentingAction::Hold);
    const double securityBefore=baby.development.emotionalSecurity;
    const double stressBefore=baby.development.stress;
    CHECK(applyParentingAction(
        parent,baby,parentToBaby,babyToParent,ParentingAction::Comfort,context)==ParentingResult::Performed);
    CHECK(baby.development.emotionalSecurity>securityBefore);
    CHECK(baby.development.stress<stressBefore);

    // Stage gates prevent age-inappropriate parenting actions.
    CHECK(!parentingActionAllowed(ParentingAction::Educate,LifeStage::Baby));
    CHECK(parentingActionAllowed(ParentingAction::Educate,LifeStage::Child));
    CHECK(!parentingActionAllowed(ParentingAction::Hold,LifeStage::Teen));
    CHECK(!parentingActionAllowed(ParentingAction::Feed,LifeStage::Adult));
    CHECK(applyParentingAction(
        parent,baby,parentToBaby,babyToParent,ParentingAction::Educate,context)==ParentingResult::NotAllowedForStage);

    // Supportive discipline builds internal regulation with limited stress.
    Character child=makeChild(3,parent.id,LifeStage::Child);
    Relationship parentToChild=bond(parent.id,child.id);
    Relationship childToParent=bond(child.id,parent.id);
    ParentingContext supportive=context;
    supportive.warmth=0.92;
    supportive.consistency=0.90;
    supportive.harshness=0.05;
    const double disciplineBefore=child.development.disciplineInternalization;
    const double childStressBefore=child.development.stress;
    CHECK(applyParentingAction(
        parent,child,parentToChild,childToParent,ParentingAction::Discipline,supportive)==ParentingResult::Performed);
    CHECK(child.development.disciplineInternalization>disciplineBefore);
    CHECK(child.development.stress<=childStressBefore+0.01);

    // Harsh discipline has a materially different developmental/social outcome.
    Character harshChild=makeChild(4,parent.id,LifeStage::Child);
    Relationship parentToHarsh=bond(parent.id,harshChild.id);
    Relationship harshToParent=bond(harshChild.id,parent.id);
    ParentingContext harsh=context;
    harsh.warmth=0.20;
    harsh.consistency=0.55;
    harsh.harshness=0.90;
    const double harshStressBefore=harshChild.development.stress;
    const double confidenceBefore=harshChild.development.confidence;
    const double conflictBefore=harshToParent.conflict;
    CHECK(applyParentingAction(
        parent,harshChild,parentToHarsh,harshToParent,ParentingAction::Discipline,harsh)==ParentingResult::Performed);
    CHECK(harshChild.development.stress>harshStressBefore);
    CHECK(harshChild.development.confidence<confidenceBefore);
    CHECK(harshToParent.conflict>conflictBefore);

    // Education contributes to both support state and gradual personality development.
    const double learningBefore=child.development.learningSupport;
    const double curiosityBefore=child.personality.curiosity;
    CHECK(applyParentingAction(
        parent,child,parentToChild,childToParent,ParentingAction::Educate,supportive)==ParentingResult::Performed);
    CHECK(child.development.learningSupport>learningBefore);
    CHECK(child.personality.curiosity>curiosityBefore);

    // Health care is a real action and resource availability affects its utility.
    child.development.health=0.35;
    const double healthUtilityFull=parentingUtility(
        ParentingAction::HealthCare,parent,child,parentToChild,supportive);
    ParentingContext scarce=supportive;
    scarce.resources=0.15;
    const double healthUtilityScarce=parentingUtility(
        ParentingAction::HealthCare,parent,child,parentToChild,scarce);
    CHECK(healthUtilityFull>healthUtilityScarce);
    const double healthBefore=child.development.health;
    CHECK(applyParentingAction(
        parent,child,parentToChild,childToParent,ParentingAction::HealthCare,supportive)==ParentingResult::Performed);
    CHECK(child.development.health>healthBefore);

    // Only an actual parent can apply parenting actions in this layer.
    Character stranger=makeParent(99);
    Relationship strangerToChild=bond(stranger.id,child.id);
    Relationship childToStranger=bond(child.id,stranger.id);
    CHECK(applyParentingAction(
        stranger,child,strangerToChild,childToStranger,ParentingAction::Play,context)==ParentingResult::NotParent);

    return 0;
}
