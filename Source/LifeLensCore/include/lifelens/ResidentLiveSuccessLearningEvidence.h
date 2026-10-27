#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "ResidentSensedResourceAgency.h"

namespace lifelens {

// Strictly read-only: describes a DIFFERENCE between the same resident's
// executable Core source decision WITH and WITHOUT a real firsthand success
// memory. This is not a model plan, command, harvest receipt or an invented
// alternative; callers may publish it only AFTER Core issues the matching
// pending Gather action token.
struct ResidentLiveSuccessLearningEvidence {
    CharacterId actor=0;
    ResourceNodeId originalSource=0;
    ResourceNodeId learnedSource=0;
    MaterialKind material=MaterialKind::Unknown;
    int firsthandSuccessMinute=-1;
    int choiceMinute=-1;
};

inline std::optional<ResidentLiveSuccessLearningEvidence>
observeResidentLiveSuccessLearningEvidence(
    const World& world,const Character& resident,GridPos position,
    const ResidentAgencyChoice& original,
    const ResidentSensedResourceGoalChoice& result,
    const SettlementPopulation* population=nullptr)
{
    const auto& before=original.decision;
    const auto& after=result.agency.decision;
    if(world.minute<0 ||resident.id==0 ||!resident.alive
       ||before.kind!=UnifiedDecisionKind::Civilization
       ||after.kind!=UnifiedDecisionKind::Civilization
       ||before.civilization.intent!=CivilizationIntent::Gather
       ||after.civilization.intent!=CivilizationIntent::Gather
       ||before.civilization.resourceNode==0
       ||after.civilization.resourceNode==0
       ||before.civilization.resourceNode==after.civilization.resourceNode
       ||before.civilization.material==MaterialKind::Unknown
       ||before.civilization.material!=after.civilization.material
       ||(original.authority==ResidentAgencyAuthority::ResidentCognition
          &&(result.agency.authority!=original.authority
             ||result.agency.contextFingerprint!=original.contextFingerprint
             ||result.agency.cognitiveIntent!=original.cognitiveIntent)))
        return std::nullopt;

    const auto local=observeLocalResourceProximity(
        world,resident.id,position);
    if(local.actor!=resident.id) return std::nullopt;
    std::vector<ResourceNodeId> allowed;
    allowed.reserve(local.nearbyResources.size());
    for(const auto& fact:local.nearbyResources)
        if(fact.nodeId!=0
           &&fact.material==before.civilization.material)
            allowed.push_back(fact.nodeId);
    if(allowed.size()<2
       ||std::find(allowed.begin(),allowed.end(),
                    before.civilization.resourceNode)==allowed.end()
       ||std::find(allowed.begin(),allowed.end(),
                    after.civilization.resourceNode)==allowed.end())
        return std::nullopt;

    const auto index=buildResidentResourceOutcomeIndex(resident,world.minute);
    const std::string key=std::string("Civilization:Gather:")
        +materialName(after.civilization.material);
    const auto success=index.find(after.civilization.resourceNode,key);
    const auto priorSource=index.find(before.civilization.resourceNode,key);
    if(!success.hasEvidence ||!success.succeeded
       ||success.reason!=ResidentResourceOutcomeReason::Success
       ||success.minute<0
       ||(priorSource.hasEvidence &&!priorSource.succeeded
          &&priorSource.reason
             ==ResidentResourceOutcomeReason::AccessUnavailable))
        return std::nullopt;

    // Recompute exactly the competing live Core gather candidates. If the
    // same source wins WITHOUT the memory, do not mislabel a native choice
    // as a learned change. No world, RNG or resident mind is mutated.
    const auto unlearned=bestGatherDecisionAtPosition(
        world,resident,position,population,after.civilization.material,
        nullptr,nullptr,&allowed);
    const auto learned=bestGatherDecisionAtPosition(
        world,resident,position,population,after.civilization.material,
        nullptr,&index,&allowed);
    if(unlearned.resourceNode!=before.civilization.resourceNode
       ||learned.resourceNode!=after.civilization.resourceNode
       ||learned.intent!=CivilizationIntent::Gather
       ||!std::isfinite(learned.utility) ||learned.utility<0.14)
        return std::nullopt;

    const std::string where="resource-node:"
        +std::to_string(after.civilization.resourceNode);
    const std::string what="실제 문명 행동 "+key+" 수행에 성공했다";
    bool trueDirectWitness=false;
    for(const auto& memory:resident.memory.entries){
        if(memory.minute==success.minute
           &&memory.who==resident.id
           &&memory.sourceCharacter==resident.id
           &&memory.source==MemorySource::DirectWitness
           &&memory.witnessed
           &&memory.where==where
           &&memory.what==what
           &&memory.tags.size()==3
           &&memory.tags[0]=="action_experience"
           &&memory.tags[1]==key
           &&memory.tags[2]=="success"){
            trueDirectWitness=true;
            break;
        }
    }
    if(!trueDirectWitness) return std::nullopt;

    GridPos access{};
    std::vector<GridPos> route;
    if(!resolveCivilizationResourceAccessGridPosition(
           world,after.civilization.resourceNode,access)
       ||!buildCoreGroundRoute(world,position,access,1,route))
        return std::nullopt;
    return ResidentLiveSuccessLearningEvidence{
        resident.id,before.civilization.resourceNode,
        after.civilization.resourceNode,after.civilization.material,
        success.minute,world.minute
    };
}

} // namespace lifelens
