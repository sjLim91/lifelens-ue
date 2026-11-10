#pragma once

#include <sstream>
#include "ContextAction.h"
#include "ResidentKnownResourceMemory.h"
#include "ResidentSensedResourceAgency.h"

namespace lifelens {

// Typed, personal planning evidence. Predictions and deliberate interruptions
// never masquerade as an executed extraction failure or success receipt.
enum class GatherAssessmentCause {
    SourceExhausted, SourceRemoved, AccessUnavailable, RouteBlocked,
    DeadlineInsufficient, LocalCompetition, SurvivalPriority, ContainerMissing,
    UnknownFailure, Progress
};

inline const char* gatherAssessmentCauseName(GatherAssessmentCause cause)
{
    switch(cause){
        case GatherAssessmentCause::SourceExhausted: return "source-exhausted";
        case GatherAssessmentCause::SourceRemoved: return "source-removed";
        case GatherAssessmentCause::AccessUnavailable: return "access-unavailable";
        case GatherAssessmentCause::RouteBlocked: return "route-blocked";
        case GatherAssessmentCause::DeadlineInsufficient: return "deadline-insufficient";
        case GatherAssessmentCause::LocalCompetition: return "local-competition";
        case GatherAssessmentCause::SurvivalPriority: return "survival-priority";
        case GatherAssessmentCause::ContainerMissing: return "container-missing";
        case GatherAssessmentCause::UnknownFailure: return "unspecified-failure";
        default: return "progress";
    }
}

struct ResidentGatherAssessment {
    std::uint64_t goal=0,token=0,source=0;
    GatherAssessmentCause cause=GatherAssessmentCause::Progress;
    int minute=-1,deadline=-1,quantity=0;
    bool actualFailure=false,resumable=false;
};

inline bool readResidentGatherAssessment(const MemoryRecord& memory,
    CharacterId actor,int now,ResidentGatherAssessment& out)
{
    if(actor==0 ||now<0 ||memory.tags.size()!=2 ||memory.tags[0]!="gather_assessment_v1"
       ||memory.who!=actor ||memory.sourceCharacter!=actor
       ||memory.source!=MemorySource::DirectWitness ||!memory.witnessed
       ||memory.minute<0 ||memory.minute>now
       ||now-memory.minute>ResidentResourceOutcomeHorizonMinutes
       ||!std::isfinite(memory.confidence) ||memory.confidence<=0 ||memory.confidence>1
       ||!std::isfinite(memory.importance) ||memory.importance<0 ||memory.importance>1
       ||!std::isfinite(memory.emotionIntensity) ||memory.emotionIntensity<0
       ||memory.emotionIntensity>1 ||!std::isfinite(memory.decayPerDay)
       ||memory.decayPerDay<0) return false;
    ResidentGatherAssessment parsed;
    int cause=-1,actual=-1,resume=-1;
    std::istringstream input(memory.tags[1]);
    if(!(input>>parsed.goal>>parsed.token>>parsed.source>>cause
         >>parsed.deadline>>parsed.quantity>>actual>>resume)) return false;
    input>>std::ws;
    if(!input.eof() ||parsed.goal==0 ||parsed.token==0 ||parsed.source==0
       ||cause<0 ||cause>static_cast<int>(GatherAssessmentCause::Progress)
       ||actual<0 ||actual>1 ||resume<0 ||resume>1 ||parsed.quantity<0
       ||static_cast<std::int64_t>(parsed.deadline)-memory.minute < -ResidentGatherGoalMaxMinutes
       ||static_cast<std::int64_t>(parsed.deadline)-memory.minute > ResidentGatherGoalMaxMinutes)
        return false;
    parsed.cause=static_cast<GatherAssessmentCause>(cause);
    // A stream's unsigned parser accepts textual negative IDs. Round-trip
    // exactly the canonical typed payload, and validate the accompanying
    // description/location, before trusting persisted personal evidence.
    std::ostringstream canonical;
    canonical<<parsed.goal<<' '<<parsed.token<<' '<<parsed.source<<' '<<cause<<' '
        <<parsed.deadline<<' '<<parsed.quantity<<' '<<actual<<' '<<resume;
    if(canonical.str()!=memory.tags[1]
       ||memory.what!=std::string("자원 목표 재평가: ")+gatherAssessmentCauseName(parsed.cause)
       ||memory.where!="resource-node:"+std::to_string(parsed.source)) return false;
    if(actual &&(parsed.cause==GatherAssessmentCause::DeadlineInsufficient
        ||parsed.cause==GatherAssessmentCause::LocalCompetition
        ||parsed.cause==GatherAssessmentCause::SurvivalPriority
        ||parsed.cause==GatherAssessmentCause::Progress)) return false;
    parsed.minute=memory.minute;
    parsed.actualFailure=actual!=0;
    parsed.resumable=resume!=0;
    out=parsed;
    return true;
}

inline void rememberResidentGatherAssessment(Character& actor,
    const PendingContextAction& pending,int now,GatherAssessmentCause cause,
    bool actualFailure,bool resumable,int quantity)
{
    if(!actor.alive ||!validResidentGatherGoal(pending.gatherGoal)
       ||pending.gatherGoal.actor!=actor.id ||pending.token!=pending.gatherGoal.token
       ||pending.kind!=ContextActionKind::Civilization
       ||pending.civilization.intent!=CivilizationIntent::Gather
       ||pending.civilization.resourceNode==0
       ||pending.civilization.material!=pending.gatherGoal.material
       ||now<pending.gatherGoal.issuedMinute ||quantity<0)
        return;
    MemoryRecord memory;
    memory.who=memory.sourceCharacter=actor.id;
    memory.minute=now;
    memory.source=MemorySource::DirectWitness;
    memory.witnessed=true;
    memory.confidence=1.0;
    memory.importance=0.42;
    memory.what=std::string("자원 목표 재평가: ")+gatherAssessmentCauseName(cause);
    memory.where="resource-node:"+std::to_string(pending.civilization.resourceNode);
    std::ostringstream fields;
    fields<<pending.gatherGoal.goalId<<' '<<pending.token<<' '
        <<pending.civilization.resourceNode<<' '<<static_cast<int>(cause)<<' '
        <<pending.gatherGoal.expiresMinute<<' '<<std::max(0,quantity)<<' '
        <<actualFailure<<' '<<resumable;
    memory.tags={"gather_assessment_v1",fields.str()};
    ResidentGatherAssessment valid;
    if(!readResidentGatherAssessment(memory,actor.id,now,valid)) return;
    // Bounded episodes; repeating the same diagnosis is not new evidence.
    std::size_t count=0,oldest=actor.memory.entries.size();
    for(std::size_t i=0;i<actor.memory.entries.size();++i){
        const auto& entry=actor.memory.entries[i];
        if(entry.tags.empty() ||entry.tags[0]!="gather_assessment_v1"
           ||entry.who!=actor.id ||entry.sourceCharacter!=actor.id) continue;
        ResidentGatherAssessment prior;
        if(readResidentGatherAssessment(entry,actor.id,now,prior)
           &&prior.token==pending.token &&prior.cause==cause){
            if(now>=prior.minute &&(cause==GatherAssessmentCause::LocalCompetition
               ||(actualFailure &&!prior.actualFailure)))
                actor.memory.entries[i]=std::move(memory);
            return;
        }
        ++count;
        if(oldest==actor.memory.entries.size()
           ||actor.memory.entries[i].minute<actor.memory.entries[oldest].minute) oldest=i;
    }
    if(count>=12 &&oldest<actor.memory.entries.size())
        actor.memory.entries.erase(actor.memory.entries.begin()+oldest);
    actor.memory.add(std::move(memory));
}

struct ResidentGatherAlternative {
    ResourceNodeId source=0;
    GridPos position{};
    int observedMinute=-1,quantity=0,travelMinutes=0;
    double confidence=0,score=0;
    bool remembered=false,reachable=false,feasible=false;
    const char* reason="unknown";
    std::vector<GridPos> route;
};

// The executor uses these same weather/health intervals and existing Core
// routes. Estimate with current weather; subsequent reviews correct changes.
inline int residentGatherTravelMinutes(const World& world,const Character& actor,
    GridPos from,const std::vector<GridPos>& route,int firstTickDelay=1)
{
    const int penalty=static_cast<int>(std::lround(
        2.0*(1.0-healthFunctionalCapacity01(actor.health))));
    int elapsed=std::max(0,firstTickDelay);
    for(std::size_t n=0;n<route.size();++n){
        const int interval=std::max(1,coreGroundStepIntervalMinutes(world,from)+penalty);
        const int minute=world.minute+elapsed;
        elapsed+=(interval-minute%interval)%interval;
        from=route[n];
        if(n+1<route.size()) ++elapsed;
    }
    // A zero-length route still requires the next actual executor tick.
    return elapsed;
}

inline bool residentGatherSurvivalAllows(const Character& actor,MaterialKind material,
    int travelMinutes,const SimulationRuleset& rules=DefaultSimulationRuleset)
{
    Needs projected=actor.needs;
    projected.apply({rules.needs.hungerPerMinute*actor.metabolism*travelMinutes,
        rules.needs.thirstPerMinute*actor.metabolism*travelMinutes,
        rules.needs.sleepPerMinute*actor.sleepTendency*travelMinutes,
        rules.needs.bladderPerMinute*actor.metabolism*travelMinutes,
        rules.needs.hygienePerMinute*travelMinutes});
    const double strongest=std::max({projected.hunger,projected.thirst,
        projected.sleep,projected.bladder,projected.hygiene});
    if(strongest<rules.utilityAI.urgentThreshold) return true;
    return (material==MaterialKind::PlantFood &&projected.hunger>=strongest-1e-12)
        ||(material==MaterialKind::Water &&projected.thirst>=strongest-1e-12);
}

inline std::vector<ResidentGatherAlternative> assessResidentGatherAlternatives(
    const World& world,const Character& actor,GridPos position,
    const CivilizationUtilityDecision& gather,int deadline,int needed,
    const SimulationRuleset& rules=DefaultSimulationRuleset)
{
    std::map<ResourceNodeId,ResidentGatherAlternative> known;
    // Remote stock is never read here: memories are dated estimates, not an
    // oracle for the world's current resource ledger.
    for(const auto& memory:residentKnownResources(actor,world.minute)){
        if(memory.material!=gather.material) continue;
        auto& c=known[memory.nodeId];
        c.source=memory.nodeId;c.position=memory.position;
        c.observedMinute=memory.minute;c.quantity=memory.observedQuantity;
        c.confidence=memory.confidence;c.remembered=true;
    }
    const auto local=observeLocalResourceProximity(
        world,actor.id,position,ResidentLocalPerceptionMaxRadius);
    for(const auto& observation:local.nearbyResources){
        if(observation.material!=gather.material) continue;
        const auto* node=findCivilizationResourceNodeSpatial(world,observation.nodeId);
        if(!node) continue;
        auto& c=known[observation.nodeId];
        c.source=observation.nodeId;c.position=observation.position;
        c.observedMinute=world.minute;c.quantity=node->quantity;
        c.confidence=0.60;c.remembered=false;
    }
    const auto outcomes=buildResidentResourceOutcomeIndex(actor,world.minute);
    const std::string key="Civilization:Gather:"+std::string(materialName(gather.material));
    std::vector<ResidentGatherAlternative> result;
    for(auto& entry:known){
        auto c=std::move(entry.second);
        if(c.source==gather.resourceNode) continue;
        if(perceptionGridDistance(position,c.position)<=6){
            const auto* current=findCivilizationResourceNodeSpatial(world,c.source);
            if(!current ||!sameGridPos(current->pos,c.position)
               ||current->material!=gather.material ||current->quantity<=0){
                c.quantity=0;c.reason="locally-unavailable";
                result.push_back(std::move(c));continue;
            }
            c.quantity=current->quantity;c.observedMinute=world.minute;
            c.remembered=false;c.confidence=0.60;
        }
        const auto history=outcomes.find(c.source,key);
        if(history.hasEvidence &&!history.succeeded &&history.minute>=c.observedMinute
           &&(history.reason==ResidentResourceOutcomeReason::SourceUnavailable
              ||history.reason==ResidentResourceOutcomeReason::AccessUnavailable)){
            c.reason="personal-failure";result.push_back(std::move(c));continue;
        }
        GridPos access{};
        c.reachable=resolveCivilizationResourceAccessAtKnownPosition(
            world,c.source,gather.material,c.position,access)
            &&buildCoreGroundRoute(world,position,access,1,c.route);
        if(c.reachable) c.position=access;
        if(!c.reachable) c.reason="route-blocked";
        else{
            c.travelMinutes=residentGatherTravelMinutes(world,actor,position,c.route);
            if(c.quantity<=0 ||c.confidence<=0) c.reason="uncertain-stock";
            else if(world.minute+c.travelMinutes>=deadline) c.reason="deadline-insufficient";
            else if(!residentGatherSurvivalAllows(actor,gather.material,c.travelMinutes,rules))
                c.reason="survival-priority";
            else{
                c.feasible=true;
                c.reason=c.quantity>=needed ? "known-stock-within-budget" : "partial-progress-within-budget";
                c.score=c.confidence*std::min(c.quantity,std::max(1,needed))
                    /(1.0+c.travelMinutes)+history.utilityAdjustment;
            }
        }
        result.push_back(std::move(c));
    }
    return result;
}

} // namespace lifelens
