#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "lifelens/Simulation.h"
#include "lifelens/ResidentSensedResourceAgency.h"
#include "lifelens/ResidentResourceOutcomePreference.h"

using namespace lifelens;

// Zero fixture injection. An honest census of actual CHANCES for a resident
// to re-use a firsthand successful resource source, NOT evidence of learning
// merely because a favorable number can be calculated in a hypothetical slot.
// Only the production Core may issue/extract any real action.
struct OpportunityCounts {
    int activeSuccessMemoryWindows=0;
    int optionalCadenceWindows=0;
    int knownMaterialPairWindows=0;
    int unlearnedLearnedRankingFlips=0;
    int populationWeightedRankingFlips=0;
    int populationWeightedNativeGatherEligible=0;
    int subsequentSameMaterialCoreGathers=0;
    int issuedLearnedChoiceTokens=0;
    int completedLearnedChoiceTokens=0;
};

struct Window {
    MaterialKind material=MaterialKind::Unknown;
    bool optional=false;
    bool pair=false;
    bool rankingFlip=false;
    ResourceNodeId nativeSource=0;
    ResourceNodeId learnedSource=0;
};

static bool recentlyWitnessedSuccess(
    const Character& resident,int minute,
    MaterialKind& material,ResourceNodeId& source)
{
    int mostRecent=-1;
    material=MaterialKind::Unknown;
    source=0;
    for(const auto& memory:resident.memory.entries){
        if(memory.who!=resident.id
           ||memory.sourceCharacter!=resident.id
           ||memory.source!=MemorySource::DirectWitness
           ||!memory.witnessed
           ||memory.minute<0 ||memory.minute>minute
           ||minute-memory.minute>ResidentResourceOutcomeHorizonMinutes
           ||memory.tags.size()!=3
           ||memory.tags[0]!="action_experience"
           ||memory.tags[2]!="success"
           ||memory.tags[1].rfind("Civilization:Gather:",0)!=0
           ||memory.minute<mostRecent) continue;
        ResourceNodeId node=0;
        if(!resident_resource_outcome_detail::parseSourceNode(
               memory.where,node)) continue;
        // The source/material identity is an immutable Core enum on the
        // actual node, not text parsed from a model or inferred memory.
        // The caller checks the source against the current live World.
        mostRecent=memory.minute;
        source=node;
    }
    return source!=0;
}

static void auditSeed(WorldSeed seed)
{
    Simulation sim(seed);
    sim.setupNewGame();
    assert(sim.world().characters.size()==4);
    OpportunityCounts counts;

    constexpr int ObservedMinutes=14*1440;
    for(int tick=0;tick<ObservedMinutes;++tick){
        const int minute=sim.world().minute;
        std::map<CharacterId,Window> windows;
        if(minute%15==0){
            for(const auto& resident:sim.world().characters){
                MaterialKind ignoredMaterial=MaterialKind::Unknown;
                ResourceNodeId experiencedNode=0;
                if(!resident.alive
                   ||!recentlyWitnessedSuccess(
                       resident,minute,ignoredMaterial,experiencedNode))
                    continue;
                ++counts.activeSuccessMemoryWindows;
                const ResourceNode* witnessedSource=
                    findCivilizationResourceNodeSpatial(
                        sim.world(),experiencedNode);
                if(witnessedSource==nullptr) continue;

                Window w;
                w.material=witnessedSource->material;
                const auto before=sim.observePendingContextAction(resident.id);
                const bool nonUrgent=maximumResidentNeed(resident)<0.55;
                if(!before.active &&nonUrgent
                   &&lifeStageProfile(resident.lifeStage).canWork){
                    ++counts.optionalCadenceWindows;
                    w.optional=true;
                    GridPos position{};
                    if(sim.runtimePosition(resident.id,position)){
                        const auto local=sim.observeResidentLocalPerception(
                            resident.id,ResidentLocalPerceptionMaxRadius);
                        std::vector<ResourceNodeId> allowed;
                        for(const auto& fact:local.nearbyResources)
                            if(fact.material==w.material &&fact.nodeId!=0)
                                allowed.push_back(fact.nodeId);
                        if(allowed.size()>=2){
                            ++counts.knownMaterialPairWindows;
                            w.pair=true;
                            const auto history=buildResidentResourceOutcomeIndex(
                                resident,minute);
                            const auto raw=bestGatherDecisionAtPosition(
                                sim.world(),resident,position,nullptr,
                                w.material,nullptr,nullptr,&allowed);
                            const auto learned=bestGatherDecisionAtPosition(
                                sim.world(),resident,position,nullptr,
                                w.material,nullptr,&history,&allowed);
                            // Keep the original unweighted census for
                            // comparison; production uses population scores.
                            const auto live=sim.observeResidentCoreGatherLearningWindow(
                                resident.id,w.material);
                            const bool weightedFlip=live.localMaterialPair
                                &&live.unlearned.intent==CivilizationIntent::Gather
                                &&live.learned.intent==CivilizationIntent::Gather
                                &&live.unlearned.resourceNode!=live.learned.resourceNode;
                            const auto& unified=live.nativeDecision;
                            const bool nativeGather=
                                unified.kind==UnifiedDecisionKind::Civilization
                                &&unified.civilization.intent==CivilizationIntent::Gather;
                            const bool weightedNativeEligible=weightedFlip
                                &&nativeGather
                                &&unified.civilization.material==w.material
                                &&unified.civilization.resourceNode
                                    ==live.unlearned.resourceNode;
                            if(weightedFlip) ++counts.populationWeightedRankingFlips;
                            if(weightedNativeEligible)
                                ++counts.populationWeightedNativeGatherEligible;
                            if(raw.intent==CivilizationIntent::Gather
                               &&learned.intent==CivilizationIntent::Gather
                               &&raw.resourceNode!=learned.resourceNode){
                                ++counts.unlearnedLearnedRankingFlips;
                                w.rankingFlip=true;
                                w.nativeSource=raw.resourceNode;
                                w.learnedSource=learned.resourceNode;
                                // Production population-weighted fallback,
                                // not yet an accepted strategic model choice.
                                const auto snapshot=sim.captureSnapshot();
                                const auto running=snapshot.runtime.find(resident.id);
                                assert(running!=snapshot.runtime.end());
                                const auto& state=running->second;
                                std::cout
                                    <<"AL1_RANK_FLIP_GATE seed="<<seed
                                    <<" actor="<<resident.id
                                    <<" minute="<<minute
                                    <<" material="<<materialName(w.material)
                                    <<" prior_source="<<raw.resourceNode
                                    <<" learned_source="<<learned.resourceNode
                                    <<" remembered_source="<<experiencedNode
                                    <<" resident_need_max="<<maximumResidentNeed(resident)
                                    <<" native_slot_kind="<<static_cast<int>(unified.kind)
                                    <<" native_slot_intent="<<(
                                        unified.kind==UnifiedDecisionKind::Civilization
                                            ?static_cast<int>(unified.civilization.intent):-1)
                                    <<" native_slot_source="<<(nativeGather
                                        ?unified.civilization.resourceNode:0)
                                    <<" native_slot_material="<<(
                                        unified.kind==UnifiedDecisionKind::Civilization
                                            ?materialName(unified.civilization.material):"Unknown")
                                    <<" weighted_prior_source="<<live.unlearned.resourceNode
                                    <<" weighted_learned_source="<<live.learned.resourceNode
                                    <<" weighted_ranking_flip="<<weightedFlip
                                    <<" native_gather_learning_eligible="<<weightedNativeEligible
                                    <<" planning_penalty_active="<<(
                                        minute<state.penaltyUntilMinute)
                                    <<" plan_empty="<<state.plan.empty()
                                    <<" pending_active="<<state.pendingContext.active()
                                    <<" optional_gate_eligible="<<w.optional
                                    <<"\n";

                            }
                        }
                    }
                }
                windows.emplace(resident.id,w);
            }
        }
        sim.step();
        assert(sim.world().minute==minute+1);
        for(const auto& [id,w]:windows){
            if(!w.optional) continue;
            const auto pending=sim.observePendingContextAction(id);
            if(w.rankingFlip){
                std::cout
                    <<"AL1_RANK_FLIP_STEP seed="<<seed
                    <<" actor="<<id
                    <<" minute="<<minute
                    <<" prior_source="<<w.nativeSource
                    <<" learned_source="<<w.learnedSource
                    <<" pending_active="<<pending.active
                    <<" pending_kind="<<(pending.active
                        ?static_cast<int>(pending.kind):-1)
                    <<" pending_intent="<<(pending.active
                        ?static_cast<int>(pending.civilizationIntent):-1)
                    <<" pending_source="<<(pending.active
                        ?pending.resourceNode:0)
                    <<" learned_source_issued="<<(pending.active
                        &&pending.kind==ContextActionKind::Civilization
                        &&pending.civilizationIntent==CivilizationIntent::Gather
                        &&pending.resourceNode==w.learnedSource)
                    <<"\n";
            }
            if(pending.active &&pending.kind==ContextActionKind::Civilization
               &&pending.civilizationIntent==CivilizationIntent::Gather
               &&pending.material==w.material)
                ++counts.subsequentSameMaterialCoreGathers;
        }
    }

    for(const auto& log:sim.logs()){
        if(log.find("AL1_CORE_SUCCESS_MEMORY_CHOICE actor=")
              !=std::string::npos)
            ++counts.issuedLearnedChoiceTokens;
    }
    // Do not infer an execution from a token issuer alone. The separate
    // AL1_CORE_GATHER_RESULT source/token audit in the main ablation test
    // remains the authoritative per-token evidence.
    std::cout<<"AL1_NATURAL_LEARNING_OPPORTUNITY"
             <<" seed="<<seed
             <<" observed_days=14"
             <<" firsthand_success_windows="
             <<counts.activeSuccessMemoryWindows
             <<" optional_cadence_windows="<<counts.optionalCadenceWindows
             <<" local_same_material_pair_windows="
             <<counts.knownMaterialPairWindows
             <<" memory_ranking_flip_windows="
             <<counts.unlearnedLearnedRankingFlips
             <<" weighted_ranking_flip_windows="
             <<counts.populationWeightedRankingFlips
             <<" weighted_native_gather_eligible_windows="
             <<counts.populationWeightedNativeGatherEligible
             <<" issued_same_material_gather_windows="
             <<counts.subsequentSameMaterialCoreGathers
             <<" actual_learned_core_tokens="
             <<counts.issuedLearnedChoiceTokens
             <<" acceptance=DIAGNOSTIC_ONLY"
             <<"\n";
}
int main(){
    auditSeed(874213954ULL);
    auditSeed(4242001ULL);
}
