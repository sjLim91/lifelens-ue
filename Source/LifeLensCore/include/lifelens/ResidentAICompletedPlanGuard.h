#pragma once

#include "ResidentCivilizationOutcomeLearning.h"
#include "CognitiveDecisionJournal.h"
#include "ResidentResourceOutcomePreference.h"

namespace lifelens {

// Do not replay a model's OLD preference after the resident already
// completed/attempted the selected Core action. That outcome belongs in the
// NEXT model context, not in an automatic 15-minute repetition loop.
//
// The action completion must be firsthand and Core-recorded: the source
// actor is this resident, the timestamp is later than the thought, and the
// exact closed-enum action key matches the candidate. No extra snapshot
// fields or model text are trusted. A later independent model decision has
// a newer minute and is free to choose the same action again.
inline bool residentAIPlanAlreadyAttempted(
    const Character& actor,
    const AcceptedCognitiveDecision& thought,
    const CivilizationUtilityDecision& candidate,
    int nowMinute,
    bool verifiedMissingRecipeIngredient=false)
{
    if(actor.id==0 ||thought.actor!=actor.id
       ||thought.minute<0 ||nowMinute<thought.minute
       ||candidate.intent==CivilizationIntent::None)
        return false;
    const std::string key=civilizationActionExperienceKey(candidate);
    if(key.empty()) return false;
    for(const MemoryRecord& entry:actor.memory.entries){
        if(entry.source!=MemorySource::DirectWitness
           ||!entry.witnessed ||entry.sourceCharacter!=actor.id
           ||entry.who!=actor.id
           ||entry.minute<=thought.minute
           ||entry.minute>nowMinute ||entry.tags.size()!=3
           ||entry.tags[0]!="action_experience"
           ||entry.tags[1]!=key)
            continue;
        // Only an executed Core action consumes the resident's plan.
        // A plausible tag or claimed witnessed flag cannot make an invented
        // failure block every subsequent physical affordance for 15 minutes.
        const std::string prefix="실제 문명 행동 "+key;
        const std::string failed=prefix+" 수행을 시도했지만 실패했다";
        if(entry.tags[2]=="success"){
            if(entry.what!=prefix+" 수행에 성공했다") continue;
        }else if(entry.tags[2]=="failure"){
            if(entry.what!=failed
               &&entry.what!=failed+" (직접 확인한 원인: source-removed)"
               &&entry.what!=failed+" (직접 확인한 원인: source-exhausted)"
               &&entry.what!=failed+" (직접 확인한 원인: container-missing)"
               &&entry.what!=failed+" (직접 확인한 원인: access-unavailable)")
                continue;
        }else continue;
        if(candidate.intent!=CivilizationIntent::Gather
           &&entry.where!="내가 직접 시도한 행동") continue;
        if(candidate.intent==CivilizationIntent::Gather
           &&candidate.resourceNode!=0){
            const std::string source="resource-node:"
                +std::to_string(candidate.resourceNode);
            // Do not trust an arbitrary where string as a legacy record.
            // Only Core's real legacy source-less form is conservative; an
            // attempted node A is never credited to node B.
            if(entry.where!=source
               &&entry.where!="내가 직접 시도한 행동") continue;
            if(entry.where==source){
                std::uint64_t parsed=0;
                if(!resident_resource_outcome_detail::parseSourceNode(
                    entry.where,parsed) ||parsed!=candidate.resourceNode)
                    continue;
            }
        }
        if(candidate.intent==CivilizationIntent::Gather
           &&candidate.resourceNode==0
           &&entry.where!="내가 직접 시도한 행동"){
            std::uint64_t node=0;
            if(!resident_resource_outcome_detail::parseSourceNode(
                entry.where,node)) continue;
        }
        // This exception applies ONLY to a real successful partial
        // prerequisite gather selected again from a still-unsatisfied Core
        // recipe. A failed attempt or a generic resource goal is NEVER
        // repeatedly retried under an old model thought.
        if(verifiedMissingRecipeIngredient
           &&thought.intent==CognitiveIntentKind::CraftUsefulTools
           &&candidate.intent==CivilizationIntent::Gather
           &&entry.tags[2]=="success")
            continue;
        return true;
    }
    return false;
}

} // namespace lifelens
