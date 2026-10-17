#pragma once

#include <algorithm>
#include <string>
#include "ActionExperienceLearning.h"
#include "CivilizationDecision.h"

namespace lifelens {

// The label identifies ONLY a real Core civilization action, not a model's
// claimed plan, an invented material, or an assumed completed objective.
// All components are closed Core enums; user/model prose is never persisted.
inline std::string civilizationActionExperienceKey(
    const CivilizationUtilityDecision& decision)
{
    std::string label="Civilization:";
    label+=civilizationIntentName(decision.intent);
    if(decision.intent==CivilizationIntent::None) return {};
    if(decision.intent==CivilizationIntent::Gather
       ||decision.intent==CivilizationIntent::Retrieve
       ||decision.intent==CivilizationIntent::Store
       ||decision.intent==CivilizationIntent::Explore){
        label+=':';
        label+=materialName(decision.material);
    }else if(decision.intent==CivilizationIntent::Craft
             ||decision.intent==CivilizationIntent::Experiment){
        label+=':';
        // Experiments store their identity as ExperimentKind, not always as
        // decision.technique. Avoid collapsing every failed discovery attempt
        // into the same misleading "Experiment:None" belief.
        const TechniqueId learnedTechnique=
            decision.intent==CivilizationIntent::Experiment
                ?experimentTechnique(decision.experiment)
                :decision.technique;
        label+=techniqueName(learnedTechnique);
        if(decision.facilityAction!=FacilityBuildAction::None){
            label+=':';
            label+=facilityBuildActionName(decision.facilityAction);
        }
    }
    return label;
}

// Diagnosed strictly from the authoritative world AFTER a real gather
// attempt failed. Never classify a pending proposal, an unseen resource,
// the model's rationale, or an unrelated resident's private experience.
enum class ResidentGatherFailureCause {
    Unknown,
    SourceRemoved,
    SourceExhausted,
    ContainerMissing,
    ResourceAccessUnavailable
};

inline const char* residentGatherFailureCauseName(
    ResidentGatherFailureCause cause)
{
    switch(cause){
        case ResidentGatherFailureCause::SourceRemoved: return "source-removed";
        case ResidentGatherFailureCause::SourceExhausted: return "source-exhausted";
        case ResidentGatherFailureCause::ContainerMissing: return "container-missing";
        case ResidentGatherFailureCause::ResourceAccessUnavailable:
            return "access-unavailable";
        default: return "unspecified";
    }
}

inline ResidentGatherFailureCause diagnoseResidentGatherFailure(
    const World& world,
    const Character& actor,
    const CivilizationUtilityDecision& decision,
    GridPos position)
{
    if(!actor.alive ||actor.id==0
       ||decision.intent!=CivilizationIntent::Gather
       ||decision.resourceNode==0) return ResidentGatherFailureCause::Unknown;
    const auto nodeIt=std::find_if(
        world.resourceNodes.begin(),world.resourceNodes.end(),
        [&](const ResourceNode& node){
            return node.id==decision.resourceNode;
        });
    if(nodeIt==world.resourceNodes.end())
        return ResidentGatherFailureCause::SourceRemoved;
    const ResourceNode* node=&*nodeIt;
    if(node->material!=decision.material)
        return ResidentGatherFailureCause::Unknown;
    if(node->quantity<=0) return ResidentGatherFailureCause::SourceExhausted;
    if(node->material==MaterialKind::Water
       &&emptySimpleContainerCount(actor.civilization.inventory)<=0)
        return ResidentGatherFailureCause::ContainerMissing;
    GridPos target{};
    if(!resolveCivilizationResourceAccessGridPosition(
            world,decision.resourceNode,target)
       ||!civilizationExecutionNearTarget(position,target))
        return ResidentGatherFailureCause::ResourceAccessUnavailable;
    return ResidentGatherFailureCause::Unknown;
}

// Called only at the authoritative execution attempt site, AFTER Core has
// actually tried the selected action and observed whether it succeeded.
// A submitted/accepted AI thought, a planned pending token, or browser text
// can NEVER generate this evidence.
inline void rememberCivilizationAttemptOutcome(
    Character& actor,
    const CivilizationUtilityDecision& decision,
    int minute,
    bool succeeded,
    ResidentGatherFailureCause cause=ResidentGatherFailureCause::Unknown)
{
    if(actor.id==0 || !actor.alive ||minute<0) return;
    const std::string key=civilizationActionExperienceKey(decision);
    if(key.empty()) return;
    rememberActionOutcome(actor,key.c_str(),minute,succeeded);

    // Existing snapshot-backed memory/belief storage is the single truth.
    // Make the latest direct evidence legible to the next local-model
    // CognitiveRequest without pretending the action achieved a life goal.
    const std::string outcome=succeeded ? "success" : "failure";
    for(auto& memory:actor.memory.entries){
        if(memory.tags.size()!=3
           ||memory.tags[0]!="action_experience"
           ||memory.tags[1]!=key
           ||memory.tags[2]!=outcome
           ||memory.minute!=minute) continue;
        memory.what="실제 문명 행동 "+key
            +(succeeded ? " 수행에 성공했다" : " 수행을 시도했지만 실패했다");
        if(!succeeded &&cause!=ResidentGatherFailureCause::Unknown
           &&decision.intent==CivilizationIntent::Gather)
            memory.what+=" (직접 확인한 원인: "+
                std::string(residentGatherFailureCauseName(cause))+")";
        memory.where="내가 직접 시도한 행동";
        memory.sourceCharacter=actor.id;
        return;
    }
}

} // namespace lifelens
