#pragma once

#include <optional>
#include <vector>

#include "ResidentSensedResourceAgency.h"

namespace lifelens {

// Read-only audit of an ACTUAL Core decision-slot substitution, never a
// prediction, a new agency, or a fabricated Gather outcome. The caller is
// responsible for logging this only AFTER issuing the matching Core pending
// action. It remains deliberately unsaved and cannot grant any action.
struct ResidentLiveRecoveryEvidence {
    CharacterId actor=0;
    ResourceNodeId failedSource=0;
    ResourceNodeId selectedSource=0;
    MaterialKind material=MaterialKind::Unknown;
    int firsthandFailureMinute=-1;
    std::uint64_t failedPendingToken=0;
    int selectionMinute=-1;
    bool directNavigationFailure=false;
};

inline std::optional<ResidentLiveRecoveryEvidence>
observeResidentLiveRecoveryEvidence(
    const World& world,
    const Character& actor,
    GridPos authoritativePosition,
    const ResidentAgencyChoice& before,
    const ResidentSensedResourceGoalChoice& after)
{
    const auto& original=before.decision;
    const auto& selected=after.agency.decision;
    if(world.minute<0 || !actor.alive || actor.id==0
       || original.kind!=UnifiedDecisionKind::Civilization
       || selected.kind!=UnifiedDecisionKind::Civilization
       || original.civilization.intent!=CivilizationIntent::Gather
       || selected.civilization.intent!=CivilizationIntent::Gather
       || original.civilization.resourceNode==0
       || selected.civilization.resourceNode==0
       || selected.civilization.resourceNode==original.civilization.resourceNode
       || original.civilization.material==MaterialKind::Unknown
       || selected.civilization.material!=original.civilization.material
       || (before.authority==ResidentAgencyAuthority::ResidentCognition
           &&after.agency.authority!=ResidentAgencyAuthority::ResidentCognition)
       || (before.authority==ResidentAgencyAuthority::ResidentCognition
           &&(after.agency.contextFingerprint!=before.contextFingerprint
              ||after.agency.cognitiveIntent!=before.cognitiveIntent)))
        return std::nullopt;

    const std::string key=std::string("Civilization:Gather:")
        +materialName(original.civilization.material);
    const auto evidence=buildResidentResourceOutcomeIndex(actor,world.minute)
        .find(original.civilization.resourceNode,key);
    if(!evidence.hasEvidence ||evidence.succeeded
       ||evidence.reason!=ResidentResourceOutcomeReason::AccessUnavailable
       ||evidence.minute<0 ||evidence.minute>world.minute)
        return std::nullopt;

    // Do not call a model-proposed coordinate "reachable". The SAME Core
    // ground/pathing authority that executes pending movement must accept it.
    const ResourceNode* node=findCivilizationResourceNodeSpatial(
        world,selected.civilization.resourceNode);
    if(node==nullptr ||node->material!=selected.civilization.material
       ||node->quantity<=0)
        return std::nullopt;
    GridPos access{};
    std::vector<GridPos> route;
    if(!resolveCivilizationResourceAccessGridPosition(
           world,selected.civilization.resourceNode,access)
       ||!buildCoreGroundRoute(
           world,authoritativePosition,access,1,route))
        return std::nullopt;

    ResidentLiveRecoveryEvidence result;
    result.actor=actor.id;
    result.failedSource=original.civilization.resourceNode;
    result.selectedSource=selected.civilization.resourceNode;
    result.material=selected.civilization.material;
    result.firsthandFailureMinute=evidence.minute;
    result.selectionMinute=world.minute;
    // A failed pre-extraction Core movement is NOT an extraction attempt.
    // Preserve that distinction in observational evidence.
    for(const auto& memory:actor.memory.entries)
        if(memory.minute==evidence.minute
           &&memory.source==MemorySource::DirectWitness
           &&memory.witnessed &&memory.sourceCharacter==actor.id
           &&memory.who==actor.id
           &&memory.where=="resource-node:"
                +std::to_string(result.failedSource)
           &&(memory.tags.size()==3 ||memory.tags.size()==4)
           &&memory.tags[0]=="navigation_experience"
           &&memory.tags[1]==key
           &&memory.tags[2]=="route-failed"
           &&memory.what=="Core 이동 경로 실패로 자원 채집을 시작하지 못했다"){
            result.directNavigationFailure=true;
            result.failedPendingToken=
                residentGatherRouteFailurePendingToken(memory);
        }
    return result;
}

} // namespace lifelens
