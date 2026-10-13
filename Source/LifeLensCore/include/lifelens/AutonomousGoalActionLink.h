#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include "SocialUtility.h"

namespace lifelens {

// COG B4c-2: The resident's own trust goal is linked to a *real* Core
// pending-social-action token. There can be at most one active link per
// PendingContextAction. Nothing here authorizes additional actions.
// This is stored in the optional v8 snapshot tail, not packed into an
// unrelated civilization field or a fabricated memory.
struct AutonomousGoalActionLink {
    std::uint64_t token=0;
    CharacterId actor=0;
    CharacterId target=0;
    SocialIntent intent=SocialIntent::None;
    int issuedMinute=-1;
    std::uint64_t contextFingerprint=0;
    double trustThreshold=0.0;
    double trustAtIssue=0.0;

    bool active() const { return token!=0; }
    void clear() { *this=AutonomousGoalActionLink{}; }
};

inline bool validAutonomousGoalActionLink(const AutonomousGoalActionLink& link)
{
    return link.token!=0 &&link.actor!=0 &&link.target!=0
        &&link.actor!=link.target
        &&(link.intent==SocialIntent::Approach
           ||link.intent==SocialIntent::Repair)
        &&link.issuedMinute>=0
        &&link.contextFingerprint!=0
        &&std::isfinite(link.trustThreshold)
        &&std::isfinite(link.trustAtIssue)
        &&link.trustThreshold>link.trustAtIssue
        &&link.trustThreshold<=1.0
        &&link.trustAtIssue>=0.0;
}

enum class AutonomousGoalActionOutcome {
    Unverifiable,
    AlreadyReachedBeforeExecution,
    AchievedDuringExecution,
    ProgressButNotAchieved,
    NotAchievedNoProgress
};

inline const char* autonomousGoalActionOutcomeName(
    AutonomousGoalActionOutcome outcome)
{
    switch(outcome){
        case AutonomousGoalActionOutcome::AlreadyReachedBeforeExecution:
            return "already-reached-before-action";
        case AutonomousGoalActionOutcome::AchievedDuringExecution:
            return "achieved-during-action";
        case AutonomousGoalActionOutcome::ProgressButNotAchieved:
            return "progress-but-not-achieved";
        case AutonomousGoalActionOutcome::NotAchievedNoProgress:
            return "not-achieved-no-progress";
        default: return "unverifiable";
    }
}

// Compare a resident's OWN directional trust immediately BEFORE and AFTER
// the confirmed Core action. Never credit changes that occurred while
// traveling, never inspect the recipient's private trust as actor evidence.
// A completed social action can leave the original goal unachieved.
inline AutonomousGoalActionOutcome classifyAutonomousGoalActionOutcome(
    const AutonomousGoalActionLink& link,
    std::optional<double> before,
    std::optional<double> after)
{
    if(!validAutonomousGoalActionLink(link)
       ||!before ||!after
       ||!std::isfinite(*before) ||!std::isfinite(*after)
       ||*before<0.0 ||*before>1.0
       ||*after<0.0 ||*after>1.0)
        return AutonomousGoalActionOutcome::Unverifiable;
    if(*before>=link.trustThreshold)
        return AutonomousGoalActionOutcome::AlreadyReachedBeforeExecution;
    if(*after>=link.trustThreshold)
        return AutonomousGoalActionOutcome::AchievedDuringExecution;
    if(*after>*before+1e-12)
        return AutonomousGoalActionOutcome::ProgressButNotAchieved;
    return AutonomousGoalActionOutcome::NotAchievedNoProgress;
}

inline std::string autonomousGoalProgressProposition(SocialIntent intent)
{
    return std::string("experienced-self-trust-goal-progress:")
        +socialIntentName(intent);
}

// The effect modifies future CLOSE social choices only; no survival need,
// actor status, trust, inventory, destination or social opportunity is altered.
inline double learnedAutonomousGoalProgressDisposition(
    const Character& actor,
    CharacterId target,
    SocialIntent intent)
{
    if(target==0 ||target==actor.id
       ||(intent!=SocialIntent::Approach
          &&intent!=SocialIntent::Repair)) return 0.0;
    const BeliefRecord* evidence=actor.beliefs.find(
        target,autonomousGoalProgressProposition(intent));
    return evidence ? std::clamp(
        evidence->stance*evidence->confidence,-1.0,1.0) : 0.0;
}

// Requires a completed Core execution and an exact link matching the
// executed token. The personal belief is about the actor's OWN experienced
// trust progress, not an inference about another resident's inner state.
inline bool rememberAutonomousGoalActionOutcome(
    Character& actor,
    const AutonomousGoalActionLink& link,
    std::uint64_t completedToken,
    int minute,
    AutonomousGoalActionOutcome outcome)
{
    if(!validAutonomousGoalActionLink(link)
       ||completedToken!=link.token
       ||actor.id!=link.actor
       ||!actor.alive
       ||minute<link.issuedMinute
       ||outcome==AutonomousGoalActionOutcome::Unverifiable)
        return false;

    const bool measurable=
        outcome!=AutonomousGoalActionOutcome::AlreadyReachedBeforeExecution;
    const bool progressed=
        outcome==AutonomousGoalActionOutcome::AchievedDuringExecution
        ||outcome==AutonomousGoalActionOutcome::ProgressButNotAchieved;

    // No retrospective credit if the objective was already met BEFORE
    // the action; it may have changed while the resident was traveling.
    if(measurable){
        BeliefRecord& belief=actor.beliefs.getOrCreate(
            link.target,autonomousGoalProgressProposition(link.intent));
        if(belief.lastUpdatedMinute==minute
           &&belief.supportCount+belief.contradictionCount>0)
            return false; // no double reinforcement per minute
        if(belief.supportWeight+belief.contradictionWeight>=12.0){
            belief.supportWeight*=0.80;
            belief.contradictionWeight*=0.80;
        }
        belief.applyEvidence(progressed,1.0,minute);
    }

    const std::string action=socialIntentName(link.intent);
    // One latest firsthand recollection per partner/method, replacing
    // old outcomes rather than expanding memory once per simulation minute.
    for(MemoryRecord& memory:actor.memory.entries){
        if(memory.who!=link.target ||memory.tags.size()!=3
           ||memory.tags[0]!="goal_action_feedback"
           ||memory.tags[1]!="self_trust"
           ||memory.tags[2]!=action) continue;
        memory.what=autonomousGoalActionOutcomeName(outcome);
        memory.minute=minute;
        memory.confidence=1.0;
        memory.importance=progressed ? 0.40 : 0.52;
        memory.emotionValence=actor.emotion.valence;
        memory.emotionIntensity=actor.emotion.intensity();
        memory.source=MemorySource::DirectWitness;
        memory.witnessed=true;
        return true;
    }

    MemoryRecord memory;
    memory.who=link.target;
    memory.sourceCharacter=actor.id;
    memory.what=autonomousGoalActionOutcomeName(outcome);
    memory.where="personal goal outcome";
    memory.minute=minute;
    memory.emotionValence=actor.emotion.valence;
    memory.emotionIntensity=actor.emotion.intensity();
    memory.importance=progressed ? 0.40 : 0.52;
    memory.confidence=1.0;
    memory.witnessed=true;
    memory.source=MemorySource::DirectWitness;
    memory.tags={"goal_action_feedback","self_trust",action};
    actor.memory.add(std::move(memory));
    return true;
}

} // namespace lifelens
