#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <utility>

#include "ActionExperienceLearning.h"
#include "SimulationSnapshot.h"

// COG-AUDIT B1: read-only observation of authoritative Core checkpoints.
// No Simulation pointer, mutation, RNG sampling, implicit ticking or journal.
// Snapshot sampling is intentionally NOT an action-event recorder: a goal
// transition between samples and a proposal never executed are unobservable.
namespace lifelens {

inline constexpr std::size_t AutonomyPhysicalGoalCount=6;
// Evidence types come from actual `rememberActionOutcome` beliefs, not
// completed-action events. Do not label these counters as execution counts.
inline constexpr std::size_t AutonomyExperienceActionCount=7;
inline constexpr const char* AutonomyExperienceActionNames[AutonomyExperienceActionCount]={
    "Eat","Drink","Sleep","UseToilet","Wash","Idle","Other"
};
inline std::size_t autonomyExperienceActionIndex(const std::string& proposition)
{
    constexpr const char prefix[]="experienced-action-outcome:";
    constexpr std::size_t prefixLen=sizeof(prefix)-1;
    if(proposition.compare(0,prefixLen,prefix)!=0)
        return AutonomyExperienceActionCount-1;
    const std::string action=proposition.substr(prefixLen);
    for(std::size_t index=0;index+1<AutonomyExperienceActionCount;++index){
        if(action==AutonomyExperienceActionNames[index]) return index;
    }
    return AutonomyExperienceActionCount-1;
}

inline std::size_t autonomyPhysicalGoalIndex(Goal goal)
{
    switch(goal){
        case Goal::Eat: return 0;
        case Goal::Drink: return 1;
        case Goal::Sleep: return 2;
        case Goal::UseToilet: return 3;
        case Goal::Wash: return 4;
        case Goal::Idle:
        default: return 5;
    }
}

struct AutonomyResidentCheckpoint {
    CharacterId id=0;
    bool hasRuntime=false;
    Goal physicalGoal=Goal::Idle;
    bool contextActionPending=false;
    bool socialContextPending=false;
    bool civilizationContextPending=false;
    bool knowledgeContextPending=false;
    bool tradeContextPending=false;
    std::uint64_t successfulPhysicalEvidence=0;
    std::uint64_t failedPhysicalEvidence=0;
    std::array<std::uint64_t,AutonomyExperienceActionCount> successEvidenceByAction{};
    std::array<std::uint64_t,AutonomyExperienceActionCount> failureEvidenceByAction{};
    std::size_t experienceBeliefKinds=0;
    std::size_t directMemories=0;
    std::size_t reportedMemories=0;
    std::size_t inferredMemories=0;
};

struct AutonomyCheckpoint {
    WorldSeed seed=0;
    PopulationSeed populationSeed=0;
    WorldGenerationVersion generationVersion=0;
    int minute=0;
    std::size_t aliveResidents=0;
    std::size_t residentsWithRuntime=0;
    std::array<std::uint64_t,AutonomyPhysicalGoalCount> physicalGoals{};
    std::uint64_t contextActionPending=0;
    std::uint64_t socialContextPending=0;
    std::uint64_t civilizationContextPending=0;
    std::uint64_t knowledgeContextPending=0;
    std::uint64_t tradeContextPending=0;
    std::size_t socialFacts=0;
    std::size_t directKnowledgeReceipts=0;
    std::size_t transmittedKnowledgeReceipts=0;
    std::vector<AutonomyResidentCheckpoint> residents;
};

inline AutonomyCheckpoint observeAutonomyCheckpoint(
    const SimulationStateSnapshot& snapshot)
{
    AutonomyCheckpoint observation;
    observation.seed=snapshot.world.seed;
    observation.populationSeed=snapshot.world.populationSeed;
    observation.generationVersion=snapshot.world.generationVersion;
    observation.minute=snapshot.world.minute;
    observation.socialFacts=snapshot.socialKnowledge.facts().size();
    for(const KnowledgeReceipt& receipt:snapshot.socialKnowledge.receipts()){
        if(receipt.source==MemorySource::DirectWitness){
            ++observation.directKnowledgeReceipts;
        }else if(receipt.source==MemorySource::ToldByOther){
            ++observation.transmittedKnowledgeReceipts;
        }
    }
    for(const Character& resident:snapshot.world.characters){
        if(!resident.alive) continue;
        ++observation.aliveResidents;
        AutonomyResidentCheckpoint row;
        row.id=resident.id;
        const auto it=snapshot.runtime.find(resident.id);
        if(it!=snapshot.runtime.end()){
            row.hasRuntime=true;
            ++observation.residentsWithRuntime;
            row.physicalGoal=it->second.goal;
            ++observation.physicalGoals[autonomyPhysicalGoalIndex(row.physicalGoal)];
            const auto kind=it->second.pendingContext.kind;
            row.contextActionPending=it->second.pendingContext.active();
            row.socialContextPending=row.contextActionPending
                &&kind==ContextActionKind::Social;
            row.civilizationContextPending=row.contextActionPending
                &&kind==ContextActionKind::Civilization;
            row.knowledgeContextPending=row.contextActionPending
                &&kind==ContextActionKind::KnowledgeTeaching;
            row.tradeContextPending=row.contextActionPending
                &&kind==ContextActionKind::Trade;
            observation.contextActionPending+=row.contextActionPending;
            observation.socialContextPending+=row.socialContextPending;
            observation.civilizationContextPending+=row.civilizationContextPending;
            observation.knowledgeContextPending+=row.knowledgeContextPending;
            observation.tradeContextPending+=row.tradeContextPending;
        }
        for(const BeliefRecord& belief:resident.beliefs.beliefs){
            if(belief.subject!=0
                ||belief.proposition.compare(
                    0,std::string("experienced-action-outcome:").size(),
                    "experienced-action-outcome:")!=0) continue;
            ++row.experienceBeliefKinds;
            row.successfulPhysicalEvidence+=belief.supportCount;
            row.failedPhysicalEvidence+=belief.contradictionCount;
            const auto action=autonomyExperienceActionIndex(belief.proposition);
            row.successEvidenceByAction[action]+=belief.supportCount;
            row.failureEvidenceByAction[action]+=belief.contradictionCount;
        }
        for(const MemoryRecord& memory:resident.memory.entries){
            if(memory.source==MemorySource::DirectWitness) ++row.directMemories;
            else if(memory.source==MemorySource::ToldByOther) ++row.reportedMemories;
            else if(memory.source==MemorySource::Inferred) ++row.inferredMemories;
        }
        observation.residents.push_back(std::move(row));
    }
    std::sort(observation.residents.begin(),observation.residents.end(),
        [](const AutonomyResidentCheckpoint& a,const AutonomyResidentCheckpoint& b){
            return a.id<b.id;
        });
    return observation;
}

struct AutonomyActorAudit {
    CharacterId id=0;
    std::uint64_t observedSamples=0;
    std::uint64_t observedGoalChanges=0;
    std::uint64_t observedSuccessEvidenceDelta=0;
    std::uint64_t observedFailureEvidenceDelta=0;
    std::array<std::uint64_t,AutonomyExperienceActionCount> observedSuccessEvidenceByActionDelta{};
    std::array<std::uint64_t,AutonomyExperienceActionCount> observedFailureEvidenceByActionDelta{};
    std::array<std::uint64_t,AutonomyExperienceActionCount> lastSuccessEvidenceByAction{};
    std::array<std::uint64_t,AutonomyExperienceActionCount> lastFailureEvidenceByAction{};
    std::size_t lastExperienceBeliefKinds=0;
    std::size_t lastDirectMemories=0;
    std::size_t lastReportedMemories=0;
    std::size_t lastInferredMemories=0;
    bool lastRuntimePresent=false;
    Goal lastGoal=Goal::Idle;
    std::uint64_t lastSuccessEvidence=0;
    std::uint64_t lastFailureEvidence=0;
};

struct AutonomyAuditSeries {
    WorldSeed seed=0;
    PopulationSeed populationSeed=0;
    WorldGenerationVersion generationVersion=0;
    int firstMinute=-1;
    int lastMinute=-1;
    std::uint64_t checkpoints=0;
    std::uint64_t residentSamples=0;
    std::uint64_t runtimeSamples=0;
    std::array<std::uint64_t,AutonomyPhysicalGoalCount> sampledPhysicalGoals{};
    std::uint64_t sampledPendingContext=0;
    std::uint64_t sampledSocialContext=0;
    std::uint64_t sampledCivilizationContext=0;
    std::uint64_t sampledKnowledgeContext=0;
    std::uint64_t sampledTradeContext=0;
    std::array<std::uint64_t,AutonomyExperienceActionCount> observedSuccessEvidenceByActionDelta{};
    std::array<std::uint64_t,AutonomyExperienceActionCount> observedFailureEvidenceByActionDelta{};
    std::size_t lastKnowledgeFacts=0;
    std::size_t lastDirectKnowledgeReceipts=0;
    std::size_t lastTransmittedKnowledgeReceipts=0;
    std::map<CharacterId,AutonomyActorAudit> actors;

    // Duplicate minute, rewind, cross-world, population/generation mismatch
    // and malformed snapshot version are rejected without modifying metrics.
    bool observe(const SimulationStateSnapshot& snapshot)
    {
        if(snapshot.version!=SimulationSnapshotVersion) return false;
        const AutonomyCheckpoint current=observeAutonomyCheckpoint(snapshot);
        if(current.seed==0 ||current.generationVersion==0
           ||current.minute<0) return false;
        if(checkpoints>0
           &&(current.seed!=seed
              ||current.populationSeed!=populationSeed
              ||current.generationVersion!=generationVersion
              ||current.minute<=lastMinute)) return false;
        if(checkpoints==0){
            seed=current.seed;
            populationSeed=current.populationSeed;
            generationVersion=current.generationVersion;
            firstMinute=current.minute;
        }
        lastMinute=current.minute;
        ++checkpoints;
        residentSamples+=current.aliveResidents;
        runtimeSamples+=current.residentsWithRuntime;
        for(std::size_t i=0;i<AutonomyPhysicalGoalCount;++i){
            sampledPhysicalGoals[i]+=current.physicalGoals[i];
        }
        sampledPendingContext+=current.contextActionPending;
        sampledSocialContext+=current.socialContextPending;
        sampledCivilizationContext+=current.civilizationContextPending;
        sampledKnowledgeContext+=current.knowledgeContextPending;
        sampledTradeContext+=current.tradeContextPending;
        lastKnowledgeFacts=current.socialFacts;
        lastDirectKnowledgeReceipts=current.directKnowledgeReceipts;
        lastTransmittedKnowledgeReceipts=current.transmittedKnowledgeReceipts;

        for(const auto& row:current.residents){
            auto& actor=actors[row.id];
            const bool previouslyObserved=actor.observedSamples!=0;
            actor.id=row.id;
            if(previouslyObserved &&actor.lastRuntimePresent &&row.hasRuntime
               &&actor.lastGoal!=row.physicalGoal) ++actor.observedGoalChanges;
            if(previouslyObserved){
                if(row.successfulPhysicalEvidence>actor.lastSuccessEvidence){
                    actor.observedSuccessEvidenceDelta+=
                        row.successfulPhysicalEvidence-actor.lastSuccessEvidence;
                }
                if(row.failedPhysicalEvidence>actor.lastFailureEvidence){
                    actor.observedFailureEvidenceDelta+=
                        row.failedPhysicalEvidence-actor.lastFailureEvidence;
                }
            }
            // Evidence counts are monotonically accumulated only when they
            // increase between observed checkpoints. A reset/decrease may
            // reflect removed/rebuilt beliefs; never invent negative events.
            for(std::size_t i=0;i<AutonomyExperienceActionCount;++i){
                if(previouslyObserved
                    &&row.successEvidenceByAction[i]>actor.lastSuccessEvidenceByAction[i]){
                    const auto delta=row.successEvidenceByAction[i]
                        -actor.lastSuccessEvidenceByAction[i];
                    actor.observedSuccessEvidenceByActionDelta[i]+=delta;
                    observedSuccessEvidenceByActionDelta[i]+=delta;
                }
                if(previouslyObserved
                    &&row.failureEvidenceByAction[i]>actor.lastFailureEvidenceByAction[i]){
                    const auto delta=row.failureEvidenceByAction[i]
                        -actor.lastFailureEvidenceByAction[i];
                    actor.observedFailureEvidenceByActionDelta[i]+=delta;
                    observedFailureEvidenceByActionDelta[i]+=delta;
                }
            }
            ++actor.observedSamples;
            actor.lastSuccessEvidenceByAction=row.successEvidenceByAction;
            actor.lastFailureEvidenceByAction=row.failureEvidenceByAction;
            actor.lastRuntimePresent=row.hasRuntime;
            actor.lastGoal=row.physicalGoal;
            actor.lastSuccessEvidence=row.successfulPhysicalEvidence;
            actor.lastFailureEvidence=row.failedPhysicalEvidence;
            actor.lastExperienceBeliefKinds=row.experienceBeliefKinds;
            actor.lastDirectMemories=row.directMemories;
            actor.lastReportedMemories=row.reportedMemories;
            actor.lastInferredMemories=row.inferredMemories;
        }
        return true;
    }

    static void writeEvidenceByActionJson(
        std::ostringstream& out,
        const std::array<std::uint64_t,AutonomyExperienceActionCount>& counts)
    {
        out<<"{";
        for(std::size_t i=0;i<AutonomyExperienceActionCount;++i){
            if(i) out<<",";
            out<<"\""<<AutonomyExperienceActionNames[i]<<"\":"<<counts[i];
        }
        out<<"}";
    }

    std::string json() const
    {
        // Stable field/order and no untrusted strings in the serialized audit.
        std::ostringstream out;
        out<<"{\"schema\":\"CoreAutonomyCheckpointAuditV1\"";
        out<<",\"source\":\"CoreSnapshotReadOnly\"";
        out<<",\"worldSeed\":\""<<seed<<"\"";
        out<<",\"populationSeed\":\""<<populationSeed<<"\"";
        out<<",\"generationVersion\":"<<generationVersion;
        out<<",\"firstMinute\":"<<firstMinute;
        out<<",\"lastMinute\":"<<lastMinute;
        out<<",\"checkpointCount\":"<<checkpoints;
        out<<",\"residentSamples\":"<<residentSamples;
        out<<",\"runtimeSamples\":"<<runtimeSamples;
        out<<",\"sampledPhysicalGoals\":{";
        constexpr const char* names[]={
            "Eat","Drink","Sleep","UseToilet","Wash","Idle"};
        for(std::size_t i=0;i<AutonomyPhysicalGoalCount;++i){
            if(i) out<<",";
            out<<"\""<<names[i]<<"\":"<<sampledPhysicalGoals[i];
        }
        out<<"}";
        out<<",\"sampledPendingContexts\":{";
        out<<"\"all\":"<<sampledPendingContext;
        out<<",\"social\":"<<sampledSocialContext;
        out<<",\"civilization\":"<<sampledCivilizationContext;
        out<<",\"knowledgeTeaching\":"<<sampledKnowledgeContext;
        out<<",\"trade\":"<<sampledTradeContext<<"}";
        out<<",\"successEvidenceDeltaByAction\":";
        writeEvidenceByActionJson(out,observedSuccessEvidenceByActionDelta);
        out<<",\"failureEvidenceDeltaByAction\":";
        writeEvidenceByActionJson(out,observedFailureEvidenceByActionDelta);
        out<<",\"lastSocialFacts\":"<<lastKnowledgeFacts;
        out<<",\"lastDirectKnowledgeReceipts\":"<<lastDirectKnowledgeReceipts;
        out<<",\"lastTransmittedKnowledgeReceipts\":"<<lastTransmittedKnowledgeReceipts;
        out<<",\"actors\":[";
        bool first=true;
        for(const auto& [id,actor]:actors){
            if(!first) out<<",";
            first=false;
            out<<"{\"actorId\":\""<<id<<"\"";
            out<<",\"observedSamples\":"<<actor.observedSamples;
            out<<",\"observedGoalChanges\":"<<actor.observedGoalChanges;
            out<<",\"physicalSuccessEvidenceDelta\":"<<actor.observedSuccessEvidenceDelta;
            out<<",\"physicalFailureEvidenceDelta\":"<<actor.observedFailureEvidenceDelta;
            out<<",\"successEvidenceDeltaByAction\":";
            writeEvidenceByActionJson(out,actor.observedSuccessEvidenceByActionDelta);
            out<<",\"failureEvidenceDeltaByAction\":";
            writeEvidenceByActionJson(out,actor.observedFailureEvidenceByActionDelta);
            out<<",\"experienceBeliefKindsLast\":"<<actor.lastExperienceBeliefKinds;
            out<<",\"directMemoriesLast\":"<<actor.lastDirectMemories;
            out<<",\"reportedMemoriesLast\":"<<actor.lastReportedMemories;
            out<<",\"inferredMemoriesLast\":"<<actor.lastInferredMemories;
            out<<"}";
        }
        out<<"]";
        // Never conflate sampled goal-state with actual events or capability.
        out<<",\"notInstrumented\":["
           <<"\"novelGoalCreation\",\"hypothesisGeneration\","
           <<"\"composedPrimitivePlans\",\"acceptedCognitionEvents\","
           <<"\"causalMethodLearning\",\"emergentCivilization\"]";
        out<<",\"evidenceCaveat\":\"Per-action deltas are observed belief evidence, not executed-action counts\"";
        out<<",\"samplingCaveat\":\"Snapshots are observations, not all completed actions\"";
        out<<"}";
        return out.str();
    }
};

} // namespace lifelens
