#pragma once

// Observer-only instrumentation. No writes to Simulation, World, rules or RNG.
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#if defined(__unix__) || defined(__APPLE__)
#include <sys/resource.h>
#endif
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

namespace lifelens::audit {

inline std::string quote(const std::string& value) {
    std::ostringstream out; out<<'"';
    for(unsigned char c:value){
        switch(c){
            case '"': out<<"\\\""; break;
            case '\\': out<<"\\\\"; break;
            case '\n': out<<"\\n"; break;
            case '\r': out<<"\\r"; break;
            case '\t': out<<"\\t"; break;
            default:
                if(c<32){out<<"\\u00"<<std::hex<<std::setw(2)<<std::setfill('0')<<int(c)<<std::dec;}
                else out<<c;
        }
    }
    out<<'"'; return out.str();
}
struct Json {
    std::ostringstream out; bool first=true;
    Json(){out<<'{';out<<std::setprecision(17);}
    void key(const std::string& k){if(!first)out<<',';first=false;out<<quote(k)<<':';}
    template<class T> void add(const std::string& k,T v){key(k);out<<v;}
    void add(const std::string& k,const std::string& v){key(k);out<<quote(v);}
    void add(const std::string& k,const char* v){add(k,std::string(v));}
    void raw(const std::string& k,const std::string& v){key(k);out<<v;}
    std::string str(){return out.str()+"}";}
};
inline std::string idString(std::uint64_t id){return std::to_string(id);}
inline const char* facilityName(FacilityKind kind){
    constexpr std::array<const char*,7> names={"PrimitiveStorage","FirePit","WorkSurface","SleepingPlace","Shelter","Furnace","CultivatedPlot"};
    return int(kind)>=0 && int(kind)<7 ? names[int(kind)] : "Unknown";
}
inline std::uint64_t hashBytes(std::uint64_t hash,const std::string& value){
    for(unsigned char c:value){hash^=c;hash*=1099511628211ULL;}return hash;
}
inline std::string hexHash(std::uint64_t hash){std::ostringstream out;out<<std::hex<<std::setw(16)<<std::setfill('0')<<hash;return out.str();}
inline constexpr std::array<const char*,5> NeedNames={"hunger","thirst","sleep","bladder","hygiene"};
inline constexpr std::array<const char*,22> BudgetNames={
    "survival","gather","storage_logistics","construction","repair","production",
    "cultivation","social","dating","family","parenting","health_care","teaching",
    "learning","institution","economy","trade","exploration","migration","idle","waiting","failed_retry"};
inline constexpr std::array<MaterialKind,16> Materials={MaterialKind::Water,MaterialKind::PlantFood,
    MaterialKind::Wood,MaterialKind::Stone,MaterialKind::Flint,MaterialKind::Fiber,MaterialKind::Clay,
    MaterialKind::Bone,MaterialKind::Hide,MaterialKind::CopperOre,MaterialKind::TinOre,
    MaterialKind::IronOre,MaterialKind::Charcoal,MaterialKind::CopperMetal,MaterialKind::TinMetal,MaterialKind::Bronze};
struct NeedStats {
    double sum=0,max=0; std::uint64_t count=0,entries=0,minutes=0,streak=0,longest=0;
    std::array<std::uint64_t,1001> histogram{};
    void observe(double value,double threshold){
        sum+=value;max=std::max(max,value);++count;
        ++histogram[std::min(1000,std::max(0,int(std::round(value*1000))))];
        if(value>=threshold){if(streak==0)++entries;++minutes;++streak;longest=std::max(longest,streak);}else streak=0;
    }
    double percentile(double p)const{
        if(!count)return 0;std::uint64_t total=0,target=std::uint64_t(std::ceil(count*p));
        for(std::size_t i=0;i<histogram.size();++i){total+=histogram[i];if(total>=target)return double(i)/1000;}return 1;
    }
    std::string json()const{Json j;j.add("samples",count);j.add("mean",count?sum/count:0);j.add("max",max);
        j.add("p50",percentile(.5));j.add("p95",percentile(.95));j.add("p99",percentile(.99));
        j.add("critical_entries",entries);j.add("critical_minutes",minutes);j.add("longest_critical_streak",longest);return j.str();}
};
struct ResidentAudit {
    std::array<NeedStats,5> needs; std::array<std::uint64_t,22> budget{};
    std::array<std::uint64_t,5> travel{},resolvedMinutes{},resolvedDistance{},resolvedCount{};
    GridPos previous{}; bool positioned=false; int sessionStart=-1; Goal sessionGoal=Goal::Idle;
    std::uint64_t sessionDistance=0,observations=0,illMinutes=0,waterExposureEntries=0;
    double capacitySum=0,wasteExposureSum=0,waterDoseSum=0; int lastExposure=-1;
    HealthState previousHealth{}; bool healthObserved=false,previousTrade=false,tradeWasPreempted=false;
    std::uint64_t preemptions=0,routeFailures=0,timeouts=0,retries=0;
    std::string lastFailure; int lastFailureMinute=-1;
    std::map<std::uint64_t,std::uint64_t> pairs;
    std::uint64_t lastContextToken=0,revisitedChunks=0; int explorationStart=-1; GridPos explorationOrigin{};
    std::pair<int,int> previousChunk{}; bool hasPreviousChunk=false;
    MaterialKind explorationMaterial=MaterialKind::Unknown;
    std::uint64_t explorationDistance=0; std::set<std::pair<int,int>> visits;
    std::map<int,int> learned,firstPracticalUse; std::map<int,int> lastUses;
};

class SystemBalanceAudit {
    Simulation& sim; std::ofstream records,events;
    std::map<CharacterId,ResidentAudit> residents;
    std::set<CharacterId> founders;
    std::array<NeedStats,5> needs; std::array<std::uint64_t,16> zeroStock{},zeroStreak{},longestZero{};
    std::array<std::uint64_t,16> gathered{},stored{},retrieved{};
    std::map<std::string,std::uint64_t> eventCounts;
    std::map<CharacterId,int> tradeDeparted;
    std::map<CharacterId,std::uint64_t> tradeMissions;
    std::map<FacilityId,double> durability;std::map<FacilityId,int> repairCounts;
    std::map<std::pair<int,int>,int> chunkFirstVisit;
    std::map<std::pair<int,int>,std::set<CharacterId>> chunkVisitors;
    std::set<ResourceNodeId> newResourceNodes,usedNewResourceNodes;
    std::map<ResourceNodeId,int> previousNodeQuantity;
    std::set<std::pair<int,int>> recordedChunks;
    std::uint64_t eventHash=14695981039346656037ULL,eventSequence=0;
    std::uint64_t explorationAttempts=0,explorationSuccess=0,criticalExploration=0,explorationTravel=0;
    int startMinute=0; std::string directory;
    std::chrono::steady_clock::time_point wallStart=std::chrono::steady_clock::now();
    std::size_t previousChunks=0,lastNodeScanChunks=0; int previousDay=0; bool enabled=false,failed=false;

    void emit(Json& j){if(enabled)records<<j.str()<<'\n';}
    Json base(const char* type,int day=-1){Json j;j.add("type",type);j.add("seed",idString(sim.world().seed));
        j.add("minute",sim.world().minute);j.add("elapsed_day",double(sim.world().minute-startMinute)/1440);
        if(day>=0)j.add("day",day);return j;}
    void invariant(const std::string& error,CharacterId id=0){
        failed=true;auto j=base("invariant_failure");j.add("severity","P0");j.add("error",error);j.add("resident",idString(id));emit(j);
    }
    CharacterId actorFor(const std::string& line)const{
        // Simulation::emit adds its authoritative calendar prefix before invoking callbacks.
        const auto close=line.find("] ");
        const std::size_t start=line.rfind("[Day ",0)==0 && close!=std::string::npos ? close+2 : 0;
        for(const auto& c:sim.world().characters)if(line.compare(start,c.name.size()+1,c.name+" ")==0)return c.id;
        return 0;
    }
    int budgetCategory(const ResidentPresentationObservation& p)const{
        if(!p.active || p.phase==PresentationActionPhase::Idle)return 19;
        switch(p.kind){
            case PresentationActionKind::Physical:return 0;
            case PresentationActionKind::Social:return 7;
            case PresentationActionKind::Parenting:return p.parentingAction==ParentingAction::HealthCare?11:10;
            case PresentationActionKind::KnowledgeTeaching:return 12;
            case PresentationActionKind::Trade:return 16;
            case PresentationActionKind::Civilization:
                switch(p.facilityAction){
                    case FacilityBuildAction::Plan:case FacilityBuildAction::DeliverMaterial:case FacilityBuildAction::Work:return 3;
                    case FacilityBuildAction::Repair:return 4;
                    case FacilityBuildAction::Plant:case FacilityBuildAction::Water:case FacilityBuildAction::Tend:case FacilityBuildAction::Harvest:return 6;
                    case FacilityBuildAction::Fuel:case FacilityBuildAction::Ignite:case FacilityBuildAction::CollectCharcoal:
                    case FacilityBuildAction::LoadSmeltCharge:case FacilityBuildAction::CollectMetal:return 5;
                    default:break;
                }
                switch(p.civilizationIntent){
                    case CivilizationIntent::Gather:return 1;
                    case CivilizationIntent::Store:case CivilizationIntent::Retrieve:return 2;
                    case CivilizationIntent::Explore:return 17;
                    case CivilizationIntent::Experiment:return 13;
                    case CivilizationIntent::Craft:return 5;
                    default:return 20;
                }
            default:return 19;
        }
    }
    int generation(CharacterId id,std::set<CharacterId> seen={})const{
        if(!seen.insert(id).second)return 0;
        for(const auto& c:sim.world().characters)if(c.id==id){int depth=0;
            for(auto parent:c.parentIds)depth=std::max(depth,generation(parent,seen)+1);return depth;}return 0;
    }
    void endExploration(CharacterId id,bool success){
        auto& r=residents[id];if(r.explorationStart<0)return;
        auto j=base("exploration_end");j.add("resident",idString(id));j.add("started_minute",r.explorationStart);
        j.add("success",success?1:0);j.add("duration_minutes",sim.world().minute-r.explorationStart);
        j.add("travel_grid",r.explorationDistance);j.add("material",materialName(r.explorationMaterial));emit(j);
        explorationTravel+=r.explorationDistance;r.explorationStart=-1;
    }
public:
    SystemBalanceAudit(Simulation& s,const std::string& path):sim(s),directory(path){
        startMinute=s.world().minute;previousChunks=s.world().generatedNaturalChunks.size();
        for(const auto& c:s.world().characters)if(c.parentIds.empty())founders.insert(c.id);
        for(const auto& n:s.world().resourceNodes)previousNodeQuantity[n.id]=n.quantity;
        for(const auto& c:s.world().generatedNaturalChunks)recordedChunks.insert({c.coord.x,c.coord.y});
        if(path.empty())return;
        std::filesystem::create_directories(path);records.open(path+"/metrics.jsonl");events.open(path+"/events.jsonl");
        if(!records || !events)throw std::runtime_error("cannot open system audit outputs");enabled=true;
        auto j=base("metadata");j.add("schema_version",1);j.add("base_main","67fbf1e2c133499b2271f885ef066df30efdf665");
        j.add("population_seed",idString(s.world().populationSeed));j.add("generation_version",s.world().generationVersion);
        j.add("ruleset_version",s.ruleset().version);j.add("start_minute",startMinute);
        j.add("critical_threshold",CriticalSurvivalPreemptThreshold);j.add("histogram_resolution",.001);
        j.add("scope","elapsed since this invocation; loaded snapshot does not restore metric accumulators");emit(j);
    }
    bool active()const{return enabled;}
    void probePlanning(){
        if(!enabled)throw std::runtime_error("--audit-probe-only requires --audit-directory");
        const auto snapshot=sim.captureSnapshot();SettlementPopulation population;
        for(const auto& entry:snapshot.runtime)population[entry.first]=entry.second.pos;
        std::vector<std::uint8_t> before,after;std::string error;
        if(!encodeSimulationSnapshot(snapshot,before,&error))throw std::runtime_error(error);
        for(const auto& c:sim.world().characters){
            if(!c.alive || requiresDirectCare(c.lifeStage))continue;
            const auto runtime=snapshot.runtime.find(c.id);if(runtime==snapshot.runtime.end())continue;
            const auto physical=bestPhysicalUtility(sim.world(),c);
            const auto social=chooseSocialUtilityDecision(sim.world(),c,sim.relationships());
            const auto provision=urgentSurvivalProvisionDecisionAtPosition(sim.world(),c,runtime->second.pos);
            const auto unified=chooseUnifiedUtilityDecisionAtPosition(sim.world(),c,sim.relationships(),runtime->second.pos,.18,.14,
                &population,&sim.socialKnowledge(),&sim.households());
            auto j=base("planning_probe");j.add("resident",idString(c.id));j.add("physical_goal",goalName(physical.first));j.add("physical_utility",physical.second);
            j.add("social_intent",socialIntentName(social.intent));j.add("social_target",idString(social.target));j.add("social_utility",social.utility);
            j.add("provision_intent",civilizationIntentName(provision.intent));j.add("provision_material",materialName(provision.material));j.add("provision_utility",provision.utility);
            j.add("unified_kind",int(unified.kind));j.add("unified_utility",unified.utility);
            j.add("civilization_intent",civilizationIntentName(unified.civilization.intent));j.add("civilization_utility",unified.civilization.utility);
            j.add("max_need",maximumResidentNeed(c));j.add("penalty_until_minute",runtime->second.penaltyUntilMinute);
            j.add("plan_size",runtime->second.plan.size());j.add("pending_context_kind",int(runtime->second.pendingContext.kind));
            j.add("scope","pure candidate evaluation at saved checkpoint, not proof of an executed planning boundary");emit(j);
        }
        if(!encodeSimulationSnapshot(sim.captureSnapshot(),after,&error) || before!=after)invariant("planning probe mutated authoritative snapshot");
    }
    void event(const std::string& line){
        if(!enabled)return;
        ++eventSequence;eventHash=hashBytes(eventHash,std::to_string(sim.world().minute)+":"+line+"\n");
        const CharacterId actor=actorFor(line);auto& r=residents[actor];
        auto j=base("core_event");j.add("sequence",idString(eventSequence));j.add("resident",idString(actor));j.add("message",line);
        if(line.find("became ill")!=std::string::npos || line.find(" died")!=std::string::npos || line.find("recovered from illness")!=std::string::npos){
            for(const auto& c:sim.world().characters)if(c.id==actor){j.add("pathogen",c.health.pathogenLoad);j.add("illness",c.health.illnessSeverity);
                j.add("immunity",c.health.immunity01);j.add("care_knowledge",c.health.careKnowledge01);j.add("capacity",healthFunctionalCapacity01(c.health));
                j.add("hunger",c.needs.hunger);j.add("thirst",c.needs.thirst);j.add("sleep",c.needs.sleep);j.add("hygiene",c.needs.hygiene);}
        }
        events<<j.str()<<'\n';
        const auto contains=[&](const char* token){return line.find(token)!=std::string::npos;};
        for(const char* token:{"route failed","timed out","preempted","became ill","recovered from illness"," died",
            "departed settlement","and exchanged","returned from inter-settlement trade","cancelled inter-settlement trade journey",
            "ended outbound trade leg","taught","cared for","birth:","married","engaged","migration","discovered","crafted"}){
            if(contains(token))++eventCounts[token];
        }
        if(contains("route failed")){++r.routeFailures;r.lastFailureMinute=sim.world().minute;}
        if(contains("timed out"))++r.timeouts;
        if(contains("preempted"))++r.preemptions;
        if(contains("preempted") && tradeDeparted.count(actor)){
            ++eventCounts["trade_survival_preemption"];r.tradeWasPreempted=true;
        }
        if(contains("route failed") || contains("timed out")){if(line==r.lastFailure)++r.retries;r.lastFailure=line;}
        if(contains("departed settlement")&&contains("to trade with")){
            tradeDeparted[actor]=sim.world().minute;++tradeMissions[actor];
        }
        if(contains("returned from inter-settlement trade") || contains("cancelled inter-settlement trade journey")){
            auto t=tradeDeparted.find(actor);if(t!=tradeDeparted.end()){
                auto e=base("trade_end");e.add("resident",idString(actor));e.add("departure_minute",t->second);
                e.add("duration_minutes",sim.world().minute-t->second);e.add("returned",contains("returned")?1:0);emit(e);tradeDeparted.erase(t);}
        }
        if(contains("Civilization Explore frontier")){++explorationSuccess;endExploration(actor,true);}
        for(std::size_t m=0;m<Materials.size();++m){
            const std::string mat=materialName(Materials[m]);
            for(const auto& operation:std::array<std::pair<const char*,std::array<std::uint64_t,16>*>,3>{{{"Gather",&gathered},{"Store",&stored},{"Retrieve",&retrieved}}}){
                const std::string token=std::string(" -> Civilization ")+operation.first+" "+mat+" x";
                const auto pos=line.find(token);if(pos!=std::string::npos)(*operation.second)[m]+=std::strtoull(line.c_str()+pos+token.size(),nullptr,10);
            }
        }
        for(std::size_t n=0;n<NeedNames.size();++n){
            const std::array<const char*,5> goal={"Eat","Drink","Sleep","UseToilet","Wash"};
            if(line.find(std::string(" completed ")+goal[n])!=std::string::npos && r.sessionStart>=0 && int(r.sessionGoal)==int(n)){
                ++r.resolvedCount[n];r.resolvedMinutes[n]+=sim.world().minute-r.sessionStart;r.resolvedDistance[n]+=r.sessionDistance;r.sessionStart=-1;
            }
        }
    }
    void minute(){
        if(!enabled)return;
        const auto& w=sim.world();
        for(const auto& c:w.characters){
            auto p=sim.observeResidentPresentation(c.id);
            if(!c.alive){if(p.active)invariant("dead resident has active presentation",c.id);continue;}
            auto& r=residents[c.id];++r.observations;GridPos pos{};const bool hasPos=sim.runtimePosition(c.id,pos);
            const auto distance=r.positioned&&hasPos?manhattan(r.previous,pos):0;
            const std::array<double,5> v={c.needs.hunger,c.needs.thirst,c.needs.sleep,c.needs.bladder,c.needs.hygiene};
            for(std::size_t i=0;i<v.size();++i){
                if(v[i]>=CriticalSurvivalPreemptThreshold && r.needs[i].entries==0){auto j=base("first_need_pressure");
                    j.add("resident",idString(c.id));j.add("need",NeedNames[i]);j.add("value",v[i]);emit(j);}
                r.needs[i].observe(v[i],CriticalSurvivalPreemptThreshold);needs[i].observe(v[i],CriticalSurvivalPreemptThreshold);
            }
            const auto diagnostic=sim.observeSleepRuntimeDiagnostic(c.id);
            const int category=(!p.active && diagnostic.penaltyUntilMinute>w.minute)?21:budgetCategory(p);
            ++r.budget[category];
            if(p.kind==PresentationActionKind::Physical && p.active){
                const int goal=int(p.physicalGoal);
                if(goal>=0 && goal<5){if(p.phase==PresentationActionPhase::Moving)++r.travel[goal];
                    if(r.sessionStart<0 || r.sessionGoal!=p.physicalGoal){r.sessionStart=w.minute;r.sessionGoal=p.physicalGoal;r.sessionDistance=0;}
                    r.sessionDistance+=distance;
                }
            }else if(r.sessionStart>=0)r.sessionStart=-1;
            r.capacitySum+=healthFunctionalCapacity01(c.health);r.wasteExposureSum+=w.environmentalResidues.exposureAt(pos);
            r.waterDoseSum+=c.health.pendingWaterContaminationDose;
            if(r.healthObserved && c.health.pendingWaterContaminationDose>r.previousHealth.pendingWaterContaminationDose){
                auto j=base("water_exposure");j.add("resident",idString(c.id));j.add("x",pos.x);j.add("y",pos.y);
                j.add("dose_increase",c.health.pendingWaterContaminationDose-r.previousHealth.pendingWaterContaminationDose);
                j.add("local_residue_exposure_after_tick",w.environmentalResidues.exposureAt(pos));emit(j);
            }
            if(w.minute%FamilyProgressionDayMinutes==FamilyProgressionDecisionMinuteOfDay && r.healthObserved){
                auto j=base("health_transition");j.add("resident",idString(c.id));
                j.add("before_pathogen",r.previousHealth.pathogenLoad);j.add("after_pathogen",c.health.pathogenLoad);
                j.add("before_illness",r.previousHealth.illnessSeverity);j.add("after_illness",c.health.illnessSeverity);
                j.add("pending_water_dose_before_tick",r.previousHealth.pendingWaterContaminationDose);
                j.add("immunity_before_tick",r.previousHealth.immunity01);j.add("care_knowledge_before_tick",r.previousHealth.careKnowledge01);
                j.add("local_residue_exposure_after_tick",w.environmentalResidues.exposureAt(pos));
                j.add("genetic_health_potential",c.genetics.healthPotential);j.add("physical_health",c.lifeCondition.physicalHealth);
                j.add("hunger",c.needs.hunger);j.add("thirst",c.needs.thirst);j.add("sleep",c.needs.sleep);j.add("hygiene",c.needs.hygiene);
                j.add("sanitation_knowledge",c.civilization.knowledge.knowsAtLeast(TechniqueId::DesignatedSanitationArea,KnowledgeLevel::Reproducible)?1:0);
                j.add("capacity_after",healthFunctionalCapacity01(c.health));emit(j);
            }
            r.previousHealth=c.health;r.healthObserved=true;
            const bool trading=p.active && p.kind==PresentationActionKind::Trade;
            if(trading && !r.previousTrade && r.tradeWasPreempted){++eventCounts["trade_resumed_after_preemption"];r.tradeWasPreempted=false;}
            r.previousTrade=trading;
            if(c.health.lastExposureMinute!=r.lastExposure && c.health.lastExposureMinute>=0){++r.waterExposureEntries;r.lastExposure=c.health.lastExposureMinute;}
            if(c.health.illnessSeverity>=.18)++r.illMinutes;
            const bool exploring=p.active&&p.kind==PresentationActionKind::Civilization&&p.civilizationIntent==CivilizationIntent::Explore;
            if(exploring && p.contextActionToken!=r.lastContextToken){
                if(r.explorationStart>=0)endExploration(c.id,false);
                r.lastContextToken=p.contextActionToken;r.explorationStart=w.minute;r.explorationOrigin=pos;
                r.explorationMaterial=p.civilizationMaterial;r.explorationDistance=0;++explorationAttempts;
                auto j=base("exploration_start");j.add("resident",idString(c.id));j.add("token",idString(p.contextActionToken));
                j.add("material",materialName(p.civilizationMaterial));j.add("origin_x",pos.x);j.add("origin_y",pos.y);
                j.add("target_x",p.targetGrid.x);j.add("target_y",p.targetGrid.y);j.add("hunger",c.needs.hunger);j.add("thirst",c.needs.thirst);
                const bool critical=std::max(c.needs.hunger,c.needs.thirst)>=CriticalSurvivalPreemptThreshold;
                if(critical)++criticalExploration;j.add("critical",critical?1:0);
                const auto pressure=sim.observeResidentMigrationPressure(c.id);j.add("migration_candidate",pressure.candidate?1:0);
                j.add("migration_pressure",pressure.pressure01);j.add("last_route_failure_minute",r.lastFailureMinute);
                j.add("cause_evidence",p.civilizationMaterial==MaterialKind::Water?"water target":p.civilizationMaterial==MaterialKind::PlantFood?"food target":"material target");emit(j);
            }
            if(exploring)r.explorationDistance+=distance;else if(r.explorationStart>=0)endExploration(c.id,false);
            if(hasPos){const auto chunk=chunkCoordForGrid(pos);const auto key=std::make_pair(chunk.x,chunk.y);
                if(r.visits.insert(key).second){chunkVisitors[key].insert(c.id);if(!chunkFirstVisit.count(key))chunkFirstVisit[key]=w.minute;}
                else if(r.hasPreviousChunk && key!=r.previousChunk)++r.revisitedChunks;
                r.previousChunk=key;r.hasPreviousChunk=true;
                r.previous=pos;r.positioned=true;
            }
            if(p.active && p.targetResidentId!=0 && p.phase==PresentationActionPhase::Interacting)++r.pairs[p.targetResidentId];
            for(const auto& k:c.civilization.knowledge.all()){
                const int t=int(k.technique);if(!r.learned.count(t))r.learned[t]=w.minute;
                // Core successfulUses also includes experiment discovery; only later increments prove subsequent use.
                if(r.lastUses.count(t) && k.successfulUses>r.lastUses[t] && !r.firstPracticalUse.count(t))r.firstPracticalUse[t]=w.minute;
                r.lastUses[t]=k.successfulUses;
            }
            for(const auto& stack:c.civilization.inventory.stacks())if(stack.quantity<0)invariant("negative resident inventory",c.id);
        }
        for(std::size_t m=0;m<Materials.size();++m){int quantity=0;
            for(const auto& c:w.characters)if(c.alive)quantity+=c.civilization.inventory.count(ItemKind::RawMaterial,Materials[m]);
            for(const auto& s:w.storageSites)quantity+=s.inventory.count(ItemKind::RawMaterial,Materials[m]);
            if(quantity==0){++zeroStock[m];++zeroStreak[m];longestZero[m]=std::max(longestZero[m],zeroStreak[m]);}else zeroStreak[m]=0;
        }
        // Resource-node quantity differences are evidence of depletion, not labelled gross consumption.
        if(w.minute%1440==0 || w.generatedNaturalChunks.size()!=lastNodeScanChunks){
            for(const auto& chunk:w.generatedNaturalChunks)if(recordedChunks.insert({chunk.coord.x,chunk.coord.y}).second){
                auto j=base("chunk_materialized");j.add("chunk_x",chunk.coord.x);j.add("chunk_y",chunk.coord.y);
                j.add("materialized_minute",chunk.materializedMinute);j.add("resource_patches",chunk.resourcePatches.size());emit(j);}
            for(const auto& n:w.resourceNodes){auto it=previousNodeQuantity.find(n.id);
                if(it==previousNodeQuantity.end())newResourceNodes.insert(n.id);
                else if(newResourceNodes.count(n.id)&&n.quantity<it->second)usedNewResourceNodes.insert(n.id);
                previousNodeQuantity[n.id]=n.quantity;
            }
            lastNodeScanChunks=w.generatedNaturalChunks.size();
        }
    }
    void sample(int day,bool checkpoint){
        if(!enabled)return;const auto& w=sim.world();const auto overview=sim.observeWorldOverview();
        const auto settlements=sim.observeSettlementNetwork();const auto lifecycle=sim.observeSettlementLifecycle();
        if(settlements.residentAssignedCount>int(overview.livingResidents))invariant("assigned settlement residents exceed living population");
        for(const auto& storage:w.storageSites)for(const auto& stack:storage.inventory.stacks())if(stack.quantity<0)invariant("negative storage inventory");
        auto j=base(checkpoint?"checkpoint":"daily",day);
        j.add("total",w.characters.size());j.add("living",overview.livingResidents);j.add("deceased",overview.deceasedResidents);
        j.add("births",sim.births().all().size());j.add("pregnancies",overview.activePregnancies);j.add("dating",overview.datingCouples);
        j.add("engaged",overview.engagedCouples);j.add("married",overview.marriedCouples);j.add("households",overview.households);
        j.add("settlements",settlements.settlementCount);j.add("active_settlements",settlements.activeSettlementCount);
        j.add("assigned",settlements.residentAssignedCount);j.add("abandoned_settlements",lifecycle.abandonedCount);
        j.add("facilities",w.facilities.size());j.add("resource_nodes",w.resourceNodes.size());j.add("chunks",w.generatedNaturalChunks.size());
        j.add("chunks_per_day",day>previousDay?double(w.generatedNaturalChunks.size()-previousChunks)/(day-previousDay):0);
        j.add("events",eventSequence);j.add("event_fingerprint",hexHash(eventHash));
        j.add("social_facts",sim.socialKnowledge().facts().size());j.add("knowledge_receipts",sim.socialKnowledge().receipts().size());
        j.add("relationships",sim.relationships().all().size());
        int workers=0,dependent=0,juveniles=0,foundersLiving=0,maxGeneration=0,bornAdults=0;std::array<int,8> stages{};
        std::size_t memories=0,beliefs=0,history=0;std::vector<double> survival;
        for(const auto& c:w.characters){memories+=c.memory.entries.size();beliefs+=c.beliefs.beliefs.size();history+=c.lifeHistory.size();
            maxGeneration=std::max(maxGeneration,generation(c.id));
            if(c.alive){++stages[int(c.lifeStage)];if(lifeStageProfile(c.lifeStage).canWork)++workers;
                if(requiresDirectCare(c.lifeStage))++dependent;if(founders.count(c.id))++foundersLiving;
                if(int(c.lifeStage)<int(LifeStage::YoungAdult))++juveniles;
                if(!founders.count(c.id)&&lifeStageProfile(c.lifeStage).canWork)++bornAdults;}
            const int origin=founders.count(c.id)?startMinute:c.birthMinute;
            survival.push_back(double((c.alive?w.minute:c.deathMinute)-origin)/1440);
        }
        std::sort(survival.begin(),survival.end());double sum=0;for(double value:survival)sum+=value;
        j.add("capable_workers",workers);j.add("direct_care_dependents",dependent);j.add("founders_living",foundersLiving);
        j.add("juvenile_dependents",juveniles);j.add("non_worker_living",overview.livingResidents-workers);
        j.add("generation_depth",maxGeneration);j.add("born_after_start_adults",bornAdults);
        j.add("generation_count",w.characters.empty()?0:maxGeneration+1);
        int descendantPregnancies=0,descendantBirths=0;
        for(const auto& pregnancy:sim.pregnancies().all())if(!founders.count(pregnancy.gestationalParent))++descendantPregnancies;
        for(const auto& birth:sim.births().all())if(!founders.count(birth.parentA))++descendantBirths;
        j.add("descendant_pregnancy_records",descendantPregnancies);j.add("descendant_birth_records",descendantBirths);
        j.raw("dependent_worker_ratio",workers?std::to_string(double(juveniles)/workers):"null");
        j.add("observed_survival_mean_days",survival.empty()?0:sum/survival.size());
        j.add("observed_survival_median_days",survival.empty()?0:(survival[(survival.size()-1)/2]+survival[survival.size()/2])/2);
        j.add("memory_entries",memories);j.add("belief_entries",beliefs);j.add("life_history",history);
        Json stage;for(int i=0;i<8;++i)stage.add(lifeStageName(LifeStage(i)),stages[i]);j.raw("stages",stage.str());
        Json counts;for(const auto& e:eventCounts)counts.add(e.first,e.second);j.raw("event_counts",counts.str());
        Json budget;std::array<std::uint64_t,22> totals{};for(const auto& r:residents)for(int i=0;i<22;++i)totals[i]+=r.second.budget[i];
        for(int i=0;i<22;++i)budget.add(BudgetNames[i],totals[i]);j.raw("activity_minutes",budget.str());
        Json n;for(int i=0;i<5;++i)n.raw(NeedNames[i],needs[i].json());j.raw("needs",n.str());
        j.add("exploration_attempts",explorationAttempts);j.add("exploration_success",explorationSuccess);
        j.add("critical_exploration",criticalExploration);j.add("exploration_completed_travel_grid",explorationTravel);
        j.add("discovered_resource_nodes",newResourceNodes.size());j.add("subsequently_depleted_discovered_nodes",usedNewResourceNodes.size());
        j.add("visited_chunks",chunkFirstVisit.size());
        int radius=0;for(const auto& ch:w.generatedNaturalChunks)radius=std::max(radius,std::max(std::abs(ch.coord.x-w.initialStartRegionCoord.x),std::abs(ch.coord.y-w.initialStartRegionCoord.y)));
        j.add("chunk_bounding_chebyshev_radius",radius);
#if defined(__unix__) || defined(__APPLE__)
        struct rusage usage{};getrusage(RUSAGE_SELF,&usage);
#if defined(__APPLE__)
        j.add("peak_rss_kib",usage.ru_maxrss/1024);
#else
        j.add("peak_rss_kib",usage.ru_maxrss);
#endif
#else
        j.raw("peak_rss_kib","null");
#endif
        j.add("elapsed_wall_ms",std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-wallStart).count());
        if(checkpoint){
            auto snap=sim.captureSnapshot();std::vector<std::uint8_t> bytes,reencoded;std::string error;SimulationStateSnapshot decoded;
            const bool encoded=encodeSimulationSnapshot(snap,bytes,&error);
            if(!encoded || !decodeSimulationSnapshot(bytes,decoded,&error) || !encodeSimulationSnapshot(decoded,reencoded,&error) || bytes!=reencoded)invariant("snapshot roundtrip: "+error);
            Simulation restored(w.seed,w.populationSeed,w.generationVersion,snap.ruleset);
            if(encoded && !restored.restoreSnapshot(decoded,&error))invariant("snapshot restore: "+error);
            j.add("snapshot_bytes",bytes.size());
            const std::string raw(reinterpret_cast<const char*>(bytes.data()),bytes.size());j.add("snapshot_fnv1a64",hexHash(hashBytes(14695981039346656037ULL,raw)));
            for(const auto& entry:snap.runtime){const auto& r=entry.second;
                if(r.tradeJourney.active){auto t=base("trade_active",day);t.add("resident",idString(entry.first));t.add("returning",r.tradeJourney.returning?1:0);
                    t.add("exchanged",r.tradeJourney.exchanged?1:0);t.add("leg_started_minute",r.tradeJourney.legStartedMinute);
                    t.add("departure_minute",tradeDeparted.count(entry.first)?tradeDeparted[entry.first]:-1);emit(t);}
            }
        }
        emit(j);
        for(const auto& c:w.characters){auto& r=residents[c.id];auto x=base("resident",day);x.add("id",idString(c.id));x.add("name",c.name);x.add("alive",c.alive?1:0);
            x.add("stage",lifeStageName(c.lifeStage));x.add("founder",founders.count(c.id)?1:0);x.add("generation",generation(c.id));
            x.add("birth_minute",c.birthMinute);x.add("death_minute",c.deathMinute);x.add("observed_minutes",r.observations);
            Json b;for(int i=0;i<22;++i)b.add(BudgetNames[i],r.budget[i]);x.raw("activity_minutes",b.str());
            Json ns;for(int i=0;i<5;++i)ns.raw(NeedNames[i],r.needs[i].json());x.raw("needs",ns.str());
            x.add("illness",c.health.illnessSeverity);x.add("pathogen",c.health.pathogenLoad);x.add("immunity",c.health.immunity01);
            x.add("genetic_health_potential",c.genetics.healthPotential);x.add("physical_health",c.lifeCondition.physicalHealth);
            x.add("capacity",healthFunctionalCapacity01(c.health));x.add("mean_capacity",r.observations?r.capacitySum/r.observations:0);
            x.add("illness_minutes",r.illMinutes);x.add("care_knowledge",c.health.careKnowledge01);
            x.add("last_exposure_minute",c.health.lastExposureMinute);x.add("last_illness_minute",c.health.lastIllnessMinute);
            x.add("last_recovery_minute",c.health.lastRecoveryMinute);x.add("infection_episodes",c.health.infectionEpisodes);x.add("recoveries",c.health.recoveryEpisodes);
            x.add("pending_water_dose",c.health.pendingWaterContaminationDose);x.add("waste_exposure",w.environmentalResidues.exposureAt(r.previous));
            x.add("mean_waste_exposure",r.observations?r.wasteExposureSum/r.observations:0);x.add("x",r.previous.x);x.add("y",r.previous.y);
            x.add("route_failures",r.routeFailures);x.add("timeouts",r.timeouts);x.add("preemptions",r.preemptions);x.add("repeated_failures",r.retries);
            const auto pressure=sim.observeResidentMigrationPressure(c.id);x.add("migration_candidate",pressure.candidate?1:0);x.add("migration_pressure",pressure.pressure01);
            x.add("known_chunks_visited",r.visits.size());x.add("chunk_reentries",r.revisitedChunks);x.add("trade_missions",tradeMissions[c.id]);
            Json pairs;for(const auto& pair:r.pairs)pairs.add(idString(pair.first),pair.second);x.raw("pair_interaction_minutes",pairs.str());emit(x);
            for(int i=0;i<5;++i){auto a=base("need_resolution",day);a.add("resident",idString(c.id));a.add("need",NeedNames[i]);
                a.add("completed_sessions_observed",r.resolvedCount[i]);a.add("completed_session_minutes",r.resolvedMinutes[i]);
                a.add("completed_session_distance_grid",r.resolvedDistance[i]);a.add("moving_minutes",r.travel[i]);emit(a);}
            for(const auto& k:c.civilization.knowledge.all()){auto a=base("knowledge",day);const int t=int(k.technique);a.add("resident",idString(c.id));a.add("technique",techniqueName(k.technique));
                a.add("level",int(k.level));a.add("core_successful_uses",k.successfulUses);a.add("first_observed_minute",r.learned[t]);
                a.add("first_subsequent_use_minute",r.firstPracticalUse.count(t)?r.firstPracticalUse[t]:-1);
                a.add("days_without_subsequent_use",r.firstPracticalUse.count(t)?0:double(w.minute-r.learned[t])/1440);emit(a);}
        }
        for(std::size_t m=0;m<Materials.size();++m){auto a=base("material",day);a.add("material",materialName(Materials[m]));int natural=0,carried=0,deadCarried=0,stock=0,demand=0,delivered=0;
            for(const auto& node:w.resourceNodes)if(node.material==Materials[m])natural+=node.quantity;
            for(const auto& c:w.characters){int q=c.civilization.inventory.count(ItemKind::RawMaterial,Materials[m]);if(c.alive)carried+=q;else deadCarried+=q;}
            for(const auto& s:w.storageSites)stock+=s.inventory.count(ItemKind::RawMaterial,Materials[m]);
            for(const auto& f:w.facilities)for(const auto& req:f.requirements)if(req.material==Materials[m]){demand+=std::max(0,req.required-req.delivered);delivered+=req.delivered;}
            a.add("materialized_natural_quantity",natural);a.add("carried_living",carried);a.add("carried_dead",deadCarried);a.add("stored",stock);
            a.add("gathered_event_units",gathered[m]);a.add("stored_event_units",stored[m]);a.add("retrieved_event_units",retrieved[m]);
            a.add("construction_delivered_current",delivered);a.add("construction_outstanding_demand",demand);
            a.add("zero_stock_minutes",zeroStock[m]);a.add("longest_zero_stock_streak",longestZero[m]);
            // These gross flows cannot be distinguished from inventory deltas alone.
            a.raw("consumed","null");a.raw("produced","null");a.raw("spoiled","null");a.raw("trade_sent","null");a.raw("trade_received","null");emit(a);
        }
        for(const auto& f:w.facilities){auto a=base("facility",day);a.add("id",idString(f.id));a.add("kind",facilityName(f.kind));a.add("state",int(f.state));
            a.add("operational",facilityOperationalAndActive(f)?1:0);a.add("durability",f.durability);a.add("usage_count",f.usageCount);a.add("last_used_minute",f.lastUsedMinute);
            a.add("started_minute",f.startedMinute);a.add("completed_minute",f.completedMinute);a.add("x",f.pos.x);a.add("y",f.pos.y);
            if(durability.count(f.id)&&f.durability>durability[f.id])++repairCounts[f.id];durability[f.id]=f.durability;
            a.add("daily_durability_increase_observations",repairCounts[f.id]);emit(a);}
        for(const auto& s:settlements.settlements){auto a=base("settlement",day);a.add("id",idString(s.id));a.add("residents",s.residentCount);a.add("facilities",s.facilityCount);
            a.add("operational_facilities",s.operationalFacilityCount);a.add("active",s.active?1:0);a.add("x",s.anchor.x);a.add("y",s.anchor.y);
            int water=0,food=0;for(const auto& storage:w.storageSites)if(settlementClusterServesPosition(s,storage.pos)){
                water+=storage.inventory.count(ItemKind::RawMaterial,MaterialKind::Water);food+=storage.inventory.count(ItemKind::RawMaterial,MaterialKind::PlantFood);}
            a.add("service_area_stored_water",water);a.add("service_area_stored_food",food);emit(a);}
        double waste=0;for(const auto& residue:w.environmentalResidues.all())waste+=residue.amount;
        auto h=base("sanitation",day);h.add("residue_records",w.environmentalResidues.all().size());h.add("waste_amount",waste);
        int active=0,uses=0;for(const auto& site:w.primitiveSanitationSites){active+=site.active?1:0;uses+=site.useCount;}h.add("active_sites",active);h.add("usage_count",uses);emit(h);
        const auto society=sim.observeSocietyWorld();auto so=base("society",day);
        so.add("specialized_residents",society.specializedResidentCount);so.add("educators",society.educatorCount);
        so.add("caregivers",society.caregiverCount);so.add("producers",society.producerCount);so.add("storekeepers",society.storekeeperCount);
        so.add("exchange_facts",society.exchangeFactCount);so.add("apprenticeships",society.apprenticeshipCount);
        so.add("institution_memberships",society.institutionMembershipCount);so.add("active_institutions",society.activeInstitutionCount);
        so.add("shared_contribution_facts",society.sharedContributionFactCount);so.add("durable_record_facts",society.durableRecordFactCount);
        so.add("record_media_units",society.recordMediaUnits);so.add("coordinated_residents",society.coordinatedResidentCount);emit(so);
        if(checkpoint){const auto civilization=sim.observeCivilizationWorld();auto tech=base("technology_population",day);
            tech.add("known_types",civilization.uniqueKnownTechniqueTypes);tech.add("reproducible_types",civilization.uniqueReproducibleTechniqueTypes);
            tech.add("common_technologies",civilization.commonTechnologyCount);tech.add("lost_technologies",civilization.lostTechnologyCount);
            tech.add("declining_technologies",civilization.decliningTechnologyCount);tech.add("established_technologies",civilization.establishedTechnologyCount);emit(tech);
            for(const auto& c:w.characters){const auto observation=sim.observeResidentCivilization(c.id);auto cap=base("capabilities",day);
                cap.add("resident",idString(c.id));cap.add("available",observation.availableCapabilityCount);
                cap.add("known_technologies",observation.knownTechnologyCount);cap.add("operational",observation.operationalTechnologyCount);cap.add("adopted",observation.adoptedTechnologyCount);emit(cap);}
        }
        previousChunks=w.generatedNaturalChunks.size();previousDay=day;records.flush();events.flush();
        if(!records || !events)throw std::runtime_error("system audit write failed");
    }
    bool finish(){if(enabled){auto j=base("complete");j.add("invariant_failed",failed?1:0);j.add("event_fingerprint",hexHash(eventHash));emit(j);records.flush();events.flush();}return !failed;}
};
} // namespace lifelens::audit
