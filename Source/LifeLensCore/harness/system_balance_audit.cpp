#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "lifelens/FamilyProgression.h"
#include "lifelens/Health.h"
#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

namespace {

using namespace lifelens;
constexpr int MinutesPerDay=24*60;
constexpr int NeedHistogramBins=101;

constexpr std::array<MaterialKind,12> AuditedMaterials{{
    MaterialKind::PlantFood,MaterialKind::Water,MaterialKind::Wood,
    MaterialKind::Stone,MaterialKind::Fiber,MaterialKind::Clay,
    MaterialKind::Flint,MaterialKind::CopperOre,MaterialKind::TinOre,
    MaterialKind::Charcoal,MaterialKind::CopperMetal,MaterialKind::Bronze
}};

const char* materialLabel(MaterialKind m){
    switch(m){
        case MaterialKind::PlantFood:return "food";
        case MaterialKind::Water:return "water";
        case MaterialKind::Wood:return "wood";
        case MaterialKind::Stone:return "stone";
        case MaterialKind::Fiber:return "fiber";
        case MaterialKind::Clay:return "clay";
        case MaterialKind::Flint:return "flint";
        case MaterialKind::CopperOre:return "copperOre";
        case MaterialKind::TinOre:return "tinOre";
        case MaterialKind::Charcoal:return "charcoal";
        case MaterialKind::CopperMetal:return "copperMetal";
        case MaterialKind::Bronze:return "bronze";
        default:return "other";
    }
}
const char* facilityLabel(FacilityKind k){
    switch(k){
        case FacilityKind::PrimitiveStorage:return "primitiveStorage";
        case FacilityKind::FirePit:return "firePit";
        case FacilityKind::WorkSurface:return "workSurface";
        case FacilityKind::SleepingPlace:return "sleepingPlace";
        case FacilityKind::Shelter:return "shelter";
        case FacilityKind::Furnace:return "furnace";
        case FacilityKind::CultivatedPlot:return "cultivatedPlot";
    }
    return "unknown";
}
std::size_t materialIndex(MaterialKind material){
    for(std::size_t i=0;i<AuditedMaterials.size();++i) if(AuditedMaterials[i]==material) return i;
    return AuditedMaterials.size();
}

enum class TimeBucket : std::size_t {
    Survival=0,Gather,Storage,ConstructionRepair,Production,Cultivation,
    Social,DatingFamily,ParentingCare,TeachingLearning,InstitutionEconomy,
    Exploration,Migration,Trade,Idle,Count
};
constexpr std::array<const char*,static_cast<std::size_t>(TimeBucket::Count)> TimeBucketNames{{
    "survival","gatherResource","storageLogistics","constructionRepair",
    "productionCrafting","cultivation","social","datingFamily","parentingCare",
    "teachingLearning","institutionEconomy","exploration","migration","trade",
    "idleWaiting"
}};

struct ResidentAudit {
    std::uint64_t observedMinutes=0;
    std::array<double,5> needSum{};
    std::array<double,5> needMax{};
    std::array<std::array<std::uint64_t,NeedHistogramBins>,5> needHistogram{};
    std::array<std::uint64_t,5> urgentMinutes{};
    std::array<std::uint64_t,5> urgentEntries{};
    std::array<bool,5> urgentNow{};
    std::array<std::uint64_t,2> criticalMinutes{};
    std::array<std::uint64_t,2> criticalEntries{};
    std::array<bool,2> criticalNow{};
    std::array<std::uint64_t,5> saturatedMinutes{};
    std::array<std::uint64_t,5> travelDistance{};
    std::array<std::uint64_t,static_cast<std::size_t>(TimeBucket::Count)> time{};
    int lastCivilizationActivityMinute=-1;
    std::uint64_t lastExplorationContextToken=0;
    GridPos previousPosition{};
    bool hasPreviousPosition=false;
    int illnessMinutes=0;
    int firstExposureMinute=-1;
    int firstPathogenMinute=-1;
    int firstIllnessMinute=-1;
    int firstCareMinute=-1;
    int firstRecoveryMinute=-1;
    int deathMinute=-1;
    double maxPathogen=0.0,maxIllness=0.0,maxContaminationDose=0.0,maxEnvironmentalStress=0.0,maxImmunity=0.0;
};

struct MaterialFlow {
    std::uint64_t gathered=0;
    std::uint64_t produced=0;
    std::uint64_t consumed=0;
    std::uint64_t constructionConsumed=0;
    std::uint64_t stored=0;
    std::uint64_t retrieved=0;
    std::uint64_t tradedTransfer=0;
    std::uint64_t shortageSampleMinutes=0;
};

struct EventAudit {
    std::uint64_t preemptions=0,routeFailures=0,timeouts=0;
    std::uint64_t socialEvents=0,civilizationEvents=0;
    std::uint64_t migrationStarts=0,migrationAssignmentTransitions=0;
    std::uint64_t tradeDepartures=0,tradeArrivals=0,tradeExchanges=0,tradeReturns=0,tradeFailed=0,tradeCancelled=0;
    std::uint64_t illnesses=0,recoveries=0,careEvents=0;
    std::array<std::uint64_t,5> survivalStarts{};
    std::array<std::uint64_t,5> survivalCompletions{};
    std::array<std::uint64_t,5> deaths{}; // illness accident exposure deprivation other
    std::uint64_t explorationAttempts=0,explorationSuccess=0,criticalExploration=0,ordinaryExploration=0;
    std::uint64_t explorationDistanceSum=0,explorationDistanceMax=0;
    int firstCritical=-1,firstIllness=-1,firstDeath=-1,firstBirth=-1,firstMarriage=-1,firstPregnancy=-1;
    int firstSecondSettlement=-1,firstChunkGrowth=-1,firstTradeDeparture=-1,firstTradeExchange=-1,firstTradeReturn=-1;
};

struct FacilityRuntime {
    double durability=1.0;
    FacilityState state=FacilityState::Planned;
    int usageCount=0;
};
struct WorldRuntimeAudit {
    std::map<FacilityId,FacilityRuntime> facilities;
    std::map<std::pair<FacilityId,MaterialKind>,int> delivered;
    std::map<CharacterId,SettlementClusterId> settlementAssignment;
    std::map<TechniqueId,int> firstKnownMinute;
    std::map<TechniqueId,int> firstUseMinute;
    std::set<ChunkCoord,bool(*)(const ChunkCoord&,const ChunkCoord&)> seenChunks{
        [](const ChunkCoord&a,const ChunkCoord&b){return a.x<b.x || (a.x==b.x && a.y<b.y);}
    };
    std::uint64_t repairObservations=0;
    std::size_t initialChunkCount=0;
};

int needBin(double v){
    return std::clamp(static_cast<int>(std::lround(std::clamp(v,0.0,1.0)*100.0)),0,100);
}
double percentile(const std::array<std::uint64_t,NeedHistogramBins>& h,double p){
    const std::uint64_t total=std::accumulate(h.begin(),h.end(),std::uint64_t{0});
    if(total==0) return 0.0;
    const std::uint64_t target=std::max<std::uint64_t>(1,static_cast<std::uint64_t>(std::ceil(total*p)));
    std::uint64_t seen=0;
    for(std::size_t i=0;i<h.size();++i){seen+=h[i];if(seen>=target)return static_cast<double>(i)/100.0;}
    return 1.0;
}
std::array<double,5> needs(const Needs& n){return {n.hunger,n.thirst,n.sleep,n.bladder,n.hygiene};}
std::size_t goalIndex(Goal g){
    switch(g){case Goal::Eat:return 0;case Goal::Drink:return 1;case Goal::Sleep:return 2;case Goal::UseToilet:return 3;case Goal::Wash:return 4;default:return 4;}
}
bool activePartner(const Simulation& sim,CharacterId a,CharacterId b){
    if(a==0||b==0) return false;
    for(const auto& pair:sim.romances().all()) if(pair.active() && pair.contains(a) && pair.contains(b)) return true;
    return false;
}
bool sameHousehold(const Simulation& sim,CharacterId a,CharacterId b){
    const Household* x=sim.households().householdOf(a);
    const Household* y=sim.households().householdOf(b);
    return x&&y&&x->id==y->id;
}
TimeBucket classifyTime(Simulation& sim,const Character& c,const ResidentPresentationObservation& p){
    if(!p.active || p.phase==PresentationActionPhase::Idle) return TimeBucket::Idle;
    switch(p.kind){
        case PresentationActionKind::Physical:return TimeBucket::Survival;
        case PresentationActionKind::Social:
            return (activePartner(sim,c.id,p.targetResidentId)||sameHousehold(sim,c.id,p.targetResidentId))
                ? TimeBucket::DatingFamily:TimeBucket::Social;
        case PresentationActionKind::Parenting:return TimeBucket::ParentingCare;
        case PresentationActionKind::KnowledgeTeaching:return TimeBucket::TeachingLearning;
        case PresentationActionKind::Trade:return TimeBucket::Trade;
        case PresentationActionKind::Civilization:{
            switch(p.facilityAction){
                case FacilityBuildAction::Plan:
                case FacilityBuildAction::DeliverMaterial:
                case FacilityBuildAction::Work:
                case FacilityBuildAction::Repair:
                    return TimeBucket::ConstructionRepair;
                case FacilityBuildAction::Plant:
                case FacilityBuildAction::Water:
                case FacilityBuildAction::Tend:
                case FacilityBuildAction::Harvest:
                    return TimeBucket::Cultivation;
                case FacilityBuildAction::Fuel:
                case FacilityBuildAction::Ignite:
                case FacilityBuildAction::CollectCharcoal:
                case FacilityBuildAction::LoadSmeltCharge:
                case FacilityBuildAction::CollectMetal:
                    return TimeBucket::Production;
                case FacilityBuildAction::None:
                default: break;
            }
            switch(p.civilizationIntent){
                case CivilizationIntent::Gather:return TimeBucket::Gather;
                case CivilizationIntent::Store:
                case CivilizationIntent::Retrieve:return TimeBucket::Storage;
                case CivilizationIntent::Explore:return TimeBucket::Exploration;
                case CivilizationIntent::Experiment:
                case CivilizationIntent::Craft:return TimeBucket::Production;
                case CivilizationIntent::None:
                default:return TimeBucket::Production;
            }
        }
        case PresentationActionKind::None:
        default:return TimeBucket::Idle;
    }
}
int natural(const World&w,MaterialKind m){int n=0;for(const auto&x:w.resourceNodes)if(x.material==m)n+=std::max(0,x.quantity);return n;}
int carried(const World&w,MaterialKind m){int n=0;for(const auto&c:w.characters)n+=c.civilization.inventory.count(ItemKind::RawMaterial,m);return n;}
int stored(const World&w,MaterialKind m){int n=0;for(const auto&s:w.storageSites)n+=s.inventory.count(ItemKind::RawMaterial,m);return n;}
std::size_t snapshotBytes(Simulation& sim){
    std::vector<std::uint8_t>b;std::string e;
    return encodeSimulationSnapshot(sim.captureSnapshot(),b,&e)?b.size():0;
}
int generationDepth(const World&w,CharacterId id,std::set<CharacterId>&vis){
    if(vis.count(id)) return 0;vis.insert(id);
    const Character* c=nullptr;for(const auto&x:w.characters)if(x.id==id){c=&x;break;}
    if(!c||c->parentIds.empty()){vis.erase(id);return 1;}
    int best=1;for(CharacterId p:c->parentIds)best=std::max(best,1+generationDepth(w,p,vis));
    vis.erase(id);return best;
}
int maxGeneration(const World&w){int g=0;for(const auto&c:w.characters){std::set<CharacterId>v;g=std::max(g,generationDepth(w,c.id,v));}return g;}
std::vector<double> deceasedSurvivalDays(const World&w){
    std::vector<double> out;
    for(const auto&c:w.characters){
        if(c.alive)continue;
        int death=-1,birth=0;
        for(const auto&e:c.lifeHistory){if(e.type==LifeEventType::Birth)birth=e.minute;if(e.type==LifeEventType::Death)death=e.minute;}
        if(death>=0)out.push_back(static_cast<double>(std::max(0,death-birth))/MinutesPerDay);
    }
    std::sort(out.begin(),out.end());return out;
}
double median(std::vector<double>v){if(v.empty())return 0;std::sort(v.begin(),v.end());return v.size()%2?v[v.size()/2]:(v[v.size()/2-1]+v[v.size()/2])*0.5;}
SettlementClusterId assignmentFor(const SettlementNetworkObservation& net,GridPos p){
    SettlementClusterId best=0;int dist=std::numeric_limits<int>::max();
    for(const auto&s:net.settlements){
        if(!s.active)continue;const int d=settlementClusterServiceDistance(s,p);
        if(d<dist){dist=d;best=s.id;}
    }
    return dist<=SettlementServiceRadiusGrid?best:0;
}

void observeResidentMinute(Simulation& sim,Character& c,ResidentAudit& a){
    ++a.observedMinutes;
    const auto n=needs(c.needs);
    const double urgent=sim.ruleset().utilityAI.urgentThreshold;
    for(std::size_t i=0;i<n.size();++i){
        a.needSum[i]+=n[i];a.needMax[i]=std::max(a.needMax[i],n[i]);++a.needHistogram[i][needBin(n[i])];
        const bool u=n[i]>=urgent;
        if(u){++a.urgentMinutes[i];if(!a.urgentNow[i])++a.urgentEntries[i];}
        a.urgentNow[i]=u;if(n[i]>=0.999)++a.saturatedMinutes[i];
    }
    for(std::size_t i=0;i<2;++i){
        const bool critical=n[i]>=CriticalSurvivalPreemptThreshold;
        if(critical){++a.criticalMinutes[i];if(!a.criticalNow[i])++a.criticalEntries[i];}
        a.criticalNow[i]=critical;
    }
    GridPos pos{};const bool hasPos=sim.runtimePosition(c.id,pos);
    const auto p=sim.observeResidentPresentation(c.id);
    if(hasPos&&a.hasPreviousPosition&&p.kind==PresentationActionKind::Physical&&p.phase==PresentationActionPhase::Moving){
        const std::size_t gi=goalIndex(p.physicalGoal);a.travelDistance[gi]+=manhattan(a.previousPosition,pos);
    }
    if(hasPos){a.previousPosition=pos;a.hasPreviousPosition=true;}
    ++a.time[static_cast<std::size_t>(classifyTime(sim,c,p))];

    if(c.health.lastExposureMinute>=0&&a.firstExposureMinute<0)a.firstExposureMinute=c.health.lastExposureMinute;
    if(c.health.pathogenLoad>0&&a.firstPathogenMinute<0)a.firstPathogenMinute=sim.world().minute;
    if(c.health.illnessSeverity>0.0){++a.illnessMinutes;if(a.firstIllnessMinute<0)a.firstIllnessMinute=sim.world().minute;}
    if(c.health.lastRecoveryMinute>=0&&a.firstRecoveryMinute<0)a.firstRecoveryMinute=c.health.lastRecoveryMinute;
    if(p.kind==PresentationActionKind::Parenting&&p.parentingAction==ParentingAction::HealthCare&&p.targetResidentId!=0){
        // target care timing is assigned by caller after all residents exist
    }
    a.maxPathogen=std::max(a.maxPathogen,c.health.pathogenLoad);
    a.maxIllness=std::max(a.maxIllness,c.health.illnessSeverity);
    a.maxContaminationDose=std::max(a.maxContaminationDose,c.health.pendingWaterContaminationDose);
    a.maxEnvironmentalStress=std::max(a.maxEnvironmentalStress,c.health.environmentalStress01);
    a.maxImmunity=std::max(a.maxImmunity,c.health.immunity01);
    if(!c.alive&&a.deathMinute<0){
        for(auto it=c.lifeHistory.rbegin();it!=c.lifeHistory.rend();++it)if(it->type==LifeEventType::Death){a.deathMinute=it->minute;break;}
    }
}

void observeCivilizationEvents(Simulation& sim,std::map<CharacterId,ResidentAudit>& residents,
    std::array<MaterialFlow,AuditedMaterials.size()>& flows,EventAudit& ev,WorldRuntimeAudit& rt){
    const int minute=sim.world().minute;
    for(const auto&c:sim.world().characters){
        ResidentAudit& resident=residents[c.id];
        const auto p=sim.observeResidentPresentation(c.id);
        if(p.kind==PresentationActionKind::Civilization
           && p.civilizationIntent==CivilizationIntent::Explore
           && p.contextActionToken!=0
           && p.contextActionToken!=resident.lastExplorationContextToken){
            resident.lastExplorationContextToken=p.contextActionToken;
            ++ev.explorationAttempts;
            const bool critical=
                c.needs.hunger>=CriticalSurvivalPreemptThreshold
                || c.needs.thirst>=CriticalSurvivalPreemptThreshold;
            if(critical)++ev.criticalExploration;else ++ev.ordinaryExploration;
        }

        const auto a=sim.observeResidentCivilizationActivity(c.id);
        if(a.active && a.minute>=0 && a.minute!=resident.lastCivilizationActivityMinute){
            resident.lastCivilizationActivityMinute=a.minute;
            const auto mi=materialIndex(a.material);
            if(a.kind==CivilizationActivityKind::Gather&&a.success){
                if(mi<flows.size())flows[mi].gathered+=std::max(0,a.quantity);
            }else if(a.kind==CivilizationActivityKind::Store&&a.success){
                if(mi<flows.size())flows[mi].stored+=std::max(0,a.quantity);
            }else if(a.kind==CivilizationActivityKind::Retrieve&&a.success){
                if(mi<flows.size())flows[mi].retrieved+=std::max(0,a.quantity);
            }else if(a.kind==CivilizationActivityKind::Craft&&a.success){
                if(mi<flows.size())flows[mi].produced+=std::max(0,a.quantity);
            }else if(a.kind==CivilizationActivityKind::Explore&&a.success){
                ++ev.explorationSuccess;
                GridPos pos{};
                if(sim.runtimePosition(c.id,pos)){
                    int d=0;
                    const auto net=sim.observeSettlementNetwork();
                    if(!net.settlements.empty()){
                        d=std::numeric_limits<int>::max();
                        for(const auto&s:net.settlements){
                            d=std::min(d,settlementClusterServiceDistance(s,pos));
                        }
                        if(d==std::numeric_limits<int>::max())d=0;
                    }
                    ev.explorationDistanceSum+=std::max(0,d);
                    ev.explorationDistanceMax=std::max<std::uint64_t>(
                        ev.explorationDistanceMax,std::max(0,d));
                }
            }
            if(a.technique!=TechniqueId::None&&a.success){
                const auto civ=sim.observeResidentCivilization(c.id);
                for(const auto&t:civ.techniques){
                    if(t.technique==a.technique&&t.successfulUses>0
                       &&!rt.firstUseMinute.count(a.technique)){
                        rt.firstUseMinute[a.technique]=a.minute;
                    }
                }
            }
        }

        if(p.kind==PresentationActionKind::Parenting
           &&p.parentingAction==ParentingAction::HealthCare
           &&p.targetResidentId!=0){
            auto it=residents.find(p.targetResidentId);
            if(it!=residents.end()&&it->second.firstCareMinute<0){
                it->second.firstCareMinute=minute;
            }
        }

        if(minute%60!=0) continue;
        const auto civ=sim.observeResidentCivilization(c.id);
        for(const auto&t:civ.techniques){
            if(t.technique!=TechniqueId::None&&!rt.firstKnownMinute.count(t.technique)){
                rt.firstKnownMinute[t.technique]=t.learnedMinute>=0?t.learnedMinute:minute;
            }
            if(t.successfulUses>0&&!rt.firstUseMinute.count(t.technique)){
                rt.firstUseMinute[t.technique]=minute;
            }
        }
    }
}
void sampleHourly(Simulation& sim,std::array<MaterialFlow,AuditedMaterials.size()>& flows,EventAudit&ev,WorldRuntimeAudit&rt){
    const World&w=sim.world();
    for(std::size_t i=0;i<AuditedMaterials.size();++i){
        const SocietyDemandSignal demand=observeSocietyMaterialDemand(w,AuditedMaterials[i]);
        if(demand.deficitUnits>0)flows[i].shortageSampleMinutes+=60;
    }
    for(const auto&f:w.facilities){
        auto &prev=rt.facilities[f.id];
        if(prev.state==FacilityState::Operational&&f.state==FacilityState::Operational&&f.durability>prev.durability+1e-9)++rt.repairObservations;
        prev={f.durability,f.state,f.usageCount};
        for(const auto&r:f.requirements){
            auto key=std::make_pair(f.id,r.material);
            const int before=rt.delivered.count(key)?rt.delivered[key]:0;
            if(r.delivered>before){const auto mi=materialIndex(r.material);if(mi<flows.size())flows[mi].constructionConsumed+=static_cast<std::uint64_t>(r.delivered-before);}
            rt.delivered[key]=r.delivered;
        }
    }
    const auto net=sim.observeSettlementNetwork();
    for(const auto&c:w.characters){
        if(!c.alive)continue;GridPos p{};if(!sim.runtimePosition(c.id,p))continue;
        const auto now=assignmentFor(net,p);
        auto it=rt.settlementAssignment.find(c.id);
        if(it!=rt.settlementAssignment.end()&&it->second!=0&&now!=0&&it->second!=now)++ev.migrationAssignmentTransitions;
        rt.settlementAssignment[c.id]=now;
    }
    if(net.activeSettlementCount>=2&&ev.firstSecondSettlement<0)ev.firstSecondSettlement=w.minute;
    if(w.generatedNaturalChunks.size()>1&&ev.firstChunkGrowth<0)ev.firstChunkGrowth=w.minute;
}
void updateFamilyTimeline(const Simulation&sim,EventAudit&ev){
    for(const auto&c:sim.world().characters)for(const auto&e:c.lifeHistory){
        if(e.type==LifeEventType::ChildBorn&&(ev.firstBirth<0||e.minute<ev.firstBirth))ev.firstBirth=e.minute;
        if(e.type==LifeEventType::Married&&(ev.firstMarriage<0||e.minute<ev.firstMarriage))ev.firstMarriage=e.minute;
        if(e.type==LifeEventType::PregnancyStarted&&(ev.firstPregnancy<0||e.minute<ev.firstPregnancy))ev.firstPregnancy=e.minute;
        if(e.type==LifeEventType::Death&&(ev.firstDeath<0||e.minute<ev.firstDeath))ev.firstDeath=e.minute;
    }
}

void emitCheckpoint(Simulation&sim,std::uint64_t seed,int day,
    const std::map<CharacterId,ResidentAudit>&residents,
    const std::array<MaterialFlow,AuditedMaterials.size()>&flows,
    const EventAudit&ev,const WorldRuntimeAudit&rt,
    std::chrono::steady_clock::time_point start){
    const World&w=sim.world();const auto ov=sim.observeWorldOverview();const auto net=sim.observeSettlementNetwork();
    const auto life=sim.observeSettlementLifecycle();const auto trade=sim.observeSettlementTradeNetwork();
    const auto civ=sim.observeCivilizationWorld(64);const auto society=sim.observeSocietyWorld();
    std::array<std::uint64_t,static_cast<std::size_t>(TimeBucket::Count)> tb{};
    std::array<std::array<std::uint64_t,NeedHistogramBins>,5> hist{};
    std::array<double,5> needSum{};std::array<std::uint64_t,5> needObs{},urgent{},entries{},sat{},travel{};
    std::array<std::uint64_t,2> criticalMinutes{},criticalEntries{};
    int capable=0,dependent=0;double pathogenSum=0,illnessSum=0,immunitySum=0;int livingHealth=0;
    for(const auto&c:w.characters){
        if(c.alive){if(lifeStageProfile(c.lifeStage).canWork)++capable;else ++dependent;
            pathogenSum+=c.health.pathogenLoad;illnessSum+=c.health.illnessSeverity;immunitySum+=c.health.immunity01;++livingHealth;}
        auto it=residents.find(c.id);if(it==residents.end())continue;const auto&a=it->second;
        for(std::size_t b=0;b<tb.size();++b)tb[b]+=a.time[b];
        for(std::size_t i=0;i<2;++i){criticalMinutes[i]+=a.criticalMinutes[i];criticalEntries[i]+=a.criticalEntries[i];}
        for(std::size_t n=0;n<5;++n){needSum[n]+=a.needSum[n];needObs[n]+=a.observedMinutes;urgent[n]+=a.urgentMinutes[n];entries[n]+=a.urgentEntries[n];sat[n]+=a.saturatedMinutes[n];travel[n]+=a.travelDistance[n];for(int k=0;k<NeedHistogramBins;++k)hist[n][k]+=a.needHistogram[n][k];}
    }
    auto survival=deceasedSurvivalDays(w);double avgSurv=survival.empty()?0:std::accumulate(survival.begin(),survival.end(),0.0)/survival.size();
    int sanitationActive=0,sanitationUses=0;for(const auto&s:w.primitiveSanitationSites){if(s.active)++sanitationActive;sanitationUses+=s.useCount;}
    int contaminatedWaterNodes=0,exposureResidents=0;double maxWaterContamination=0.0;
    for(const auto&node:w.resourceNodes){if(node.material!=MaterialKind::Water||node.quantity<=0)continue;GridPos access{};if(!resolveCivilizationResourceAccessGridPosition(w,node.id,access))continue;const double contamination=w.environmentalResidues.exposureAt(access);maxWaterContamination=std::max(maxWaterContamination,contamination);if(contamination>0.01)++contaminatedWaterNodes;}
    for(const auto&c:w.characters)if(c.health.lastExposureMinute>=0)++exposureResidents;
    int opFacilities=0,unusedOp=0,damaged=0,ruined=0,constructed=0,totalUses=0;
    std::array<int,7> facilityKinds{};
    for(const auto&f:w.facilities){if(f.state==FacilityState::Operational){++opFacilities;if(f.usageCount==0)++unusedOp;}if(f.state==FacilityState::Ruined)++ruined;if(f.durability<0.999&&f.state!=FacilityState::Ruined)++damaged;if(f.completedMinute>=0)++constructed;totalUses+=f.usageCount;++facilityKinds[static_cast<std::size_t>(f.kind)];}
    int minX=0,maxX=0,minY=0,maxY=0;bool hasChunk=false;int newly1=0,newly7=0,newly30=0,usedChunks=0,discoveredNodes=0,usedDiscoveredNodes=0;
    for(const auto&ch:w.generatedNaturalChunks){if(!hasChunk){minX=maxX=ch.coord.x;minY=maxY=ch.coord.y;hasChunk=true;}else{minX=std::min(minX,ch.coord.x);maxX=std::max(maxX,ch.coord.x);minY=std::min(minY,ch.coord.y);maxY=std::max(maxY,ch.coord.y);}
        const int age=w.minute-ch.materializedMinute;if(age<=MinutesPerDay)++newly1;if(age<=7*MinutesPerDay)++newly7;if(age<=30*MinutesPerDay)++newly30;
        bool used=false;for(const auto&p:ch.resourcePatches){++discoveredNodes;for(const auto&node:w.resourceNodes)if(node.id==p.nodeId&&node.quantity<node.maxQuantity){used=true;++usedDiscoveredNodes;break;}}
        for(const auto&f:w.facilities)if(chunkCoordForGrid(f.pos).x==ch.coord.x&&chunkCoordForGrid(f.pos).y==ch.coord.y){used=true;break;}
        if(used)++usedChunks;
    }
    int maxLivedDistance=0;for(const auto&c:w.characters)if(c.alive){GridPos p{};if(sim.runtimePosition(c.id,p)&&!net.settlements.empty()){int d=std::numeric_limits<int>::max();for(const auto&s:net.settlements)d=std::min(d,settlementClusterServiceDistance(s,p));if(d<std::numeric_limits<int>::max())maxLivedDistance=std::max(maxLivedDistance,d);}}
    int neverUsed=0,usedKnowledge=0;for(const auto&kv:rt.firstKnownMinute){if(rt.firstUseMinute.count(kv.first))++usedKnowledge;else ++neverUsed;}
    const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
    const std::uint64_t totalTime=std::accumulate(tb.begin(),tb.end(),std::uint64_t{0});
    std::cout<<"AUDIT_CHECKPOINT seed="<<seed<<" day="<<day<<" minute="<<w.minute
      <<" totalPopulation="<<ov.totalResidents<<" living="<<ov.livingResidents<<" deceased="<<ov.deceasedResidents
      <<" births="<<(ov.totalResidents>=4?ov.totalResidents-4:0)<<" pregnancies="<<ov.activePregnancies<<" married="<<ov.marriedCouples
      <<" households="<<ov.households<<" baby="<<ov.lifeStages.baby<<" toddler="<<ov.lifeStages.toddler<<" child="<<ov.lifeStages.child
      <<" teen="<<ov.lifeStages.teen<<" youngAdult="<<ov.lifeStages.youngAdult<<" adult="<<ov.lifeStages.adult<<" middleAge="<<ov.lifeStages.middleAge<<" elder="<<ov.lifeStages.elderly
      <<" capableWorkers="<<capable<<" dependents="<<dependent<<" dependentWorkerRatio="<<std::fixed<<std::setprecision(6)<<(capable?static_cast<double>(dependent)/capable:static_cast<double>(dependent))
      <<" generationCount="<<maxGeneration(w)<<" avgObservedSurvivalDays="<<avgSurv<<" medianObservedSurvivalDays="<<median(survival)
      <<" snapshotBytes="<<snapshotBytes(sim)<<" elapsedMs="<<elapsed<<" resourceNodes="<<w.resourceNodes.size()<<" facilities="<<w.facilities.size()
      <<" chunks="<<w.generatedNaturalChunks.size()<<" historyEntries="<<([&](){std::size_t n=0;for(const auto&c:w.characters)n+=c.lifeHistory.size();return n;})()
      <<" socialFacts="<<sim.socialKnowledge().facts().size()<<" logs="<<sim.logs().size()<<"\n";

    std::cout<<"AUDIT_TIME seed="<<seed<<" day="<<day<<" totalResidentMinutes="<<totalTime;
    for(std::size_t i=0;i<tb.size();++i)std::cout<<" "<<TimeBucketNames[i]<<"Min="<<tb[i]<<" "<<TimeBucketNames[i]<<"Pct="<<(totalTime?100.0*tb[i]/totalTime:0.0);
    std::cout<<"\n";
    const std::array<const char*,5> nn{{"hunger","thirst","fatigue","bladder","hygiene"}};
    for(std::size_t i=0;i<5;++i){
        std::cout<<"AUDIT_NEED seed="<<seed<<" day="<<day<<" need="<<nn[i]<<" avg="<<(needObs[i]?needSum[i]/needObs[i]:0)
          <<" p50="<<percentile(hist[i],0.50)<<" p90="<<percentile(hist[i],0.90)<<" p95="<<percentile(hist[i],0.95)<<" p99="<<percentile(hist[i],0.99)
          <<" urgentEntries="<<entries[i]<<" urgentMinutes="<<urgent[i]
          <<" criticalEntries="<<(i<2?criticalEntries[i]:0)<<" criticalMinutes="<<(i<2?criticalMinutes[i]:0)
          <<" saturatedMinutes="<<sat[i]<<" travelDistance="<<travel[i]
          <<" completions="<<ev.survivalCompletions[i]<<" avgTravelPerCompletion="<<(ev.survivalCompletions[i]?static_cast<double>(travel[i])/ev.survivalCompletions[i]:0.0)<<"\n";
    }
    for(std::size_t i=0;i<AuditedMaterials.size();++i){
        const auto m=AuditedMaterials[i];const auto&f=flows[i];
        const SocietyDemandSignal demand=observeSocietyMaterialDemand(w,m);
        std::cout<<"AUDIT_RESOURCE seed="<<seed<<" day="<<day<<" material="<<materialLabel(m)
          <<" accessibleWorld="<<natural(w,m)<<" carried="<<carried(w,m)<<" stored="<<stored(w,m)
          <<" desiredUnits="<<demand.desiredUnits<<" societyAvailableUnits="<<demand.availableUnits<<" deficitUnits="<<demand.deficitUnits<<" demand01="<<demand.demand01
          <<" gathered="<<f.gathered<<" produced="<<f.produced<<" consumed="<<f.consumed<<" constructionConsumed="<<f.constructionConsumed
          <<" storedFlow="<<f.stored<<" retrievedFlow="<<f.retrieved<<" tradedTransfer="<<f.tradedTransfer
          <<" spoiledLostObserved=-1 shortageSampleMinutes="<<f.shortageSampleMinutes<<"\n";
    }
    std::cout<<"AUDIT_FACILITY seed="<<seed<<" day="<<day<<" total="<<w.facilities.size()<<" operational="<<opFacilities<<" damaged="<<damaged<<" ruined="<<ruined
      <<" constructed="<<constructed<<" repairObservations="<<rt.repairObservations<<" totalUses="<<totalUses<<" unusedOperational="<<unusedOp
      <<" unusedOperationalRatio="<<(opFacilities?static_cast<double>(unusedOp)/opFacilities:0.0)<<" perLiving="<<(ov.livingResidents?static_cast<double>(w.facilities.size())/ov.livingResidents:0.0);
    for(std::size_t i=0;i<facilityKinds.size();++i)std::cout<<" "<<facilityLabel(static_cast<FacilityKind>(i))<<"="<<facilityKinds[i];
    std::cout<<"\n";
    std::cout<<"AUDIT_WORLD_GROWTH seed="<<seed<<" day="<<day<<" explorationAttempts="<<ev.explorationAttempts<<" explorationSuccess="<<ev.explorationSuccess
      <<" ordinaryExploration="<<ev.ordinaryExploration<<" criticalExploration="<<ev.criticalExploration<<" explorationMeanDistance="<<(ev.explorationSuccess?static_cast<double>(ev.explorationDistanceSum)/ev.explorationSuccess:0.0)
      <<" explorationMaxDistance="<<ev.explorationDistanceMax<<" maxDistanceFromSettlement="<<maxLivedDistance
      <<" chunks="<<w.generatedNaturalChunks.size()<<" materializedAfterStart="<<(w.generatedNaturalChunks.size()>=rt.initialChunkCount?w.generatedNaturalChunks.size()-rt.initialChunkCount:0)<<" chunkMinX="<<minX<<" chunkMaxX="<<maxX<<" chunkMinY="<<minY<<" chunkMaxY="<<maxY
      <<" chunksLast1d="<<newly1<<" chunksLast7d="<<newly7<<" chunksLast30d="<<newly30<<" usedChunks="<<usedChunks
      <<" discoveredResourceNodes="<<discoveredNodes<<" usedDiscoveredResourceNodes="<<usedDiscoveredNodes<<" discoveryFollowupRate="<<(discoveredNodes?static_cast<double>(usedDiscoveredNodes)/discoveredNodes:0.0)<<"\n";
    std::cout<<"AUDIT_SETTLEMENT seed="<<seed<<" day="<<day<<" settlements="<<net.settlementCount<<" activeSettlements="<<net.activeSettlementCount
      <<" inhabited="<<life.inhabitedCount<<" declining="<<life.decliningCount<<" abandoned="<<life.abandonedCount<<" migrationStarts="<<ev.migrationStarts
      <<" migrationAssignmentTransitions="<<ev.migrationAssignmentTransitions<<" tradeDepartures="<<ev.tradeDepartures<<" tradeArrivals="<<ev.tradeArrivals
      <<" tradeExchanges="<<ev.tradeExchanges<<" tradeReturns="<<ev.tradeReturns<<" tradeFailed="<<ev.tradeFailed<<" tradeCancelled="<<ev.tradeCancelled
      <<" tradeRoutes="<<trade.routeCount<<" activeTradeRoutes="<<trade.activeRouteCount<<" tradeEvidence="<<trade.exchangeEvidenceCount<<"\n";
    for(const auto&s:net.settlements)std::cout<<"AUDIT_SETTLEMENT_DETAIL seed="<<seed<<" day="<<day<<" id="<<s.id<<" residents="<<s.residentCount<<" facilities="<<s.facilityCount<<" operationalFacilities="<<s.operationalFacilityCount<<" plannedFacilities="<<s.plannedFacilityCount<<" storages="<<s.storageSiteCount<<" active="<<(s.active?1:0)<<" established="<<(s.established?1:0)<<"\n";
    std::cout<<"AUDIT_HEALTH seed="<<seed<<" day="<<day<<" pathogenAvg="<<(livingHealth?pathogenSum/livingHealth:0.0)<<" illnessAvg="<<(livingHealth?illnessSum/livingHealth:0.0)
      <<" immunityAvg="<<(livingHealth?immunitySum/livingHealth:0.0)<<" illnessIncidence="<<ev.illnesses<<" recoveries="<<ev.recoveries<<" careEvents="<<ev.careEvents
      <<" deathsIllness="<<ev.deaths[0]<<" deathsAccident="<<ev.deaths[1]<<" deathsExposure="<<ev.deaths[2]<<" deathsDeprivation="<<ev.deaths[3]<<" deathsOther="<<ev.deaths[4]
      <<" sanitationSites="<<w.primitiveSanitationSites.size()<<" activeSanitationSites="<<sanitationActive<<" sanitationUses="<<sanitationUses
      <<" humanWasteResidues="<<w.environmentalResidues.all().size()
      <<" contaminationExposureResidents="<<exposureResidents<<" contaminatedWaterNodes="<<contaminatedWaterNodes
      <<" maxWaterContamination="<<maxWaterContamination<<"\n";
    for(const auto&kv:residents){
        const auto&a=kv.second;
        std::cout<<"AUDIT_HEALTH_CHAIN seed="<<seed<<" day="<<day<<" resident="<<kv.first<<" firstExposure="<<a.firstExposureMinute<<" firstPathogen="<<a.firstPathogenMinute
          <<" firstIllness="<<a.firstIllnessMinute<<" firstCare="<<a.firstCareMinute<<" firstRecovery="<<a.firstRecoveryMinute<<" death="<<a.deathMinute
          <<" illnessMinutes="<<a.illnessMinutes<<" maxPathogen="<<a.maxPathogen<<" maxIllness="<<a.maxIllness<<" maxContaminationDose="<<a.maxContaminationDose
          <<" maxEnvironmentalStress="<<a.maxEnvironmentalStress<<" maxImmunity="<<a.maxImmunity<<"\n";
    }
    std::cout<<"AUDIT_CIVILIZATION seed="<<seed<<" day="<<day<<" knownTechniqueTypes="<<civ.uniqueKnownTechniqueTypes<<" reproducibleTechniqueTypes="<<civ.uniqueReproducibleTechniqueTypes
      <<" techniqueFacts="<<civ.techniqueFactCount<<" teachingReceipts="<<civ.transmissionReceiptCount<<" lostTechnologies="<<civ.lostTechnologyCount
      <<" commonTechnologies="<<civ.commonTechnologyCount<<" establishedTechnologies="<<civ.establishedTechnologyCount<<" specializedResidents="<<society.specializedResidentCount
      <<" activeInstitutions="<<society.activeInstitutionCount<<" institutionMemberships="<<society.institutionMembershipCount<<" apprenticeship="<<society.apprenticeshipCount
      <<" barterExchangeFacts="<<society.exchangeFactCount<<" durableRecords="<<society.durableRecordFactCount<<" recordMedia="<<society.recordMediaUnits
      <<" discoveredTracked="<<rt.firstKnownMinute.size()<<" usedKnowledge="<<usedKnowledge<<" neverUsedKnowledge="<<neverUsed<<"\n";
    for(const auto&kv:rt.firstKnownMinute){
        const auto use=rt.firstUseMinute.find(kv.first);std::cout<<"AUDIT_KNOWLEDGE seed="<<seed<<" day="<<day<<" technique="<<static_cast<int>(kv.first)<<" discoveredMinute="<<kv.second<<" firstUseMinute="<<(use==rt.firstUseMinute.end()?-1:use->second)<<" lagMinutes="<<(use==rt.firstUseMinute.end()?-1:std::max(0,use->second-kv.second))<<"\n";
    }
    std::cout<<"AUDIT_OBSERVABILITY seed="<<seed<<" day="<<day<<" migrationTime=event_or_assignment_only institutionEconomyTime=coordination_not_exclusive materialTradeTransfer=not_authoritatively_exposed spoilLoss=not_authoritatively_exposed"<<"\n";
    std::cout<<"AUDIT_EVENTS seed="<<seed<<" day="<<day<<" preemptions="<<ev.preemptions<<" routeFailures="<<ev.routeFailures<<" timeouts="<<ev.timeouts
      <<" socialEvents="<<ev.socialEvents<<" civilizationEvents="<<ev.civilizationEvents<<" firstCritical="<<ev.firstCritical<<" firstIllness="<<ev.firstIllness
      <<" firstDeath="<<ev.firstDeath<<" firstBirth="<<ev.firstBirth<<" firstMarriage="<<ev.firstMarriage<<" firstPregnancy="<<ev.firstPregnancy
      <<" firstSecondSettlement="<<ev.firstSecondSettlement<<" firstChunkGrowth="<<ev.firstChunkGrowth<<" firstTradeDeparture="<<ev.firstTradeDeparture
      <<" firstTradeExchange="<<ev.firstTradeExchange<<" firstTradeReturn="<<ev.firstTradeReturn<<"\n";
    for(const auto&kv:residents){
        const auto&a=kv.second;
        const std::array<const char*,5> residentNeedNames{{"hunger","thirst","fatigue","bladder","hygiene"}};
        for(std::size_t i=0;i<5;++i)std::cout<<"AUDIT_RESIDENT_NEED seed="<<seed<<" day="<<day<<" resident="<<kv.first<<" need="<<residentNeedNames[i]
          <<" avg="<<(a.observedMinutes?a.needSum[i]/a.observedMinutes:0.0)<<" p90="<<percentile(a.needHistogram[i],0.90)<<" p99="<<percentile(a.needHistogram[i],0.99)
          <<" urgentEntries="<<a.urgentEntries[i]<<" urgentMinutes="<<a.urgentMinutes[i]<<" criticalEntries="<<(i<2?a.criticalEntries[i]:0)<<" criticalMinutes="<<(i<2?a.criticalMinutes[i]:0)
          <<" saturatedMinutes="<<a.saturatedMinutes[i]<<" travelDistance="<<a.travelDistance[i]<<"\n";
        std::cout<<"AUDIT_RESIDENT_TIME seed="<<seed<<" day="<<day<<" resident="<<kv.first<<" observedMinutes="<<a.observedMinutes;
        for(std::size_t i=0;i<a.time.size();++i)std::cout<<" "<<TimeBucketNames[i]<<"Min="<<a.time[i];
        std::cout<<"\n";
    }
}

std::vector<int> parseCheckpoints(const std::string&raw,int maxDay){
    std::vector<int>v;std::stringstream ss(raw);std::string x;while(std::getline(ss,x,',')){int d=std::atoi(x.c_str());if(d>0&&d<=maxDay)v.push_back(d);}
    if(v.empty())v.push_back(maxDay);std::sort(v.begin(),v.end());v.erase(std::unique(v.begin(),v.end()),v.end());if(v.back()!=maxDay)v.push_back(maxDay);return v;
}

} // namespace

int main(int argc,char**argv){
    int days=1000;std::uint64_t seed=4242001;std::string checkpointsArg="1,7,30,100,365,1000";std::string snapshotDir;
    for(int i=1;i<argc;++i){std::string a=argv[i];if(a=="--days"&&i+1<argc)days=std::max(1,std::atoi(argv[++i]));else if(a=="--seed"&&i+1<argc)seed=std::strtoull(argv[++i],nullptr,10);else if(a=="--checkpoints"&&i+1<argc)checkpointsArg=argv[++i];else if(a=="--snapshot-directory"&&i+1<argc)snapshotDir=argv[++i];}
    const auto checkpoints=parseCheckpoints(checkpointsArg,days);Simulation sim(seed);sim.setupNewGame();
    if(!snapshotDir.empty())std::filesystem::create_directories(snapshotDir);
    std::map<CharacterId,ResidentAudit> residents;for(const auto&c:sim.world().characters)residents[c.id];
    std::array<MaterialFlow,AuditedMaterials.size()> flows{};EventAudit ev{};WorldRuntimeAudit rt{};
    rt.initialChunkCount=sim.world().generatedNaturalChunks.size();
    for(const auto&ch:sim.world().generatedNaturalChunks)rt.seenChunks.insert(ch.coord);
    sim.onEvent([&](const std::string&line){
        const int minute=sim.world().minute;
        if(line.find("preempted current activity")!=std::string::npos){++ev.preemptions;if(ev.firstCritical<0)ev.firstCritical=minute;}
        if(line.find("route failed")!=std::string::npos)++ev.routeFailures;if(line.find("timed out")!=std::string::npos)++ev.timeouts;
        if(line.find(" -> Approach ")!=std::string::npos||line.find(" -> Avoid ")!=std::string::npos||line.find(" -> Repair ")!=std::string::npos||line.find(" -> Comfort ")!=std::string::npos)++ev.socialEvents;
        if(line.find(" -> Civilization ")!=std::string::npos)++ev.civilizationEvents;
        if(line.find("began group migration")!=std::string::npos)++ev.migrationStarts;
        if(line.find("departed settlement")!=std::string::npos&&line.find("to trade with")!=std::string::npos){++ev.tradeDepartures;if(ev.firstTradeDeparture<0)ev.firstTradeDeparture=minute;}
        if(line.find("arrived")!=std::string::npos&&line.find("trade")!=std::string::npos)++ev.tradeArrivals;
        if(line.find("traveled from settlement")!=std::string::npos&&line.find("and exchanged")!=std::string::npos){++ev.tradeExchanges;if(ev.firstTradeExchange<0)ev.firstTradeExchange=minute;}
        if(line.find("returned from inter-settlement trade")!=std::string::npos){++ev.tradeReturns;if(ev.firstTradeReturn<0)ev.firstTradeReturn=minute;}
        if(line.find("trade")!=std::string::npos&&line.find("failed")!=std::string::npos)++ev.tradeFailed;if(line.find("trade")!=std::string::npos&&line.find("cancel")!=std::string::npos)++ev.tradeCancelled;
        if(line.find("became ill after accumulated pathogen exposure")!=std::string::npos){++ev.illnesses;if(ev.firstIllness<0)ev.firstIllness=minute;}
        if(line.find("recovered from illness and gained resilience")!=std::string::npos)++ev.recoveries;
        if(line.find(" -> HealthCare")!=std::string::npos)++ev.careEvents;
        if(line.find("died from illness")!=std::string::npos)++ev.deaths[0];else if(line.find("died from an accident")!=std::string::npos)++ev.deaths[1];else if(line.find("died from environmental exposure")!=std::string::npos)++ev.deaths[2];else if(line.find("died from severe deprivation")!=std::string::npos)++ev.deaths[3];else if(line.find(" died")!=std::string::npos)++ev.deaths[4];
        const std::array<const char*,5> names{{"Eat","Drink","Sleep","UseToilet","Wash"}};
        for(std::size_t i=0;i<names.size();++i){if(line.find(std::string(" -> ")+names[i]+" (need ")!=std::string::npos)++ev.survivalStarts[i];if(line.find(std::string(" completed ")+names[i])!=std::string::npos){++ev.survivalCompletions[i];if(i==0)++flows[materialIndex(MaterialKind::PlantFood)].consumed;else if(i==1||i==4)++flows[materialIndex(MaterialKind::Water)].consumed;}}
    });
    const auto started=std::chrono::steady_clock::now();std::size_t cp=0;
    for(int m=1;m<=days*MinutesPerDay;++m){
        sim.step();
        for(auto&c:sim.world().characters){auto&a=residents[c.id];if(c.alive)observeResidentMinute(sim,c,a);else if(a.deathMinute<0)observeResidentMinute(sim,c,a);}
        observeCivilizationEvents(sim,residents,flows,ev,rt);
        if(m%60==0)sampleHourly(sim,flows,ev,rt);
        if(m%MinutesPerDay==0)updateFamilyTimeline(sim,ev);
        if(cp<checkpoints.size()&&m>=checkpoints[cp]*MinutesPerDay){
            emitCheckpoint(sim,seed,checkpoints[cp],residents,flows,ev,rt,started);
            if(!snapshotDir.empty()){std::vector<std::uint8_t>b;std::string e;if(!encodeSimulationSnapshot(sim.captureSnapshot(),b,&e)){std::cerr<<e<<"\n";return 2;}std::ofstream o(snapshotDir+"/day-"+std::to_string(checkpoints[cp])+".llsave",std::ios::binary);o.write(reinterpret_cast<const char*>(b.data()),static_cast<std::streamsize>(b.size()));}
            ++cp;std::cout.flush();
        }
    }
    std::cout<<"AUDIT_COMPLETE seed="<<seed<<" days="<<days<<" checkpoints="<<checkpoints.size()<<"\n";
    return 0;
}
