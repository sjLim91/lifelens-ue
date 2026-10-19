#pragma once

#include "ResidentCivilizationOutcomeLearning.h"
#include "CognitiveDecisionJournal.h"

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
    int nowMinute)
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
           ||entry.minute<=thought.minute
           ||entry.minute>nowMinute ||entry.tags.size()!=3
           ||entry.tags[0]!="action_experience"
           ||entry.tags[1]!=key)
            continue;
        if(entry.tags[2]!="success" &&entry.tags[2]!="failure")
            continue;
        if(candidate.intent==CivilizationIntent::Gather
           &&candidate.resourceNode!=0){
            const std::string source="resource-node:"
                +std::to_string(candidate.resourceNode);
            // An attempt at node A says nothing about actually attempting B.
            // Legacy recollections without a source stay conservatively
            // protected, rather than assigning an invented source ID.
            if(entry.where.rfind("resource-node:",0)==0
               &&entry.where!=source)
                continue;
        }
        return true;
    }
    return false;
}

} // namespace lifelens
